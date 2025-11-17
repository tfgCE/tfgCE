#include "splashScreen.h"

#include "splashInfo.h"

#include "..\buildInformation.h"

#include "..\globalInclude.h"

#include "..\utils.h"

#include "..\debug\debug.h"
#include "..\types\string.h"

using namespace Splash;

//

#ifdef AN_WINDOWS
#include <windef.h>
#include <Windows.h>
#include "CommCtrl.h"

class Splash::ScreenImpl
{
public:
	ScreenImpl(tchar const * _fileName)
	{		
		an_assert(!impl);
		impl = this;

		logo = (HBITMAP)LoadImage(NULL, _fileName, IMAGE_BITMAP, 0, 0, LR_LOADFROMFILE);

		register_class();

		create_window();
	}

	~ScreenImpl()
	{
		if (window)
		{
			DestroyWindow(window);
		}

		unregister_class();

		an_assert(impl);
		impl = nullptr;
	}

private:
	static ScreenImpl* impl;
	const tchar * const windowClassName = TXT("Splash");

	HWND window = nullptr;
	HBITMAP logo = nullptr;
	bool classRegistered = false;

	void register_class()
	{
		WNDCLASS splashClass;

		splashClass.style = CS_HREDRAW | CS_VREDRAW | CS_NOCLOSE;
		splashClass.lpfnWndProc = window_func;
		splashClass.cbClsExtra = 0;
		splashClass.cbWndExtra = 0;
		splashClass.hInstance = nullptr; // instance
		splashClass.hIcon = LoadIcon(NULL, IDI_APPLICATION);
		splashClass.hCursor = LoadCursor(NULL, IDC_ARROW);
		splashClass.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
		splashClass.lpszMenuName = NULL;
		splashClass.lpszClassName = windowClassName;

		if (RegisterClass(&splashClass))
		{
			classRegistered = true;
		}
	}

	void unregister_class()
	{
		if (classRegistered)
		{
			UnregisterClass(windowClassName, nullptr);
		}
	}

	void create_window()
	{
		if (!classRegistered || !logo)
		{
			return;
		}

		BITMAP bm;
		GetObject(logo, sizeof(BITMAP), &bm);

		int width = bm.bmWidth;
		int height = bm.bmHeight;

		int left = (GetSystemMetrics(SM_CXSCREEN) - width) / 2;
		int top = (GetSystemMetrics(SM_CYSCREEN) - height) / 2;

		window = CreateWindowEx(WS_EX_NOACTIVATE, windowClassName, TXT("splash"), WS_POPUP | WS_BORDER, left, top, width, height, nullptr, nullptr, nullptr, nullptr);

		RECT windowRect;
		GetClientRect(window, &windowRect);

		int newWidth = width +(width - windowRect.right);
		int newHeight = height +(height - windowRect.bottom);
		SetWindowPos(window, HWND_TOP, left, top, newWidth, newHeight, 0); // we want all windows to be top most

		ShowWindow(window, SW_SHOW);
		UpdateWindow(window);
		RedrawWindow(window, nullptr, nullptr, 0);
	}

	static void draw_text(HDC _hdc, tchar const * _text, int _x, int _y, DWORD _textColor = RGB(150, 150, 150), DWORD _bkColor = RGB(255, 255, 255))
	{
		SetBkMode(_hdc, TRANSPARENT);
		RECT rect;
		rect.left = _x;
		rect.right = _x + 10000;
		rect.bottom = _y;
		rect.top = _y - 50;
		SetTextColor(_hdc, _bkColor);
		for (int oy = -1; oy <= 1; ++oy)
		{
			for (int ox = -1; ox <= 1; ++ox)
			{
				RECT or = rect;
				or.left += ox;
				or.right += ox;
				or.top += oy;
				or.bottom += oy;
				DrawText(_hdc, _text, -1, &or, DT_BOTTOM | DT_SINGLELINE);
			}
		}
		SetTextColor(_hdc, _textColor);
		DrawText(_hdc, _text, -1, &rect, DT_BOTTOM | DT_SINGLELINE);
	}

	static LRESULT CALLBACK window_func(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
	{
		switch (message)
		{
		case WM_PAINT:
		{
			HBITMAP hBMP = impl->logo;
			BITMAP bm; 
			GetObject(hBMP, sizeof(BITMAP), &bm);
			int er = GetLastError();
			HDC hdc;
			PAINTSTRUCT ps;
			RECT rt;
			GetWindowRect(hwnd, &rt);
			hdc = BeginPaint(hwnd, &ps);
			HDC memDC = CreateCompatibleDC(hdc);
			SelectObject(memDC, hBMP);
			StretchBlt(hdc, 0, 0, bm.bmWidth, bm.bmHeight, memDC, 0, 0, bm.bmWidth, bm.bmHeight, SRCCOPY);

			int y = bm.bmHeight;
			draw_text(hdc, get_build_information(), 0, y);
			y -= 20;
			/*
			for_every_reverse(info, Splash::Info::get_all_info())
			{
				draw_text(hdc, info->to_char(), 0, y);
				y -= 20;
			}
			*/

			EndPaint(hwnd, &ps);
			return 0;
		}

		break;

		case WM_CLOSE:
			return 0;
		}
		return DefWindowProc(hwnd, message, wParam, lParam);
	}
};

ScreenImpl* ScreenImpl::impl = nullptr;

#endif
#ifdef AN_LINUX_OR_ANDROID
class Splash::ScreenImpl
{
public:
	ScreenImpl(tchar const* _fileName)
	{
		todo_note(TXT("show loading icon?"));
	}
	~ScreenImpl()
	{

	}
};
#endif

//

ScreenImpl* Screen::impl = nullptr;

void Screen::show(tchar const * _fileName)
{
	close();

	an_assert(!impl);

	impl = new ScreenImpl(_fileName);
}

void Screen::close()
{
	delete_and_clear(impl);
}
