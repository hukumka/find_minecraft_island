#include "finder.h"
#include "filters.h"
#include "cubiomes/finders.h"
#include <stdio.h>
#include <string.h>


struct Stack_Pos {
    Pos* stack;
    int head;
};

static inline void fill_partition_try_push(struct Map* map, int x, int z, struct Stack_Pos* stack) {
    if (map_is_ocean(map, x, z) == 0 && map->spacePartition[x + z * map->width] == PartitionLand) {
        stack->stack[stack->head].x = x;
        stack->stack[stack->head].z = z;
        stack->head += 1;
    }
}

void fill_partition(struct Map* map, int x, int z) {
    struct Stack_Pos stack;
    // Since no cell can be inserted twice, it is enough to allocate enought space to fit all the cells.
    // #TODO: maybe reuse allocation by moving into to the top of call stack?
    stack.stack = (Pos*)malloc(sizeof(Pos) * map->width * map->height); 
    stack.head = 0;

    memset((void*)map->spacePartition, 0, sizeof(int) * map->width * map->height);
    // Traverse island depth-first to mark all island chunks as such.
    fill_partition_try_push(map, x, z, &stack);
    while(stack.head > 0) {
        stack.head -= 1;
        x = stack.stack[stack.head].x;
        z = stack.stack[stack.head].z;
        map->spacePartition[x + z * map->width] = PartitionIsland;
        fill_partition_try_push(map, x - 1, z, &stack);
        fill_partition_try_push(map, x + 1, z, &stack);
        fill_partition_try_push(map, x, z - 1, &stack);
        fill_partition_try_push(map, x, z + 1, &stack);
    }
    free((void*)stack.stack);
    // Set all ocean cells as such
    for (int i = 0; i < map->width * map->height; ++i) {
        if (is_ocean(map->map[i])) {
            map->spacePartition[i] = PartitionOcean;
        }
    }
}

int process_island(struct Map* map) {
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
    fill_partition(map, shore.x - 1, shore.z);
    int biome_value = count_biomes(map);
    double shore_value = count_shore_width(map, shore.x, shore.z, 10000);
    int area = island_area(map, &res);
    printf(
        "%d:%d, %d:%d, %d:%d, circumvent=%d, biome_value=%d, shore_value=%lf, area=%d\n", 
        w, h, res.minX - 128, res.maxX - 128, res.minZ - 128, res.maxZ - 128, res.length, 
        biome_value, shore_value, area 
    );
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
    int* buffer2 = allocCache(layer, width, height);

    // Run my shit
    for (int64_t seed=seed_start; seed<=seed_end; ++seed) {
        setWorldSeed(layer, seed);
        genArea(layer, buffer, centerX - width/2, centerZ - height/2, width, height);

        struct Map map = {width, height, buffer, buffer2};
        int value = process_island(&map);
        if (value > 0) {
            printf("seed=%lld value=%d\n", seed, value);
        }
    }
    free(buffer);
    free(buffer2);
}
