#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include "gen_underground.h"
#include <stdio.h>

void init_level(struct level* l)
{
	l->map = NULL;
	l->h = 0;
	l->w = 0;
	l->d = 0;
}

void deinit_level(struct level* l)
{
	free(l->map);
	free(l->zone_info.map);
	free(l->zone_info.zones);
}

void copy_level(struct level* a, struct level* b)
{
	b->map = realloc(b->map, a->d * (a->h * (a->w + 1) + 1));
	memcpy(b->map, a->map, a->d * (a->h * (a->w + 1) + 1));
	b->h = a->h;
	b->w = a->w;
	b->d = a->d;
}

void add_zone(struct level* l, struct coords at)
{
	struct zone* r;
	l->zone_info.zones = realloc(l->zone_info.zones,
								sizeof(struct zone) * ++(l->zone_info.count));
	
	r = &l->zone_info.zones[l->zone_info.count - 1];
	r->at = at;
	r->size = 0;
}

/* reads level data from specified file descriptor */
struct level* read_level(int fd)
{
	const ssize_t bsize = 4096;
	char* data = malloc(bsize);
	ssize_t rb = 0, size = 0, i, j;
	struct level *result;

	if(data == NULL) return NULL;

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
	/* determining level width */
	/* level z-levels are separated by a single newline*/
	for(i = 0; i < size && data[i] != '\n'; ++i);
	for(j = 0; j < size && data[j * (i+1)] != '\n'; ++j)
	result->w = i;
	result->h = j;
	/* determining level depth */
	result->d = (size / ((i+1) * j + 1));

	result->zone_info.zones = NULL;
	result->zone_info.map = NULL;
	result->zone_info.count = 0;
	fprintf(stderr, "level %d x %d x %d, size %lu\n", result->w, result->h, result->d, size+1);
	return result;
}

int is_floor(char a)
{
	return a == '.' || a == '<' || a == '>' || a == '+';
}

void zone_flood_fill(struct level* l, struct coords c, int zone)
{
	int i;
	if(!VALID_COORDS(l, c)) return;
	if(!is_floor(at(l, c.x, c.y, c.z))) return;
	if(l->zone_info.map[c.x + c.y*l->w ] != 0) return;

	l->zone_info.map[c.x + c.y*l->w] = zone;
	l->zone_info.zones[zone-1].size++;

	for(i = 0; i < 16; i += 2)
		zone_flood_fill(l, (struct coords) { c.x + dirs[i], c.y + dirs[i+1], c.z }, zone);
}

void write_level(struct level* l, int fd)
{
	puts(l->map);
}