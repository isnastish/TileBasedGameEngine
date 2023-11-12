#include "os.h"

#ifdef function
#undef function
#endif

#include <windows.h>
#include <gl/gl.h>

#define DebugOut(format, ...)\
{\
char buf[1024] = {};\
sprintf_s(buf, sizeof(buf), format, ## __VA_ARGS__);\
OutputDebugStringA(buf);\
}

#define function

#include "win32_game.h"

#define HARDWARE_RENDERER 1

static Win32Variables win32_variables;
static Os os_instance;

function void *win32_alloc_memory(size_t size)
{
    // NOTE(alexey): Accept allocation flags as uint32 type?
    // Accept base address?
    assert(size > 0);
    void *result = VirtualAlloc(0, size, MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);
    assert(result);
    return result;
}

function void win32_free_memory(void *memory)
{
    if(memory)
    {
        VirtualFree(memory, 0, MEM_RELEASE);
    }
}

function uint64 win32_frequency()
{
    uint64 result = 0;
    QueryPerformanceFrequency((LARGE_INTEGER *)&result);
    return result;
}

function uint64 win32_qpc()
{
    uint64 result = 0;
    QueryPerformanceCounter((LARGE_INTEGER *)&result);
    return result;
}

function real32 win32_elapsed_seconds(uint64 start_counts,
                                      uint64 frequency)
{
    real32 result = 0.0f;
    
    uint64 end_counts = win32_qpc();
    int64 elapsed_counts = (int64)(end_counts - start_counts);
    result = ((real32)elapsed_counts * (1.0f / (real32)frequency));
        
    return result;
}

static Win32GameCode win32_load_game_code(Win32Variables *variables)
{
    Win32GameCode result = {};
    result.dll = LoadLibraryA(variables->game_dll_full_path);
    
    if(result.dll)
    {
        result.update_and_render = 
        (GameUpdateAndRenderPtr)GetProcAddress(result.dll, "game_update_and_render");
        if(result.update_and_render)
        {
            result.is_valid = true;
        }
    }
    
    if(!result.is_valid)
    {
        result.update_and_render = game_update_and_render_stub;
    }
    
    return result;
}

static void win32_init_opengl(HDC window_dc)
{
    PIXELFORMATDESCRIPTOR pfd = {};
    
    // size of the data structure.
    pfd.nSize = sizeof(PIXELFORMATDESCRIPTOR);
    // version of the data structure.
    pfd.nVersion = 1;
    // properties of the pixel buffer.
    pfd.dwFlags = PFD_SUPPORT_OPENGL | PFD_DOUBLEBUFFER | PFD_DRAW_TO_WINDOW;
    // type of the pixel data.
    pfd.iPixelType = PFD_TYPE_RGBA;
    pfd.cColorBits = 32; // Excluding alpha bitplanes?
    // 24-bit depth buffer.
    pfd.cDepthBits = 24;
    pfd.cStencilBits = 8;
    pfd.cAuxBuffers = 0;
    pfd.iLayerType = PFD_MAIN_PLANE;
    
    int32 pixel_format_index = ChoosePixelFormat(window_dc, &pfd);
    
    // TODO(alexey): Do some reasearch, that might not be needed any longer.
    PIXELFORMATDESCRIPTOR suggested_pfd;
    DescribePixelFormat(window_dc, pixel_format_index,sizeof(PIXELFORMATDESCRIPTOR), &suggested_pfd);
    
    SetPixelFormat(window_dc, pixel_format_index, &suggested_pfd);
    
    HGLRC dummy_opengl_rendering_context = wglCreateContext(window_dc);
    
    // make OpenGL rendering context the calling thread's current rendering context.
    if(wglMakeCurrent(window_dc, dummy_opengl_rendering_context) != TRUE)
    {
        wglMakeCurrent(window_dc, 0);
        wglDeleteContext(dummy_opengl_rendering_context);
    }
    
    // TODO(alexey): For the future, try initializing modern OpenGL, and if we fail,
    // switch back to the basic version.
}

static Vec2 win32_get_window_size(HWND window)
{
    Vec2 result = {};
    
    RECT client_rect;
    GetClientRect(window, &client_rect);
    result.x = (real32)(client_rect.right - client_rect.left);
    result.y = (real32)(client_rect.bottom - client_rect.top);
    
    return result;
}

static Vec2 win32_get_cursor_pos(HWND window)
{
    Vec2 result = {};
    
    POINT cursor;
    GetCursorPos(&cursor);
    ScreenToClient(window, &cursor);
    result.x = (real32)cursor.x;
    result.y = (real32)cursor.y;
    
    return result;
}

static void win32_resize_dib_section(Win32OffscreenBuffer *buffer, int32 width, int32 height)
{
    if((width > 0) && (height > 0))
    {
        if(buffer->data)
        {
            VirtualFree(buffer->data, 0, MEM_RELEASE);
        }
        
        BITMAPINFO bitmap_info = {};
        
        bitmap_info.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
        bitmap_info.bmiHeader.biWidth = width;
        
        // If height is positive, the bitmap is bottom-up with origin at the lower-left corner.
        // If the height is negative, it's a top-down bitmap with the origin at the upper-left corner.
        bitmap_info.bmiHeader.biHeight = height;
        bitmap_info.bmiHeader.biPlanes = 1;
        bitmap_info.bmiHeader.biBitCount = 32;
        bitmap_info.bmiHeader.biCompression = BI_RGB;
        bitmap_info.bmiHeader.biSizeImage = 0;
        
        buffer->bpp = sizeof(int32);
        buffer->info = bitmap_info;
        buffer->width = width;
        buffer->height = height;
        buffer->pitch = buffer->width * buffer->bpp;
        
        uint64 alloc_size = (buffer->width * buffer->height * buffer->bpp);
        buffer->data = VirtualAlloc(0, alloc_size, MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);
        assert(buffer->data);
    }
}

static void win32_display_offscreen_buffer_in_window(HDC device_context, Win32OffscreenBuffer *buffer, int32 window_width, int32 window_height)
{
     StretchDIBits(device_context, 
                  0, 0, buffer->width, buffer->height, 
                  0, 0, buffer->width, buffer->height,
                   buffer->data,
                   &buffer->info,
                   DIB_RGB_COLORS,
                   SRCCOPY);
}

LRESULT win32_main_window_proc(HWND window, UINT msg, WPARAM wparam, LPARAM lparam)
{
    LRESULT result = 0;
    
    switch(msg)
    {
        case WM_DESTROY:
        case WM_CLOSE:
        {
            win32_variables.is_running = false;
        }break;
        
        case WM_SYSKEYDOWN:
        case WM_SYSKEYUP:
        case WM_KEYDOWN:
        case WM_KEYUP:
        {
            uint64 vk_code = (uint32)wparam;
            int32 was_down = ((lparam & (1 << 30)) != 0);
            int32 is_down = ((lparam & (1 << 31)) == 0);
#if 0
            if(vk_code >= VK_F1 && vk_code <= VK_F11)
            {
                event.key = Key_F1 + (vk_code - VK_F1);
            }
#endif
            
            Event event = {};
            
            // NOTE(alexey): Can we have multiple key modifiers?
            if(GetKeyState(VK_MENU) & 0x8000)
            {
                event.key_modifiers |= KeyModifier_Alt;
            }
            
            if(GetKeyState(VK_SHIFT) & 0x8000)
            {
                event.key_modifiers |= KeyModifier_Shift;
            }
            
            if(GetKeyState(VK_CONTROL) & 0x8000)
            {
                event.key_modifiers |= KeyModifier_Ctrl;
            }
            

#if 0            
            if((GetKeyState('W') & 0x8000) && vk_code == 'D')
            {
                __debugbreak();
            }
#endif

            if(vk_code == 'W')
            {
                event.key = Key_W;
            }
            else if(vk_code == 'A')
            {
                event.key = Key_A;
            }
            else if(vk_code == 'S')
            {
                event.key = Key_S;
            }
            else if(vk_code == 'D')
            {
                event.key = Key_D;
            }
            else if(vk_code == VK_LEFT)
            {
                event.key = Key_Left;
            }
            else if(vk_code == VK_UP)
            {
                event.key = Key_Up;
            }
            else if(vk_code == VK_RIGHT)
            {
                event.key = Key_Right;
            }
            else if(vk_code == VK_DOWN)
            {
                event.key = Key_Down;
            }
            else if(vk_code == VK_F1)
            {
                event.key = Key_F1;
            }
            else if(vk_code == VK_F2)
            {
                event.key = Key_F2;
            }
            else if(vk_code == VK_F3)
            {
                event.key = Key_F3;
            }
            else if(vk_code == VK_F4)
            {
                event.key = Key_F4;
            }
            else if(vk_code == VK_F5)
            {
                event.key = Key_F5;
            }
            else if(vk_code == VK_F6)
            {
                event.key = Key_F6;
            }
            else if(vk_code == VK_F7)
            {
                event.key = Key_F7;
            }
            else if(vk_code == VK_F8)
            {
                event.key = Key_F8;
            }
            else if(vk_code == VK_F9)
            {
                event.key = Key_F9;
            }
            else if(vk_code == VK_F10)
            {
                event.key = Key_F10;
            }
            else if(vk_code == VK_F11)
            {
                event.key = Key_F11;
            }
            else if(vk_code == VK_MENU)
            {
                if(event.key_modifiers & KeyModifier_Alt)
                {
                    event.key_modifiers ^= KeyModifier_Alt;
                }
                event.key = Key_Alt; 
            }
            else if(vk_code == VK_CONTROL)
            {
                if(event.key_modifiers & KeyModifier_Ctrl)
                {
                    event.key_modifiers ^= KeyModifier_Ctrl;
                }   
                event.key = Key_Ctrl; 
            }
            else if(vk_code == VK_SHIFT)
            {
                if(event.key_modifiers & KeyModifier_Shift)
                {
                    event.key_modifiers ^= KeyModifier_Shift;
                }
                event.key = Key_Shift; 
            }
            
            if(is_down)
            {
                event.type = EventType_KeyPressed; 
            }
            else
            {
                event.type = EventType_KeyReleased;
            }
            
            os_instance.events.push_back(event);
            
            if((lparam & (1 << 29)) && (vk_code == VK_F4))
            {
                win32_variables.is_running = false;
            }
        }break;
        
        case WM_MOUSEMOVE:
        {
            Vec2 cursor_pos = win32_get_cursor_pos(window);
            
            Event event = {};
            event.type = EventType_MouseMoved;
            event.cursor.x = cursor_pos.x;
            event.cursor.y = cursor_pos.y;
            
            os_instance.events.push_back(event);
        }break;
        
        // NOTE(alexey): We have code duplication, probably it would be better to use GetKeyState?
        case WM_LBUTTONDOWN:
        {
            Event event = {};
            event.type = EventType_MouseButtonPressed;
            event.button = MouseButton_Left;
            os_instance.events.push_back(event);
        }break;
        case WM_LBUTTONUP:
        {
            Event event = {};
            event.type = EventType_MouseButtonReleased;
            event.button = MouseButton_Left;
            os_instance.events.push_back(event);
        }break;
        
        // TODO(alexey): Track down when the mouse leaves the client area.
        
        default:
        {
            result = DefWindowProcA(window, msg, wparam, lparam);
        }break;
    }
    
    return result;
}

static void win32_get_exe_full_path(Win32Variables *variables)
{
    GetModuleFileNameA(0, variables->exe_file_path, sizeof(variables->exe_file_path));
    DWORD error = GetLastError();
    assert(error != ERROR_INSUFFICIENT_BUFFER);
    
    char *start, *end;
    start = end = variables->exe_file_path;
    
    for(char *at = start; *at; ++at)
    {
        if(*at == '\\')
        {
            end = at + 1; // points one past last slash, like end() iterator.
        }
    }
    
    strncpy_s(variables->one_past_slash, start, (end - start));
}

static void win32_build_game_dll_path(Win32Variables *variables, const char *game_dll_name)
{
    strcat_s(variables->game_dll_full_path, variables->one_past_slash);
    strcat_s(variables->game_dll_full_path, game_dll_name);
}

int WINAPI WinMain(HINSTANCE hInstance,
                   HINSTANCE hPrevInstance,
                   LPSTR cmd_line,
                   int show_code)
{
    // TODO(alexey): Initialize Os at one place, don't spread out the initialization.
    
    uint64 frequency = win32_frequency();
    os_instance.frequency = frequency;
    
    uint64 start_counts, end_counts;
    
    win32_get_exe_full_path(&win32_variables);
    win32_build_game_dll_path(&win32_variables, "game.dll");
    
    Win32GameCode game_code = win32_load_game_code(&win32_variables);
    
    WNDCLASSA window_class = {};
    
    // NOTE(alexey): CS_OWNDC allocates unique device context for each window in a class.
    window_class.style = CS_HREDRAW | CS_VREDRAW | CS_OWNDC;
    window_class.lpfnWndProc = win32_main_window_proc;
    window_class.hInstance = GetModuleHandle(0);
//    window_class.hIcon;
//    window_class.hCursor;
    window_class.hbrBackground = (HBRUSH)GetStockObject(DKGRAY_BRUSH);
//    window_class.lpszMenuName;
    window_class.lpszClassName = "GameWindowClass";
    
    real32 seconds_per_frame = 1.0f/60.0f;
    
    os_instance.dt_for_frame = seconds_per_frame;
    os_instance.get_qpc = win32_qpc;
    os_instance.alloc_memory = win32_alloc_memory;
    os_instance.free_memory = win32_free_memory;
        
    bool32 sleep_is_accurate = false;
    
    TIMECAPS timer_resolution;
    timeGetDevCaps(&timer_resolution, sizeof(TIMECAPS));
    Win32TimePeriodScoped time_period;
    if(time_period.result == TIMERR_NOERROR)
    {
        sleep_is_accurate = true;
    }
    
    if(RegisterClassA(&window_class))
    {
        HWND main_window = CreateWindowExA(0,
                                           window_class.lpszClassName, 
                                           "GameMainWindow",
                                           WS_OVERLAPPEDWINDOW, 
                                           CW_USEDEFAULT,
                                           CW_USEDEFAULT,
                                           CW_USEDEFAULT,
                                           CW_USEDEFAULT,
                                           0, 
                                           0, 
                                           window_class.hInstance,
                                           0);
        if(main_window)
        {
            win32_resize_dib_section(&win32_variables.buffer, 1080, 720);
            Win32DeviceContextScoped device_context(main_window);
            win32_init_opengl(device_context.dc);
            
            os_instance.permanent_memory_size = Gb(2);
            os_instance.frame_memory_size = Gb(2);
            
            uint64 alloc_size = (os_instance.permanent_memory_size + os_instance.frame_memory_size);
            os_instance.permanent_memory = win32_alloc_memory(alloc_size);
            os_instance.frame_memory = (void *)((char *)os_instance.permanent_memory + os_instance.permanent_memory_size);
            
            os_instance.buffer.width = win32_variables.buffer.width;
            os_instance.buffer.height = win32_variables.buffer.height;
            os_instance.buffer.pitch = win32_variables.buffer.pitch;
            os_instance.buffer.data = win32_variables.buffer.data;
            os_instance.buffer.bpp = win32_variables.buffer.bpp;
            
            ShowWindow(main_window, SW_SHOW);
            win32_variables.is_running = true;
            
            start_counts = win32_qpc();
            while(win32_variables.is_running)
            {
                MSG msg;
                while(PeekMessageA(&msg, 0, 0, 0, PM_REMOVE))
                {
//                    TranslateMessage(&msg);
                    DispatchMessageA(&msg);
                }
                
                Vec2 window_size = win32_get_window_size(main_window);
                os_instance.width = window_size.x;
                os_instance.height = window_size.y;
                
                game_code.update_and_render(&os_instance);
                
                // TODO(alexey): Do I have to include time spend to displaying the buffer
                // into the frame's time?
                window_size = win32_get_window_size(main_window);
                win32_display_offscreen_buffer_in_window(device_context.dc, 
                                                         &win32_variables.buffer, 
                                                         (int32)window_size.x, 
                                                         (int32)window_size.y);
                
                // NOTE(alexey): We have to sleep after we've updated and rendered out game
                // but before displaying an offscreen buffer.
                if(sleep_is_accurate)
                {
                    real32 elapsed_seconds = win32_elapsed_seconds(start_counts, frequency);
                    while(elapsed_seconds < seconds_per_frame)
                    {
                        DWORD sleep_time = (DWORD)((seconds_per_frame - elapsed_seconds) * 1000.0f);
                        Sleep(sleep_time);
                        elapsed_seconds = win32_elapsed_seconds(start_counts, frequency);
                    }
                }
                
#ifdef HARDWARE_RENDERER
                //SwapBuffers(device_context.dc);
#endif
                
                end_counts = win32_qpc();
                int64 elapsed_counts = (int64)(end_counts - start_counts);
                real32 elapsed_seconds = ((real32)elapsed_counts * (1.0f / (real32)frequency));
                real32 elapsed_milliseconds = elapsed_seconds * 1000.0f;
                real32 fps = (1.0f / (real32)(elapsed_counts)) * (real32)frequency;
                    
#if 0
                DebugOut("ElapsedMl: %f\nFps: %f\n\n", elapsed_milliseconds, fps);
#endif
                start_counts = end_counts;
            }
        }
        else
        {
            // TODO(alexey): Error handling, failed to create main window.
        }
    }
    else
    {
        // TODO(alexey): Error handling, failed to register window class.
    }
    
    return 0;
}