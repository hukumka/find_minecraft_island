#include "filters.h"

double count_biomes(const struct Map* map) {
    double value = 0.0;
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