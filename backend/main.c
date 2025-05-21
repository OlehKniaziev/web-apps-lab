#include <stdio.h>
#include "http.h"

static http_response_status IndexHandler(http_response_context *Context) {
    Context->Content = SV_LIT("HELLO");
    return HTTP_STATUS_OK;
}

int main() {
    http_server Server;
    HttpServerInit(&Server);

    u16 ServerPort = 5959;

    HttpServerAttachHandler(&Server, "/", IndexHandler);

    printf("Starting the server on port %u\n", ServerPort);
    HttpServerStart(&Server, ServerPort);
}
