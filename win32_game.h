/* date = October 5th 2023 8:16 pm */
#ifndef WIN32_GAME_H

struct Win32OffscreenBuffer
{
    BITMAPINFO info;
    int32 width;
    int32 height;
    int32 pitch;
    void *data;
    int32 bpp;
    //    bool32 is_top_down;
};

struct Win32Variables
{
    bool32 is_running;
    Win32OffscreenBuffer buffer;
    
    char exe_file_path[256];
    char one_past_slash[256];
    
    char game_dll_full_path[256];
};

struct Win32GameCode
{
    HMODULE dll;
    GameUpdateAndRenderPtr update_and_render;
    bool32 is_valid;
};

struct Win32DeviceContextScoped
{
    Win32DeviceContextScoped(HWND window_) : window(window_), dc(GetDC(window)){}
    ~Win32DeviceContextScoped(){ ReleaseDC(window, dc); }
    
    HWND window;
    HDC dc;
};

struct Win32TimePeriodScoped
{
    Win32TimePeriodScoped(real32 milliseconds_= 1.0f)
        : milliseconds(milliseconds_), result(timeBeginPeriod((UINT)milliseconds))
    {}
    ~Win32TimePeriodScoped() { timeBeginPeriod((UINT)milliseconds);  }
    
    real32 milliseconds;
    MMRESULT result;
};

#define WIN32_GAME_H
#endif //WIN32_GAME_H
