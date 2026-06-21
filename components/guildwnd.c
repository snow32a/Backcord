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
			guildwnd->uiglds[i].icon = icon;
			InvalidateRect(hwnd, NULL, TRUE);
			break;
		}
	}
}
void GuildView_SetMentionCount(HWND hwnd, char *id, int cnt) {
	GuildWnd *guildwnd = GetGuildControlDetails(hwnd);
	for (int i = 0; i < guildwnd->uigldcnt; i++) {
		if (strcmp(guildwnd->uiglds[i].id, id) == 0) {
			guildwnds->uiglds[i].MentionCount = cnt;
			InvalidateRect(hwnd, NULL, TRUE);
			break;
		}
	}
	InvalidateRect(hwnd, NULL, FALSE);
}
void GuildView_SetDMsIcon(HWND hwnd, HDC icon) {
	GuildWnd *guildwnd = GetGuildControlDetails(hwnd);
	guildwnd->dmsicon = icon;
	InvalidateRect(hwnd, NULL, TRUE);
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
		guildwnds[guildwndcount - 1].GuildSize = 48;
		guildwnds[guildwndcount - 1].selIndex = -2;
		SetWindowLongPtr(hwnd, GWL_STYLE,
						 GetWindowLongPtr(hwnd, GWL_STYLE) | WS_VSCROLL);
		SetWindowPos(hwnd, NULL, 0, 0, 0, 0,
					 SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER | SWP_FRAMECHANGED);
		return 0;
		break;
	case WM_PAINT: {
		RECT ctlrect;
		GetClientRect(hwnd, &ctlrect);
		GuildWnd *gwnd = GetGuildControlDetails(hwnd);
		PAINTSTRUCT ps;
		HDC hdc = BeginPaint(hwnd, &ps);
		FillRect(hdc, &ctlrect, (HBRUSH)(COLOR_3DFACE + 1));
		int starty;
		int x =
			(ctlrect.right - ctlrect.left) /
				2 -
			gwnd->GuildSize / 2;
		SetStretchBltMode(hdc, HALFTONE);
		int dmregy = 8 + gwnd->GuildSize + 8 + 10;
		int hry = 8 + gwnd->GuildSize + 8;
		int dmregstarty = 8;

		HBITMAP hdmsbmp = GetCurrentObject(gwnd->dmsicon, OBJ_BITMAP);
		BITMAP dmsbmp;
		GetObject(hdmsbmp, sizeof(BITMAP), &dmsbmp);

		StretchBlt(hdc, x, dmregstarty-gwnd->scrollOffset,
				   gwnd->GuildSize, gwnd->GuildSize, gwnd->dmsicon, 0, 0, dmsbmp.bmWidth, dmsbmp.bmHeight,
				   SRCCOPY);
		COLORREF hrup = GetSysColor(COLOR_3DHILIGHT);
		COLORREF hrdown = GetSysColor(COLOR_3DSHADOW);
		HPEN penhrup = CreatePen(PS_SOLID, 1, hrup);
		HPEN penhrdown = CreatePen(PS_SOLID, 1, hrdown);
		HPEN oldPen = SelectPen(hdc, penhrdown);

        MoveToEx(hdc, x, hry, NULL);
		LineTo(hdc, x + gwnd->GuildSize, hry);

		SelectPen(hdc, penhrup);

        MoveToEx(hdc, x, hry+1, NULL);
		LineTo(hdc, x + gwnd->GuildSize, hry + 1);
		SelectPen(hdc,oldPen);


		int gldstarty=dmregy-gwnd->scrollOffset;
		for (int i = 0; i < gwnd->uigldcnt; i++) {
			GUIGuild gld = gwnd->uiglds[i];
			if (gwnd->selIndex == i) {
				RECT lprc;
				lprc.left = 0;
				lprc.right = ctlrect.right;
				lprc.top = (gwnd->GuildSize + 8) * i + gldstarty-4;
				lprc.bottom = lprc.top + gwnd->GuildSize+8;
				FillRect(hdc, &lprc, (HBRUSH)(COLOR_HIGHLIGHT + 1));
			}
			if (gld.icon) {
				int srcw, srch;
				HBITMAP hbmp = GetCurrentObject(gld.icon, OBJ_BITMAP);
				BITMAP bmp;
				GetObject(hbmp, sizeof(BITMAP), &bmp);
				srcw = bmp.bmWidth;
				srch = bmp.bmHeight;
				StretchBlt(hdc, x, (gwnd->GuildSize+8) * i + gldstarty, gwnd->GuildSize,
						   gwnd->GuildSize, gld.icon, 0, 0, srcw, srch,
						   SRCCOPY);
			} else {
				RECT lprc;
				lprc.left = x;
				lprc.right = x + gwnd->GuildSize;
				lprc.top = (gwnd->GuildSize + 8) * i + gldstarty;
				lprc.bottom = lprc.top + gwnd->GuildSize;

				FillRect(hdc, &lprc, (HBRUSH)(COLOR_3DSHADOW + 1));
				char *initials = malloc(2);
				initials[0] = gld.title[0];
				int sizeofinitials = 1;
				for (char *c = gld.title + 1; *c; c++) {
					if (*(c - 1) == ' ') {
						sizeofinitials++;
						initials=realloc(initials, sizeofinitials+1);
						initials[sizeofinitials-1]=*c;
					}
				}
				SetBkMode(hdc, TRANSPARENT);
				SetTextColor(hdc, RGB(255, 255, 255));
				SetBkColor(hdc,TRANSPARENT);
				DrawTextA(hdc,initials,sizeofinitials,&lprc,DT_SINGLELINE | DT_VCENTER | DT_CENTER);
				free(initials);
			}
		}

		EndPaint(hwnd, &ps);


		// Set scroll stuff

		int contentHeight = gwnd->uigldcnt * (gwnd->GuildSize+8)+dmregy;

		RECT rc;
		GetClientRect(hwnd, &rc);

		SCROLLINFO si = {sizeof(si), SIF_RANGE | SIF_PAGE | SIF_POS,
						 0,			 contentHeight - 1,
						 rc.bottom,	 gwnd->scrollOffset};

		SetScrollInfo(hwnd, SB_VERT, &si, TRUE);

		return 0;
		break;
	}
	case WM_VSCROLL:
	{
		GuildWnd *gwnd = GetGuildControlDetails(hwnd);
		SCROLLINFO si = { sizeof(si), SIF_ALL };
		GetScrollInfo(hwnd, SB_VERT, &si);
	
		switch (LOWORD(wParam))
		{
			case SB_LINEUP:      si.nPos -= 20; break;
			case SB_LINEDOWN:    si.nPos += 20; break;
			case SB_PAGEUP:      si.nPos -= si.nPage; break;
			case SB_PAGEDOWN:    si.nPos += si.nPage; break;
			case SB_THUMBTRACK:  si.nPos = si.nTrackPos; break;
		}
	
		SetScrollInfo(hwnd, SB_VERT, &si, TRUE);
		GetScrollInfo(hwnd, SB_VERT, &si);
	
		gwnd->scrollOffset = si.nPos;
		InvalidateRect(hwnd, NULL, FALSE);
		return 0;
	}
	case WM_LBUTTONUP: {
		GuildWnd *gwnd = GetGuildControlDetails(hwnd);
		int x = GET_X_LPARAM(lParam);
		int y = GET_Y_LPARAM(lParam)+gwnd->scrollOffset;
	
		NMGUILDVIEW nm = {0};
	
		nm.hdr.hwndFrom = hwnd;
		nm.hdr.idFrom   = GetDlgCtrlID(hwnd);
		nm.hdr.code     = GVN_ITEMCHANGED;

		if (y < 8 + gwnd->GuildSize && gwnd->selIndex != GUILDVIEW_DMS) {
			gwnd->selIndex = GUILDVIEW_DMS;
			nm.index = GUILDVIEW_DMS;
			nm.guild = (GUIGuild){0};
		} else if (y > 8 + gwnd->GuildSize + 8 + 10) {
			nm.index = (y - 16 - 10 - gwnd->GuildSize) / (gwnd->GuildSize + 8);
			if (nm.index == gwnd->selIndex) {
				return 0;
			}
			gwnd->selIndex=nm.index;
			nm.guild = gwnd->uiglds[nm.index];
		} else {
			return 0;
		}
		nm.pt.x = x;
		nm.pt.y = y;

		SendMessage(GetParent(hwnd), WM_NOTIFY, nm.hdr.idFrom, (LPARAM)&nm);
		InvalidateRect(hwnd, NULL, true);
		return 0;
	}
	}

	return DefWindowProc(hwnd, uMsg, wParam, lParam);
}