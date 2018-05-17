// vim: sw=4 ts=4 et :
#include <stdlib.h>
#include <stdio.h>

#include "gen_dungeon_parts.h"

// Hardcoded static dungeon parts examples
dg_gen_part_t
xbone_room = (dg_gen_part_t) {
	.width = 13, .height = 11, .depth = 1, // Dimensions
	.data = (char *[]) {                   // Map, array of C-string rows
	/* Z-level = 0 */                      // as if piece is directed NORTH
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
	.class = GEN_ROOM,                      // Room class
	.gen_type = GEN_STATIC,
	.weight = 3,                            // Generation weight
	.conns = (coords_t[]) {                 // 3d internal coordinates of
		{ 0,  5, 0},                        // possible connection points
		{12,  5, 0},
		{ 2,  0, 0},
		{10,  0, 0},                        // MUST have a connection at
		{ 2, 10, 0},                        // 'southern' (lowest) edge
		{10, 10, 0} },
	.max_conns = 6,                         // Length of 'conns' array
	.max_count = DG_ANY_COUNT,              // Maximum allowed count
	.gen_fptr = NULL, .build_fptr = NULL    // Unused dynamic gen fptrs
};

dg_gen_part_t
stairwell = (dg_gen_part_t) {
	.width = 7, .height = 5, .depth = 2,
	.data = (char *[]) {
	// Z-level = 0
		"###.###",
		"#.....#",
		"#.....#",
		"#..#.>#",
		"#.#####",
	// Z-level = 1
		"###.###",
		"#.....#",
		"#.....#",
		"#..#.<#",
		"#.#####" },
	.class = GEN_ROOM,
	.gen_type = GEN_STATIC,
	.weight = 4,
	.conns = (coords_t[]) {
		{ 3, 0, 0 },
		{ 1, 4, 0 },
		{ 3, 0, 1 },
		{ 1, 4, 1 }	},
	.max_conns = 4,
	.max_count = DG_ANY_COUNT,
	.gen_fptr = NULL, .build_fptr = NULL
};

// Hardcoded dynamic dungeon part example
dg_gen_part_t
simple_room = (dg_gen_part_t) {
	.data = NULL,                           // No predetermined map
	.class = GEN_ROOM,
	.gen_type = GEN_DYNAMIC,
	.weight = 40,
	.conns = NULL,                          // No predetermined connections
	.max_conns = 0,
	.max_count = DG_ANY_COUNT,
	.gen_fptr = &simple_room_gen,           // Function ptrs for generation
	.build_fptr = &simple_room_build,       // and building
	.flags = DG_PART_FLAGS_ENTRANCE         // Capable of being the entrance
};                                          // to the dungeon

dg_gen_part_t
column_room = (dg_gen_part_t) {
	.class = GEN_ROOM,
	.gen_type = GEN_DYNAMIC,
	.weight = 10,
	.max_count = DG_ANY_COUNT,
	.gen_fptr = &simple_room_gen,
	.build_fptr = &column_room_build,
	.flags = DG_PART_FLAGS_ENTRANCE
};

dg_gen_part_t
entrance_room = (dg_gen_part_t) {
	.width = 7, .height = 7, .depth = 1,
	.data = (char*[]) {
		"#######",
		"##...##",
		"#.....#",
		"#.....#",
		"#.....#",
		"##...##",
		"#######" },
	.class = GEN_ROOM,
	.gen_type = GEN_STATIC,
	.weight = 1,
	.conns = (coords_t[]) {
		{ 3, 0, 0 },
		{ 0, 3, 0 },
		{ 6, 3, 0 },
		{ 3, 6, 0 },
		{ 3, 3, 0 } },
	.max_conns = 5,
	.max_count = DG_ANY_COUNT,
	.gen_fptr = NULL, .build_fptr = NULL
};


dg_piece_t* simple_room_gen(level_t* l, dg_gen_part_t *p, coords_t a, dir_t dir,
		dg_list_t * pieces) {
	int w = rand() % 9 + 5;
	int h = rand() % 9 + 5;
	coords_t smc;

	if (dir == UP || dir == DOWN) {
		smc = (coords_t) {
			rand() % (w-2) + 1,
			rand() % (h-2) + 1,
			0
		};

		dir = rand() % 4;
	}
	else {
		// Internal coordinates of the connection point tile
		smc = (coords_t) {
			rand() % (w-2) + 1,
			h - 1,
			0
		};
	}

	// Level coordinates of the top-left corner
	struct coords cc = sm2l(smc, 
		ADJ_WIDTH(dir, w, h), 
		ADJ_HEIGHT(dir, w, h), 1, dir);
	a.x -= cc.x;
	a.y -= cc.y;
	a.z -= cc.z;

	// and bottom-right corner
	struct coords b = (struct coords) {
		a.x+ADJ_WIDTH(dir, w, h) - 1,
		a.y+ADJ_HEIGHT(dir, w, h) - 1,
		a.z
	};

	if (!VALID_COORDS(l, a) || !VALID_COORDS(l, b)) return NULL;

	// Success if piece can be placed on the level
	if (intersected_piece(l, a, b, pieces) == NULL) {
		return create_piece(a, w, h, 1, dir, p, (struct coords){ a.x + cc.x, a.y + cc.y, a.z + cc.z});
	}
	return NULL;
}

void simple_room_build(level_t *l, dg_piece_t *p, dg_list_t *pieces, 
		dg_list_t *build_order, dg_parts_array_t *parts) {
	for (int j = 0; j < p->height; ++j) {
		for (int i = 0; i < p->width; ++i) {
			if (i == 0 || j == 0 || i == p->width - 1 || j == p->height - 1) {
				at(l, p->pos.x + i, p->pos.y + j, p->pos.z) = '#';
			}
			else at(l, p->pos.x + i, p->pos.y + j, p->pos.z) = '.';
		}
	}

	// If anchor is not on the sides, it's a staircase
	if (p->anchor.x != p->pos.x && p->anchor.y != p->pos.y &&
		p->anchor.x != p->pos.x+p->width-1 && p->anchor.y != p->pos.y+p->height-1) {
		at(l, p->anchor.x, p->anchor.y, p->anchor.z) = '<';
	} else {
		at(l, p->anchor.x, p->anchor.y, p->anchor.z) = '.';
	}

	// Approximately one possible connection per every 3 tiles of perimeter.
	int conns = (2*(p->width-1) + 2*(p->height-1)) / 3; 

	// Queueing more dungeon pieces randomly
	for (int i = 0; i < conns; ++i) {
		dir_t d = rand() % 4;
		coords_t c;
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
			default:
				fprintf(stderr, "bad room direction %d\n", p->dir);
				exit(EXIT_FAILURE);
				break;
		}
		queue_piece(l, c, d, p, pieces, build_order, parts);
	}
}

void column_room_build(level_t *l, dg_piece_t *p, dg_list_t *pieces,
		dg_list_t *build_order, dg_parts_array_t *parts) {
	for (int j = 0; j < p->height; ++j) {
		for (int i = 0; i < p->width; ++i) {
			if (i == 0 || j == 0 || i == p->width - 1 || j == p->height - 1) {
				at(l, p->pos.x+i, p->pos.y+j, p->pos.z) = '#';
			}
			else {
				if (((p->width % 2 && i % 2 == 0) || (p->width % 2 == 0 && (i+1)%3 == 0))
					&& ((p->height % 2 && j % 2 == 0) || (p->height % 2 == 0 && (j+1)%3 == 0))) {
					at(l, p->pos.x+i, p->pos.y+j, p->pos.z) = '#';
				} else { 
					at(l, p->pos.x+i, p->pos.y+j, p->pos.z) = '.';
				}
			}
		}
	}

	if (p->anchor.x != p->pos.x && p->anchor.y != p->pos.y &&
		p->anchor.x != p->pos.x+p->width-1 && p->anchor.y != p->pos.y+p->height-1) {
		at(l, p->anchor.x, p->anchor.y, p->anchor.z) = '<';
	} else {
		at(l, p->anchor.x, p->anchor.y, p->anchor.z) = '.';
	}

	int conns = (2*(p->width-1) + 2*(p->height-1)) / 3; 

	for (int i = 0; i < conns; ++i) {
		coords_t c;
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
			default:
				fprintf(stderr, "bad room direction %d\n", p->dir);
				exit(EXIT_FAILURE);
				break;
		}
		queue_piece(l, c, d, p, pieces, build_order, parts);
	}
}
