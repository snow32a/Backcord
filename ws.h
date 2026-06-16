
#include <openssl/ssl.h>
int OpenWebSocket(const char *hostname, const char *path, SSL **out_ssl);
int ReadWebSocket(SSL* ssl, char** out_buffer, size_t* out_length);
int SendWebSocket(SSL *ssl, const char *data, size_t length, unsigned char flags);
void WebSocketOnDataArrival(SSL *ssl, char *buffer, size_t length);

#define FIN_LAST 0x80
#define FIN_MORE 0x00
#define WS_TEXT 0x01