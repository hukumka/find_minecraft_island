#include "finder.h"
#include "filters.h"
#include "cubiomes/finders.h"
#include <stdio.h>
#include <string.h>
#include "ocl_generator.h"

#define SEED_RANGE 64

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

struct ProcessingResult {
    struct ClockwiseTraversal island;
    int width;
    int height;
    int biome_value;
    int area;
    double shore_value;
    int map_biome_value;
    int discard;
};

struct ProcessingResult process_island(struct Map* map) {
    struct ProcessingResult result;
    result.discard = 1;
    struct PosRes shore = find_shore(map);
    if(shore.not_found) {
        return result;
    }
    result.island = traverse_clockwise(map, shore.x, shore.z, 10000);
    if(result.island.broken) {
        return result;
    }
    if(result.island.turns != 4) {
        return result;
    }
    int w = result.width = result.island.maxX - result.island.minX;
    int h = result.height = result.island.maxZ - result.island.minZ;
    if (w*16 < 1000 || h*16 < 1000) {
        return result;
    }
    fill_partition(map, shore.x - 1, shore.z);
    result.biome_value = count_biomes(map, &result.island);
    if (result.biome_value < 25) {
        return result;
    }
    result.shore_value = count_shore_width(map, shore.x, shore.z, 10000);
    if (result.shore_value > 3) {
        return result;
    }
    result.area = island_area(map, &result.island);
    if (result.area < 2000) {
        return result;
    }
    result.map_biome_value = count_map_biomes(map);
    result.discard = 0;
    return result;
}

void make_header(const char* path) {
    FILE* file = fopen(path, "w");
    fprintf(file, "seed, width, height, x, X, z, Z, circumvent, biome_value, area, shore_value, map_biome_value\n");
    fclose(file);
}

void save_to_file(const char* path, int64_t seed, struct ProcessingResult* result) {
    printf("%ld", seed);
    FILE* file = fopen(path, "a");
    // seed, width, height, x, X, z, Z, lenght, biome_value, area, shore_value, map_biome_value
    fprintf(file, "%ld, %d, %d, %d, %d, %d, %d, %d, %d, %d, %lf, %d\n",
        seed,
        result->width, result->height,
        result->island.minX, result->island.maxX,
        result->island.minZ, result->island.maxZ,
        result->island.length,
        result->biome_value,
        result->area,
        result->shore_value,
        result->map_biome_value
    );
    fclose(file);
}

int main(int argc, char *argv[]) {
    struct GeneratorContext context;
    init_generator_context(&context, MC_1_16, SEED_RANGE, 256, 256);

    // this checks for two params which will be used as first and last seeds
    // and converts them into numbers
    if (argc != 4){
        printf("Check given params (Usage: find_island.exe FROM_SEED TO_SEED OUTFILE).");
	    return 1;
    }
    int64_t seed_start = atol(argv[1]);
    int64_t seed_end = atol(argv[2]);	
    const char* path = argv[3];
    printf("Seed range is from %ld to %ld\n", seed_start, seed_end);
    
    // Set search properties (Coordinates are in chunks (16x16 blocks))
    int width = 256;
    int height = 256;
    int centerX = 0;
    int centerZ = 0;

    cl_int4 dims = {{centerX - width/2, centerZ - height/2, width, height}};

    //int* buffer = allocCache(layer, width, height);
    //int* buffer2 = allocCache(layer, width, height);
    cl_int* buffer = (cl_int*) malloc(SEED_RANGE*width*height*sizeof(cl_int));
    cl_int* buffer2 = (cl_int*) malloc(SEED_RANGE*width*height*sizeof(cl_int));
    cl_int* buffer3 = (cl_int*) malloc(SEED_RANGE*width*height*sizeof(cl_int));

    make_header(path);
    cl_event e0;
    cl_event e1;
    set_world_seed(&context, seed_start, &e0);
    generate_layer(&context, L_SHORE_16, dims, SEED_RANGE, buffer, &e0, &e1);
    clWaitForEvents(1, &e1);
    // Run my shit
    for (int64_t seed=seed_start+1; seed<=seed_end; seed += SEED_RANGE) {
        set_world_seed(&context, seed, &e0);
        generate_layer(&context, L_SHORE_16, dims, SEED_RANGE, buffer3, &e0, &e1);
        //setWorldSeed(layer, seed);
        //genArea(layer, buffer, centerX - width/2, centerZ - height/2, width, height);

        for (int i=0; i<SEED_RANGE; ++i) {
            struct Map map = {width, height, buffer + i*width*height, buffer2};
            struct ProcessingResult result = process_island(&map);
            if (!result.discard) {
                save_to_file(path, seed, &result);
            }
        }

        clWaitForEvents(1, &e1);
        int* tmp = buffer;
        buffer = buffer3;
        buffer3 = tmp;
    }
    struct Map map = {width, height, buffer, buffer2};
    struct ProcessingResult result = process_island(&map);
    if (!result.discard) {
        save_to_file(path, seed_end, &result);
    }
    free(buffer);
    free(buffer2);
    free(buffer3);
}
