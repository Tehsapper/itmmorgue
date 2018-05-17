// vim: sw=4 ts=4 et :
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <stdio.h>

#include "gen_underground.h"

void init_level(level_t *l) {
	l->map = NULL;
	l->roommap = NULL;
	l->h = 0;
	l->w = 0;
	l->d = 0;
}

void deinit_level(level_t* l) {
	free(l->map);
	free(l->roommap);
	free(l->zone_info.map);
	free(l->zone_info.zones);
}

void copy_level(level_t *a, level_t *b)
{
	b->map = realloc(b->map, a->d * (a->h * (a->w + 1) + 1));
	b->roommap = realloc(b->roommap, a->d * (a->h * (a->w + 1) + 1));
	memcpy(b->map, a->map, a->d * (a->h * (a->w + 1) + 1));
	memcpy(b->roommap, a->roommap, a->d * (a->h * (a->w + 1) + 1));
	b->h = a->h;
	b->w = a->w;
	b->d = a->d;
}

void add_zone(level_t *l, coords_t at)
{
	zone_t *r;
	l->zone_info.zones = realloc(l->zone_info.zones,
								sizeof(zone_t) * ++(l->zone_info.count));
	
	r = &l->zone_info.zones[l->zone_info.count - 1];
	r->at = at;
	r->size = 0;
}

/*
 * Reads level data from specified file descriptor.
 */
level_t* read_level(int fd)
{
	const ssize_t bsize = 4096;
	char* data = malloc(bsize);
	ssize_t rb = 0, size = 0, i, j;
	level_t *result;

	if (data == NULL) return NULL;

	do
	{
		rb = read(fd, data + size, bsize);
		if(rb == -1) {
			free(data);
			return NULL;
		}
		size += rb;
		data = realloc(data, size + bsize);
		if(data == NULL) return NULL;
	} while(rb == bsize);

	data = realloc(data, size + 1);

	data[size] = 0;

	result = malloc(sizeof(struct level));
	result->map = data;
	result->roommap = calloc(size+1, sizeof(void*));
	// Determining level width
	// Z-levels are separated by a single newline
	for (i = 0; i < size && data[i] != '\n'; ++i);
	for (j = 0; j < size && data[j * (i+1)] != '\n'; ++j)
	result->w = i;
	result->h = j;
	// Determining level depth
	result->d = (size / ((i+1) * j + 1));

	result->zone_info.zones = NULL;
	result->zone_info.map = NULL;
	result->zone_info.count = 0;
	fprintf(stderr, "level %d x %d x %d, size %lu, rsize %lu\n", result->w,
			result->h, result->d, size+1, sizeof(void*) * (size+1));
	return result;
}

int is_floor(char a)
{
	return a == '.' || a == '<' || a == '>' || a == '+';
}

void zone_flood_fill(level_t *l, coords_t c, int zone)
{
	int i;
	if (!VALID_COORDS(l, c)) return;
	if (!is_floor(at(l, c.x, c.y, c.z))) return;
	if (l->zone_info.map[c.x + c.y*l->w ] != 0) return;

	l->zone_info.map[c.x + c.y*l->w] = zone;
	l->zone_info.zones[zone-1].size++;

	for (i = 0; i < 16; i += 2) {
		zone_flood_fill(l, (coords_t){c.x+dirs[i], c.y+dirs[i+1], c.z }, zone);
	}
}

void write_level(level_t *l, int fd)
{
	dprintf(fd, "%s\n", l->map);
}