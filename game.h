/* date = October 8th 2023 0:23 pm */
#ifndef GAME_H

// TODO(alexey): Player's position should always be tile relative.
// Either ralative to top-left corner, or a center of the tile.
// If we know the tile that the player is currently in, we can easily
// recompute its position.

struct TileMap
{
    uint32 *tiles;
};

struct WorldPos
{
    Vec2 tile;
    Vec2 tile_center_rel;
    Vec2 tile_map;
};

struct GameWorld
{
    GameWorld(TileMap *maps, 
              int32 tile_map_count_x,
              int32 tile_map_count_y, 
              int32 tile_count_x, 
              int32 tile_count_y, 
              int32 offset_x,
              int32 offset_y, 
              real32 tile_dim,
              real32 tile_side_in_pixels, 
              real32 tile_side_in_meters);
    
    uint32 getTileValue(TileMap *tile_map, int32 test_tile_x, int32 test_tile_y);
    bool32 isDoorTile(TileMap *tile_map, int32 test_tile_x, int32 test_tile_y);
    bool32 isTileEmpty(TileMap *tile_map, int32 test_tile_x, int32 test_tile_y);
    TileMap *getWorldTileMap(int32 test_tile_map_x, int32 test_tile_map_y); // NOTE(alexey): Return const?
    WorldPos recomputeWorldPos(WorldPos world_pos);
    bool32 isTileMapPointEmpty(WorldPos world_pos);
        
    TileMap *m_maps;
    
    int32 m_tile_map_count_x;
    int32 m_tile_map_count_y;
    
    int32 m_tile_count_x;
    int32 m_tile_count_y;
    
    int32 m_offset_x;
    int32 m_offset_y;
    
    real32 m_tile_dim;
    real32 m_tile_half_dim;
    
    real32 m_tile_side_in_pixels;
    real32 m_tile_side_in_meters;
    real32 m_half_tile_side_in_meters;
    
    real32 m_pixels_per_meter;
    
    // NOTE(alexey): The lower 8 bits correspond where in the chunk we are.
    // If chunk for example, 256x256, that will give us the exact tile of where we are in the chunk.
    // The remaining 24 bits corresponds to where we are in our world.
    // (not initialized);
    uint32 m_tile_x;
    uint32 m_tile_y;
};

function GameWorld makeGameWorld(TileMap* tile_maps);

struct GameState
{
    // TODO(alexey): When we create a game state, we should initialize a game world as well!
    // Allocate memory for the tile maps etc.
    void update(Input *input/*...*/);
    void render(OffscreenBuffer *buffer);
    
    // TODO(alexey): Make the world be a part of game state since it's really is.
    // Move all the updates inside update() function,
    // and all the rendering code into render() function.
    // Maybe the input has to be a part of game state as well?
    
    GameWorld world;
    WorldPos world_pos;
    Vec2 player_dim;
    real32 player_speed_in_meters;
    bool32 is_initialized;
};

#define GAME_H
#endif //GAME_H
