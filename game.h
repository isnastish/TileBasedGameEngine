/* date = October 8th 2023 0:23 pm */
#ifndef GAME_H

// TODO(alexey): Player's position should always be tile relative.
// Either ralative to top-left corner, or a center of the tile.
// If we know the tile that the player is currently in, we can easily
// recompute its position.

struct TileMap
{
    U32 *tiles;
};

struct WorldPos
{
    I32 tile_x;
    I32 tile_y;
    
    I32 tile_map_x;
    I32 tile_map_y;
    
    F32 tile_center_rel_x;
    F32 tile_center_rel_y;
};

struct GameWorld
{
    GameWorld(TileMap *maps, 
              I32 tile_map_count_x,
              I32 tile_map_count_y, 
              I32 tile_count_x, 
              I32 tile_count_y, 
              I32 offset_x,
              I32 offset_y, 
              F32 tile_dim,
              F32 tile_side_in_pixels, 
              F32 tile_side_in_meters);
    
    U32 getTileValue(TileMap *tile_map, I32 test_tile_x, I32 test_tile_y);
    Bool32 isDoorTile(TileMap *tile_map, I32 test_tile_x, I32 test_tile_y);
    Bool32 isTileEmpty(TileMap *tile_map, I32 test_tile_x, I32 test_tile_y);
    TileMap *getWorldTileMap(I32 test_tile_map_x, I32 test_tile_map_y); // NOTE(alexey): Return const?
    WorldPos recomputeWorldPos(WorldPos world_pos);
    Bool32 isTileMapPointEmpty(WorldPos world_pos);
        
    TileMap *m_maps;
    
    I32 m_tile_map_count_x;
    I32 m_tile_map_count_y;
    
    I32 m_tile_count_x;
    I32 m_tile_count_y;
    
    I32 m_offset_x;
    I32 m_offset_y;
    
    F32 m_tile_dim;
    F32 m_tile_half_dim;
    
    F32 m_tile_side_in_pixels;
    F32 m_tile_side_in_meters;
    F32 m_half_tile_side_in_meters;
    
    F32 m_pixels_per_meter;
    
    // NOTE(alexey): The lower 8 bits correspond where in the chunk we are.
    // If chunk for example, 256x256, that will give us the exact tile of where we are in the chunk.
    // The remaining 24 bits corresponds to where we are in our world.
    // (not initialized);
    U32 m_tile_x;
    U32 m_tile_y;
};

function GameWorld makeGameWorld(TileMap* tile_maps);

struct GameState
{
    GameState(GameWorld* world)
        : m_world(world)
    {
    }
    
    // TODO(alexey): When we create a game state, we should initialize a game world as well!
    // Allocate memory for the tile maps etc.
    void update(Input *input/*...*/);
    void render(OffscreenBuffer *buffer);
    
    // TODO(alexey): Make the world be a part of game state since it's really is.
    // Move all the updates inside update() function,
    // and all the rendering code into render() function.
    // Maybe the input has to be a part of game state as well?
    
    GameWorld *m_world;
    WorldPos m_world_pos;
    Vec2 m_player_dim;
    F32 m_player_speed_in_meters;
    
    Bool32 m_is_initialized;
};

#define GAME_H
#endif //GAME_H
