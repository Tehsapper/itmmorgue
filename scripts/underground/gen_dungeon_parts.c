#include "gen_dungeon_parts.h"
#include <stdlib.h>

/* static dungeon part examples */
struct dg_gen_part
xbone_room = (struct dg_gen_part) {
	.width = 13, .height = 11, .depth = 1,	/* Dimensions                     */
	.data = (char *[]) {					/* Map, array of c-string rows    */
	/* Z-level = 0 */						/* as if piece is directed NORTH  */
	/* X ---> */
/* Y */	"##.##   ##.##", 
		"#...#   #...#", 
/* | */	"#...#   #...#",
/* | */	"#...#####...#",
/* V */	"#...........#",
		".............",
		"#...........#",
		"#...#####...#",
		"#...#   #...#",
		"#...#   #...#", 
		"##.##   ##.##" },
	.class = GEN_ROOM, 						/* Room class					  */
	.gen_type = GEN_STATIC,
	.weight = 3,							/* Generation weight              */
	.conns = (struct coords[]) {			/* Array of 3d coordinates of     */
		{ 0,  5, 0},						/* possible connection points     */
		{12,  5, 0},						
		{ 2,  0, 0},						
		{10,  0, 0},						/* MUST have a connection at 	  */
		{ 2, 10, 0},						/* 'southern' (lowest) edge		  */
		{10, 10, 0} },
	.max_conns = 6,							/* Length of 'conns' array */
	.max_count = DG_ANY_COUNT,				/* Maximum allowed count */
	.gen_fptr = NULL, .build_fptr = NULL	/* Unused dynamic gen fptrs */
};

struct dg_gen_part
stairwell = (struct dg_gen_part) {
	.width = 7, .height = 5, .depth = 2,
	.data = (char *[]) {
	/* Z-level = 0 */
		"###.###",
		"#.....#",
		"#.....#",
		"#..#.>#",
		"#.#####",
	/* Z-level = 1 */
		"###.###",
		"#.....#",
		"#.....#",
		"#..#.<#",
		"#.#####" },
	.class = GEN_ROOM,
	.gen_type = GEN_STATIC,
	.weight = 4,
	.conns = (struct coords[]) {
		{ 3, 0, 0 },
		{ 1, 4, 0 },
		{ 3, 0, 1 },
		{ 1, 4, 1 }	},
	.max_conns = 4,
	.max_count = DG_ANY_COUNT,
	.gen_fptr = NULL, .build_fptr = NULL
};

/* dynamic dungeon part example */
struct dg_gen_part
simple_room = (struct dg_gen_part) {
	.data = NULL,							/* No predetermined map */
	.class = GEN_ROOM,
	.gen_type = GEN_DYNAMIC,
	.weight = 10,
	.conns = NULL,							/* No predetermined connections */
	.max_conns = 0,
	.max_count = DG_ANY_COUNT,
	.gen_fptr = &simple_room_gen,			/* Function ptrs for generation */
	.build_fptr = &simple_room_build		/* and building */
};

struct dg_gen_part
column_room = (struct dg_gen_part) {
	.class = GEN_ROOM,
	.gen_type = GEN_DYNAMIC,
	.weight = 10,
	.max_count = DG_ANY_COUNT,
	.gen_fptr = &simple_room_gen,
	.build_fptr = &column_room_build
};


struct dg_piece* simple_room_gen(struct level* l, struct dg_gen_part *p,
		struct coords a, dir_t dir, struct dg_list * pieces)
{
	int w = rand() % 9 + 5;
	int h = rand() % 9 + 5;

	/* piece internal coordinates of the tile used as connection point */
	struct coords smc = (struct coords) {
		rand() % (w-2) + 1,
		h - 1,
		0
	};

	/* calculating actual level coordinates of top-left corner */
	struct coords cc = sm2l(smc, 
		ADJ_WIDTH(dir, w, h), 
		ADJ_HEIGHT(dir, w, h), 1, dir);
	a.x -= cc.x;
	a.y -= cc.y;
	a.z -= cc.z;

	/* and bottom-right corner */
	struct coords b = (struct coords) {
		a.x+ADJ_WIDTH(dir, w, h) - 1,
		a.y+ADJ_HEIGHT(dir, w, h) - 1,
		a.z
	};

	if(!VALID_COORDS(l, a) || !VALID_COORDS(l, b)) return NULL;

	/* success if we can place it on the level */
	if(intersected_piece(l, a, b, pieces) == NULL) {
		return create_piece(a, w, h, 1, dir, p, (struct coords){ a.x + cc.x, a.y + cc.y, a.z + cc.z});
	}
	return NULL;
}

void simple_room_build(struct level* l, struct dg_piece* p,
		struct dg_list* pieces, struct dg_list* build_order, struct dg_parts_array* parts)
{
	for(int j = 0; j < p->height; ++j) {
		for(int i = 0; i < p->width; ++i) {
			if(i == 0 || j == 0 || i == p->width - 1 || j == p->height - 1) {
				at(l, p->pos.x + i, p->pos.y + j, p->pos.z) = '#';
			}
			else at(l, p->pos.x + i, p->pos.y + j, p->pos.z) = '.';
		}
	}

	/* a bit of a hack, if anchor == -1, -1, -1 it's the first dungeon piece
	 * which does not have an anchor point */
	if(p->anchor.x >= 0 && p->anchor.y >= 0 && p->anchor.z >= 0)
		at(l, p->anchor.x, p->anchor.y, p->anchor.z) = '.';

	/* approximately a possible connection per every 3 tiles of perimeter */
	int conns = (2*(p->width-1) + 2*(p->height-1)) / 3; 

	/* queueing more pieces randomly */
	for(int i = 0; i < conns; ++i) {
		dir_t d = rand() % 4;
		struct coords c;
		c.z = p->pos.z;

		switch(d) {
		case NORTH: 
			c.x = p->pos.x + rand() % (p->width-2) + 1;
			c.y = p->pos.y;
			break;
		case WEST:
			c.x = p->pos.x;
			c.y = p->pos.y + rand() % (p->height-2) + 1;
			break;
		case SOUTH: 
			c.x = p->pos.x + rand() % (p->width-2) + 1;
			c.y = p->pos.y + p->height-1;
			break;
		case EAST:
			c.x = p->pos.x + p->width-1;
			c.y = p->pos.y + rand() % (p->height-2) + 1;
			break;
		}
		queue_piece(l, c, d, p, pieces, build_order, parts);
	}
}

void column_room_build(struct level* l, struct dg_piece* p, 
		struct dg_list* pieces, struct dg_list* build_order, struct dg_parts_array* parts)
{
	for(int j = 0; j < p->height; ++j) {
		for(int i = 0; i < p->width; ++i) {
			if(i == 0 || j == 0 || i == p->width - 1 || j == p->height - 1) {
				at(l, p->pos.x+i, p->pos.y+j, p->pos.z) = '#';
			}
			else {
				if(((p->width % 2 && i % 2 == 0) || (p->width % 2 == 0 && (i+1)%3 == 0))
					&& ((p->height % 2 && j % 2 == 0) || (p->height % 2 == 0 && (j+1)%3 == 0)))
					at(l, p->pos.x+i, p->pos.y+j, p->pos.z) = '#';
				else 
					at(l, p->pos.x+i, p->pos.y+j, p->pos.z) = '.';
				
			}
		}
	}

	int conns = (2*(p->width-1) + 2*(p->height-1)) / 3; 

	for(int i = 0; i < conns; ++i) {
		struct coords c;
		dir_t d = rand() % 4;
		c.z = p->pos.z;

		switch(d) {
		case NORTH: 
			c.x = p->pos.x + rand() % (p->width-2) + 1;
			c.y = p->pos.y;
			break;
		case WEST:
			c.x = p->pos.x;
			c.y = p->pos.y + rand() % (p->height-2) + 1;
			break;
		case SOUTH:
			c.x = p->pos.x + rand() % (p->width-2) + 1;
			c.y = p->pos.y + p->height-1;
			break;
		case EAST:
			c.x = p->pos.x + p->width-1;
			c.y = p->pos.y + rand() % (p->height-2) + 1;
			break;
		}
		queue_piece(l, c, d, p, pieces, build_order, parts);
	}
}