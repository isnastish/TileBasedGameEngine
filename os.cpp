#include "os.h"
#include "game.h"

#ifdef _WIN32
# ifdef function
# undef function
# include <windows.h>
# include <gl/gl.h>
# define function
# endif
# define DebugBreak() __debugbreak()
# define DebugOut(format, ...)\
{\
char buf[1024] = {};\
sprintf_s(buf, sizeof(buf), format, ## __VA_ARGS__);\
OutputDebugStringA(buf);\
}
#undef min
#undef max
#endif

#define TilesCountX 17 
#define TilesCountY 9

GameWorld::GameWorld(TileMap *maps, int32 tile_map_count_x, int32 tile_map_count_y, int32 tile_count_x, 
                     int32 tile_count_y, 
                     int32 offset_x, int32 offset_y, real32 tile_dim, real32 tile_side_in_pixels, 
                     real32 tile_side_in_meters) 
: m_maps(maps),
m_tile_map_count_x(tile_map_count_x), 
m_tile_map_count_y(tile_map_count_y),
m_tile_count_x(tile_count_x),
m_tile_count_y(tile_count_y),
m_offset_x(offset_x),
m_offset_y(offset_y),
m_tile_dim(tile_dim),
m_tile_half_dim(0.5f*tile_dim),
m_tile_side_in_pixels(tile_side_in_pixels),
m_tile_side_in_meters(tile_side_in_meters),
m_half_tile_side_in_meters(0.5f*tile_side_in_meters),
m_pixels_per_meter(tile_side_in_pixels / tile_side_in_meters)
{}

uint32 GameWorld::getTileValue(TileMap *tile_map, int32 test_tile_x, int32 test_tile_y)
{
    assert((test_tile_x >= 0 && test_tile_x < m_tile_count_x) &&
           (test_tile_y >= 0 && test_tile_y < m_tile_count_y));
    
    int32 index = ((m_tile_count_y - test_tile_y - 1)*m_tile_count_x) + test_tile_x;
    uint32 value = tile_map->tiles[index];
    
    return value;
}

bool32 GameWorld::isDoorTile(TileMap *tile_map, int32 test_tile_x, int32 test_tile_y)
{
    bool32 result = false;
    
    if((test_tile_x >= 0 && test_tile_x < m_tile_count_x) &&
       (test_tile_y >= 0 && test_tile_y < m_tile_count_y))
    {
        uint32 tile_value = getTileValue(tile_map, test_tile_x, test_tile_y);
        result = (tile_value == 2);
    }
    
    return result;
}

bool32 GameWorld::isTileEmpty(TileMap *tile_map, int32 test_tile_x, int32 test_tile_y)
{
    bool32 result = false;
    
    if((test_tile_x >= 0 && test_tile_x < m_tile_count_x) &&
       (test_tile_y >= 0 && test_tile_y < m_tile_count_y))
    {
        uint32 value = getTileValue(tile_map, test_tile_x, test_tile_y);
        result = (value == 0);
    }
    
    return result;
}

TileMap *GameWorld::getWorldTileMap(int32 test_tile_map_x, int32 test_tile_map_y)
{
    TileMap *result = NULL;
    
    if((test_tile_map_x >= 0 && test_tile_map_x < m_tile_map_count_x) &&
       (test_tile_map_y >= 0 && test_tile_map_y < m_tile_map_count_y))
    {
        int32 index = (m_tile_map_count_y - test_tile_map_y - 1)*m_tile_map_count_x + test_tile_map_x;
        result = &m_maps[index];
    }
    
    return result;
}

WorldPos GameWorld::recomputeWorldPos(WorldPos world_pos)
{
    WorldPos result = world_pos;
    
    int32 tile_x_offset = 
        floor_real32_to_int32((result.tile_center_rel_x+m_half_tile_side_in_meters) / m_tile_side_in_meters);
    
    int32 tile_y_offset =
        floor_real32_to_int32((result.tile_center_rel_y+m_half_tile_side_in_meters) / m_tile_side_in_meters);
    
    result.tile_x += tile_x_offset;
    result.tile_y += tile_y_offset;
    
    result.tile_center_rel_x -= tile_x_offset*m_tile_side_in_meters;
    result.tile_center_rel_y -= tile_y_offset*m_tile_side_in_meters;
    
    // TODO(alexey): We might end up in a situation when result.tile_rel_x/y is presicely equal to 60.0f
    // which is world->tile_dim.
    // In order not to hit this assertion all the time, we might use <= world->tile_dim, 
    // but it's wrong and should be fixed properly.
    /*     
        real32 e = 0.0000020;
        assert(result.tile_rel_x >= -world->tile_half_dim - e);
        assert(result.tile_rel_x <= world->tile_half_dim + e);
        assert(result.tile_rel_y >= -world->tile_half_dim - e);
        assert(result.tile_rel_y <= world->tile_half_dim + e);
     */
    
    // NOTE(alexey): When we switched from pixels to meters the problem vanished.
    assert(result.tile_center_rel_x > -m_half_tile_side_in_meters);
    assert(result.tile_center_rel_x < m_half_tile_side_in_meters);
    assert(result.tile_center_rel_y > -m_half_tile_side_in_meters);
    assert(result.tile_center_rel_y < m_half_tile_side_in_meters);
    
#if INTERNAL_BUILD
    DebugOut("TileOffset: (%i, %i)\nTileRel:(%.2f, %.2f)\nTile:(%i, %i)\n\n",
             tile_x_offset, 
             tile_y_offset, 
             result.tile_center_rel_x, 
             result.tile_center_rel_y, 
             (int32)result.tile_x, 
             (int32)result.tile_y);
#endif
    
    if(result.tile_x < 0)
    {
        result.tile_x += m_tile_count_x;
        result.tile_map_x -= 1;
    }
    
    if(result.tile_x >= m_tile_count_x)
    {
        result.tile_x -= m_tile_count_x;
        result.tile_map_x += 1;
    }
    
    if(result.tile_y < 0)
    {
        result.tile_y += m_tile_count_y;
        result.tile_map_y -= 1;
    }
    
    if(result.tile_y >= m_tile_count_y)
    {
        result.tile_y -= m_tile_count_y;
        result.tile_map_y += 1;
    }
    
    return result;
}

bool32 GameWorld::isTileMapPointEmpty( WorldPos world_pos)
{
    TileMap *tile_map = getWorldTileMap((int32)world_pos.tile_map_x, (int32)world_pos.tile_map_y);
    bool32 result = isTileEmpty(tile_map, (int32)world_pos.tile_x, (int32)world_pos.tile_y);
    if(!result)
    {
        result = isDoorTile(tile_map, (int32)world_pos.tile_x, (int32)world_pos.tile_y);
    }
    
    return result;
}

enum RectangleStyle
{
    RectangleStyle_Filled = 1,
    RectangleStyle_Wireframe,
};

// NOTE(alexey): This is a part of the renderer class!

/*
(minx, miny)
      +------------+
      |            |
      |            |
      |            |
      |            |
      +------------+ (maxx, maxy)
*/
function void draw_rectangle(OffscreenBuffer *buffer,
                             RectangleStyle draw_style,
                             real32 rminx, real32 rminy, 
                             real32 rmaxx, real32 rmaxy,
                             Vec4 color)
{
    I32 minx = round_real32_to_int32(rminx);
    I32 miny = round_real32_to_int32(rminy);
    I32 maxx = round_real32_to_int32(rmaxx);
    I32 maxy = round_real32_to_int32(rmaxy);
    
    // minx, miny
    if(minx < 0)
    {
        minx = 0;
    }
    
    if(minx > buffer->width)
    {
        minx = buffer->width;
    }
    
    if(miny < 0)
    {
        miny = 0;
    }
    
    if(miny > buffer->height)
    {
        miny = buffer->height;
    }
    
    // maxx, maxy
    if(maxx < 0)
    {
        maxx = 0;
    }
    
    if(maxx > buffer->width)
    {
        maxx = buffer->width;
    }
    
    if(maxy < 0)
    {
        maxy = 0;
    }
    
    if(maxy > buffer->height)
    {
        maxy = buffer->height;
    }
    
    uint32 coloru32 = 
    round_real32_to_uint32(color.b * 255.0f) | 
    (round_real32_to_uint32(color.g * 255.0f) << 8) | 
    (round_real32_to_uint32(color.r * 255.0f) << 16) |  
    (round_real32_to_uint32(color.a * 255.0f) << 24); 
    
    uint8 *row = (uint8 *)buffer->data + miny*buffer->pitch + minx*buffer->bpp;
    
    if(draw_style == RectangleStyle_Filled)
    {
        for(I32 y = miny; y < maxy; ++y)
        {
            uint32 *pixels = (uint32 *)row;
            for(I32 x = minx; x < maxx; ++x)
            {
                // 0x BB GG RR AA
                *pixels++ = coloru32;
            }
            row += buffer->pitch;
        }    
    }
    else if(draw_style == RectangleStyle_Wireframe)
    {
        for(I32 y = miny; y < maxy; ++y)
        {
            uint32 *pixels = (uint32 *)row;
            for(I32 x = minx; x < maxx; ++x)
            {
                int32 frame_y = (y == miny) || (y == maxy - 1);
                int32 frame_x = (x == minx) || (x == maxx - 1);
                if(frame_y || frame_x)
                {
                    // 0x BB GG RR AA
                    *pixels = coloru32;
                }
                ++pixels;
            }
            row += buffer->pitch;
        }
    }
    else
    {
        assert(!"Unknown draw style!");
    }
}

function void handleOsEvents()
{
    for(I32 event_index = 0;
        event_index < os->events.length();
        ++event_index)
    {
        Event& event = os->events[event_index];
        if(event_equal(event, EventType_KeyPressed))
        {
            os->input.keys[event.key] = 1;
        }
        else if(event_equal(event, EventType_KeyReleased))
        {
            os->input.keys[event.key] = 0;
        }
        else if(event_equal(event, EventType_MouseButtonPressed))
        {
            os->input.mouse_buttons[event.button] = 1;
        }
        else if(event_equal(event, EventType_MouseButtonReleased))
        {
            os->input.mouse_buttons[event.button] = 0;
        }
    }
    os->events.clear();
}

void GameState::update(Input *input/*...*/)
{
    F32 dt_player_x = 0.0f;
    F32 dt_player_y = 0.0f;
    
    if(input->onKeyPressed(Key_W) ||
       input->onKeyPressed(Key_Up))
    {
        dt_player_y += 1.0f;
    }
    
    if(input->onKeyPressed(Key_A) ||
       input->onKeyPressed(Key_Left))
    {
        dt_player_x -= 1.0f;
    }
    
    if(input->onKeyPressed(Key_S) ||
       input->onKeyPressed(Key_Down))
    {
        dt_player_y -= 1.0f;
    }
    
    if(input->onKeyPressed(Key_D) || 
       input->onKeyPressed(Key_Right))
    {
        dt_player_x += 1.0f;
    }
    
    // Going from pixels to meters
    dt_player_x *= m_player_speed_in_meters;
    dt_player_y *= m_player_speed_in_meters;
    
    // NOTE(alexey): To make player's movement frame independent.
    // So it moves the same amount of pixels regardless of frame rate.
    // For example: 100 pixels/second when it's 60/30fps.
    WorldPos new_world_pos = m_world_pos;
    new_world_pos.tile_center_rel_x += (dt_player_x * input->dt_for_frame);
    new_world_pos.tile_center_rel_y += (dt_player_y * input->dt_for_frame);
    new_world_pos = m_world->recomputeWorldPos(new_world_pos);
    
    /*
      Player's rectangle: 
      |______|
      * <- left canonical pos.
    */
    WorldPos left_world_pos = m_world_pos;
    left_world_pos.tile_center_rel_x += (dt_player_x*input->dt_for_frame) - (0.5f * m_player_dim.x);
    left_world_pos.tile_center_rel_y += (dt_player_y * input->dt_for_frame);
    left_world_pos = m_world->recomputeWorldPos(left_world_pos);
    
    /*
      Player's rectangle:
      |______|
             * <- right canonical pos.
    */
    WorldPos right_world_pos = m_world_pos;
    right_world_pos.tile_center_rel_x += (dt_player_x*input->dt_for_frame) + (0.5f*m_player_dim.x);
    right_world_pos.tile_center_rel_y += (dt_player_y*input->dt_for_frame);
    right_world_pos = m_world->recomputeWorldPos(right_world_pos);
    
    /*
      Player's rectangle:
     *______ 
     |      |
    */
    WorldPos left_top_world_pos = left_world_pos;
    left_top_world_pos.tile_center_rel_y += 0.45f*m_player_dim.y;
    left_top_world_pos = m_world->recomputeWorldPos(left_top_world_pos);
    
    /*
      Player's rectangle:
      ______*
     |      |
    */
    WorldPos right_top_world_pos = right_world_pos;
    right_top_world_pos.tile_center_rel_y += 0.45f*m_player_dim.y;
    right_top_world_pos = m_world->recomputeWorldPos(right_top_world_pos);
    
    if(m_world->isTileMapPointEmpty(new_world_pos) &&
       m_world->isTileMapPointEmpty(left_world_pos) &&
       m_world->isTileMapPointEmpty(right_world_pos) &&
       m_world->isTileMapPointEmpty(left_top_world_pos) &&
       m_world->isTileMapPointEmpty(right_top_world_pos))
    {
        m_world_pos = new_world_pos;
    }
}

void GameState::render(OffscreenBuffer *buffer)
{
    // flush background.
    draw_rectangle(buffer, RectangleStyle_Filled,
                   0.0f, 0.0f, 
                   Cast(F32, buffer->width),
                   Cast(F32, buffer->height), 
                   Vec4(236, 213, 160));
    
    TileMap *map = m_world->getWorldTileMap(Cast(I32, m_world_pos.tile_map_x),
                                            Cast(I32, m_world_pos.tile_map_y));
    for(I32 tile_y = 0;
        tile_y < m_world->m_tile_count_y;
        ++tile_y)
    {
        for(I32 tile_x = 0;
            tile_x < m_world->m_tile_count_x;
            ++tile_x)
        {
            /*
              +------------+ (maxy, maxy) (world->tile_dim, world->tile_dim).
              |            |
              |            |
              |            |
              |            |
              |            |
              +------------+
             (minx, miny), (0, 0)
            */
            F32 minx = (tile_x * m_world->m_tile_dim) + m_world->m_offset_x;
            F32 miny = (tile_y * m_world->m_tile_dim) + m_world->m_offset_y;
            F32 maxx = minx + m_world->m_tile_dim;
            F32 maxy = miny + m_world->m_tile_dim;
            
            if(m_world_pos.tile_x == tile_x &&
               m_world_pos.tile_y == tile_y)
            {
                Vec4 color(0.8f, 0.7f, 0.54f);
                draw_rectangle(buffer, RectangleStyle_Filled, minx, miny, maxx, maxy, color);
            }
            else if(!m_world->isTileEmpty(map, tile_x, tile_y))
            {
                U32 tile_value = m_world->getTileValue(map, tile_x, tile_y);
                Vec4 color = (tile_value == 2) ? Vec4(0.25f, 0.5f, 0.5f) : Vec4(0.25f);
                draw_rectangle(buffer, RectangleStyle_Filled, minx, miny, maxx, maxy, color);
            }
            
            // draw debug points.
            Vec4 color(1.0f, 1.0f, 0.0f);
            
            Rect2 r0(Vec2(minx - 3, miny - 3), Vec2(minx + 3, miny + 3));
            draw_rectangle(&os->buffer, RectangleStyle_Filled, r0.min.x, r0.min.y, r0.max.x, r0.max.y, color);
            
            Rect2 r1(Vec2(maxx - 3, miny - 3), Vec2(maxx + 3, miny + 3));
            draw_rectangle(buffer, RectangleStyle_Filled, r1.min.x, r1.min.y, r1.max.x, r1.max.y, color);
            
            Rect2 r2(Vec2(minx - 3, maxy - 3), Vec2(minx + 3, maxy + 3));
            draw_rectangle(buffer, RectangleStyle_Filled, r2.min.x, r2.min.y, r2.max.x, r2.max.y, color);
            
            Rect2 r3(Vec2(maxx - 3, maxy - 3), Vec2(maxx + 3, maxy + 3));
            draw_rectangle(buffer, RectangleStyle_Filled, r3.min.x, r3.min.y, r3.max.x, r3.max.y, color);
            
            // Draw fram for each tile.
            Vec4 frame_color(0.8f, 0.788f, 0.65f);
            draw_rectangle(buffer, RectangleStyle_Wireframe, minx, miny, maxx, maxy, frame_color);
        }
    }
    
    // If we have meters, we would have to multiply meters * world->pixels_per_meter.
    // In order to convert to pixels.
    // compute player's absolute position
    F32 player_abs_x = 
    (m_world_pos.tile_x*m_world->m_tile_dim) + m_world->m_offset_x +
    (m_world_pos.tile_center_rel_x*m_world->m_pixels_per_meter) + m_world->m_tile_half_dim; 
    
    F32 player_abs_y = 
    (m_world_pos.tile_y * m_world->m_tile_dim) + m_world->m_offset_y +
    (m_world_pos.tile_center_rel_y*m_world->m_pixels_per_meter) + m_world->m_tile_half_dim;
    
    F32 player_minx = player_abs_x - (0.5f*m_player_dim.x*m_world->m_pixels_per_meter);
    F32 player_miny = player_abs_y;
    
    F32 player_maxx = player_minx + m_player_dim.x*m_world->m_pixels_per_meter;
    F32 player_maxy = player_miny + m_player_dim.y*m_world->m_pixels_per_meter;
    
#if 0    
    DebugOut("PlayerAbsolute: (%.2f, %.2f)\nPlayerMin: (%.2f, %.2f)\nPlayerMax(%.2f, %.2f)\n\n", 
             player_abs_x, 
             player_abs_y,
             player_minx, 
             player_miny, 
             player_maxx, 
             player_maxy);
#endif
    
    // draw player
    Vec4 player_color(0.80f, 1.0f, 0.44f);
    draw_rectangle(&os->buffer, RectangleStyle_Filled, player_minx, player_miny, 
                   player_maxx, player_maxy, player_color);
    
    // Draw player's collision box.
    F32 maxy = player_abs_y + 0.45f*m_player_dim.y*m_world->m_pixels_per_meter;
    draw_rectangle(buffer, RectangleStyle_Wireframe, player_minx, player_miny,
                   player_maxx, maxy, Vec4(0.95f, 0.21f, 1.0f));
    
    Vec4 debug_color(1.0f, 0.0f, 0.47f);
    // Draw debug rect for player's gravity center.
    {
        Vec2 min(player_abs_x - 4.0f, player_abs_y - 4.0f);
        Vec2 max(player_abs_x + 4.0f, player_abs_y + 4.0f);
        draw_rectangle(buffer, RectangleStyle_Wireframe, min.x, min.y, max.x, max.y, debug_color);
    }
    
    // Draw debug rect for player's left point.
    {
        Vec2 min(player_minx - 4.0f, player_abs_y - 4.0f);
        Vec2 max(player_minx + 4.0f, player_abs_y + 4.0f);
        draw_rectangle(buffer,RectangleStyle_Wireframe,  min.x, min.y, max.x, max.y, debug_color);
    }
    
    // Draw debug rect for player's right point.
    {
        F32 x = player_abs_x + (0.5f*m_player_dim.x*m_world->m_pixels_per_meter); 
        Vec2 min(x - 4.0f, player_abs_y - 4.0f);
        Vec2 max(x + 4.0f, player_abs_y + 4.0f);
        draw_rectangle(buffer, RectangleStyle_Wireframe, min.x, min.y, max.x, max.y, debug_color);
    }
    
    // Draw debug rect of player's top left point
    {
        F32 x = (player_abs_x - 0.5f*m_player_dim.x*m_world->m_pixels_per_meter);
        F32 y = (player_abs_y + 0.45f*m_player_dim.y*m_world->m_pixels_per_meter);
        Vec2 min(x - 4.0f, y - 4.0f);
        Vec2 max(x + 4.0f, y + 4.0f);
        draw_rectangle(buffer, RectangleStyle_Wireframe, min.x, min.y, max.x, max.y, debug_color);
    }
    
    // Draw debug rect of player's top right point
    {
        F32 x = (player_abs_x + 0.5f*m_player_dim.x*m_world->m_pixels_per_meter);
        F32 y = (player_abs_y + 0.45f*m_player_dim.y*m_world->m_pixels_per_meter);
        Vec2 min(x - 4.0f, y - 4.0f);
        Vec2 max(x + 4.0f, y + 4.0f);
        draw_rectangle(buffer, RectangleStyle_Wireframe, min.x, min.y, max.x, max.y, debug_color);
    }
}

function void init_game_state(GameState *state)
{
    if(!state->m_is_initialized)
    {
        state->m_world_pos.tile_x = 2;
        state->m_world_pos.tile_y = 2;
        state->m_world_pos.tile_center_rel_x = 0;
        state->m_world_pos.tile_center_rel_y = 0;
        state->m_world_pos.tile_map_x = 0;
        state->m_world_pos.tile_map_y = 0;
        state->m_player_dim.x = 1.4f * 0.85f;
        state->m_player_dim.y = 1.4f;
        state->m_player_speed_in_meters = 3.5f;
        
        state->m_is_initialized = true;
    }
}

extern "C" __declspec(dllexport) GAME_UPDATE_AND_RENDER(game_update_and_render)
{
    os = os_;
    
    assert(sizeof(GameState) <= os->permanent_memory_size);
    GameState *game_state = (GameState *)os->permanent_memory;
    
    // subtract the size of GameState from permanent storage size.
    os->permanent_memory_size -= sizeof(GameState);
    
    init_game_state(game_state);
    
    // Process events from the platform layer.
    handleOsEvents();
    
    // NOTE(alexey): Currently we mem-copying tile maps into memory every frame,
    // but we have to do it only once.
    uint8 *at = (uint8 *)os->permanent_memory + sizeof(GameState);
    U32 *tiles = (U32 *)((uint8 *)os->permanent_memory + sizeof(GameState));
    
    U32 tile_map00[TilesCountY][TilesCountX] =
    {
        {1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1},
        {1, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 1, 1},
        {1, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, 2},
        {1, 0, 0, 1, 0, 0, 0, 0, 1, 1, 0, 0, 0, 1, 0, 0, 1},
        {1, 0, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 0, 0, 1},
        {1, 0, 0, 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 1},
        {1, 0, 0, 1, 0, 0, 0, 0, 1, 1, 1, 1, 1, 1, 1, 0, 1},
        {1, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 1},
        {1, 2, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1},
    };

/*     
    size_t tile_map_size = sizeof(tile_map00);
    if(tile_map_size <= os->permanent_memory_size)
    {
        memcpy(at, tile_map00, tile_map_size);
        at += tile_map_size;
        os->permanent_memory_size -= tile_map_size;
    }
     */

    U32 tile_map01[TilesCountY][TilesCountX] = 
    {
        {1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1},
        {1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1},
        {2, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1},
        {1, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 1},
        {1, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 0, 0, 1},
        {1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 1},
        {1, 0, 0, 1, 0, 0, 0, 0, 1, 1, 1, 1, 1, 0, 0, 0, 1},
        {1, 0, 0, 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 1, 0, 1},
        {1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 2, 1},
    };

/*     
    if(tile_map_size <= os->permanent_memory_size)
    {
        memcpy(at, tile_map01, tile_map_size);
        at += tile_map_size;
        os->permanent_memory_size -= tile_map_size;
    }
     */

    U32 tile_map10[TilesCountY][TilesCountX] = 
    {
        {1, 2, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1},
        {1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1},
        {1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 1},
        {1, 0, 0, 0, 1, 1, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 1},
        {1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 1, 0, 0, 0, 1},
        {1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 1},
        {1, 0, 0, 1, 1, 1, 1, 0, 1, 1, 1, 1, 1, 0, 0, 0, 1},
        {1, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 2},
        {1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1},
    };

/*     
    if(tile_map_size <= os->permanent_memory_size)
    {
        memcpy(at, tile_map10, tile_map_size);
        at += tile_map_size;
        os->permanent_memory_size -= tile_map_size;
    }
     */

    U32 tile_map11[TilesCountY][TilesCountX] = 
    {
        {1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 2, 1},
        {1, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 1},
        {1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 1},
        {1, 0, 1, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 1, 0, 0, 1},
        {1, 0, 1, 0, 0, 1, 0, 0, 0, 0, 1, 0, 1, 1, 0, 0, 1},
        {1, 0, 1, 0, 1, 1, 1, 0, 0, 0, 0, 1, 1, 0, 0, 0, 1},
        {1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 1},
        {2, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 1},
        {1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1},
    };

    /*     
    if(tile_map_size <= os->permanent_memory_size)
    {
        memcpy(at, tile_map11, tile_map_size);
        at += tile_map_size;
        os->permanent_memory_size -= tile_map_size;
    }
    */

    TileMap map00;
    map00.tiles = (U32 *)tile_map00;
    
    TileMap map10;
    map10.tiles = (U32 *)tile_map10;
    
    TileMap map01;
    map01.tiles = (U32 *)tile_map01;
    
    TileMap map11;
    map11.tiles = (U32 *)tile_map11;
    
    TileMap maps[2][2];
    
    maps[0][0] = map00;
    maps[1][0] = map10;
    maps[0][1] = map01;
    maps[1][1] = map11;

    GameWorld game_world((TileMap *)maps, 2, 2, TilesCountX, TilesCountY, 50, 30, 60, 60.0f, 1.4f);
    
    game_state->m_world = &game_world;
    
#if 0
    game_state->update(&os->input);
    game_state->render(&os->buffer);
#else 
    F32 dt_player_x = 0.0f;
    F32 dt_player_y = 0.0f;
    
    Input *input = &os->input;
    if(input->onKeyPressed(Key_W) ||
       input->onKeyPressed(Key_Up))
    {
        dt_player_y += 1.0f;
    }
    
    if(input->onKeyPressed(Key_A) ||
       input->onKeyPressed(Key_Left))
    {
        dt_player_x -= 1.0f;
    }
    
    if(input->onKeyPressed(Key_S) ||
       input->onKeyPressed(Key_Down))
    {
        dt_player_y -= 1.0f;
    }
    
    if(input->onKeyPressed(Key_D) || 
       input->onKeyPressed(Key_Right))
    {
        dt_player_x += 1.0f;
    }
    
    // Going from pixels to meters
    dt_player_x *= game_state->m_player_speed_in_meters;
    dt_player_y *= game_state->m_player_speed_in_meters;
    
    // NOTE(alexey): To make player's movement frame independent.
    // So it moves the same amount of pixels regardless of frame rate.
    // For example: 100 pixels/second when it's 60/30fps.
    WorldPos new_world_pos = game_state->m_world_pos;
    new_world_pos.tile_center_rel_x += (dt_player_x * os->dt_for_frame);
    new_world_pos.tile_center_rel_y += (dt_player_y * os->dt_for_frame);
    new_world_pos = game_world.recomputeWorldPos(new_world_pos);
    
    /*
      Player's rectangle: 
      |______|
      * <- left canonical pos.
    */
    WorldPos left_world_pos = game_state->m_world_pos;
    left_world_pos.tile_center_rel_x += (dt_player_x*os->dt_for_frame) - (0.5f * game_state->m_player_dim.x);
    left_world_pos.tile_center_rel_y += (dt_player_y * os->dt_for_frame);
    left_world_pos = game_world.recomputeWorldPos(left_world_pos);
    
    /*
      Player's rectangle:
      |______|
             * <- right canonical pos.
    */
    WorldPos right_world_pos = game_state->m_world_pos;
    right_world_pos.tile_center_rel_x += (dt_player_x*os->dt_for_frame) + (0.5f*game_state->m_player_dim.x);
    right_world_pos.tile_center_rel_y += (dt_player_y * os->dt_for_frame);
    right_world_pos = game_world.recomputeWorldPos(right_world_pos);
    
    /*
      Player's rectangle:
     *______ 
     |      |
    */
    WorldPos left_top_world_pos = left_world_pos;
    left_top_world_pos.tile_center_rel_y += 0.45f*game_state->m_player_dim.y;
    left_top_world_pos = game_world.recomputeWorldPos(left_top_world_pos);
    
    /*
      Player's rectangle:
      ______*
     |      |
    */
    WorldPos right_top_world_pos = right_world_pos;
    right_top_world_pos.tile_center_rel_y += 0.45f*game_state->m_player_dim.y;
    right_top_world_pos = game_world.recomputeWorldPos(right_top_world_pos);

    if(game_world.isTileMapPointEmpty(new_world_pos) &&
       game_world.isTileMapPointEmpty(left_world_pos) &&
       game_world.isTileMapPointEmpty(right_world_pos) &&
       game_world.isTileMapPointEmpty(left_top_world_pos) &&
       game_world.isTileMapPointEmpty(right_top_world_pos))
    {
        game_state->m_world_pos = new_world_pos;
    }
    
    // flush background.
    draw_rectangle(&os->buffer, RectangleStyle_Filled, 
                   0.0f, 0.0f, (F32)os->buffer.width, (F32)os->buffer.height, Vec4(236, 213, 160));
    // Draw tile map.
    TileMap *map = game_world.getWorldTileMap(Cast(I32, game_state->m_world_pos.tile_map_x),
                                              Cast(I32, game_state->m_world_pos.tile_map_y));
    for(I32 tile_y = 0;
        tile_y < game_world.m_tile_count_y;
        ++tile_y)
    {
        for(I32 tile_x = 0;
            tile_x < game_world.m_tile_count_x;
            ++tile_x)
        {
            /*
              +------------+ (maxy, maxy) (world->tile_dim, world->tile_dim).
              |            |
              |            |
              |            |
              |            |
              |            |
              +------------+
             (minx, miny), (0, 0)
            */
            F32 minx = (tile_x * game_world.m_tile_dim) + game_world.m_offset_x;
            F32 miny = (tile_y * game_world.m_tile_dim) + game_world.m_offset_y;
            F32 maxx = minx + game_world.m_tile_dim;
            F32 maxy = miny + game_world.m_tile_dim;
            
            if(game_state->m_world_pos.tile_x == tile_x &&
               game_state->m_world_pos.tile_y == tile_y)
            {
                Vec4 color(0.8f, 0.7f, 0.54f);
                draw_rectangle(&os->buffer, RectangleStyle_Filled, minx, miny, maxx, maxy, color);
            }
            else if(!game_world.isTileEmpty(map, tile_x, tile_y))
            {
                U32 tile_value = game_world.getTileValue(map, tile_x, tile_y);
                Vec4 color = (tile_value == 2) ? Vec4(0.25f, 0.5f, 0.5f) : Vec4(0.25f);
                draw_rectangle(&os->buffer, RectangleStyle_Filled, minx, miny, maxx, maxy, color);
            }
            
            // draw debug points.
            Vec4 color(1.0f, 1.0f, 0.0f);
            
            Rect2 r0(Vec2(minx - 3, miny - 3), Vec2(minx + 3, miny + 3));
            draw_rectangle(&os->buffer, RectangleStyle_Filled, r0.min.x, r0.min.y, r0.max.x, r0.max.y, color);
            
            Rect2 r1(Vec2(maxx - 3, miny - 3), Vec2(maxx + 3, miny + 3));
            draw_rectangle(&os->buffer, RectangleStyle_Filled, r1.min.x, r1.min.y, r1.max.x, r1.max.y, color);
            
            Rect2 r2(Vec2(minx - 3, maxy - 3), Vec2(minx + 3, maxy + 3));
            draw_rectangle(&os->buffer, RectangleStyle_Filled, r2.min.x, r2.min.y, r2.max.x, r2.max.y, color);
            
            Rect2 r3(Vec2(maxx - 3, maxy - 3), Vec2(maxx + 3, maxy + 3));
            draw_rectangle(&os->buffer, RectangleStyle_Filled, r3.min.x, r3.min.y, r3.max.x, r3.max.y, color);

            // Draw fram for each tile.
            Vec4 frame_color(0.8f, 0.788f, 0.65f);
            draw_rectangle(&os->buffer, RectangleStyle_Wireframe, minx, miny, maxx, maxy, frame_color);
        }
    }
    
    // If we have meters, we would have to multiply meters * world->pixels_per_meter.
    // In order to convert to pixels.
    // compute player's absolute position
    F32 player_abs_x = 
    (game_state->m_world_pos.tile_x*game_world.m_tile_dim) + game_world.m_offset_x +  
    (game_state->m_world_pos.tile_center_rel_x*game_world.m_pixels_per_meter) + game_world.m_tile_half_dim; 
    
    F32 player_abs_y = 
    (game_state->m_world_pos.tile_y * game_world.m_tile_dim) + game_world.m_offset_y +
    (game_state->m_world_pos.tile_center_rel_y*game_world.m_pixels_per_meter) + game_world.m_tile_half_dim;
    
    F32 player_minx = player_abs_x - (0.5f*game_state->m_player_dim.x*game_world.m_pixels_per_meter);
    F32 player_miny = player_abs_y;
    
    F32 player_maxx = player_minx + game_state->m_player_dim.x*game_world.m_pixels_per_meter;
    F32 player_maxy = player_miny + game_state->m_player_dim.y*game_world.m_pixels_per_meter;

#if 0    
        DebugOut("PlayerAbsolute: (%.2f, %.2f)\nPlayerMin: (%.2f, %.2f)\nPlayerMax(%.2f, %.2f)\n\n", 
                 player_abs_x, 
                 player_abs_y,
                 player_minx, 
                 player_miny, 
                 player_maxx, 
                 player_maxy);
#endif
    
    // draw player
    Vec4 player_color(0.80f, 1.0f, 0.44f);
    draw_rectangle(&os->buffer, RectangleStyle_Filled, player_minx, player_miny, 
                   player_maxx, player_maxy, player_color);
    
    // Draw player's collision box.
    F32 maxy = player_abs_y + 0.45f*game_state->m_player_dim.y*game_world.m_pixels_per_meter;
    draw_rectangle(&os->buffer, RectangleStyle_Wireframe, player_minx, player_miny,
                       player_maxx, maxy, Vec4(0.95f, 0.21f, 1.0f));
    
    Vec4 debug_color(1.0f, 0.0f, 0.47f);
    // Draw debug rect for player's gravity center.
    {
        Vec2 min(player_abs_x - 4.0f, player_abs_y - 4.0f);
        Vec2 max(player_abs_x + 4.0f, player_abs_y + 4.0f);
        draw_rectangle(&os->buffer, RectangleStyle_Wireframe, min.x, min.y, max.x, max.y, debug_color);
    }

    // Draw debug rect for player's left point.
    {
        Vec2 min(player_minx - 4.0f, player_abs_y - 4.0f);
        Vec2 max(player_minx + 4.0f, player_abs_y + 4.0f);
        draw_rectangle(&os->buffer,RectangleStyle_Wireframe,  min.x, min.y, max.x, max.y, debug_color);
    }

    // Draw debug rect for player's right point.
    {
        F32 x = player_abs_x + (0.5f*game_state->m_player_dim.x*game_world.m_pixels_per_meter); 
        Vec2 min(x - 4.0f, player_abs_y - 4.0f);
        Vec2 max(x + 4.0f, player_abs_y + 4.0f);
        draw_rectangle(&os->buffer, RectangleStyle_Wireframe, min.x, min.y, max.x, max.y, debug_color);
    }
    
    // Draw debug rect of player's top left point
    {
        F32 x = (player_abs_x - 0.5f*game_state->m_player_dim.x*game_world.m_pixels_per_meter);
        F32 y = (player_abs_y + 0.45f*game_state->m_player_dim.y*game_world.m_pixels_per_meter);
        Vec2 min(x - 4.0f, y - 4.0f);
        Vec2 max(x + 4.0f, y + 4.0f);
        draw_rectangle(&os->buffer, RectangleStyle_Wireframe, min.x, min.y, max.x, max.y, debug_color);
    }
    
    // Draw debug rect of player's top right point
    {
        F32 x = (player_abs_x + 0.5f*game_state->m_player_dim.x*game_world.m_pixels_per_meter);
        F32 y = (player_abs_y + 0.45f*game_state->m_player_dim.y*game_world.m_pixels_per_meter);
        Vec2 min(x - 4.0f, y - 4.0f);
        Vec2 max(x + 4.0f, y + 4.0f);
        draw_rectangle(&os->buffer, RectangleStyle_Wireframe, min.x, min.y, max.x, max.y, debug_color);
    }
#endif
}