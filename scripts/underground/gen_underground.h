#pragma once

#include <sys/types.h>
/* these ought to be somewhere else */
#define min(a, b) ((a)<(b)?(a):(b))

#define at(l, x, y, z) (l)->map[(x) + (y)*((l)->w + 1) + (z)*(((l)->w + 1) * (l)->h + 1)]
#define room_at(l, x, y, z) ((struct dg_piece**)((l)->roommap))[(x) + (y)*((l)->w + 1) + (z)*(((l)->w + 1) * (l)->h + 1)]
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

typedef struct coords
{
	int x, y, z;
} coords_t;

/* zone = separated open space */
typedef struct zone
{
	/* anchor point */
	struct coords at;
	long size;
} zone_t;

typedef struct zone_info
{
	int* map;
	zone_t *zones;
	size_t count;
} zone_info_t;

typedef struct level
{
	char *map;
	int h, w, d;
	zone_info_t zone_info;
	void* roommap;
} level_t;

void init_level(level_t* l);
void deinit_level(level_t* l);
void copy_level(level_t* a, level_t* b);
void add_zone(level_t* l, coords_t at);
level_t* read_level(int fd);
void write_level(level_t* l, int fd);
int is_floor(char a);
void zone_flood_fill(level_t* l, coords_t c, int zone);
