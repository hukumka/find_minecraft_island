#include "filters.h"
#include <math.h>

int count_biomes(const struct Map* map) {
    int value = 0.0;
    int biome_count[128] = {0};
    for (int i = 0; i < map->width * map->height; ++i) {
        int biome = map->map[i];
        // Reduce variant biome to main biome.
        if (biome > 128) {
            biome -= 128;
        }
        if (biomes >= 0 && biome < 128) {
            biome_count[biome] += 1;
        }
    }
    if (biome_count[plains] >= 40) {
        value += 10.0;
    }
    if (biome_count[forest] >= 20) {
        value += 5.0;
    }
    if (biome_count[mountains] >= 20) {
        value += 5.0;
    }
    for (int i=0; i<128; ++i) {
        if (biome_count[i] > 0) {
            value += 1.0;
        }
    }
    return value;
}

void check_for_shore(const struct Map* map, int x, int z, int dx, int dz, void* data) {
    double* value = (double*) data;
    int consecutive = 0;
    int total = 0;
    int broken = 0;
    for (int i = 0; i < 32; ++i) {
        int cell = map_partition(map, x - i*dz, z + i*dx);
        if (cell != PartitionLand) {
            if (!broken) {
                consecutive += 1;
            }
            total += 1;
        } else {
            broken = 1;
        }
    }
    int shore_check = consecutive > 12 // Nearest land pretty far
        || total > consecutive; // There is water shortly after land, so it is just mini island.
    if (!shore_check) {
        *value += expf((12.0 / (double) consecutive) - 1.0) - 1.0;
    }
}

double count_shore_width(const struct Map* map, int startX, int startZ, int maxLength) {
    double value = 0.0;
    traverse_clockwise_do(map, startX, startZ, maxLength, (void*)&value, check_for_shore);
    return value;
}

int island_area(const struct Map* map, const struct ClockwiseTraversal* island) {
    int count = 0;
    for (int z = island->minZ; z <= island->maxZ; ++z) {
        for (int x = island->minX; x <= island->maxX; ++x) {
            if (map_partition(map, x, z) == PartitionIsland) {
                count += 1;
            }
        }
    }
    return count;
}