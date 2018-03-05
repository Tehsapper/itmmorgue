#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <time.h>
#include "gen_underground.h"

static
void encase_zone(struct level* l, struct coords a, int zone)
{
	int i;
	if(!VALID_COORDS(l, a)) return;
	if(!is_floor(at(l, a.x, a.y, a.z))) return;
	if(l->zone_info.map[a.x + a.y*l->w ] != zone) return;

	l->zone_info.map[a.x + a.y*l->w] = 0;
	at(l, a.x, a.y, a.z) = '#';
	l->zone_info.zones[zone-1].size--;

	if(l->zone_info.zones[zone-1].size == 0) {
		/* should also update zone info map values
		 * and remove zone from zone array */
		return;
	}

	for(i = 0; i < 16; i += 2)
		encase_zone(l, (struct coords){a.x+dirs[i], a.y+dirs[i+1], a.z}, zone);
}

void encase_small_zones(struct level* l, int threshold)
{
	int i;
	for(i = 0; i < l->zone_info.count; ++i)
		if(l->zone_info.zones[i].size < threshold) {
			/* zone IDs can get mixed up currently */
			encase_zone(l, l->zone_info.zones[i].at, i+1);
		}
}

/* floodfills z-level and populates zone info */
void zone_test(struct level* l, int z)
{
	int i, j, zone;
	ssize_t rtm_size = l->w * l->h * sizeof(int);

	l->zone_info.count = 0;
	l->zone_info.map = realloc(l->zone_info.map, rtm_size);
	memset(l->zone_info.map, 0, rtm_size);

	zone = 1;

	for(j = 0; j < l->h; ++j) {
		for(i = 0; i < l->w; ++i) {
			if(at(l, i, j, z) == '.' && l->zone_info.map[i + j*l->w] == 0) {
				add_zone(l, (struct coords){i, j, z});
				zone_flood_fill(l, (struct coords){i, j, z}, zone++);
			}
		}
	}

}

void smooth_pass(struct level* l, int d, int close_threshold, int open_threshold)
{
	int i, j, k, c;
	struct level temp;
	temp.map = NULL;

	if(d < 0 || d >= l->d) return;

	copy_level(l, &temp);

	for(j = 0; j < l->h; ++j ) {
		for(i = 0; i < l->w; ++i) {
			if(at(l, i, j, d) != '#' && at(l, i, j, d) != '.') continue;
			if(i == 0 || j == 0 || i == l->w-1 || j == l->h-1 )
				continue;
			
			c = 0;

			for(k = 0; k < 16; k += 2)
				if(is_floor(at(l, i+dirs[k], j+dirs[k+1], d))) ++c;

			if(c >= open_threshold) at(&temp, i, j, d) = '.';
			if(c <= close_threshold) at(&temp, i, j, d) = '#';
		}
	}

	copy_level(&temp, l);
}

void simple_noise(struct level* l, int wall_rate)
{
	int i, j, k;
	for(k = 0; k < l->d; ++k) {
		for(j = 0; j < l->h; ++j ) {
			for(i = 0; i < l->w; ++i) {
				if((rand() % 100) > wall_rate) at(l, i, j, k) = '#';
				else at(l, i, j, k) = '.';
			}
		}
	}
}

static
struct coords place_stairs(struct level* l, int z, char down)
{
	/* it can happen */
	int try = 1000000;
	int x, y;
	struct stairs *result;

	while(try--)
	{
		x = rand() % l->w;
		y = rand() % l->h;
		if(at(l, x, y, z) == '.') {
			at(l, x, y, z) = down ? '>' : '<';
			return (struct coords) { x, y, z };
		}
	}

	/* if simple noise walled everything, let's open it up */
	for(int k = 0; k < 8; ++k) {
		at(l, x+dirs[2*k], y+dirs[2*k+1], z) = '.';
	}
	at(l, x,y,z) = down ? '>' : '<';
	return (struct coords) { x, y, z};
}

int do_corridor(struct level* l, struct coords a, struct coords b)
{
	int tx = a.x, ty = a.y;
	int dx, dy;
	if(!VALID_COORDS(l, a) || !VALID_COORDS(l, b))
		return 1;

	/* 2d corridors only */
	if(a.z != b.z || (a.x == b.x && a.y == b.y))
		return 2;

	dx = b.x - a.x > 0 ? 1 : -1;
	dy = b.y - a.y > 0 ? 1 : -1;

	while(tx != b.x || ty != b.y)
	{
		if(abs(tx - b.x) >= abs(ty - b.y)) {
			tx += dx;
		} else {
			ty += dy;
		}

		if(!is_floor(at(l, tx, ty, a.z))) {
			at(l, tx, ty, a.z) = '.';
		}
	}

	return 0;
}

int gen_cave(struct level* l, int z, struct coords stairs_pos, int openness)
{
	struct coords downstairs;

	if(!VALID_COORDS(l, stairs_pos)) return 1;

	simple_noise(l, 40);

	for(int k = z; k < l->d; ++k) {
		at(l, stairs_pos.x, stairs_pos.y, k) = '<';
		smooth_pass(l, k, 1, 4);
		
		zone_test(l, k);
		encase_small_zones(l, 50);

		downstairs = place_stairs(l, z, 1);

		if(zone_at(l, stairs_pos.x, stairs_pos.y) 
			!= zone_at(l, downstairs.x, downstairs.y)) {
			do_corridor(l, stairs_pos, downstairs);
		}

		for(int i = 0; i < openness; ++i) {
			smooth_pass(l, k, 2, 5);
		}

		stairs_pos = downstairs;
		stairs_pos.z = k + 1;
	}

	return 0;
}

int main(int argc, char* argv[])
{
	int err;
	struct level *l = read_level(0);

	srand(time(NULL));

	struct coords upstairs = (struct coords) {
		rand() % l->w,
		rand() % l->h,
		0
	};
	
	if(err = gen_cave(l, 0, upstairs, 2)) {
		fprintf(stderr, "failed to generate caves");
		return err;
	}
	write_level(l, 1);
	return EXIT_SUCCESS;
}