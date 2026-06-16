#pragma once
#include <windows.h>
#include <wingdi.h>
#include "snoctrl.h"
#include "../discordtypes.h"
#include "../hmap/hashmap.h"
typedef struct {
    DiscordMessage msg;
    HWND vw;
} GUIMessage;
typedef struct {
    GUIMessage* uimsgs;
	int uimsgcnt;
	HWND hWnd;
	int scrollOffset;
	int contentHeight;
	hmap pfpmap;
} ChatWnd;

typedef struct{
	char *key;
	HDC bmp;
} IntPfpTableItem;

ChatWnd* GetChatControlDetails(HWND hwnd);
void InsertChatMessage(DiscordMessage msg, HWND hwnd);
void ClearChatControl(HWND hwnd);
LRESULT CALLBACK ChatWndProc(HWND hwnd, UINT uMsg, WPARAM wParam,
                             LPARAM lParam);
void ChatView_SetUserPfp(char *path, char *user);