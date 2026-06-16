#include "http.h"
#include <winsock2.h>
#include <ws2tcpip.h>
#include <windows.h>
#include <stdio.h>
#include <openssl/rand.h>
#include <openssl/ssl.h>
#include <openssl/err.h>

static void AppendHeader(char* buf,
    const char* key, const char* value)
{
    strcat(buf, key);
    strcat(buf, ": ");
    strcat(buf, value);
    strcat(buf, "\r\n");
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
