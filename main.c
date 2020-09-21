#include "finder.h"
#include <stdio.h>

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

int main(int argc, char *argv[]) {
    // this checks for two params which will be used as first and last seeds
    // and converts them into numbers
    if (argc != 3){
        printf("Check given params (Usage: find_island.exe FROM_SEED TO_SEED).");
	exit(1);
    }
    int64_t seed_start = atoi(argv[1]);
    int64_t seed_end = atoi(argv[2]);	
    printf("Seed range is from %d to %d\n", seed_start, seed_end);
    
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
