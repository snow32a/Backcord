#include <windows.h>
#include "../hmap/hashmap.h"

#define GVN_ITEMCLICK   (0U - 1800U)
#define GUILDVIEW_MAGIC 0xACC0
#define GUILDVIEW_DMS -1
typedef struct {
	char *title;
	char *id;
	HDC icon;
	int MentionCount;
	void* data;
} GUIGuild;
typedef struct {
    NMHDR hdr;
	int index;
	GUIGuild guild;
    POINT pt;
} NMGUILDVIEW;
typedef NMGUILDVIEW* LPNMGUILDVIEW;
typedef struct {
    GUIGuild* uiglds;
	int uigldcnt;
	HWND hWnd;
	int scrollOffset;
	int contentHeight;
	HDC dmsicon;
	int selIndex;
} GuildWnd;

LRESULT CALLBACK GuildWndProc(HWND hwnd, UINT uMsg, WPARAM wParam,
                              LPARAM lParam);

void GuildView_InsertGuild(HWND hwnd, GUIGuild gld);
void GuildView_SetIcon(HWND hwnd, HDC icon, char *id);
void GuildView_SetDMsIcon(HWND hwnd, HDC icon);
void GuildView_SetMentionCount(HWND hwnd, char *id, int cnt);