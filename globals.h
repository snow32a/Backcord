#pragma once
#include <openssl/ssl.h>
#include <windows.h>
#include <commctrl.h>
#include "discordtypes.h"
extern char* token;
extern SSL *GatewaySSL;
typedef struct{
    HTREEITEM itm;
    char* id;
} ChannelUIEntry;
