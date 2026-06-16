void InitSockets();
int SendShortHTTPReq(const char *hostname, const char *reqtype,
                     const char *endpoint, const char *extra_headers,
                     const char *user_agent, const char *content_type,
                     void *payload, unsigned long payload_size, char** response, long* responselen);


static void AppendHeader(char *buf, const char *key, const char *value);

int InitHTTP();
int CleanupHTTP();
