
#include <winsock2.h>
#include <ws2tcpip.h>
#include <openssl/ssl.h>
// keepalive state
typedef struct {
    SOCKET sock;
    SSL* ssl;
    SSL_CTX* ctx;
    char hostname[256];
    int connected;
    CRITICAL_SECTION lock;
} HTTPConnection;
void InitSockets();
int SendShortHTTPReq(const char *hostname, const char *reqtype,
                     const char *endpoint, const char *extra_headers,
                     const char *user_agent, const char *content_type,
                     void *payload, unsigned long payload_size, char** response, long* responselen);

HTTPConnection* HTTPConnect(const char* hostname);
int SendHTTPRequest(HTTPConnection* conn, const char* method, const char* endpoint, const char* extra_headers,
    const char* user_agent, const char* content_type, void* payload,
    unsigned long payload_size, char** response, long* responselen);
int CloseHTTPConnection(HTTPConnection* conn);
static void AppendHeader(char *buf, const char *key, const char *value);

int InitHTTP();
int CleanupHTTP();
