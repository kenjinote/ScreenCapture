#pragma comment(linker,"\"/manifestdependency:type='win32' name='Microsoft.Windows.Common-Controls' version='6.0.0.0' processorArchitecture='*' publicKeyToken='6595b64144ccf1df' language='*'\"")

#pragma comment(lib, "gdiplus")
#pragma comment(lib, "dwmapi")

#include <windows.h>
#include <windowsx.h>
#include <gdiplus.h>
#include <dwmapi.h>

TCHAR szClassName[] = TEXT("Window");

Gdiplus::Bitmap * WindowCapture(HWND hWnd)
{
	Gdiplus::Bitmap * pBitmap = 0;
	RECT rect1;
	GetWindowRect(hWnd, &rect1);
	RECT rect2;
	if (DwmGetWindowAttribute(hWnd, DWMWA_EXTENDED_FRAME_BOUNDS, &rect2, sizeof(rect2)) != S_OK) rect2 = rect1;
	HDC hdc = GetDC(0);
	HDC hMem = CreateCompatibleDC(hdc);
	HBITMAP hBitmap = CreateCompatibleBitmap(hdc, rect2.right - rect2.left, rect2.bottom - rect2.top);
	if (hBitmap) {
		HBITMAP hOldBitmap = (HBITMAP)SelectObject(hMem, hBitmap);
		SetForegroundWindow(hWnd);
		InvalidateRect(hWnd, 0, 1);
		UpdateWindow(hWnd);
		BitBlt(hMem, 0, 0, rect2.right - rect2.left, rect2.bottom - rect2.top, hdc, rect2.left, rect2.top, SRCCOPY);
		pBitmap = Gdiplus::Bitmap::FromHBITMAP(hBitmap, 0);
		SelectObject(hMem, hOldBitmap);
		DeleteObject(hBitmap);
	}
	DeleteDC(hMem);
	ReleaseDC(0, hdc);
	return pBitmap;
}

Gdiplus::Bitmap * ScreenCapture(LPRECT lpRect)
{
	Gdiplus::Bitmap * pBitmap = 0;
	HDC hdc = GetDC(0);
	HDC hMem = CreateCompatibleDC(hdc);
	HBITMAP hBitmap = CreateCompatibleBitmap(hdc, lpRect->right - lpRect->left, lpRect->bottom - lpRect->top);
	if (hBitmap) {
		HBITMAP hOldBitmap = (HBITMAP)SelectObject(hMem, hBitmap);
		BitBlt(hMem, 0, 0, lpRect->right - lpRect->left, lpRect->bottom - lpRect->top, hdc, lpRect->left, lpRect->top, SRCCOPY);
		pBitmap = Gdiplus::Bitmap::FromHBITMAP(hBitmap, 0);
		SelectObject(hMem, hOldBitmap);
		DeleteObject(hBitmap);
	}
	DeleteDC(hMem);
	ReleaseDC(0, hdc);
	return pBitmap;
}

LRESULT CALLBACK LayerWndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	static BOOL bDrag;
	static BOOL bDown;
	static POINT posStart;
	static RECT OldRect;
	switch (msg) {
	case WM_KEYDOWN:
	case WM_RBUTTONDOWN:
		SendMessage(hWnd, WM_CLOSE, 0, 0);
		break;
	case WM_LBUTTONDOWN:
		{
			int xPos = GET_X_LPARAM(lParam);
			int yPos = GET_Y_LPARAM(lParam);
			POINT point = { xPos, yPos };
			ClientToScreen(hWnd, &point);
			posStart = point;
			SetCapture(hWnd);
		}
		break;
	case WM_MOUSEMOVE:
		if (GetCapture() == hWnd)
		{
			int xPos = GET_X_LPARAM(lParam);
			int yPos = GET_Y_LPARAM(lParam);
			POINT point = { xPos, yPos };
			ClientToScreen(hWnd, &point);
			if (!bDrag) {
				if (abs(xPos - posStart.x) > GetSystemMetrics(SM_CXDRAG) && abs(yPos - posStart.y) > GetSystemMetrics(SM_CYDRAG)) {
					bDrag = TRUE;
				}
			} else {
				HDC hdc = GetDC(hWnd);
				RECT rect = { min(point.x, posStart.x), min(point.y, posStart.y), max(point.x, posStart.x), max(point.y, posStart.y) };
				HBRUSH hBrush = CreateSolidBrush(RGB(255, 0, 0));
				HRGN hRgn1 = CreateRectRgn(OldRect.left, OldRect.top, OldRect.right, OldRect.bottom);
				HRGN hRgn2 = CreateRectRgn(rect.left, rect.top, rect.right, rect.bottom);
				CombineRgn(hRgn1, hRgn1, hRgn2, RGN_DIFF);
				FillRgn(hdc, hRgn1, (HBRUSH)GetStockObject(BLACK_BRUSH));
				FillRect(hdc, &rect, hBrush);
				OldRect = rect;
				DeleteObject(hBrush);
				DeleteObject(hRgn1);
				DeleteObject(hRgn2);
				ReleaseDC(hWnd, hdc);
			}
		}
		break;
	case WM_LBUTTONUP:
		if (GetCapture() == hWnd) {
			ReleaseCapture();
			Gdiplus::Bitmap * pBitmap = 0;
			if (bDrag) {
				bDrag = FALSE;
				int xPos = GET_X_LPARAM(lParam);
				int yPos = GET_Y_LPARAM(lParam);
				POINT point = { xPos, yPos };
				ClientToScreen(hWnd, &point);
				RECT rect = { min(point.x, posStart.x), min(point.y, posStart.y), max(point.x, posStart.x), max(point.y, posStart.y) };
				ShowWindow(hWnd, SW_HIDE);
				pBitmap = ScreenCapture(&rect);
			} else {
				ShowWindow(hWnd, SW_HIDE);
				HWND hTargetWnd = WindowFromPoint(posStart);
				hTargetWnd = GetAncestor(hTargetWnd, GA_ROOT);
				if (hTargetWnd) {
					pBitmap = WindowCapture(hTargetWnd);
				}
			}
			SendMessage(GetParent(hWnd), WM_APP, 0, (LPARAM)pBitmap);
		}
		break;
	default:
		return DefWindowProc(hWnd, msg, wParam, lParam);
	}
	return 0;
}

LRESULT CALLBACK WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	static HWND hButton;
	static LPCWSTR lpszLayerWindowClass = L"LayerWindow";
	static HWND hLayerWnd;
	static Gdiplus::Bitmap *pBitmap;
	switch (msg)
	{
	case WM_CREATE:
		{
			WNDCLASS wndclass = {0,LayerWndProc,0,0,((LPCREATESTRUCT)lParam)->hInstance,0,LoadCursor(0,IDC_CROSS),(HBRUSH)GetStockObject(BLACK_BRUSH),0,lpszLayerWindowClass };
			RegisterClass(&wndclass);
		}
		hButton = CreateWindow(TEXT("BUTTON"), TEXT("スクリーンキャプチャー"), WS_VISIBLE | WS_CHILD, 0, 0, 0, 0, hWnd, (HMENU)IDOK, ((LPCREATESTRUCT)lParam)->hInstance, 0);
		break;
	case WM_APP:
		{
			delete pBitmap;
			pBitmap = (Gdiplus::Bitmap*)lParam;
			if (pBitmap) {
				InvalidateRect(hWnd, 0, 1);
			}
			SetForegroundWindow(hWnd);
		}
		break;
	case WM_PAINT:
		{
			PAINTSTRUCT ps;
			HDC hdc = BeginPaint(hWnd, &ps);
			if (pBitmap) {
				RECT rect;
				GetClientRect(hWnd, &rect);
				Gdiplus::Graphics g(hdc);
				g.DrawImage(pBitmap, 0, 90, min(rect.right, (int)pBitmap->GetWidth()), min(rect.bottom - 90, (int)pBitmap->GetHeight()));
			}
			EndPaint(hWnd, &ps);
		}
		break;
	case WM_SIZE:
		MoveWindow(hButton, 10, 10, 256, 32, TRUE);
		break;
	case WM_COMMAND:
		if (LOWORD(wParam) == IDOK) {
			hLayerWnd = CreateWindowEx(WS_EX_LAYERED | WS_EX_TOPMOST, lpszLayerWindowClass, 0, WS_POPUP, 0, 0, 0, 0, hWnd, 0, GetModuleHandle(0), 0);
			SetLayeredWindowAttributes(hLayerWnd, RGB(255, 0, 0), 64, LWA_ALPHA | LWA_COLORKEY);
			SetWindowPos(hLayerWnd, HWND_TOPMOST, 0, 0, GetSystemMetrics(SM_CXVIRTUALSCREEN), GetSystemMetrics(SM_CYVIRTUALSCREEN), SWP_NOSENDCHANGING);
			ShowWindow(hLayerWnd, SW_NORMAL);
			UpdateWindow(hLayerWnd);
		}
		break;
	case WM_DESTROY:
		delete pBitmap;
		PostQuitMessage(0);
		break;
	default:
		return DefWindowProc(hWnd, msg, wParam, lParam);
	}
	return 0;
}

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPreInst, LPSTR pCmdLine, int nCmdShow) {
	ULONG_PTR gdiToken;
	Gdiplus::GdiplusStartupInput gdiSI;
	Gdiplus::GdiplusStartup(&gdiToken, &gdiSI, NULL);
	MSG msg;
	WNDCLASS wndclass = {
		CS_HREDRAW | CS_VREDRAW,
		WndProc,
		0,
		0,
		hInstance,
		0,
		LoadCursor(0,IDC_ARROW),
		(HBRUSH)(COLOR_WINDOW + 1),
		0,
		szClassName
	};
	RegisterClass(&wndclass);
	HWND hWnd = CreateWindow(
		szClassName,
		TEXT("スクリーンキャプチャー"),
		WS_OVERLAPPEDWINDOW,
		CW_USEDEFAULT,
		0,
		CW_USEDEFAULT,
		0,
		0,
		0,
		hInstance,
		0
	);
	ShowWindow(hWnd, SW_SHOWDEFAULT);
	UpdateWindow(hWnd);
	while (GetMessage(&msg, 0, 0, 0)) {
		TranslateMessage(&msg);
		DispatchMessage(&msg);
	}
	Gdiplus::GdiplusShutdown(gdiToken);
	return (int)msg.wParam;
}
