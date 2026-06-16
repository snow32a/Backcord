#include <windows.h>
#include "snoctrl.h"
void InitSnowsControls() {
	
	NONCLIENTMETRICS ncm = { sizeof(NONCLIENTMETRICS) };
	SystemParametersInfo(SPI_GETNONCLIENTMETRICS, sizeof(ncm), &ncm, 0);
	regfont=CreateFontIndirect(&ncm.lfMessageFont);
	ncm.lfMessageFont.lfWeight=FW_BOLD;
	boldfont=CreateFontIndirect(&ncm.lfMessageFont);
	ncm.lfMessageFont.lfHeight=-32;
	h1font=CreateFontIndirect(&ncm.lfMessageFont);
	ncm.lfMessageFont.lfHeight=-24;
	h2font=CreateFontIndirect(&ncm.lfMessageFont);
	h3font = boldfont;
	
	const char CLASS_NAME_CHAT[] = "BackcordChat";

	WNDCLASS wcc = {};

	wcc.lpfnWndProc = ChatWndProc;
	wcc.hInstance = GetModuleHandle(NULL);
	wcc.lpszClassName = CLASS_NAME_CHAT;
	wcc.hCursor = LoadCursorA(NULL, IDC_ARROW);
	RegisterClass(&wcc);
	
	const char CLASS_NAME_GUILD[] = "BackcordGuild";

	WNDCLASS wcg = {};

	wcg.lpfnWndProc = GuildWndProc;
	wcg.hInstance = GetModuleHandle(NULL);
	wcg.lpszClassName = CLASS_NAME_GUILD;
	wcg.hCursor = LoadCursorA(NULL, IDC_ARROW);
	RegisterClass(&wcg);

}