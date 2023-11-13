/* date = October 5th 2023 7:07 pm */
#ifndef OS_H
#include <vector>
#include <stdint.h>
#include <assert.h>
#include <math.h>
#include <limits.h>

#define Int32Max INT_MAX
#define Uint32Max UINT_MAX

#define function static
#include "game_dynamic_array.h"

typedef int64_t int64;
typedef int32_t int32;
typedef int16_t int16;
typedef uint8_t uint8;
typedef uint16_t uint16;
typedef uint32_t uint32;
typedef uint64_t uint64;
typedef int32 bool32;
typedef float real32;

typedef int32 I32;
typedef uint8 U8;
typedef uint16 U16;
typedef uint32 U32;
typedef uint64 U64;
typedef real32 F32;
typedef bool32 Bool32;

#define Gb(n) (uint64)(Mb(n) * (1024ull))
#define Mb(n) (uint64)(Kb(n) * (1024ull))
#define Kb(n) (uint64)((n) * (1024ull))

#define Cast(type, value) (type)(value)

function int32 floor_real32_to_int32(real32 value)
{
    // TODO(alexey): Replace floor from CRT with SS2 instruction.
    int32 result = (int32)floor(value);
    return result;
}

function int32 round_real32_to_int32(real32 value)
{
    int32 result = (int32)(value + 0.5f);
    return result;
}

function uint32 round_real32_to_uint32(real32 value)
{
    uint32 result = (uint32)(value + 0.5f);
    return result;
}

struct OffscreenBuffer
{
    int32 width;
    int32 height;
    int32 pitch;
    void *data;
    int32 bpp;
};

union Vec2
{
    Vec2(real32 v=0.0f) : x(v), y(v) {}
    Vec2(real32 x_, real32 y_):  x(x_), y(y_){}
    
    Vec2& operator+=(const Vec2& rhs)
    {
        x += rhs.x;
        y += rhs.y;
        return *this;
    }
        
    struct
    {
        real32 x;
        real32 y;
    };
    real32 e[2];
};

union Vec4
{
    Vec4(real32 v=0.0f) : x(v), y(v), z(v), w(v){}
    Vec4(real32 x_, real32 y_, real32 z_, real32 w_=1.0f)
        : x(x_), y(y_), z(z_), w(w_){}
    
    struct
    {
        real32 x, y, z, w;
    };
    struct
    {
        real32 r, g, b, a;
    };
    real32 e[4];
};

union Vec3
{
    Vec3(real32 v=0.0f) : x(v), y(v), z(v){}
    Vec3(real32 x_, real32 y_, real32 z_) : x(x_), y(y_), z(z_){}
        
    struct 
    {
        real32 x, y, z;
    };
    struct
    {
        real32 r, g, b;
    };
    
    real32 e[3];
};

union Rect2
{
    Rect2(Vec2 min_=Vec2(), Vec2 max_=Vec2())
        : min(min_), max(max_){}
    
    struct 
    {
        Vec2 min;
        Vec2 max;
    };
    Vec2 e[2];
};

enum EventType
{
    EventType_KeyPressed = (1 << 0x1),
    EventType_KeyReleased,
    EventType_MouseMoved,
    
    EventType_MouseButtonPressed,
    EventType_MouseButtonReleased,
};

enum Key
{
    Key_W,
    Key_A, 
    Key_S, 
    Key_D, 
    Key_Left, 
    Key_Up,
    Key_Right, 
    Key_Down,
    Key_F1,
    Key_F2,
    Key_F3,
    Key_F4,
    Key_F5,
    Key_F6,
    Key_F7,
    Key_F8,
    Key_F9,
    Key_F10,
    Key_F11,
    Key_Alt, 
    Key_Ctrl, 
    Key_Shift,
};

enum MouseButton
{
    MouseButton_Left,
    MouseButton_Right,
    MouseButton_Middle,
    MouseButton_FirstX,
    MouseButton_SecondX,
};

enum KeyModifier
{
    // TODO(alexey): Distinguish between left and right?
    KeyModifier_Alt = (1 << 0x1),
    KeyModifier_Shift = (1 << 0x2),
    KeyModifier_Ctrl = (1 << 0x3),
};

// NOTE(alexey): In the future we can serialize, or write all the events to the file
// and play them back. The same way Casey does for the input struct.
struct Event
{
    int32 type;
    int32 key;
    int32 key_modifiers;
    
    // mouse
    Vec2 cursor;
    int32 button;
    
    // TODO(alexey): Can we hit multiple buttons simultaneously?
    // I think we does, but it won't make any sense (logically).
};

bool32 event_equal(const Event& event, EventType type)
{
    return (event.type == type);
}

struct Input
{
    F32 dt_for_frame;
    
    int32 keys[256];
    int32 mouse_buttons[5];
    
    bool32 onKeyPressed(Key key);
    bool32 onKeyReleased(Key key); // not implemented yet!
    
    bool32 onMouseButtonPressed(MouseButton button);
};

inline bool32 Input::onKeyPressed(Key key)
{ 
    assert(key >= 0 && key < 256); 
    return keys[key];
}

inline bool32 Input::onMouseButtonPressed(MouseButton button) 
{ 
    assert(button >= 0 && button < 5); 
    return mouse_buttons[button];
}

struct Os
{
    // NOTE(alexey): This is uses it's own memory allocated via malloc.
    // Think how can we make it be a part of game memory.
    Array<Event> events;
    Input input;
    
    real32 dt_for_frame;
    
    // timing
    uint64  frequency;
    uint64 (*get_qpc)();
    
    // memory
    void *(*alloc_memory)(size_t);
    void (*free_memory)(void *);
    void *permanent_memory;
    uint64 permanent_memory_size;
    void *frame_memory;
    uint64 frame_memory_size;
    
    OffscreenBuffer buffer;
    
    // window metrics
    real32 width;
    real32 height;
};

static Os *os;

#define GAME_UPDATE_AND_RENDER(name) void name(Os *os_)
GAME_UPDATE_AND_RENDER(game_update_and_render_stub) {} 
typedef void (*GameUpdateAndRenderPtr)(Os *);

#define OS_H
#endif //OS_H
