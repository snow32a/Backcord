#include "guildwnd.h"
#include <libpng18/png.h>
#include <stdbool.h>
#include "pnghlp.h"
#include <windows.h>
#include <windowsx.h>
#include <wingdi.h>
int guildwndcount = 0;
GuildWnd *guildwnds = NULL;
GuildWnd *GetGuildControlDetails(HWND hwnd) {
	for (int i = 0; i < guildwndcount; i++) {
		if (guildwnds[i].hWnd == hwnd) {
			return &guildwnds[i];
		}
	}
}
void GuildView_InsertGuild(HWND hwnd, GUIGuild gld) {
	GuildWnd *guildwnd = GetGuildControlDetails(hwnd);
	guildwnd->uigldcnt++;
	guildwnd->uiglds =
		realloc(guildwnd->uiglds, guildwnd->uigldcnt * sizeof(GUIGuild));
	GUIGuild toins = gld;
	toins.title = strdup(toins.title);
	toins.id = strdup(toins.id);
	guildwnd->uiglds[guildwnd->uigldcnt - 1] = toins;
	InvalidateRect(hwnd, NULL, TRUE);
}

void GuildView_SetIcon(HWND hwnd, HDC icon, char *id) {
	GuildWnd *guildwnd = GetGuildControlDetails(hwnd);
	for (int i = 0; i < guildwnd->uigldcnt; i++) {
		if (strcmp(guildwnd->uiglds[i].id, id) == 0) {
			guildwnds->uiglds[i].icon = icon;
			break;
		}
	}
}
void GuildView_SetMentionCount(HWND hwnd, char *id, int cnt) {
	GuildWnd *guildwnd = GetGuildControlDetails(hwnd);
	for (int i = 0; i < guildwnd->uigldcnt; i++) {
		if (strcmp(guildwnd->uiglds[i].id, id) == 0) {
			guildwnds->uiglds[i].MentionCount = cnt;
			break;
		}
	}
	InvalidateRect(hwnd, NULL, FALSE);
}
void GuildView_SetDMsIcon(HWND hwnd, HDC icon) {
	GuildWnd *guildwnd = GetGuildControlDetails(hwnd);
	guildwnds->dmsicon = icon;
}

LRESULT CALLBACK GuildWndProc(HWND hwnd, UINT uMsg, WPARAM wParam,
							  LPARAM lParam) {
	switch (uMsg) {
	case WM_CREATE:
		guildwndcount++;
		guildwnds = realloc(guildwnds, guildwndcount * sizeof(GuildWnd));
		guildwnds[guildwndcount - 1].uigldcnt = 0;
		guildwnds[guildwndcount - 1].uiglds = NULL;
		guildwnds[guildwndcount - 1].hWnd = hwnd;
		guildwnds[guildwndcount - 1].scrollOffset = 0;
		return 0;
		break;
	case WM_PAINT: {
		RECT ctlrect;
		GetClientRect(hwnd, &ctlrect);
		GuildWnd *gwnd = GetGuildControlDetails(hwnd);
		PAINTSTRUCT ps;
		HDC hdc = BeginPaint(hwnd, &ps);
		FillRect(hdc, &ctlrect, (HBRUSH)(COLOR_3DFACE + 1));
		SetStretchBltMode(hdc, HALFTONE);
		RECT rc = ctlrect;
		int ctlw = ctlrect.right - ctlrect.left;
		int guildsize = ctlw - 16;

		rc.left += 8;
		rc.right = rc.left + guildsize;
		rc.top += 8;
		rc.bottom = rc.top + guildsize;

		if (gwnd->dmsicon) {

			BITMAP bmpInfo;
			HBITMAP hSrcBmp =
				(HBITMAP)GetCurrentObject(gwnd->dmsicon, OBJ_BITMAP);
			GetObject(hSrcBmp, sizeof(BITMAP), &bmpInfo);

			StretchBlt(hdc, rc.left, rc.top, guildsize, guildsize,
					   gwnd->dmsicon, 0, 0, bmpInfo.bmWidth, bmpInfo.bmHeight,
					   SRCCOPY);
		} else {
			FillRect(hdc, &rc, (HBRUSH)(COLOR_3DSHADOW + 1));
		}
		HPEN hPenDark = CreatePen(PS_SOLID, 1, GetSysColor(COLOR_BTNSHADOW));
		HPEN hPenLight =
			CreatePen(PS_SOLID, 1, GetSysColor(COLOR_BTNHIGHLIGHT));
		HPEN hOldPen;

		hOldPen = SelectObject(hdc, hPenDark);
		MoveToEx(hdc, rc.left, rc.top + guildsize + 8, NULL);
		LineTo(hdc, rc.right, rc.top + guildsize + 8);

		SelectObject(hdc, hPenLight);
		MoveToEx(hdc, rc.left, rc.top + guildsize + 1 + 8, NULL);
		LineTo(hdc, rc.right, rc.top + guildsize + 1 + 8);

		SelectObject(hdc, hOldPen);
		DeleteObject(hPenDark);
		DeleteObject(hPenLight);

		for (int i = 0; i < gwnd->uigldcnt; i++) {
			RECT rc = ctlrect;
			rc.left += 8;
			rc.right = rc.left + guildsize;
			rc.top += i * (guildsize + 8) + 8 + guildsize + 10 + 8;
			rc.bottom = rc.top + guildsize;
			if (i == gwnd->selIndex) {
				RECT rcs = rc;
				rcs.left = ctlrect.left;
				rcs.right = ctlrect.right;
				rcs.top -= 4;
				rcs.bottom += 4;
				FillRect(hdc, &rcs, (HBRUSH)(COLOR_HIGHLIGHT + 1));
			}
			if (gwnd->uiglds[i].icon) {

				BITMAP bmpInfo;
				HBITMAP hSrcBmp =
					(HBITMAP)GetCurrentObject(gwnd->uiglds[i].icon, OBJ_BITMAP);
				GetObject(hSrcBmp, sizeof(BITMAP), &bmpInfo);

				StretchBlt(hdc, rc.left, rc.top, guildsize, guildsize,
						   gwnd->uiglds[i].icon, 0, 0, bmpInfo.bmWidth,
						   bmpInfo.bmHeight, SRCCOPY);
			} else {
				FillRect(hdc, &rc, (HBRUSH)(COLOR_3DSHADOW + 1));
			}
			if (gwnd->uiglds[i].MentionCount) {
				HBRUSH indicatorbrush = CreateSolidBrush((COLORREF)0x000000FF);

				RECT rc = ctlrect;
				rc.left += 8;
				rc.right = rc.left + guildsize;
				rc.top += i * (guildsize + 8) + 8 + guildsize + 10 + 8;
				rc.bottom = rc.top + guildsize;

				rc.top = i * (guildsize + 8) + 8 + guildsize + 10 + 8 + guildsize - 16;
				rc.left = rc.left + guildsize - 16;
				FillRect(hdc, &rc, indicatorbrush);
				char unreadtxt[16];
				wsprintf(unreadtxt,"%i",gwnd->uiglds[i].MentionCount);
				DrawText(hdc,unreadtxt,strlen(unreadtxt),&rc,DT_SINGLELINE | DT_CENTER | DT_VCENTER);
			}
		}
		EndPaint(hwnd, &ps);
		return 0;
	}
	case WM_LBUTTONUP: {
		GuildWnd *gwnd = GetGuildControlDetails(hwnd);
		RECT ctlrect;
		GetClientRect(hwnd, &ctlrect);
		int ctlw = ctlrect.right - ctlrect.left;
		int guildsize = ctlw - 16;
		if (GET_Y_LPARAM(lParam) < guildsize + 10 + 8) {
			if (gwnd->selIndex != 1) {
				gwnd->selIndex = -1;

				NMGUILDVIEW nm = {0};
				nm.hdr.hwndFrom = hwnd;
				nm.hdr.idFrom = GetDlgCtrlID(hwnd);
				nm.hdr.code = GVN_ITEMCLICK;
				nm.index = GUILDVIEW_DMS;
				nm.guild = (GUIGuild){NULL};
				nm.pt = (POINT){GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam)};

				SendMessage(GetParent(hwnd), WM_NOTIFY, (WPARAM)nm.hdr.idFrom,
							(LPARAM)&nm);
				InvalidateRect(hwnd, NULL, FALSE);
			}
		} else {
			int index = (GET_Y_LPARAM(lParam) - 8 - guildsize - 10 - 8) /
						(guildsize + 8);
			if (index != gwnd->selIndex) {
				NMGUILDVIEW nm = {0};
				nm.hdr.hwndFrom = hwnd;
				nm.hdr.idFrom = GetDlgCtrlID(hwnd);
				nm.hdr.code = GVN_ITEMCLICK;
				nm.index = index;
				nm.guild = gwnd->uiglds[index];
				nm.pt = (POINT){GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam)};
				gwnd->selIndex = index;
				SendMessage(GetParent(hwnd), WM_NOTIFY, (WPARAM)nm.hdr.idFrom,
							(LPARAM)&nm);
				InvalidateRect(hwnd, NULL, FALSE);
			}
		}
		return 0;
	}
	}

	return DefWindowProc(hwnd, uMsg, wParam, lParam);
}