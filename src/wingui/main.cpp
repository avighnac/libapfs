#ifndef WINVER
#define WINVER 0x0A00
#endif
#ifndef _WIN32_WINNT
#define _WIN32_WINNT 0x0A00
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif

#include "AppWindow.hpp"
#include "Utf8.hpp"
#include "mount_daemon.hpp"
#include <dbt.h>
#include <dwmapi.h>
#include <shellapi.h>
#include <windows.h>
#include <windowsx.h>

#include <cstdio>
#include <memory>
#include <string>
#include <vector>

LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam) {
  auto *app = (AppWindow *)GetWindowLongPtrW(hwnd, GWLP_USERDATA);

  switch (msg) {
  case WM_PAINT: {
    PAINTSTRUCT ps;
    BeginPaint(hwnd, &ps);
    if (app) {
      app->on_paint();
    }
    EndPaint(hwnd, &ps);
    return 0;
  }
  case WM_SIZE: {
    if (app) {
      app->on_resize(LOWORD(lparam), HIWORD(lparam));
    }
    return 0;
  }
  case WM_LBUTTONDOWN: {
    if (app) {
      app->on_lbutton_down(float(GET_X_LPARAM(lparam)), float(GET_Y_LPARAM(lparam)));
    }
    return 0;
  }
  case WM_MOUSEMOVE: {
    if (app) {
      app->on_mouse_move(float(GET_X_LPARAM(lparam)), float(GET_Y_LPARAM(lparam)));
    }
    return 0;
  }
  case WM_MOUSELEAVE: {
    if (app) {
      app->on_mouse_leave();
    }
    return 0;
  }
  case WM_MOUSEWHEEL: {
    if (app) {
      POINT pt{GET_X_LPARAM(lparam), GET_Y_LPARAM(lparam)};
      ScreenToClient(hwnd, &pt);
      const float notches = float(GET_WHEEL_DELTA_WPARAM(wparam)) / float(WHEEL_DELTA);
      app->on_mouse_wheel(float(pt.x), float(pt.y), notches);
    }
    return 0;
  }
  case WM_GETMINMAXINFO: {
    if (app) {
      app->on_get_min_max_info((MINMAXINFO *)lparam);
    }
    return 0;
  }
  case WM_DEVICECHANGE: {
    if (app) {
      app->refresh_physical_disks();
    }
    return 0;
  }
  case WM_DPICHANGED: {
    if (app) {
      app->on_dpi_changed(LOWORD(wparam));
    }
    auto *suggested = (RECT *)lparam;
    SetWindowPos(
      hwnd, nullptr, suggested->left, suggested->top,
      suggested->right - suggested->left, suggested->bottom - suggested->top,
      SWP_NOZORDER | SWP_NOACTIVATE
    );
    return 0;
  }
  case WM_ERASEBKGND: {
    return 1; // we handle this with direct2d
  }
  case WM_DESTROY: {
    PostQuitMessage(0);
    return 0;
  }
  }

  return DefWindowProcW(hwnd, msg, wparam, lparam);
}

int WINAPI wWinMain(HINSTANCE hInstance, HINSTANCE, PWSTR, int nCmdShow) {
  // This one executable serves double duty: with no arguments it's the GUI,
  // and with arguments it's a mount-helper instance the GUI launched against
  // itself (see MountRegistry.cpp / mount_helper/MountHelperMain.hpp).
  //
  // The CRT's __argc/__argv globals are NOT a reliable source of argv here:
  // for a WinMain/wWinMain (GUI-subsystem) entry point the CRT startup only
  // populates __argc and __wargv, leaving the narrow __argv null -- so we
  // parse the command line ourselves via CommandLineToArgvW and convert to
  // narrow (system codepage, matching what a plain main()'s __argv would
  // have contained) only for the args that actually need it.
  int argc = 0;
  wchar_t **wargv = CommandLineToArgvW(GetCommandLineW(), &argc);

  if (wargv && argc > 1) {
    std::vector<std::string> narrow_args;
    narrow_args.reserve(size_t(argc));
    for (int i = 0; i < argc; ++i) {
      narrow_args.push_back(utf8::to_system_codepage(wargv[i]));
    }
    LocalFree(wargv);

    std::vector<char *> argv_ptrs;
    argv_ptrs.reserve(narrow_args.size() + 1);
    for (auto &arg : narrow_args) {
      argv_ptrs.push_back(arg.data());
    }
    argv_ptrs.push_back(nullptr);

    return mount_daemon(argc, argv_ptrs.data());
  }
  if (wargv) {
    LocalFree(wargv);
  }

  SetProcessDpiAwarenessContext(DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2);
  const wchar_t *class_name = L"LibapfsGuiWindowClass";

  WNDCLASSEXW wc{};
  wc.cbSize = sizeof(wc);
  wc.style = CS_HREDRAW | CS_VREDRAW;
  wc.lpfnWndProc = WndProc;
  wc.hInstance = hInstance;
  wc.hCursor = LoadCursorW(nullptr, IDC_ARROW);
  wc.lpszClassName = class_name;
  RegisterClassExW(&wc);

  HWND hwnd = CreateWindowExW(
    0, class_name, L"libapfs-gui",
    WS_OVERLAPPEDWINDOW, CW_USEDEFAULT, CW_USEDEFAULT,
    1000, 600, nullptr, nullptr, hInstance, nullptr
  );
  if (!hwnd) {
    return 0;
  }

  // Force square corners
  {
    DWM_WINDOW_CORNER_PREFERENCE corner_pref = DWMWCP_DONOTROUND;
    DwmSetWindowAttribute(hwnd, DWMWA_WINDOW_CORNER_PREFERENCE, &corner_pref, sizeof(corner_pref));
  }

  auto app = std::make_unique<AppWindow>(hwnd);
  SetWindowLongPtrW(hwnd, GWLP_USERDATA, (LONG_PTR)app.get());

  ShowWindow(hwnd, nCmdShow);
  UpdateWindow(hwnd);

  MSG msg{};
  while (GetMessageW(&msg, nullptr, 0, 0)) {
    TranslateMessage(&msg);
    DispatchMessageW(&msg);
  }

  return msg.wParam;
}