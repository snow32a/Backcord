#include <windows.h>
#include <wingdi.h>
#include <commctrl.h>
#include "components/chatwnd.h"
#include "discordtypes.h"
#include "http.h"
#include "ws.h"
#include "discordapi.h"
#include <cjson/cJSON.h>
#include <winnt.h>
#include "config.h"
#include "globals.h"
#include "hmap/hashmap.h"
#include "components/snoctrl.h"
#include "components/pnghlp.h"
#include "rc/res.h"
#include "login.h"
#define GUILD_PADDING 4
static DiscordMessage *PendingRndMsg = NULL;
static DiscordMessage *LastRndMsg = NULL; // rnd is render NOT random :sob:
char *curChannel;
LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
static HWND hWnd;
static HWND hMsg;
static HWND hMsgList;
static HWND hSend;
static HWND hChTree;
static HWND hDMsList;
static HWND hGSel;
static HWND hPfpArea;
static HWND hMemList;

static HWND hPfpAreaPfp;
static HWND hPfpAreaDispName;
static HWND hPfpAreaUserName;
static HIMAGELIST hSidePfps;
static HIMAGELIST hChannelImgList;
HINSTANCE hInstance;
SSL *GatewaySSL;
char *token;
HWND hDMsProfileSide;
int WINAPI WinMain(HINSTANCE hinstance, HINSTANCE hPrevInstance, PSTR pCmdLine,
				   int nCmdShow) {
#ifdef test_token
	token = test_token;
#else
	// prompt for token
	HKEY hKey;
	RegCreateKeyExA(HKEY_CURRENT_USER, "Software\\Backcord", 0, NULL,
					REG_OPTION_NON_VOLATILE, KEY_READ | KEY_WRITE, NULL, &hKey,
					NULL);

	DWORD type, size = 0;
	LONG qres = RegQueryValueExA(hKey, "Token", NULL, &type, NULL, &size);

	if (qres == ERROR_SUCCESS) {
		token = malloc(size);
		RegQueryValueExA(hKey, "Token", NULL, &type, (BYTE *)token, &size);
	} else {
		token = PromptToken();
		RegSetValueExA(hKey, "Token", 0, REG_SZ, (const BYTE *)token,
					   (DWORD)(strlen(token) + 1));
	}

	RegCloseKey(hKey);

#endif
	INITCOMMONCONTROLSEX icex;
	icex.dwICC = ICC_WIN95_CLASSES | ICC_STANDARD_CLASSES | ICC_BAR_CLASSES |
				 ICC_LISTVIEW_CLASSES | ICC_TREEVIEW_CLASSES;
	InitCommonControls();
	InitSnowsControls();
	hSidePfps = ImageList_Create(32, 32, ILC_COLOR32 | ILC_MASK, 12, 1);
	hChannelImgList = ImageList_Create(16, 16, ILC_COLOR32 | ILC_MASK, 8, 1);
	ImageList_AddMasked(
		hChannelImgList,
		LoadBitmap(hinstance, MAKEINTRESOURCE(IDI_CHANNEL_CATEGORY)),
		RGB(255, 0, 255));
	ImageList_AddMasked(hChannelImgList,
						LoadBitmap(hinstance, MAKEINTRESOURCE(IDI_CHANNEL_TC)),
						RGB(255, 0, 255));
	ImageList_AddMasked(hChannelImgList,
						LoadBitmap(hinstance, MAKEINTRESOURCE(IDI_CHANNEL_VC)),
						RGB(255, 0, 255));
	ImageList_AddMasked(hChannelImgList,
						LoadBitmap(hinstance, MAKEINTRESOURCE(IDI_CHANNEL_ANC)),
						RGB(255, 0, 255));
	ImageList_AddMasked(
		hChannelImgList,
		LoadBitmap(hinstance, MAKEINTRESOURCE(IDI_CHANNEL_FORUM)),
		RGB(255, 0, 255));
	hInstance = hinstance;
	// register the window claws
	const char CLASS_NAME[] = "BackcordMain";

	WNDCLASS wc = {};

	wc.lpfnWndProc = WindowProc;
	wc.hInstance = hInstance;
	wc.lpszClassName = CLASS_NAME;
	wc.hCursor = LoadCursorA(NULL, IDC_ARROW);
	RegisterClass(&wc);
	hWnd = CreateWindowEx(0,				   // Optional window styles.
						  CLASS_NAME,		   // Window class
						  "Backcord",		   // Window text
						  WS_OVERLAPPEDWINDOW, // Window style

						  // Size and position
						  CW_USEDEFAULT, CW_USEDEFAULT, 800, 600,

						  NULL,		 // Parent window
						  NULL,		 // Menu
						  hInstance, // Instance handle
						  NULL		 // Additional application data
	);

	if (hWnd == NULL) {
		return 0;
	}
	ShowWindow(hWnd, nCmdShow);

	MSG msg = {};
	while (GetMessage(&msg, NULL, 0, 0) > 0) {
		TranslateMessage(&msg);
		DispatchMessage(&msg);
	}

	return 0;
}
BOOL identify = FALSE;
void onDiscordGuildLoad(DiscordGuild *guild, char *id) {

	GUIGuild uigld;
	uigld.icon = NULL;
	uigld.id = id;
	uigld.title = guild->name;
	uigld.data = guild;
	uigld.MentionCount = guild->MentionCount;
	GuildView_InsertGuild(hGSel, uigld);

	if (guild->IconHash) {
		char tempPath[MAX_PATH];
		DWORD len = GetTempPathA(MAX_PATH, tempPath);
		char *path = malloc(MAX_PATH + strlen(guild->id) + 1);
		wsprintfA(path, "%s%s.png", tempPath, guild->IconHash);
		if (GetFileAttributesA(path) == INVALID_FILE_ATTRIBUTES) {
			path = DiscordFetchTmpGuildIcon(guild->id,
											guild->IconHash); // testing
		}
		GuildView_SetIcon(hGSel, LoadPNGImage(path), id);
		free(path);
	}
}

HBITMAP ResizeBitmap(HBITMAP hOrig, int w, int h) {
	BITMAP bm;
	GetObject(hOrig, sizeof(bm), &bm);

	HDC hdcScreen = GetDC(NULL);
	HDC hdcSrc = CreateCompatibleDC(hdcScreen);
	HDC hdcDst = CreateCompatibleDC(hdcScreen);
	HBITMAP hResized = CreateCompatibleBitmap(hdcScreen, w, h);

	SelectObject(hdcSrc, hOrig);
	SelectObject(hdcDst, hResized);

	SetStretchBltMode(hdcDst, HALFTONE);
	SetBrushOrgEx(hdcDst, 0, 0, NULL);

	StretchBlt(hdcDst, 0, 0, w, h, hdcSrc, 0, 0, bm.bmWidth, bm.bmHeight,
			   SRCCOPY);

	DeleteDC(hdcSrc);
	DeleteDC(hdcDst);
	ReleaseDC(NULL, hdcScreen);
	DeleteObject(hOrig);

	return hResized;
}
void onDiscordReady(DiscordUser user, DiscordGuild *guilds, int guildscount) {
	if (user.avatar) {
		char tempPath[MAX_PATH];
		DWORD len = GetTempPathA(MAX_PATH, tempPath);
		char *path = malloc(MAX_PATH + strlen(user.id) + 1);
		wsprintfA(path, "%s%s.png", tempPath, user.avatar);
		if (GetFileAttributesA(path) == INVALID_FILE_ATTRIBUTES) {
			path = DiscordFetchTmpPfp(user.id,
									  user.avatar); // testing
		}

		RECT ClientRect;
		GetClientRect(hPfpAreaPfp, &ClientRect);
		int DestW = ClientRect.right - ClientRect.left;
		int DestH = ClientRect.bottom - ClientRect.top;

		SendMessage(hPfpAreaPfp, STM_SETIMAGE, IMAGE_BITMAP,
					(LPARAM)ResizeBitmap(LoadPNGBitmap(path), DestW, DestH));

		free(path);
	}
	if (user.DisplayName) {
		SetWindowText(hPfpAreaDispName, user.DisplayName);
		SetWindowText(hPfpAreaUserName, user.Username);
	} else {
		SetWindowText(hPfpAreaDispName, user.Username);
		ShowWindow(hPfpAreaUserName, SW_HIDE);
	}
}
void onDiscordUpdatedGuildReadState(DiscordGuild guild) {
	GuildView_SetMentionCount(hGSel, guild.id, guild.MentionCount);
}
static uint64_t channel_hash(const void *item, uint64_t seed0, uint64_t seed1) {
	const ChannelUIEntry *e = item;
	return hashmap_sip(e->id, strlen(e->id), seed0, seed1);
}
static int channel_compare(const void *a, const void *b, void *udata) {
	return strcmp(((ChannelUIEntry *)a)->id, ((ChannelUIEntry *)b)->id);
}

void onDiscordReceiveMessage(DiscordMessage msg) {
	if (curChannel && strcmp(msg.channelID, curChannel) == 0) {
		if (msg.author.avatar) {
			char tempPath[MAX_PATH];
			DWORD len = GetTempPathA(MAX_PATH, tempPath);
			char *path = malloc(MAX_PATH + strlen(msg.author.avatar) + 1);
			wsprintfA(path, "%s%s.png", tempPath, msg.author.avatar);
			if (GetFileAttributesA(path) == INVALID_FILE_ATTRIBUTES) {
				char *path = DiscordFetchTmpPfp(msg.author.id,
												msg.author.avatar); // testing
			}
			ChatView_SetUserPfp(path, msg.author.id);
		}
		InsertChatMessage(msg, hMsgList); // the backend provides strdup
	}
}
void *msgoldwndproc;
LRESULT CALLBACK MsgBarSubclassProc(HWND hwnd, UINT msg, WPARAM wParam,
									LPARAM lParam) {
	if (msg == WM_KEYDOWN) {
		if (wParam == VK_RETURN) {
			if (!(GetKeyState(VK_SHIFT) &
				  0x8000)) // if no shift then do cool stuff
			{
				char msgcontent[GetWindowTextLength(hMsg) + 1];
				GetWindowText(hMsg, msgcontent, GetWindowTextLength(hMsg) + 1);
				DiscordSendMessage(curChannel, msgcontent);

				SetWindowText(hwnd, ""); // clear input
				return 0;				 // preventing uhh newline
			}
		}
	}
	return CallWindowProc(msgoldwndproc, hwnd, msg, wParam, lParam);
}
LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam,
							LPARAM lParam) {
	switch (uMsg) {
	case WM_CREATE:

		NONCLIENTMETRICS ncm = {sizeof(NONCLIENTMETRICS)};
		SystemParametersInfo(SPI_GETNONCLIENTMETRICS, sizeof(ncm), &ncm, 0);
		HFONT SysFont = CreateFontIndirect(&ncm.lfMessageFont);
		ncm.lfMessageFont.lfWeight = FW_BOLD;
		HFONT BoldSysFont = CreateFontIndirect(&ncm.lfMessageFont);

		InitHTTP();

		RECT rc;
		GetClientRect(hwnd, &rc);
		hGSel = CreateWindowEx(0,				// Optional window styles.
							   "BackcordGuild", // Window class
							   "",				// Window text
							   WS_CHILD | WS_VISIBLE | LVS_LIST, // Window style

							   // Size and position
							   0, 0, 90, rc.bottom - rc.top - 80,

							   hwnd,	   // Parent window
							   (HMENU)102, // Menu
							   hInstance,  // Instance handle
							   NULL		   // Additional application data
		);
		GuildView_SetDMsIcon(
			hGSel, LoadPNGImageFromResource(hInstance, IDI_BACKCORDIC));
		hChTree = CreateWindowExA(WS_EX_CLIENTEDGE, WC_TREEVIEWA, NULL,
								  WS_CHILD | WS_VISIBLE | TVS_HASLINES |
									  TVS_LINESATROOT | TVS_HASBUTTONS,
								  90, 0, 128, rc.bottom - rc.top - 80, hwnd,
								  (HMENU)103, hInstance, NULL);
		TreeView_SetImageList(hChTree, hChannelImgList, 0);
		hDMsList = CreateWindowExA(WS_EX_CLIENTEDGE, WC_LISTVIEWA, NULL,
								   WS_CHILD | WS_VISIBLE | LVS_REPORT, 90, 0,
								   128, rc.bottom - rc.top - 80, hwnd,
								   (HMENU)106, hInstance, NULL);
		LVCOLUMN lvc = {0};
		lvc.mask = LVCF_WIDTH | LVCF_TEXT | LVCF_SUBITEM;
		lvc.cx = 128;
		lvc.pszText = "Direct Messages";
		lvc.iSubItem = 0;
		ListView_InsertColumn(hDMsList, 0, &lvc);
		ShowWindow(hDMsList, SW_HIDE);
		hMsgList = CreateWindowEx(0,			  // Optional window styles.
								  "BackcordChat", // Window class
								  "",			  // Window text
								  WS_CHILD | WS_VISIBLE, // Window style

								  // Size and position
								  75 + 128, 0, rc.right - 128 - 90,
								  rc.bottom - rc.top - 25,

								  hwnd,		  // Parent window
								  (HMENU)105, // Menu
								  hInstance,  // Instance handle
								  NULL);	  // Additional application data

		hPfpArea =
			CreateWindowEx(0,			   // Optional window styles.
						   "BUTTON",	   // Window class
						   "User Profile", // Window text
						   WS_CHILD | WS_VISIBLE | BS_GROUPBOX, // Window style

						   // Size and position
						   0, rc.bottom - rc.top - 80, 128 + 89, 80,

						   hwnd,	  // Parent window
						   NULL,	  // Menu
						   hInstance, // Instance handle
						   NULL		  // Additional application data
			);
		hPfpAreaPfp = CreateWindowExA(
			0, "STATIC", "", SS_BITMAP | WS_CHILD | WS_VISIBLE, 12, 24, 36, 36,
			hPfpArea, NULL, GetModuleHandle(NULL), NULL);
		hPfpAreaDispName = CreateWindowExA(
			0, "STATIC", "Display Name", WS_CHILD | WS_VISIBLE, 54, 24, 144, 16,
			hPfpArea, NULL, GetModuleHandle(NULL), NULL);
		SendMessage(hPfpAreaDispName, WM_SETFONT, (WPARAM)BoldSysFont, 0);
		hPfpAreaUserName = CreateWindowExA(
			0, "STATIC", "Username", WS_CHILD | WS_VISIBLE, 54, 38, 144, 16,
			hPfpArea, NULL, GetModuleHandle(NULL), NULL);
		SendMessage(hPfpAreaUserName, WM_SETFONT, (WPARAM)SysFont, 0);
		SendMessage(hPfpArea, WM_SETFONT, (WPARAM)regfont, 0);
		hMsg = CreateWindowEx(WS_EX_CLIENTEDGE, // Optional window styles.
							  "EDIT",			// Window class
							  "",				// Window text
							  ES_MULTILINE | ES_AUTOVSCROLL | WS_CHILD |
								  WS_VISIBLE, // Window style

							  // Size and position
							  128 + 89, rc.bottom - rc.top - 25,
							  rc.right - rc.left - 128 - 89 - 25, 25,

							  hwnd,		  // Parent window
							  (HMENU)104, // Menu
							  hInstance,  // Instance handle
							  NULL		  // Additional application data
		);
		SendMessage(hMsg, WM_SETFONT, (WPARAM)regfont, 0);
		msgoldwndproc = (void *)SetWindowLongPtr(hMsg, GWLP_WNDPROC,
												 (long)MsgBarSubclassProc);
		hSend = CreateWindowEx(
			0,									   // Optional window styles.
			"BUTTON",							   // Window class
			"E",								   // Window text
			BS_PUSHBUTTON | WS_CHILD | WS_VISIBLE, // Window style

			// Size and position
			rc.right - 25, rc.bottom - rc.top - 25, 25, 25,

			hwnd,		  // Parent window
			(HMENU)(101), // Menu
			hInstance,	  // Instance handle
			NULL		  // Additional application data
		);
		SendMessage(hSend, WM_SETFONT, (WPARAM)regfont, 0);
		OpenWebSocket("gateway.discord.gg", "/?encoding=json&v=9", &GatewaySSL);
		return 0;
		break;
	case WM_SIZE: {
		int width = LOWORD(lParam);
		int height = HIWORD(lParam);
		SetWindowPos(hPfpArea, NULL, 0, height - 80, 0, 0,
					 SWP_NOSIZE | SWP_NOZORDER);
		SetWindowPos(hGSel, NULL, 0, 0, 90, height - 80, SWP_NOZORDER);
		SetWindowPos(hMsg, NULL, 128 + 90, height - 25, width - 128 - 90 - 25,
					 25, SWP_NOZORDER);
		SetWindowPos(hChTree, NULL, 90, 0, 128, height - 80, SWP_NOZORDER);
		SetWindowPos(hMsgList, NULL, 128 + 90, 0, width - 128 - 90, height - 25,
					 SWP_NOZORDER);
		SetWindowPos(hSend, NULL, width - 25, height - 25, 25, 25,
					 SWP_NOZORDER | SWP_NOSIZE);
		return 0;
		break;
	}
	case WM_COMMAND:
		if (lParam == hSend) {
			char msgcontent[GetWindowTextLength(hMsg) + 1];
			GetWindowText(hMsg, msgcontent, GetWindowTextLength(hMsg) + 1);
			DiscordSendMessage(curChannel, msgcontent);
		}
		break;
	case WM_NOTIFY:
		NMHDR pNMHDR = *(NMHDR *)lParam;
		LPNMLVCUSTOMDRAW lvcd = (LPNMLVCUSTOMDRAW)lParam;
		if (pNMHDR.hwndFrom == hGSel) {
			LPNMGUILDVIEW pnmv = (LPNMGUILDVIEW)lParam;
			if (pnmv->index == GUILDVIEW_DMS) {
				ShowWindow(hChTree, HIDE_WINDOW);
				ShowWindow(hDMsList, 1);
				DiscordChannel *dms;
				int cnt = DiscordListPrivateChannels(&dms);
				ListView_DeleteAllItems(hDMsList);
				ListView_SetImageList(hDMsList, hSidePfps, LVSIL_SMALL);
				LVITEM lvItem = {0};
				int pfpidx = 0;
				for (int i = 0; i < cnt; i++) {
					if (dms[i].type == CHANNEL_DM) {
						printf("%s\n", dms[i].receipents[0].Username);

						if (dms[i].receipents[0].avatar) {

							char tempPath[MAX_PATH];
							DWORD len = GetTempPathA(MAX_PATH, tempPath);
							char path[MAX_PATH +
									  strlen(dms[i].receipents[0].avatar) + 1];
							wsprintfA(path, "%s%s.png", tempPath,
									  dms[i].receipents[0].avatar);

							if (GetFileAttributesA(path) ==
								INVALID_FILE_ATTRIBUTES) {
								char *path2 = DiscordFetchTmpPfp(
									dms[i].receipents[0].id,
									dms[i].receipents[0].avatar); // testing
								ImageList_Add(
									hSidePfps,
									ResizeBitmap(LoadPNGBitmap(path2), 32, 32),
									NULL);
								free(path2);
							} else {

								ImageList_Add(
									hSidePfps,
									ResizeBitmap(LoadPNGBitmap(path), 32, 32),
									NULL);
							}
						}

						lvItem.mask =
							LVIF_TEXT | LVIF_PARAM | LVIF_STATE | LVIF_IMAGE;
						lvItem.pszText = dms[i].receipents[0].DisplayName
											 ? dms[i].receipents[0].DisplayName
											 : dms[i].receipents[0].Username;
						lvItem.lParam = (LPARAM) & (dms[i]);
						lvItem.iImage =
							dms[i].receipents[0].avatar ? pfpidx : 0;
						ListView_InsertItem(hDMsList, &lvItem);
						if (dms[i].receipents[0].avatar) {
							pfpidx++;
						}
					}
				}
			} else {
				ShowWindow(hChTree, 1);
				ShowWindow(hDMsList, HIDE_WINDOW);
				DiscordGuild *guild;
				guild = pnmv->guild.data;

				DiscordChannel *Channels;
				int ChannelCount =
					DiscordListGuildChannels(guild->id, &Channels);
				TreeView_SelectItem(hChTree, NULL);
				TreeView_DeleteAllItems(hChTree);
				// TreeView_SetImageList(hChTree, hChannelImgList,
				// TVSIL_NORMAL);
				struct hashmap *ChannelUITable =
					hashmap_new(sizeof(ChannelUIEntry), 0, 0, 0, channel_hash,
								channel_compare, NULL, NULL);
				for (int i = 0; i < ChannelCount; i++) {
					char *VisualName;
					VisualName = malloc(strlen(Channels[i].name) + 1);
					strcpy(VisualName, Channels[i].name);
					TVINSERTSTRUCT tvInsert = {0};
					HTREEITEM hParent = TVI_ROOT;
					if (Channels[i].parentID) {
						ChannelUIEntry key = {0};
						key.id = Channels[i].parentID;
						if (hashmap_get(ChannelUITable, &key)) {
							ChannelUIEntry *chnl =
								(ChannelUIEntry *)(hashmap_get(ChannelUITable,
															   &key));
							hParent = chnl->itm;
						}
					}
					tvInsert.hParent = hParent; // Parent is the root item
					tvInsert.hInsertAfter = TVI_LAST;
					tvInsert.item.mask =
						TVIF_TEXT | TVIF_PARAM | TVIF_STATE | TVIF_IMAGE;
					tvInsert.item.pszText = (VisualName);
					tvInsert.item.lParam = (LPARAM) & (Channels[i]);
					tvInsert.item.state = TVIS_EXPANDED;
					tvInsert.item.stateMask = TVIS_EXPANDED;
					if (Channels[i].type == GUILD_CATEGORY) {
						tvInsert.item.iImage = 0;
					} else if (Channels[i].type == GUILD_VOICE) {
						tvInsert.item.iImage = 2;
					} else if (Channels[i].type == GUILD_ANNOUNCEMENT) {
						tvInsert.item.iImage = 3;
					} else if (Channels[i].type == GUILD_FORUM) {
						tvInsert.item.iImage = 4;
					} else {
						tvInsert.item.iImage = 1;
					}
					HTREEITEM lres = (HTREEITEM)SendMessage(
						hChTree, TVM_INSERTITEM, 0, (LPARAM)&tvInsert);
					if (Channels[i].type == GUILD_CATEGORY) {
						ChannelUIEntry *chnl = malloc(sizeof(ChannelUIEntry));
						chnl->itm = lres;
						chnl->id = strdup(Channels[i].id);
						hashmap_set(ChannelUITable, chnl);
					}
				}
			}
		} else if (pNMHDR.hwndFrom == hChTree) {
			LPNMTREEVIEW pnmv = (LPNMTREEVIEW)lParam;
			if (pNMHDR.code == TVN_SELCHANGED &&
				pnmv->itemNew.state & TVIS_SELECTED) {
				DiscordChannel *chnl = ((DiscordChannel *)pnmv->itemNew.lParam);
				if (chnl->type != GUILD_CATEGORY) {
					char *title = NULL;
					if (chnl->type == CHANNEL_DM) {
						title = malloc(12 +
									   strlen(chnl->receipents[0].DisplayName));
						wsprintf(title, "Backcord - %s",
								 chnl->receipents[0].DisplayName);
						SetWindowTextA(hwnd, title);
					} else {
						title = malloc(13 + strlen(chnl->name));
						wsprintf(title, "Backcord - #%s", chnl->name);
						SetWindowTextA(hwnd, title);
					}
					DiscordMessage *msgs;
					int msgcnt = DiscordGetChannelHistory(chnl->id, 50, &msgs);
					ClearChatControl(hMsgList);
					ListView_DeleteAllItems(hMsgList);
					if (curChannel)
						free(curChannel);

					curChannel = strdup(chnl->id);
					if (msgcnt > 0) {
						for (int i = msgcnt - 1; i >= 0; i--) {
							if (msgs[i].author.avatar) {
								char tempPath[MAX_PATH];
								DWORD len = GetTempPathA(MAX_PATH, tempPath);
								char *path =
									malloc(MAX_PATH +
										   strlen(msgs[i].author.avatar) + 1);
								wsprintfA(path, "%s%s.png", tempPath,
										  msgs[i].author.avatar);
								if (GetFileAttributesA(path) ==
									INVALID_FILE_ATTRIBUTES) {
									char *path = DiscordFetchTmpPfp(
										msgs[i].author.id,
										msgs[i].author.avatar); // testing
								}
								ChatView_SetUserPfp(path, msgs[i].author.id);
							}
							InsertChatMessage((msgs[i]), hMsgList);
						}
					}
					ListView_EnsureVisible(hMsgList, msgcnt - 1, FALSE);
					free(title);
					free(msgs);
				}
			}
			break;
		}
		break;
	case WM_PAINT: {
		PAINTSTRUCT ps;
		HDC hdc = BeginPaint(hwnd, &ps);

		// All painting occurs here, between BeginPaint and EndPaint.

		FillRect(hdc, &ps.rcPaint, (HBRUSH)(COLOR_BTNFACE + 1));
		EndPaint(hwnd, &ps);

		return 0;
		break;
	}
	case WM_CLOSE:
		DestroyWindow(hwnd);
		PostQuitMessage(0);
	case WM_DESTROY:
		CleanupHTTP();
		return TRUE;
		break;
	}
	return DefWindowProc(hwnd, uMsg, wParam, lParam);
}