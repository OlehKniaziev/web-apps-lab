#include "http.h"

#include <sys/socket.h>
#include <sys/types.h>
#include <netdb.h>

#include <errno.h>

b32 HttpRequestParse(arena *Arena, string_view Buffer, http_request *OutRequest) {
    printf("Parsing HTTP request (%zu bytes):\n" SV_FMT "\n", Buffer.Count, SV_ARG(Buffer));

    // 1. Request line. (https://datatracker.ietf.org/doc/html/rfc2616#section-5.1)
    // 1.1. Method. (https://datatracker.ietf.org/doc/html/rfc2616#section-5.1.1)

    uz I;
    for (I = 0; I < Buffer.Count; ++I) {
        if (Buffer.Items[I] == ' ') break;
    }

    if (I >= Buffer.Count) return 0;

    http_method RequestMethod;
    string_view RequestMethodSv = {.Items = Buffer.Items, .Count = I};
#define X(Method) if (StringViewEqualCStr(RequestMethodSv, #Method)) {  \
        RequestMethod = HTTP_##Method;                                  \
        goto RequestMethodSuccess;                                      \
    }

    ENUM_HTTP_METHODS

#undef X

    // Failed to parse the HTTP method.
    return 0;

 RequestMethodSuccess:
    // 1.2. Request URI. (https://datatracker.ietf.org/doc/html/rfc2616#section-5.1.2)
    // FIXME(oleh): Actually parse URI's.

    uz PathStart = I + 1;

    for (I = PathStart; I < Buffer.Count; ++I) {
        if (Buffer.Items[I] == ' ') break;
    }

    if (I >= Buffer.Count) return 0;

    string_view RequestPath = {.Items = Buffer.Items + PathStart, .Count = I - PathStart};

    // 1.3. HTTP version.

    uz VersionStart = I + 1;

    for (I = VersionStart; I < Buffer.Count; ++I) {
        if (Buffer.Items[I] == '\r') break;
    }

    if (I >= Buffer.Count) return 0;

    http_version RequestVersion;
    string_view VersionSv = {.Items = Buffer.Items + VersionStart, .Count = I - VersionStart};

#define X(Version, String) if (StringViewEqualCStr(VersionSv, String)) { \
        RequestVersion = HTTP_##Version;                                \
        goto RequestVersionSuccess;                                     \
    }

    ENUM_HTTP_VERSIONS
#undef X

    // NOTE(oleh): Failed to recognize the HTTP version.
    return 0;
 RequestVersionSuccess:
    // 1.4. CRLF.

    if (Buffer.Count - I <= 1) return 0;
    if (Buffer.Items[I + 1] != '\n') return 0;

    Buffer.Items += I + 2;
    Buffer.Count -= I + 2;

    // 2. Headers. (https://datatracker.ietf.org/doc/html/rfc2616#section-5.3)
    // FIXME(oleh): Actually parse the headers according to their specification.

    http_header *RequestHeadersItems = ARENA_NEW(Arena, http_header);
    uz RequestHeadersCount = 0;

    for (I = 0; I < Buffer.Count; ++I) {
        if (Buffer.Items[I] == '\r' && Buffer.Count - I > 1 && Buffer.Items[I + 1] == '\n') break;

        uz HeaderNameStart = I;

        for (; I < Buffer.Count; ++I) {
            if (Buffer.Items[I] == ':') break;
        }

        if (I >= Buffer.Count) return 0;

        string_view HeaderName = {.Items = Buffer.Items + HeaderNameStart, .Count = I - HeaderNameStart};

        uz HeaderValueStart = I + 1;

        for (I = HeaderValueStart; I < Buffer.Count; ++I) {
            if (Buffer.Items[I] == '\r') break;
        }

        if (Buffer.Count - I <= 1) return 0;
        if (Buffer.Items[I + 1] != '\n') return 0;

        string_view HeaderValue = {.Items = Buffer.Items + I + 2, .Count = I - HeaderValueStart - 2};

        RequestHeadersItems[RequestHeadersCount] = (http_header) {.Name = HeaderName, .Value = HeaderValue};
        ArenaPush(Arena, sizeof(http_header));
        ++RequestHeadersCount;

        ++I;
    }

    if (I >= Buffer.Count) return 0;

    // 3. Message body. (https://datatracker.ietf.org/doc/html/rfc2616#section-4.3)
    string_view RequestBody = {.Items = Buffer.Items + I + 2, .Count = Buffer.Count - I - 2};

    OutRequest->Method = RequestMethod;
    OutRequest->Path = RequestPath;
    OutRequest->Version = RequestVersion;
    OutRequest->Headers.Items = RequestHeadersItems;
    OutRequest->Headers.Count = RequestHeadersCount;
    OutRequest->Body = RequestBody;

    return 1;
}

static const char *GetHttpResponseStatusReasonPhrase(http_response_status Status) {
    switch (Status) {
#define X(Status, _Code, Phrase) case HTTP_STATUS_##Status: return Phrase;
        ENUM_HTTP_RESPONSE_STATUSES
#undef X
    default: UNREACHABLE();
    }
}

static const char *HttpVersionStrings[] = {
#define X(Version, String) [HTTP_##Version] = String,
    ENUM_HTTP_VERSIONS
#undef X
};

#define TCP_BACKLOG_SIZE 256

#define DEFAULT_REQUEST_ARENA_CAPACITY (4ll * 1024ll * 1024ll * 1024ll)

void HttpServerStart(http_server *Server, u16 Port) {
    struct addrinfo Hints = {0};
    struct addrinfo* ServerAddr;

    Hints.ai_family = AF_INET;
    Hints.ai_socktype = SOCK_STREAM;
    Hints.ai_flags = AI_PASSIVE;

    char PortString[6];
    sprintf(PortString, "%d", Port);

    int Status = getaddrinfo(NULL, PortString, &Hints, &ServerAddr);
    if (Status != 0) {
        PANIC_FMT("Call to getaddrinfo failed: %s\n", gai_strerror(Status));
    }

    int ServerSock = socket(ServerAddr->ai_family, ServerAddr->ai_socktype, 0);
    if (ServerSock == -1) {
        PANIC("call to `socket` failed");
    }

    int OptValue = 1;
    if (setsockopt(ServerSock, SOL_SOCKET, SO_REUSEADDR, &OptValue, sizeof(OptValue)) == -1) {
        PANIC("Failed to set socket options");
    }

    if (bind(ServerSock, ServerAddr->ai_addr, ServerAddr->ai_addrlen) == -1) {
        PANIC("Call to `bind` failed");
    }

    if (listen(ServerSock, TCP_BACKLOG_SIZE) == -1) {
        PANIC("Call to `listen` failed");
    }

    struct sockaddr_storage ClientAddr;
    socklen_t ClientAddrSize = sizeof(ClientAddr);

    arena RequestArena;
    ArenaInit(&RequestArena, DEFAULT_REQUEST_ARENA_CAPACITY);

    http_response_context ResponseContext = {0};
    ResponseContext.Arena = &RequestArena;

    while (1) {
    ServerLoopStart:
        ArenaReset(&RequestArena);
        ResponseContext.Content = (string_view) {0};

        int ClientSock = accept(ServerSock, (struct sockaddr*)&ClientAddr, &ClientAddrSize);
        if (ClientSock == -1) {
            int AcceptError = errno;
            close(ServerSock);
            PANIC_FMT("Could not accept a new connection: %s", strerror(AcceptError));
        }

        ssize_t ReceivedBytesCount = recv(ClientSock, RequestArena.Items, RequestArena.Capacity, 0);
        if (ReceivedBytesCount == -1) {
            printf("Could not receive data from the socket\n");
            close(ClientSock);
            continue;
        }

        ASSERT(RequestArena.Offset == 0);

        string_view ParseBuffer = {.Items = RequestArena.Items, .Count = ReceivedBytesCount};

        RequestArena.Offset = AlignForward(ReceivedBytesCount, sizeof(uz));

        http_request HttpRequest;
        b32 Success = HttpRequestParse(&RequestArena, ParseBuffer, &HttpRequest);
        if (!Success) {
            printf("Could not parse the HTTP request\n");
            close(ClientSock);
            continue;
        }

        for (uz HandlerIndex = 0; HandlerIndex < Server->HandlersCount; ++HandlerIndex) {
            string_view HandlerPath = Server->HandlersPaths[HandlerIndex];
            if (!StringViewEqual(HandlerPath, HttpRequest.Path)) continue;

            http_request_handler Handler = Server->Handlers[HandlerIndex];

            ResponseContext.Request = HttpRequest;
            http_response_status ResponseStatus = Handler(&ResponseContext);

            // 1. Status line. (https://datatracker.ietf.org/doc/html/rfc2616#section-6.1)

            const char *ReasonPhrase = GetHttpResponseStatusReasonPhrase(ResponseStatus);
            const char *VersionString = HttpVersionStrings[HttpRequest.Version];

            string_view ResponseString = ArenaFormat(&RequestArena,
                                                     "%s %u %s\r\nAccess-Control-Allow-Origin: *\r\n\r\n" SV_FMT,
                                                     VersionString,
                                                     ResponseStatus,
                                                     ReasonPhrase,
                                                     SV_ARG(ResponseContext.Content));

            int SendStatus = send(ClientSock, ResponseString.Items, ResponseString.Count, 0);
            ASSERT(SendStatus != -1);

            close(ClientSock);

            goto ServerLoopStart;
        }

        http_response_status ResponseStatus = HTTP_STATUS_NOT_FOUND;
        const char *ReasonPhrase = GetHttpResponseStatusReasonPhrase(ResponseStatus);
        const char *VersionString = HttpVersionStrings[HttpRequest.Version];

        // TODO(oleh): No handler found, just give em 404!
        string_view ResponseString = ArenaFormat(&RequestArena,
                                                 "%s %u %s\r\nAccess-Control-Allow-Origin: *\r\n\r\n",
                                                 VersionString,
                                                 ResponseStatus,
                                                 ReasonPhrase);

        int SendStatus = send(ClientSock, ResponseString.Items, ResponseString.Count, 0);
        ASSERT(SendStatus != -1);

        close(ClientSock);
    }
}

// NOTE(oleh): Need to make sure that we are running on a system with virtual memory.
#define HTTP_SERVER_ARENA_CAPACITY (4ll * 1024ll * 1024ll * 1024ll)

// If you need more, seek help.
#define HTTP_SERVER_MAX_HANDLERS (100)

void HttpServerAttachHandler(http_server *Server, const char *Path, http_request_handler Handler) {
    if (Server->HandlersCount >= HTTP_SERVER_MAX_HANDLERS)
        PANIC_FMT("Maximum amount of handlers (%d) reached!", HTTP_SERVER_MAX_HANDLERS);

    uz HandlersCount = Server->HandlersCount;
    Server->HandlersPaths[HandlersCount] = SV_LIT(Path);
    Server->Handlers[HandlersCount] = Handler;
    ++Server->HandlersCount;
}

void HttpServerInit(http_server *Server) {
    ArenaInit(&Server->Arena, HTTP_SERVER_ARENA_CAPACITY);

    Server->Handlers = ArenaPush(&Server->Arena, sizeof(*Server->Handlers) * HTTP_SERVER_MAX_HANDLERS);
    Server->HandlersPaths = ArenaPush(&Server->Arena, sizeof(*Server->HandlersPaths) * HTTP_SERVER_MAX_HANDLERS);

    Server->HandlersCount = 0;
}
