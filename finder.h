#ifndef FINDER_H
#define FINDER_H

#include "cubiomes/generator.h"

#define MAX(x, y) (((x) > (y)) ? (x) : (y))
#define MIN(x, y) (((x) < (y)) ? (x) : (y))

struct Map {
    int width;
    int height;
    int* map;
};

struct ClockwiseTraversal {
    int minX;
    int maxX;
    int minZ;
    int maxZ;
    // Total length of shore.
    int length;
    // Is shore leads to the edge of the area.
    int broken;
    // number of turns left - number of turns right.
    int turns;
};

struct PosRes {
    int x;
    int z;
    int not_found;
};

static inline int is_ocean(int biome) {
    // See `BiomeID` enum in `layers.h`
    return biome == ocean 
        || biome == frozen_ocean
        || biome == deep_ocean
        || (biome >= warm_ocean && biome <= deep_frozen_ocean);
}

/* Return value:
    1 : ocean
    0 : not ocean
    -1 : out of map bounds
*/
static inline int map_is_ocean(const struct Map* map, int x, int z) {
    if (x < 0 || x >= map->width || z < 0 || z >= map->height) {
        return -1;
    }
    int biome = map->map[x + map->width * z];
    return is_ocean(biome);
}

struct ClockwiseTraversal traverse_clockwise(const struct Map* map, int startX, int startZ, int maxLength);
struct PosRes find_shore(const struct Map* map);

#endif