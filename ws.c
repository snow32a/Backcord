#include "http.h"
#include <winsock2.h>
#include <ws2tcpip.h>
#include <windows.h>
#include <stdio.h>
#include <openssl/rand.h>
#include <openssl/ssl.h>
#include <openssl/err.h>
#include "ws.h"
DWORD WINAPI WSThreadProc(LPVOID ssl);
int OpenWebSocket(const char *hostname, const char *path, SSL **out_ssl) {
	SOCKET sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (sock == INVALID_SOCKET) {
		MessageBoxA(NULL, "socket failed", "WebSocket Error", 0);
		return 1;
	}

	struct sockaddr_in server;
	RtlZeroMemory(&server, sizeof(server));
	server.sin_family = AF_INET;
	server.sin_port = htons(443);
	server.sin_addr = **(struct in_addr **)gethostbyname(hostname)->h_addr_list;

	if (connect(sock, (struct sockaddr *)&server, sizeof(server)) ==
		SOCKET_ERROR) {
		MessageBoxA(NULL, "connect failed", "WebSocket Error", 0);
		closesocket(sock);
		return 1;
	}

	SSL_library_init();
	SSL_load_error_strings();
	OpenSSL_add_all_algorithms();

	SSL_CTX *ctx = SSL_CTX_new(TLS_client_method());
	if (!ctx) {
		MessageBoxA(NULL, "SSL_CTX_new failed", "WebSocket Error", 0);
		closesocket(sock);
		return 1;
	}

	SSL *ssl = SSL_new(ctx);
	if (!ssl) {
		MessageBoxA(NULL, "SSL_new failed", "WebSocket Error", 0);
		SSL_CTX_free(ctx);
		closesocket(sock);
		return 1;
	}

	SSL_set_fd(ssl, (int)sock);
	SSL_set_tlsext_host_name(ssl, hostname);

	if (SSL_connect(ssl) <= 0) {
		MessageBoxA(NULL, "SSL_connect failed", "WebSocket Error", 0);
		SSL_free(ssl);
		SSL_CTX_free(ctx);
		closesocket(sock);
		return 1;
	}

	// anything but a Base64 lib
	const char *ws_key = "tLGs4Zw47vMKJ8bT5AXjLw==";

	// send WebSocket upgrade request
	char request[2048];
	wsprintfA(request,
			  "GET %s HTTP/1.1\r\n"
			  "Host: %s\r\n"
			  "Upgrade: websocket\r\n"
			  "Connection: Upgrade\r\n"
			  "Sec-WebSocket-Key: %s\r\n"
			  "Sec-WebSocket-Version: 13\r\n"
			  "\r\n",
			  path, hostname, ws_key);

	SSL_write(ssl, request, (int)strlen(request));

	// read response into epic stack arr
	char buffer[4096];
	int bytes = SSL_read(ssl, &buffer, sizeof(buffer) - 1);
	if (bytes > 0) {
		buffer[bytes] = '\0';

		// check for 101 Switching Protocols otherwise it means smth fucked up
		if (strstr(buffer, "101") == NULL) {
			MessageBoxA(NULL, buffer, "websocket isnt websocketing", 0);
			SSL_shutdown(ssl);
			SSL_free(ssl);
			SSL_CTX_free(ctx);
			closesocket(sock);
			return 1;
		}
	}

	CreateThread(0, 8192, WSThreadProc, ssl, 0, 0);
	// connection upgraded to WebSocket yay
	*out_ssl = ssl;
	return 0;
}
DWORD WINAPI WSThreadProc(LPVOID ssl) {
	char *buffer;
	size_t length;

	while (1) {
		int result = ReadWebSocket(ssl, &buffer, &length);

		if (length != 0 && result != 0) {
			WebSocketOnDataArrival(ssl, buffer, length);
			// printf("%s\n", buffer);
			// LE IMPORTANT: free the buffer after processing... memleak goez
			// brr otherewise
			free(buffer);
		} else {
			break;
		}
	}
	return 0;
};
int SendWebSocket(SSL *ssl, const char *data, size_t length,
				  unsigned char flags) {
	printf("%s\n", data);
	unsigned char frame[14];
	size_t frame_size = 0;

	// Flags are to our goofy ahh param
	frame[0] = flags;
	frame_size++;

	// Mask bit set (client must mask the ahh) (stolen code)
	if (length < 126) {
		frame[1] = 0x80 | (unsigned char)length;
		frame_size++;
	} else if (length < 65536) {
		frame[1] = 0x80 | 126;
		frame[2] = (length >> 8) & 0xFF;
		frame[3] = length & 0xFF;
		frame_size += 3;
	} else {
		frame[1] = 0x80 | 127;
		frame[2] = (length >> 56) & 0xFF;
		frame[3] = (length >> 48) & 0xFF;
		frame[4] = (length >> 40) & 0xFF;
		frame[5] = (length >> 32) & 0xFF;
		frame[6] = (length >> 24) & 0xFF;
		frame[7] = (length >> 16) & 0xFF;
		frame[8] = (length >> 8) & 0xFF;
		frame[9] = length & 0xFF;
		frame_size += 9;
	}

	// whats even the purpose of ts, just fake the masks
	unsigned char mask[4];
	for (unsigned char i = 0; i < 4; i++) {
		mask[i] = (unsigned char)(i & 0xFF);
	}

	RtlCopyMemory(&frame[frame_size], mask,
				  4); // me when i rtlcopymemory to look cool
	frame_size += 4;

	// Send frame header
	if (SSL_write(ssl, frame, (int)frame_size) <= 0) {
		return 1;
	}

	// Mask and send payload
	unsigned char *masked_data = (unsigned char *)malloc(length);
	if (!masked_data) {
		return 1;
	}

	for (size_t i = 0; i < length; i++) {
		masked_data[i] = data[i] ^ mask[i % 4];
	}

	int result = SSL_write(ssl, masked_data, (int)length);
	free(masked_data);

	return (result <= 0) ? 1 : 0;
}

int ReadWebSocket(SSL *ssl, char **out_buffer, size_t *out_length) {
	*out_buffer = malloc(10); // tmp buffer
	unsigned char opcode;
	BOOL hasSetFirstOpcode = FALSE;
	unsigned long long payloadLen = 0;
	while (1) {
		unsigned long long curPayloadLen = 0;
		unsigned char fin;

		unsigned char coreHeaders;
		if (SSL_read(ssl, &coreHeaders, 1) != 1) {
			return 0;
		}
		fin = coreHeaders >> 7;

		if (hasSetFirstOpcode != TRUE) {
			opcode = (coreHeaders << 4) >> 4;
			hasSetFirstOpcode = TRUE;
		}
		unsigned char plHeaders;
		SSL_read(ssl, &plHeaders, 1);
		unsigned char tlen = (plHeaders << 1) >> 1;
		if (tlen == 127) {
			unsigned char lenbytes[8];
			SSL_read(ssl, lenbytes,8);
			curPayloadLen = 0;
			for (int i = 0; i < 4; i++) {
				unsigned char temp = lenbytes[i];
				lenbytes[i]=lenbytes[7-i];
				lenbytes[7-i]=temp;
			}
			curPayloadLen = *(unsigned long long*)lenbytes;
			payloadLen += curPayloadLen;
		} else if (tlen == 126) {
			unsigned short templen;
			SSL_read(ssl, &templen, 2);
			curPayloadLen = ntohs(templen);
			payloadLen += curPayloadLen;
		} else {
			curPayloadLen = tlen;
			payloadLen += curPayloadLen;
		}
		if (opcode == 0x01) {
			*out_buffer = realloc(*out_buffer, payloadLen + 1);
			*out_length = payloadLen + 1;
		} else {
			*out_buffer = realloc(*out_buffer, payloadLen);
			*out_length = payloadLen;
		}
		unsigned long long totalRead = 0;
		while (totalRead < curPayloadLen) {
			int bytesRead = SSL_read(
				ssl, *out_buffer + (payloadLen - curPayloadLen) + totalRead,
				curPayloadLen - totalRead);
			if (bytesRead <= 0) {
				free(*out_buffer);
				return 0;
			}
			totalRead += bytesRead;
		}
		if (fin == 1) {
			break;
		}
	}
	if (opcode == 0x01) {
		(*out_buffer)[payloadLen] = '\0';
	}
	return 1;
}
