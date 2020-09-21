#ifndef FINDER_H
#define FINDER_H

#include "cubiomes/generator.h"

#define MAX(x, y) (((x) > (y)) ? (x) : (y))
#define MIN(x, y) (((x) < (y)) ? (x) : (y))

enum SpacePart {
    PartitionLand = 0,
    PartitionIsland = 1,
    PartitionOcean = 2,
};

struct Map {
    int width;
    int height;
    int* map;
    int* spacePartition;
};

typedef void (*TraversalCallback)(const struct Map*, int x, int z, int dx, int dz, void* extra);

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

static inline int map_partition(const struct Map* map, int x, int z) {
    if (x < 0 || x >= map->width || z < 0 || z >= map->height) {
        return -1;
    }
    return map->spacePartition[x + map->width * z];
}

struct ClockwiseTraversal traverse_clockwise(const struct Map* map, int startX, int startZ, int maxLength);
struct ClockwiseTraversal traverse_clockwise_do(const struct Map* map, int startX, int startZ, int maxLength, void* extra, TraversalCallback callback);
struct PosRes find_shore(const struct Map* map);

#endif