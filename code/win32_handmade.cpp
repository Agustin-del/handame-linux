#include <windows.h>

#define internal static
#define local_persist static
#define global_variable static

global_variable BITMAPINFO BitMapInfo;
global_variable void *BitMapMemory;
global_variable HBITMAP BitmapHandle;
global_variable HDC BitmapDeviceContext;

internal void Win32ResizeDIBSection(int Width, int Heigth) {

  if (BitmapHandle) {
    DeleteObject(BitmapHandle);
  } 
  if(!BitmapDeviceContext)
  {
    BitMapDeviceContext = GetCompatibleDC(0);
  }

  BitMapInfo.bmiHeader.biSize = sizeof(BitmapInfo.bmiHeader);
  BitMapInfo.bmiHeader.biWidth = Width;
  BitMapInfo.bmiHeader.biHeight = Height;
  BitMapInfo.bmiHeader.biPlanes = 1;
  BitMapInfo.bmiHeader.biBitCount = 32;
  BitMapInfo.bmiHeader.biCompression = BI_RGB;

  BitmapHandle = CreateDIBSection(DeviceContext, &BitMapInfo, DIB_RGB_COLORS,
                                  &BitMapMemory, 0, 0);
}

internal void Win32UpdateWindow(HDC DeviceContext, int X, int Y, int Width,
                                int Height) {

  StretchDIBits(DeviceContext, X, Y, Width, Height, X, Y, Width, Height,
                BitMapMemory, &BitMapInfo, DIB_RGB_COLORS, SRCCOPY);
}

global_variable bool Running;
LRESULT CALLBACK Win32MainWindowCallback(HWND Window, UINT Message,
                                         WPARAM WParam, LPARAM Lparam) {
  LRESULT Result = 0;
  switch (Message) {
  case WM_SIZE: {
    RECT ClientRect;
    GetClientRect(Window, &ClientRect);
    int Width = ClientRect.right - ClientRect.left;
    int Height = ClientRect.bottom - ClientRect.top;
    Win32ResizeDIBSection(Width, Height);
    OutputDebugStringA("WM_SIZE\n");
  } break;
  case WM_DESTROY: {
    OutputDebugStringA("WM_DESTROY\n");
    Running = false;
  } break;
  case WM_CLOSE: {
    Running = false;
    OutputDebugStringA("WM_CLOSE\n");
  } break;
  case WM_ACTIVATEAPP: {
    OutputDebugStringA("WM_ACTIVATEAPP\n");
  } break;
  case WM_PAINT: {
    PAINTSTRUCT Paint;
    HDC DeviceContext = BeginPaint(Window, &Paint);
    int X = Paint.rcPaint.left;
    int Y = Paint.rcPaint.top;
    int Width = Paint.rcPaint.right - Paint.rcPaint.left;
    int Height = Paint.rcPaint.bottom - Paint.rcPaint.top;
    Win32UpdateWindow(DeviceContext, X, Y, Width, Height);
    EndPaint(Window, Paint);
  }
  default: {
    Result = DefWindowProc(Window, Message, WParam, LParam);
  } break;
  }
  return Result;
}

int CALLBACK WinMain(HINSTANCE Instance, HINSTANCE PrevInstance,
                     LPSTR CommandLine, int ShowCode) {
  WNDCLASS WindowClass = {};
  WindowClass.style = CS_OWNDC | CS_HREDRAW | CS_VREDRAW;
  WindowClass.lpfnWndProc = Win32MainWindowCallback;
  WindowClass.hInstance = Instance;
  // WindowClass.hIcon = ;
  WindowClass.lpszClassName = "HandmadeHeroWindowClass";

  if (RegisterClass(&WindowClass)) {
    HWND WindowHandle = CreateWindowEx(
        0, WindowClass.lpszClassName, "Handmade Hero",
        WS_OVERLAPPEDWINDOW | WS_VISIBLE, CW_USEDEFAULT, CW_USEDEFAULT,
        CW_USEDEFAULT, CW_USEDEFAULT, 0, 0, Instance, 0);

    if (WindowHandle) {
      MSG Message;
      while (Running) {
        BOOL MessageResult = GetMessage(&Message, 0, 0, 0);
        if (MessageResult > 0) {
          TranslateMessage(&Message);
          DispatchMessage(&Message);
        } else {
          break;
        }
      }
    } else {
    }
  } else {
  }

  return 0;
}
