#include "cubiomes/generator.h"
#include <stdio.h>

#define MAX(x, y) (((x) > (y)) ? (x) : (y))
#define MIN(x, y) (((x) < (y)) ? (x) : (y))

struct Map {
    int width;
    int height;
    int* map;
};

struct PosRes {
    int x;
    int z;
    int not_found;
};

int is_ocean(int biome) {
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
int map_is_ocean(const struct Map* map, int x, int z) {
    if (x < 0 || x >= map->width || z < 0 || z >= map->height) {
        return -1;
    }
    int biome = map->map[x + map->width * z];
    return is_ocean(biome);
}

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

struct ClockwiseTraversal traverse_clockwise(const struct Map* map, int startX, int startZ, int maxLength) {
    struct ClockwiseTraversal result;
    result.minX = result.maxX = startX;
    result.minZ = result.maxZ = startZ;
    result.length = 0;
    result.turns = 0;
    result.broken = 0;

    int x = startX, z = startZ;
    int dx = 0, dz = -1;
    do {
        int right_is_open = map_is_ocean(map, x + dz, z - dx);
        int left_is_open = map_is_ocean(map, x - dz, z + dx);
        int forward_is_open = map_is_ocean(map, x + dx, z + dz);
        if (right_is_open == -1 || left_is_open == -1 || forward_is_open == -1) {
            // Hit the edge of the map. Circut is broken;
            result.broken = 1;
            return result;
        }
        if (right_is_open > 0) {
            // turn right
            int tmp = dz;
            dz = -dx;
            dx = tmp;
            result.turns += 1;
        } else if (forward_is_open > 0) {
        } else if (left_is_open > 0) {
            // turn left
            int tmp = dz;
            dz = dx;
            dx = -tmp;
            result.turns -= 1;
        } else {
            // turn around
            dx = -dx;
            dz = -dz;
            result.turns -= 2;
        }
        x += dx;
        z += dz;
        result.maxX = MAX(x, result.maxX);
        result.minX = MIN(x, result.minX);
        result.maxZ = MAX(z, result.maxZ);
        result.minZ = MIN(z, result.minZ);
        result.length += 1;
        if (result.length > maxLength) {
            result.broken = 1;
            return result;
        }
    } while (!(x == startX && z == startZ));
    return result;
}

struct PosRes find_shore(const struct Map* map) {
    // start in the center and go right.
    struct PosRes res;
    res.x = map->width / 2;
    res.z = map->height / 2;
    res.not_found = 1;

    int start_on_land = 1;
    if (map_is_ocean(map, res.x, res.z)) {
        start_on_land = 0;
    }

    while(1) {
        int check = map_is_ocean(map, res.x, res.z);
        if(check == -1) {
            return res;
        }
        if (check > 0 && start_on_land) {
            res.not_found = 0;
            return res;
        } 
        if (check == 0 && !start_on_land) {
            res.not_found = 0;
            res.x +=1;
            return res;
        }
        if (start_on_land) {
            res.x += 1;
        } else {
            res.x -= 1;
        }
    }
}

int process_island(const struct Map* map) {
    struct PosRes shore = find_shore(map);
    if(shore.not_found) {
        return -1;
    }
    struct ClockwiseTraversal res = traverse_clockwise(map, shore.x, shore.z, 10000);
    if(res.broken) {
        return -1;
    }
    if(res.turns != 4) {
        return -1;
    }
    int w = res.maxX - res.minX;
    int h = res.maxZ - res.minZ;
    if (w*16 < 1000 || h*16 < 1000) {
        return -1;
    }
    printf("%d:%d, %d:%d, %d:%d, circumvent=%d, turns=%d\n", w, h, res.minX - 128, res.maxX - 128, res.minZ - 128, res.maxZ - 128, res.length, res.turns);
    return 1;
}

int main() {
    // Initialize generator
    initBiomes();
    LayerStack g;
    setupGenerator(&g, MC_1_16);
    Layer *layer = &g.layers[L_SHORE_16];

    // Set search properties (Coordinates are in chunks (16x16 blocks))
    int width = 256;
    int height = 256;
    int centerX = 0;
    int centerZ = 0;

    int* buffer = allocCache(layer, width, height);

    // Set search range
    int64_t seed_start = 1000;
    int64_t seed_end = 10000;

    // Run my shit
    for (int64_t seed=seed_start; seed<=seed_end; ++seed) {
        setWorldSeed(layer, seed);
        genArea(layer, buffer, centerX - width/2, centerZ - height/2, width, height);

        struct Map map = {width, height, buffer};
        int value = process_island(&map);
        if (value > 0) {
            printf("seed=%lld value=%d\n", seed, value);
        }
    }
    free(buffer);
}