#ifndef FILTERS_H
#define FILTERS_H

#include "finder.h"

int count_biomes(const struct Map* map);
double count_shore_width(const struct Map* map, int startX, int startZ, int maxLength);
int island_area(const struct Map* map, const struct ClockwiseTraversal* island);

#endif