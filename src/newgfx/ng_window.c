/*
        Agat Emulator
        Copyright (c) NOP, nnop@newmail.ru
        NewGfx by Sergey 'SnakE' Gromov <snake.scaly@gmail.com>
*/

#include "ng_window.h"

#include "ng_logging.h"

#include "logging/logging.h"

#include <windows.h>

#define NG_WINDOW_CLASS_NAME TEXT("NgWindowClass")
#define NG_WINDOW_NAME TEXT("Prototype Graphics")

struct NG_WINDOW
{
	ATOM ngw_window_class;
	HWND ngw_hwnd;
};

static LRESULT CALLBACK ng_wnd_proc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam)
{
	NG_WINDOW* ngw = (NG_WINDOW*)GetWindowLongPtr(hwnd, GWL_USERDATA);

	switch (msg) {
		case WM_CREATE:
		{
			LPCREATESTRUCT cs = (LPCREATESTRUCT)lparam;
			ngw = (NG_WINDOW*)cs->lpCreateParams;
			SetWindowLongPtr(hwnd, GWL_USERDATA, (LONG)ngw);
			return 0;
		}

		case WM_SIZE:
			InvalidateRect(hwnd, NULL, FALSE);
			return 0;

		default:
			return DefWindowProc(hwnd, msg, wparam, lparam);
	}
}

static int register_class(NG_WINDOW* ngw)
{
	WNDCLASS window_class;
	memset(&window_class, 0, sizeof(window_class));

	window_class.lpszClassName = NG_WINDOW_CLASS_NAME;
	window_class.lpfnWndProc = ng_wnd_proc;
	window_class.hCursor = LoadCursor(NULL, IDC_ARROW);

	ngw->ngw_window_class = RegisterClass(&window_class);
	if (ngw->ngw_window_class) return 0;

	LOG_WIN_ERROR("Failed to register window class");
	return 1;
}

static int create_window(NG_WINDOW* ngw)
{
	DWORD style = WS_POPUP | WS_CAPTION | WS_SIZEBOX | WS_VISIBLE;
	RECT wr = { 0, 0, 320, 240 };
	AdjustWindowRect(&wr, style, FALSE);

	ngw->ngw_hwnd = CreateWindow(
		(LPTSTR)ngw->ngw_window_class,
		NG_WINDOW_NAME,
		style,
		CW_USEDEFAULT, CW_USEDEFAULT, wr.right - wr.left, wr.bottom - wr.top,
		0, 0, 0, ngw);
	if (ngw->ngw_hwnd) return 0;

	LOG_WIN_ERROR("Failed to create a window");
	return 1;
}

NG_WINDOW* ng_window_create()
{
	NG_WINDOW* ngw = NULL;

	LOG_INFO("Creating a NewGfx window...");

	ngw = calloc(1, sizeof(NG_WINDOW));
	if (!ngw) {
		int err = errno;
		LOG_ERROR("Failed to allocate %zu: %s", sizeof(NG_WINDOW), strerror(err));
		goto out;
	}

	if (register_class(ngw) != 0) goto free_ngw;
	if (create_window(ngw) != 0) goto free_class;

	LOG_INFO("NewGfx window created successfully.");

	goto out;

free_class:
	UnregisterClass((LPTSTR)ngw->ngw_window_class, 0);

free_ngw:
	free(ngw);
	ngw = NULL;

out:
	return ngw;
}

void ng_window_free(NG_WINDOW* ngw)
{
	if (!ngw) return;

	LOG_INFO("Destroying the NewGfx window...");

	if (!DestroyWindow(ngw->ngw_hwnd))
		LOG_WIN_ERROR("Failed to destroy the NewGfx window");

	if (!UnregisterClass((LPTSTR)ngw->ngw_window_class, 0))
		LOG_WIN_ERROR("Failed to unregister the NewGfx window class");

	free(ngw);

	LOG_INFO("NewGfx window is destroyed.");
}
