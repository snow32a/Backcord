#include "chatwnd.h"
#include <stdbool.h>
#include <string.h>
#include <windows.h>
#include "snoctrl.h"
#include "../discordtypes.h"
#include "../hmap/hashmap.h"
#include <libpng18/png.h>
#include <wingdi.h>
#include "pnghlp.h"
ChatWnd *chatwnds = NULL;
int chatwndcount = 0;

HFONT regfont;
HFONT boldfont;
HFONT h1font;
HFONT h2font;
HFONT h3font;
hmap pfp_map;
ChatWnd *GetChatControlDetails(HWND hwnd) {
	for (int i = 0; i < chatwndcount; i++) {
		if (chatwnds[i].hWnd == hwnd) {
			return &chatwnds[i];
		}
	}
	return NULL;
}
void ChatView_SetUserPfp(char *path, char *user) {
	HANDLE f = CreateFileA(path, GENERIC_READ, FILE_SHARE_READ, NULL,
	                       OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
	if (f == INVALID_HANDLE_VALUE) {
		MessageBoxA(NULL, "Error accessing the path for the pfp set!",
		            "Snow's Controls - ChatView", 0);
		return;
	}
	LARGE_INTEGER size;
	GetFileSizeEx(f, &size);

	size_t file_size = (size_t)size.QuadPart;

	uint8_t *buf = malloc(file_size);

	DWORD read = 0;

	ReadFile(f, buf, file_size, &read, NULL);

	png_structp png =
	    png_create_read_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
	png_infop info = png_create_info_struct(png);
	PngBuffer pngbuf = {buf, file_size, 0};

	png_set_read_fn(png, &pngbuf, PngReadBufCB);
	if (setjmp(png_jmpbuf(png))) {
		png_destroy_read_struct(&png, &info, NULL);
		// err
		MessageBoxA(NULL, "zzzzzzzz", "zzzzzzzzz", 0);
	}
	png_read_info(png, info);

	BITMAPINFO bmi = {0};
	bmi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
	bmi.bmiHeader.biWidth = png_get_image_width(png, info);
	bmi.bmiHeader.biHeight = -png_get_image_height(png, info); // top-down
	bmi.bmiHeader.biPlanes = 1;
	bmi.bmiHeader.biBitCount = 32;
	bmi.bmiHeader.biCompression = BI_RGB; // THIS IS RAW - gordon ramsey
	png_set_expand(png);
	png_set_strip_16(png);
	png_set_filler(png, 0xFF,
	               PNG_FILLER_AFTER); // ensures RGBA or RGBX cause winblows
	png_read_update_info(png, info);
	int width = png_get_image_width(png, info);
	int height = png_get_image_height(png, info);
	png_bytep *rows = malloc(height * sizeof(png_bytep));
	for (int y = 0; y < height; y++)
		rows[y] = malloc(png_get_rowbytes(png, info));
	// flat BGRA buffer for GDI
	uint8_t *fbuf = malloc(width * height * 4);
	png_read_image(png, rows);

	for (int y = 0; y < height; y++) {
		png_bytep src = rows[y];
		uint8_t *dst = fbuf + y * width * 4;

		for (int x = 0; x < width; x++) {
			dst[x * 4 + 0] = src[x * 4 + 2]; // B
			dst[x * 4 + 1] = src[x * 4 + 1]; // G
			dst[x * 4 + 2] = src[x * 4 + 0]; // R
			dst[x * 4 + 3] = src[x * 4 + 3]; // A
		}
	}
	void *gdibuf = NULL;
	HBITMAP bmp =
	    CreateDIBSection(NULL, &bmi, DIB_RGB_COLORS, &gdibuf, NULL, 0);
	// GDI allocates its own dumbahh buffer so we just memcpy to it
	memcpy(gdibuf, fbuf, width * height * 4);
	free(fbuf);
	HDC memdc = CreateCompatibleDC(NULL);
	SelectObject(memdc, bmp);

	IntPfpTableItem it;
	it.key = user;
	it.bmp = memdc;

	hashmap_set(pfp_map, &it);

	CloseHandle(f);
}
void InsertChatMessage(DiscordMessage msg, HWND hwnd) {
	ChatWnd *cwnd = GetChatControlDetails(hwnd);
	cwnd->uimsgs =
	    realloc(cwnd->uimsgs, (cwnd->uimsgcnt + 1) * sizeof(GUIMessage));
	cwnd->uimsgs[cwnd->uimsgcnt].msg = msg;
	cwnd->uimsgs[cwnd->uimsgcnt].vw = hwnd;
	cwnd->uimsgcnt++;

	InvalidateRect(hwnd, NULL, FALSE);
	UpdateWindow(hwnd); // forces WM_PAINT now, contentHeight is fresh

	RECT clientRect;
	GetClientRect(hwnd, &clientRect);
	int viewHeight = clientRect.bottom - clientRect.top;

	cwnd->scrollOffset = max(0, cwnd->contentHeight - viewHeight);

	SCROLLINFO si = {sizeof(SCROLLINFO)};
	si.fMask = SIF_POS;
	si.nPos = cwnd->scrollOffset;
	SetScrollInfo(hwnd, SB_VERT, &si, TRUE);

	InvalidateRect(hwnd, NULL, FALSE); // repaint with correct offset
}
void ClearChatControl(HWND hwnd) {
	ChatWnd *cwnd = GetChatControlDetails(hwnd);
	int count = cwnd->uimsgcnt;
	cwnd->uimsgcnt = 0;
	for (int i = 0; i < count; i++) {
		DiscordMessage hmsg = (DiscordMessage)cwnd->uimsgs[i].msg;
		free(hmsg.content);
		free(hmsg.channelID);
		free(hmsg.id);
		free(hmsg.author.Username);
		free(hmsg.author.DisplayName);
		free(hmsg.author.id);
		free(hmsg.author.avatar);
	}
	cwnd->uimsgs = realloc(cwnd->uimsgs, sizeof(GUIMessage));
}
LONG MeasureTotalMessageHeight(HWND hwnd) {}
static uint64_t pfp_hash(const void *item, uint64_t seed0, uint64_t seed1) {
	const IntPfpTableItem *e = item;
	return hashmap_sip(e->key, strlen(e->key), seed0, seed1);
}
static int pfp_compare(const void *a, const void *b, void *udata) {
	return strcmp(((IntPfpTableItem *)a)->key, ((IntPfpTableItem *)b)->key);
}
static void pfp_free(void *pfpitem) {
	IntPfpTableItem *item = pfpitem;
	DeleteDC(item->bmp);
	free(item->key);
}
#define UsernameFieldHeight 15
#define PfpPadding 6
#define YPadding 4
#define PfpSize 40
#define MinMsgH PfpSize + 10
/*
        | X
   +------------+--------------------------------------------------------+
   |            |
   |            |
-X |            |
   |            |
   |            |
   +------------+
         |




*/
BOOL IsStartMsg(ChatWnd *cwnd, int i) {
	if (i > 0) {
		return (strcmp(cwnd->uimsgs[i].msg.author.id,
		               cwnd->uimsgs[i - 1].msg.author.id) == 0)
		           ? FALSE
		           : TRUE;
	} else {
		return TRUE;
	}
}

LRESULT CALLBACK ChatWndProc(HWND hwnd, UINT uMsg, WPARAM wParam,
                             LPARAM lParam) {
	switch (uMsg) {
	case WM_CREATE: {
		chatwnds = realloc(chatwnds, (chatwndcount + 1) * sizeof(ChatWnd));
		chatwnds[chatwndcount].hWnd = hwnd;
		chatwnds[chatwndcount].uimsgcnt = 0;
		chatwnds[chatwndcount].uimsgs = malloc(sizeof(GUIMessage));
		chatwnds[chatwndcount].scrollOffset = 0;
		chatwnds[chatwndcount].contentHeight = 0;
		chatwnds[chatwndcount].pfpmap = pfp_map =
		    hashmap_new(sizeof(IntPfpTableItem), 256, 0, 0, pfp_hash,
		                pfp_compare, pfp_free, NULL);
		chatwndcount++;
		break;
	}
	case WM_PAINT: {
		ChatWnd *cwnd = GetChatControlDetails(hwnd);
		PAINTSTRUCT ps;
		HDC hdc = BeginPaint(hwnd, &ps);

		RECT clientRect;
		GetClientRect(hwnd, &clientRect);
		int viewHeight = clientRect.bottom - clientRect.top;

		FillRect(hdc, &ps.rcPaint, (HBRUSH)(COLOR_WINDOW + 1));
		SetStretchBltMode(hdc, HALFTONE);
		SetBrushOrgEx(hdc, 0, 0, NULL);

		typedef struct {
			unsigned int TextContentHeight;
			unsigned int UnpaddedMessageHeight;
			unsigned int ClampedUnpaddedMessageHeight;
			unsigned int UnclampedTotalMessageHeight;
			unsigned int TotalMessageHeight;
			unsigned int MessageHeightForUse;
			BOOL StartMsg;
			BOOL PushBack;
		} MessageDetails;
		cwnd->contentHeight = 0;
		MessageDetails msgdet[cwnd->uimsgcnt];
		for (int i = 0; i < cwnd->uimsgcnt; i++) {
			RECT contr = {0, 0, clientRect.right - clientRect.left - 6 - 6 - 40,
			              0};
			DrawTextA(hdc, cwnd->uimsgs[i].msg.content,
			          strlen(cwnd->uimsgs[i].msg.content), &contr,
			          DT_CALCRECT | DT_WORDBREAK);
			// printf("%i | %i\n", i, contr.bottom - contr.top);
			msgdet[i].TextContentHeight = contr.bottom - contr.top;
			msgdet[i].StartMsg = IsStartMsg(cwnd, i);
			if (msgdet[i].StartMsg) {

				if (i + 1 < cwnd->uimsgcnt) {
					if (!IsStartMsg(cwnd, i + 1)) {
						msgdet[i].PushBack = TRUE;
					} else {
						msgdet[i].PushBack = FALSE;
					}
				} else {
					msgdet[i].PushBack = FALSE;
				}

				if (msgdet[i].PushBack) {
					// 15 is for the username field
					msgdet[i].UnpaddedMessageHeight =
					    15 + msgdet[i].TextContentHeight;

					msgdet[i].TotalMessageHeight =
					    15 + msgdet[i].TextContentHeight;
				} else {

					msgdet[i].UnpaddedMessageHeight =
					    max(40, 15 + msgdet[i].TextContentHeight);

					msgdet[i].TotalMessageHeight =
					    max(40, 15 + msgdet[i].TextContentHeight)+6;
				}

			} else {
				msgdet[i].TotalMessageHeight = msgdet[i].TextContentHeight ;

				msgdet[i].PushBack = FALSE;
			}
			cwnd->contentHeight += msgdet[i].TotalMessageHeight;
		}

		RECT msgrect;
		msgrect = clientRect;
		msgrect.top -= cwnd->scrollOffset;
		for (int i = 0; i < cwnd->uimsgcnt; i++) {

			if (msgrect.top + msgdet[i].TotalMessageHeight < 0) {
				msgrect.top += msgdet[i].TotalMessageHeight;
				continue;
			}
			if (msgrect.top > clientRect.bottom) {
				break;
			}
			DiscordMessage msg = cwnd->uimsgs[i].msg;
			if (msgdet[i].StartMsg) {
				RECT pfprect = msgrect;
				pfprect.left += 6;
				// pfprect.top += 6;
				pfprect.bottom = pfprect.top + 40;
				pfprect.right = pfprect.left + 40;

				IntPfpTableItem searchfilters;
				searchfilters.key = msg.author.id;
				IntPfpTableItem *pfplookup =
				    hashmap_get(pfp_map, &searchfilters);
				if (pfplookup) {
					HBITMAP hBitmap =
					    (HBITMAP)GetCurrentObject(hdc, OBJ_BITMAP);

					BITMAP bmp;
					GetObject(hBitmap, sizeof(BITMAP), &bmp);

					int width = bmp.bmWidth;
					int height = bmp.bmHeight;

					BITMAP bmpInfo;
					HBITMAP hSrcBmp =
					    (HBITMAP)GetCurrentObject(pfplookup->bmp, OBJ_BITMAP);
					GetObject(hSrcBmp, sizeof(BITMAP), &bmpInfo);

					StretchBlt(hdc, pfprect.left, pfprect.top, 40, 40,
					           pfplookup->bmp, 0, 0, bmpInfo.bmWidth,
					           bmpInfo.bmHeight, SRCCOPY);
				} else {
					FillRect(hdc, &pfprect, (HBRUSH)BLACK_BRUSH);
				}

				RECT usernamerect = msgrect;
				usernamerect.left += (6 + 40 + 6);
				// usernamerect.top += 6;
				char *rndname = cwnd->uimsgs[i].msg.author.DisplayName
				                    ? cwnd->uimsgs[i].msg.author.DisplayName
				                    : cwnd->uimsgs[i].msg.author.Username;
				SelectObject(hdc, boldfont);
				DrawTextA(hdc, rndname, strlen(rndname), &usernamerect, 0);
				RECT contentrect = msgrect;
				contentrect.left += (6 + 40 + 6);
				contentrect.top += 15;
				contentrect.bottom =
				    contentrect.top + msgdet[i].TextContentHeight;
				SelectObject(hdc, regfont);
				DrawTextA(hdc, cwnd->uimsgs[i].msg.content,
				          strlen(cwnd->uimsgs[i].msg.content), &contentrect,
				          DT_WORDBREAK);
			} else {

				RECT contentrect = msgrect;
				contentrect.left += (6 + 40 + 6);
				// contentrect.top += 6;
				contentrect.bottom =
				    contentrect.top + msgdet[i].TextContentHeight;
					SelectObject(hdc, regfont);
				DrawTextA(hdc, cwnd->uimsgs[i].msg.content,
				          strlen(cwnd->uimsgs[i].msg.content), &contentrect,
				          DT_WORDBREAK);
			}
			msgrect.top += msgdet[i].TotalMessageHeight;
		}

		/* ── 4. SYNC SCROLL BAR ───────────────────────────────────────────────
		 */
		SCROLLINFO si = {sizeof(SCROLLINFO)};
		si.fMask = SIF_RANGE | SIF_PAGE | SIF_POS;
		si.nMin = 0;
		si.nMax = cwnd->contentHeight - 1;
		si.nPage = viewHeight;
		si.nPos = cwnd->scrollOffset;
		SetScrollInfo(hwnd, SB_VERT, &si, TRUE);

		EndPaint(hwnd, &ps);
		break;
	}
	case WM_VSCROLL: {
		ChatWnd *cwnd = GetChatControlDetails(hwnd);
		RECT clientRect;
		GetClientRect(hwnd, &clientRect);
		int viewHeight = clientRect.bottom - clientRect.top;
		int maxScroll = max(0, cwnd->contentHeight - viewHeight);

		SCROLLINFO si = {sizeof(SCROLLINFO)};
		si.fMask = SIF_ALL;
		GetScrollInfo(hwnd, SB_VERT, &si);

		switch (LOWORD(wParam)) {
		case SB_LINEUP:
			cwnd->scrollOffset -= 20;
			break;
		case SB_LINEDOWN:
			cwnd->scrollOffset += 20;
			break;
		case SB_PAGEUP:
			cwnd->scrollOffset -= viewHeight;
			break;
		case SB_PAGEDOWN:
			cwnd->scrollOffset += viewHeight;
			break;
		case SB_THUMBTRACK:
			cwnd->scrollOffset = si.nTrackPos;
			break;
		case SB_THUMBPOSITION:
			cwnd->scrollOffset = si.nPos;
			break;
		case SB_TOP:
			cwnd->scrollOffset = 0;
			break;
		case SB_BOTTOM:
			cwnd->scrollOffset = maxScroll;
			break;
		}

		cwnd->scrollOffset = max(0, min(cwnd->scrollOffset, maxScroll));

		si.fMask = SIF_POS;
		si.nPos = cwnd->scrollOffset;
		SetScrollInfo(hwnd, SB_VERT, &si, TRUE);

		InvalidateRect(hwnd, NULL, FALSE);
		break;
	}
	case WM_DESTROY:
		break;
	}
	return DefWindowProc(hwnd, uMsg, wParam, lParam);
}
