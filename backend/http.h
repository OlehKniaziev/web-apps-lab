#ifndef HTTP_H_
#define HTTP_H_

#include "common.h"

// NOTE(oleh): https://datatracker.ietf.org/doc/html/rfc2616#section-5.1.1
#define ENUM_HTTP_METHODS                   \
    X(OPTIONS)                              \
    X(GET)                                  \
    X(POST)                                 \
    X(PUT)                                  \
    X(DELETE)                               \
    X(TRACE)                                \
    X(CONNECT)

typedef enum {
#define X(method) HTTP_##method,
    ENUM_HTTP_METHODS
#undef X
} http_method;

typedef struct {
    string_view Name;
    string_view Value;
} http_header;

typedef struct {
    http_header *Items;
    uz Count;
} http_headers;

#define ENUM_HTTP_VERSIONS \
    X(1_1, "HTTP/1.1")

typedef enum {
#define X(Version, String) HTTP_##Version,
ENUM_HTTP_VERSIONS
#undef X
} http_version;

typedef struct {
    http_method Method;
    string_view Path;
    http_version Version;
    http_headers Headers;
    string_view Body;
} http_request;

b32 HttpRequestParse(arena *Arena, string_view Buffer, http_request *Out);

#define ENUM_HTTP_RESPONSE_STATUSES                             \
    X(OK, 200, "OK")                                            \
        X(NOT_FOUND, 404, "Not Found")                          \
        X(INTERNAL_SERVER_ERROR, 500, "Internal Server Error")  \

typedef enum {
#define X(Status, Code, _Phrase) HTTP_STATUS_##Status = Code,
    ENUM_HTTP_RESPONSE_STATUSES
#undef X
} http_response_status;

typedef struct {
    arena *Arena;
    http_request Request;
    string_view Content;
} http_response_context;

typedef http_response_status (*http_request_handler)(http_response_context *);

typedef struct {
    // TODO(oleh): Probably introduce a thread pool and accepting socket fd here.
    arena Arena;
    string_view *HandlersPaths;
    http_request_handler *Handlers;
    uz HandlersCount;
} http_server;

void HttpServerInit(http_server *);

void HttpResponseWrite(http_response_context *, string_view);

void HttpServerStart(http_server *Server, u16 Port);
void HttpServerAttachHandler(http_server *Server, const char *Path, http_request_handler Handler);

#endif // HTTP_H_
