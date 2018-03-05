#pragma once

#include <sys/types.h>
/* these ought to be somewhere else */
#define min(a, b) ((a)<(b)?(a):(b))

#define at(l, x, y, z) (l)->map[(x) + (y)*((l)->w + 1) + (z)*(((l)->w + 1) * (l)->h + 1)]
#define zone_at(l, x, y) (l)->zone_info.map[(x) + (y)*((l)->w + 1)]
#define VALID_COORDS(l, c) ((c).x >= 0 && (c).y >= 0 && (c).z >= 0 && (c).x < (l)->w && (c).y < (l)->h && (c).z < (l)->d)

static const int dirs[16] = {
	-1, -1, 
 	 0, -1,
 	 1, -1,
 	-1,  0,
 	 1,  0,
 	-1,  1,
	 0,  1,
	 1,  1 };

struct coords
{
	int x, y, z;
};

/* zone = separated open space */
struct zone
{
	/* anchor point */
	struct coords at;
	long size;
};

struct zone_info_t
{
	int* map;
	struct zone *zones;
	size_t count;
};

struct level
{
	char *map;
	int h, w, d;
	struct zone_info_t zone_info;
};

void init_level(struct level* l);
void deinit_level(struct level* l);
void copy_level(struct level* a, struct level* b);
void add_zone(struct level* l, struct coords at);
struct level* read_level(int fd);
void write_level(struct level* l, int fd);
int is_floor(char a);
void zone_flood_fill(struct level* l, struct coords c, int zone);