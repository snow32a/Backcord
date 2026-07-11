#include "http.h"
#include <winsock2.h>
#include <ws2tcpip.h>
#include <windows.h>
#include <stdio.h>
#include <openssl/rand.h>
#include <openssl/ssl.h>
#include <openssl/err.h>

// keepalive state
typedef struct {
    SOCKET sock;
    SSL* ssl;
    SSL_CTX* ctx;
    char hostname[256];
    int connected;
} HTTPConnection;

static void AppendHeader(char* buf,
    const char* key, const char* value)
{
    strcat(buf, key);
    strcat(buf, ": ");
    strcat(buf, value);
    strcat(buf, "\r\n");
}
HTTPConnection* HTTPConnect(const char* hostname) {
    HTTPConnection* conn = malloc(sizeof(HTTPConnection));
    RtlZeroMemory(conn, sizeof(HTTPConnection));

    strncpy(conn->hostname, hostname, 255);
    conn->hostname[255] = '\0';

    conn->sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);

    if (conn->sock == INVALID_SOCKET) {
        MessageBoxA(NULL, "socket failed :(", "sad http announcement", 0);
        free(conn);
        return NULL;
    }

    struct sockaddr_in server;
    RtlZeroMemory(&server, sizeof(server));
    server.sin_family = AF_INET;
    server.sin_port = htons(443);
    server.sin_addr = **(struct in_addr**)gethostbyname(hostname)->h_addr_list; // why is a part of this in bold

    if (connect(conn->sock, (struct sockaddr*)&server, sizeof(server)) == SOCKET_ERROR) {
        MessageBoxA(NULL, "connect failed :(", "sad http announcement", 0);
        closesocket(conn->sock);
        free(conn);
        return NULL;
    }

    conn->ctx = SSL_CTX_new(TLS_client_method());

    if (!conn->ctx) {
        MessageBoxA(NULL, "SSL_CTX_new failed :(", "sad http announcement", 0);
        closesocket(conn->sock);
        free(conn);
        return NULL;
    }

    conn->ssl = SSL_new(conn->ctx);

    if (!conn->ssl) {
        MessageBoxA(NULL, "SSL_new failed :(", "sad http announcement", 0);
        SSL_CTX_free(conn->ctx);
        closesocket(conn->sock);
        free(conn);
        return NULL;
    }

    SSL_set_fd(conn->ssl, (int)conn->sock);
    SSL_set_tlsext_host_name(conn->ssl, hostname);

    if (SSL_connect(conn->ssl) <= 0) {
        MessageBoxA(NULL, "SSL_connect failed :(", "sad http announcement", 0);
        SSL_free(conn->ssl);
        SSL_CTX_free(conn->ctx);
        closesocket(conn->sock);
        free(conn);
        return NULL;
    }

    conn->connected = 1;
    return conn;
}
int HTTPSendRequest(HTTPConnection* conn, const char* method, const char* endpoint,
    const char* user_agent, const char* content_type, void* payload,
    unsigned long payload_size, char** response, long* responselen)
{
    if (!conn || !conn->connected) {
        MessageBoxA(NULL, "connection not established :(", "sad http announcement", 0);
        return 1;
    }

    char headerbuf[2048];
    RtlZeroMemory(headerbuf, sizeof(headerbuf));

    wsprintfA(headerbuf, "%s %s HTTP/1.1\r\n", method, endpoint);
    strcat(headerbuf, "Host: ");
    strcat(headerbuf, conn->hostname);
    strcat(headerbuf, "\r\n");
    strcat(headerbuf, "User-Agent: ");
    strcat(headerbuf, user_agent);
    strcat(headerbuf, "\r\n");

    if (payload) {
        strcat(headerbuf, "Content-Type: ");
        strcat(headerbuf, content_type);
        strcat(headerbuf, "\r\n");

        char contentlen[128];
        RtlZeroMemory(contentlen, sizeof(contentlen));
        wsprintfA(contentlen, "Content-Length: %lu\r\n", payload_size);
        strcat(headerbuf, contentlen);
    }

    strcat(headerbuf, "Connection: keep-alive\r\n");
    strcat(headerbuf, "\r\n");

    if (SSL_write(conn->ssl, headerbuf, (int)strlen(headerbuf)) <= 0) {
        MessageBoxA(NULL, "SSL_write headers failed :(", "sad http announcement", 0);
        return 1;
    }

    if (payload && payload_size > 0) {
        if (SSL_write(conn->ssl, payload, (int)payload_size) <= 0) {
            MessageBoxA(NULL, "SSL_write payload failed :(", "sad http announcement", 0);
            return 1;
        }
    }

    char* buffer = malloc(8192);
    size_t sizeofbuf = 8192;
    size_t total_bytes = 0;
    int bytes_read;

    while ((bytes_read = SSL_read(conn->ssl, buffer + total_bytes, (int)(sizeofbuf - total_bytes - 1))) > 0) {
        total_bytes += bytes_read;

        if (total_bytes + 1 >= sizeofbuf) {
            sizeofbuf += 8192;
            char* newbuf = realloc(buffer, sizeofbuf);

            if (!newbuf) {
                MessageBoxA(NULL, "realloc failed :(", "sad http announcement", 0);
                free(buffer);
                return -1;
            }

            buffer = newbuf;
        }
    }

    buffer[total_bytes] = '\0';

    char* endofhdr = strstr(buffer, "\r\n\r\n");

    if (!endofhdr) {
        MessageBoxA(NULL, "malformed response :(", "sad http announcement", 0);
        free(buffer);
        return -1;
    }

    char* body_start = endofhdr + 4;
    size_t body_len = total_bytes - (body_start - buffer);

    if (strstr(buffer, "Transfer-Encoding: chunked")) {
        size_t outlen;
        *response = ProcessChunkedTransfer(body_start, body_len, &outlen);
        *responselen = (long)outlen;
        free(buffer);
    } else {
        char* copy = malloc(body_len + 1);
        memcpy(copy, body_start, body_len);
        copy[body_len] = '\0';
        *response = copy;
        *responselen = (long)body_len;
        free(buffer);
    }

    return 0;
}
int HTTPClose(HTTPConnection* conn) {
    if (!conn) {
        return 1;
    }

    if (conn->ssl) {
        SSL_shutdown(conn->ssl);
        SSL_free(conn->ssl);
    }

    if (conn->ctx) {
        SSL_CTX_free(conn->ctx);
    }

    if (conn->sock != INVALID_SOCKET) {
        closesocket(conn->sock);
    }

    free(conn);
    return 0;
}
char* ProcessChunkedTransfer(char* chunked, size_t chunked_len, size_t *out_len) {
    char* result = malloc(chunked_len);
    size_t written = 0;
    char* curpos = chunked;

    while(1) {
        char* crlf = strstr(curpos, "\r\n");
        if(!crlf) break;

        int chunklen = (int)strtol(curpos, NULL, 16);
        if(chunklen == 0) break;

        char* data = crlf + 2;
        memcpy(result + written, data, chunklen);
        written += chunklen;
        curpos = data + chunklen + 2;
    }

    *out_len = written;
    return result;
}
int SendShortHTTPReq(const char* hostname,const char* reqtype, const char* endpoint, const char* extra_headers, const char* user_agent, const char* content_type, void* payload, unsigned long payload_size, char** response, long* responselen)
{
    SOCKET sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (sock == INVALID_SOCKET) {
        MessageBoxA(NULL, "socket failed", "Aa", 0);
        return 1;
    }

    struct sockaddr_in server;
    RtlZeroMemory(&server, sizeof(server));
    server.sin_family = AF_INET;
    server.sin_port = htons(443); // HTTPS brotatochippiee
    server.sin_addr = **(struct in_addr**)gethostbyname(hostname)->h_addr_list;

    if (connect(sock, (struct sockaddr*)&server, sizeof(server)) == SOCKET_ERROR) {
        MessageBoxA(NULL, "connect failed", "Aa", 0);
        closesocket(sock);
        return 1;
	}

    // this debugging thing served us well yall MessageBoxA(NULL, "onto ze ctx", "Aa", 0);
    SSL_CTX* ctx = SSL_CTX_new(TLS_client_method());
    if (!ctx) {
		MessageBoxA(NULL, "SSL_CTX_new failed", "Aa", 0);
        unsigned long err = ERR_get_error();
        if (err == 0) {
            MessageBoxA(NULL, "unknown OpenSSL error", "title", MB_ICONERROR);
            return 1;
        }

        char buf[256];
        ERR_error_string_n(err, buf, sizeof(buf));

        MessageBoxA(NULL, buf, "title", MB_ICONERROR);
        return 1;
    }

    SSL* ssl = SSL_new(ctx);
    if (!ssl) {
        MessageBoxA(NULL, "SSL_new failed qwq", "Aa", 0);
        return 1;
    }

    // Attach socket to SSL
    SSL_set_fd(ssl, (int)sock);

    // SNI (importante shi)
    SSL_set_tlsext_host_name(ssl, hostname);

    if (SSL_connect(ssl) <= 0) {
        MessageBoxA(NULL, "SSL_connect failed", "Aa", 0);
        SSL_free(ssl);
        SSL_CTX_free(ctx);
        closesocket(sock);
        return 1;
    }


	SSL_write(ssl, reqtype, (int)strlen(reqtype)); // Sends request type
	SSL_write(ssl, " ", 1); // stupid fuckass space
	SSL_write(ssl, endpoint, (int)strlen(endpoint)); // endpoint
	SSL_write(ssl, " HTTP/1.1\r\n", 11); // the protocol thing.. ykw i will be more formal
	SSL_write(ssl, "Host: ", 6); // Host beginning
	SSL_write(ssl, hostname, (int)(strlen(hostname))); // Host data
	SSL_write(ssl, "\r\n", 2);                         // Host end
    SSL_write(ssl, "User-Agent: ", 12); // User Agent beginning
	SSL_write(ssl, user_agent, (int)(strlen(user_agent))); // User Agent data
	SSL_write(ssl, "\r\n", 2);                             // User Agent end


	if (payload != 0) {
		SSL_write(ssl, "Content-Type: ", 14); // Content Type beginning
		SSL_write(ssl, content_type, (int)strlen(content_type));
        SSL_write(ssl, "\r\n", 2);                          // Content Type end

	SSL_write(ssl, "Content-Length: ", 16); // Content Length beginning
	char lenstr[64];
	RtlZeroMemory(lenstr,64);
	wsprintfA(lenstr,"%d",payload_size);
	SSL_write(ssl, lenstr, (int)(strlen(lenstr))); // Content Length data
	SSL_write(ssl, "\r\n", 2);                          // Content Length end
}


	SSL_write(ssl, extra_headers, (int)(strlen(extra_headers))); // Extra Header
    SSL_write(ssl, "Connection: close\r\n", 19); //Short-term connection
    SSL_write(ssl, "\r\n", 2); //Ends the header
    if(payload)
        SSL_write(ssl, payload, (int)payload_size); //Payload

    char* buffer = malloc(4096);
    size_t sizeofbuf = 4096;
    size_t total_bytes = 0;
    int bytes_read;

    while ((bytes_read = SSL_read(ssl, buffer + total_bytes, (int)(sizeofbuf - total_bytes - 1))) > 0) {
        total_bytes += bytes_read;

        if (total_bytes + 1 >= sizeofbuf) {
            sizeofbuf += 4096;
            char* newbuf = realloc(buffer, sizeofbuf);
            if (!newbuf) {
                //what the fuck had possibly happened on bros pc :sob:
                free(buffer);
                SSL_shutdown(ssl);
                SSL_free(ssl);
                SSL_CTX_free(ctx);
                closesocket(sock);
                return -1;
            }
            buffer = newbuf;
        }
    }
    buffer[total_bytes] = '\0';

    char *endofhdr = strstr(buffer, "\r\n\r\n");
    if (!endofhdr) {
        free(buffer);
        SSL_shutdown(ssl); SSL_free(ssl); SSL_CTX_free(ctx);
        closesocket(sock);
        return -1;
    }

    char *body_start = endofhdr + 4;
    size_t body_len = total_bytes - (body_start - buffer);
	if (strstr(buffer, "Transfer-Encoding: chunked")) {
		size_t outlen;
        *response = ProcessChunkedTransfer(body_start,body_len,&outlen);
        *responselen = (long)outlen;
        free(buffer);
    } else {
        char *copy = malloc(body_len + 1);
        memcpy(copy, body_start, body_len);
        copy[body_len] = '\0';
        *response = copy;
        *responselen = (long)body_len;
        free(buffer);
    }

    SSL_shutdown(ssl);
    SSL_free(ssl);
    SSL_CTX_free(ctx);
    closesocket(sock);
    return 0;
}
int InitHTTP() {
    SSL_library_init();
    SSL_load_error_strings();
    OpenSSL_add_all_algorithms();
    WSADATA wsaData;
    WSAStartup(MAKEWORD(2,2), &wsaData); //me when macros
}
int CleanupHTTP() {
	WSACleanup();
}
