#include "finder.h"

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