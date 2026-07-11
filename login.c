#include <windows.h>
#include <wingdi.h>
int ret = 1;
HWND tokenTxtField;
WNDPROC txtWndProc;
HFONT CueFont;
HWND loginbtn;
LRESULT CALLBACK loginwndproc(HWND hwnd, UINT uMsg, WPARAM wParam,
							  LPARAM lParam) {
	switch (uMsg) {
	case WM_CREATE:
		return 0;
	case WM_PAINT: {
		PAINTSTRUCT lppaint;
		HDC hdc = BeginPaint(hwnd, &lppaint);
		RECT clientrect;
		GetClientRect(hwnd, &clientrect);
		FillRect(hdc, &clientrect, (HBRUSH)(COLOR_3DFACE + 1));
		EndPaint(hwnd, &lppaint);
		return 0;
	};
	case WM_COMMAND: {
		if (lParam == (LPARAM)loginbtn) {
			ret = 0;
		}
		return 0;
	}
	case WM_DESTROY:
		return 0;
	default:
		return DefWindowProcA(hwnd, uMsg, wParam, lParam);
	}
}
LRESULT CALLBACK TokenTxtWndProc(HWND hWnd, UINT msg, WPARAM wParam,
								 LPARAM lParam) {
	if (msg == WM_PAINT) {
		if (GetWindowTextLength(hWnd) == 0) {
			PAINTSTRUCT lpPaint;
			HDC hDC = BeginPaint(hWnd, &lpPaint);
			RECT clRect;
			GetClientRect(hWnd, &clRect);
			FillRect(hDC, &clRect, (HBRUSH)(COLOR_WINDOW + 1));
			SetTextColor(hDC, RGB(128, 128, 128));
			SelectObject(hDC, CueFont);
			DrawText(hDC, "Token...", 8, &clRect, 0);
			EndPaint(hWnd, &lpPaint);
			return 0;
		} else {
			return txtWndProc(hWnd, msg, wParam, lParam);
		}
	} else {
		return txtWndProc(hWnd, msg, wParam, lParam);
	}
}
char *PromptToken() {

	char *clsname = "BackcordLogin";
	WNDCLASS wc = {0};
	wc.hbrBackground = (HBRUSH)(COLOR_3DFACE + 1);
	wc.lpszClassName = clsname;
	wc.hCursor = LoadCursorA(NULL, IDC_ARROW);
	wc.lpfnWndProc = loginwndproc;
	RegisterClassA(&wc);
	HWND hwnd = CreateWindowExA(0, clsname, "Backcord - Login", WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU,
								CW_USEDEFAULT, CW_USEDEFAULT, 320, 480, NULL,
								NULL, GetModuleHandleA(NULL), NULL);
	RECT clientrect;
	GetClientRect(hwnd, &clientrect);
	HWND banner =
		CreateWindowExA(0, "STATIC", "", WS_VISIBLE | WS_CHILD | SS_BITMAP, 0,
						0, clientrect.right - clientrect.left, 64, hwnd, NULL,
						GetModuleHandleA(NULL), NULL);
	SendMessage(
		banner, STM_SETIMAGE, IMAGE_BITMAP,
		(LPARAM)(LoadBitmap(GetModuleHandle(NULL), MAKEINTRESOURCE(102))));

	tokenTxtField =
		CreateWindowExA(WS_EX_STATICEDGE, "EDIT", "", WS_VISIBLE | WS_CHILD | ES_AUTOHSCROLL | ES_PASSWORD, 20,
						64 + 20, clientrect.right - clientrect.left - 40, 20,
						hwnd, NULL, GetModuleHandleA(NULL), NULL);
						
	SendMessage(tokenTxtField, EM_SETLIMITTEXT, 0, 0);

	NONCLIENTMETRICS ncm = {sizeof(NONCLIENTMETRICS)};
	SystemParametersInfo(SPI_GETNONCLIENTMETRICS, sizeof(ncm), &ncm, 0);
	HFONT hmsgfont = CreateFontIndirect(&ncm.lfMessageFont);
	SendMessage(tokenTxtField, WM_SETFONT, (WPARAM)hmsgfont, TRUE);
	LOGFONTA CueFontLog = ncm.lfMessageFont;
	CueFontLog.lfItalic = 1;
	CueFont = CreateFontIndirect(&CueFontLog);
	txtWndProc = (WNDPROC)GetWindowLong(tokenTxtField, GWL_WNDPROC);
	SetWindowLong(tokenTxtField, GWL_WNDPROC, (long)TokenTxtWndProc);

	loginbtn = CreateWindowExA(0, "BUTTON", "Login",
							   WS_VISIBLE | WS_CHILD | BS_DEFPUSHBUTTON,
							   clientrect.right - clientrect.left - 10 - 75,
							   clientrect.bottom - clientrect.top - 10 - 25, 75,
							   25, hwnd, NULL, GetModuleHandleA(NULL), NULL);
	SendMessage(loginbtn, WM_SETFONT, (WPARAM)hmsgfont, TRUE);

	ShowWindow(hwnd, SW_SHOW);
	while (ret) {
		MSG msg;
		if (GetMessage(&msg, NULL, 0, 0)) {
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
	}
	char *tok = malloc(GetWindowTextLength(tokenTxtField) + 1);
	GetWindowText(tokenTxtField, tok, GetWindowTextLength(tokenTxtField) + 1);
	DestroyWindow(hwnd);
	return tok;
}