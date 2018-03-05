#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include "gen_underground.h"
#include <dirent.h>
#include "gen_dungeon.h"
#include <string.h>

#include "gen_dungeon_parts.h"

#define PIECES_DIR "./pieces"

static struct dg_gen_part* const
static_parts[] = {
	&xbone_room,
	&stairwell,
	&column_room,
	&simple_room
};

/* nowhere to store this */
static int room_count = 0;
/*static struct dg_parts_array parts = (struct dg_parts_array) {
	.data = NULL,
	.length = 0,
	.total_weight = 0
};*/

void die(const char* reason)
{
	fprintf(stderr, "%s\n", reason);
	exit(EXIT_FAILURE);
}

struct dg_piece *create_piece(struct coords pos, int w, int h, int d, dir_t dir,
		 struct dg_gen_part *t, struct coords anchor)
{
	struct dg_piece *r = malloc(sizeof(struct dg_piece));
	r->pos = pos;
	r->width = ADJ_WIDTH(dir, w, h);
	r->height = ADJ_HEIGHT(dir, w, h);
	r->depth = d;
	r->dir = dir;
	r->type = t;
	r->anchor = anchor;
	r->flags = DG_FLAGS_NONE;
	fprintf(stderr, "created piece at (%d, %d, %d), w %d h %d d %d, dir %d\n", 
		r->pos.x, r->pos.y, r->pos.z, r->width, r->height, r->depth, r->dir);
	return r;
}

dir_t rotate_dir(dir_t a, dir_t b)
{
	return (a + b) % 4;
}

/* obviously does not cover all cases, especially those that'd require
 * memory inspection */
static void gen_part_sanity_check(struct dg_gen_part *p)
{
	if(p->gen_type == GEN_STATIC && (p->gen_fptr || p->build_fptr)) {
		die("static part has non-NULL generation fptrs");
	}
	if(p->gen_type == GEN_DYNAMIC && (!p->gen_fptr || !p->build_fptr)) {
		die("dynamic part has NULL generation fptrs");
	}
	if(p->class > GEN_ROOM) {
		die("invalid class type");
	}

	if(p->gen_type == GEN_STATIC) {
		for(int i = 0 ; i < p->height; ++i) {
			if(strlen(p->data[i]) != p->width) {
				die("internal map width does not match specified width");
			}
		}

		char flag = 0;
		for(int i = 0; i < p->max_conns; ++i) {
			if( p->conns[i].x < 0 || p->conns[i].x >= p->width ||
				p->conns[i].y < 0 || p->conns[i].y >= p->height ||
				p->conns[i].z < 0 || p->conns[i].z >= p->depth) {
				die("invalid connection coords");
			}
			if(p->conns[i].y == p->height-1) flag = 1;
		}
		if(!flag) {
			die("no connection at 'southern' side");
		}
	}
}

void add_gen_part(struct dg_parts_array* arr, struct dg_gen_part* p)
{
	gen_part_sanity_check(p);
	arr->data = realloc(arr->data, sizeof(struct dg_gen_part*) * (arr->length + 1));
	arr->data[arr->length] = p;
	arr->length++;
	arr->total_weight += p->weight;

	/* perhaps counts and weights should be stored elsewhere
	 * to make dungeon parts truly constant */
	p->count = 0;
}

/* extremely crude generation part loader */
struct dg_gen_part* load_static_gen_part(const char *filename)
{
	fprintf(stderr, "loading part \"%s\"\n", filename);
	FILE* f = fopen(filename, "r");
	if(f == NULL) {
		fprintf(stderr, "failed to open it.\n");
		return NULL;
	}

	int smi = 0; 
	struct dg_gen_part *r = malloc(sizeof(struct dg_gen_part));
	memset(r, 0, sizeof(struct dg_gen_part));

	r->gen_type = GEN_STATIC;
	r->gen_fptr = NULL;
	r->build_fptr = NULL;

	if(fscanf(f, "%d%d%d%d%d%d%d", &r->width, &r->height, &r->depth, &r->class,
			&r->weight, &r->max_count, &r->max_conns) != 7) {
		goto rip;
	}

	r->data = malloc(sizeof(char*) * r->height * r->depth);
	r->conns = malloc(sizeof(struct coords) * r->max_conns);
	for(int i = 0; i < r->max_conns; ++i) {
		if(fscanf(f, "%d%d%d", &r->conns[i].x, &r->conns[i].y, 
				&r->conns[i].z) != 3) {
			goto rip_conns;
		}
	}
	fgetc(f);
	for(int i = 0; i < r->depth; ++i) {
		for(smi = 0; smi < r->height; ++smi) {
			r->data[smi + i*r->height] = malloc(r->width + 1);
			int len = fread(r->data[smi + i*r->height], 1, r->width + 1, f);
			if(len != r->width + 1) goto rip_submap;
			(r->data[smi + i*r->height])[r->width] = '\0';
		}
	}

	return r;

rip_submap:
	for(int j = 0; j < r->depth; ++j) {
		for(int i = 0; i < min(smi, r->height); ++i) {
			free(r->data[i + j * r->height]);
		}
	}
rip_conns:
	free(r->conns);
	free(r->data);
rip:
	free(r);
	return NULL;
}

/* returns array of available dungeon generation parts */
struct dg_parts_array load_gen_parts()
{
	struct dg_parts_array result = (struct dg_parts_array) { 
		.data = NULL, .length = 0, .total_weight = 0 };

	/* first we add precompiled parts,
	 * e.g. those which describe dynamicly genned parts */
	for(int i = 0; i < sizeof(static_parts)/sizeof(struct dg_gen_part*); ++i) {
		add_gen_part(&result, static_parts[i]);
	}

	/* then we check dedicated piece directory
	 * for "*.idp" files describing static parts */
	DIR *d;
	struct dirent *dir;
	if((d = opendir(PIECES_DIR))) {
		while((dir = readdir(d))) {
			int l = strlen(dir->d_name);
			if(l > 4 && strncmp(dir->d_name + l - 4, ".idp", 4) == 0) {
				char name[sizeof(PIECES_DIR) + 256];
	
				strncpy(name, PIECES_DIR, sizeof(PIECES_DIR));
				name[sizeof(PIECES_DIR)-1] = '/';
				strncpy(name + sizeof(PIECES_DIR), dir->d_name, l+1);
				
				struct dg_gen_part *p = load_static_gen_part(name);
				if(p) {
					fprintf(stderr, "successfully loaded: %dx%dx%d\n", p->width, p->height, p->depth);
					add_gen_part(&result, p);
				}
			}
		}
		closedir(d);
	}

	return result;
}

void append_list(void *p, struct dg_list *l)
{
	struct dg_list_node *nn = malloc(sizeof(struct dg_list_node));
	nn->next = NULL;
	nn->who = p;

	if(l->last) l->last->next = nn; else l->first = nn;
	l->last = nn;
	l->count++;
}

void delete_list_nodes(struct dg_list_node *n)
{
	if(!n) return;
	delete_list_nodes(n->next);
	free(n);
}

void flush_list(struct dg_list *l)
{
	delete_list_nodes(l->first);
	l->first = NULL;
	l->last = NULL;
	l->count = 0;
}

/* converts part internal map coords into level coords */
struct coords sm2l(struct coords pos, int w, int h, int d, dir_t dir)
{
	switch(dir) {
	case NORTH: return (struct coords){ pos.x, pos.y, pos.z };
	case EAST: return (struct coords){ w-1 - pos.y, pos.x, pos.z };
	case SOUTH: return (struct coords){ w-1 - pos.x, h-1 - pos.y, pos.z };
	case WEST: return (struct coords){ pos.y, h-1 - pos.x, pos.z };
	}
}

struct coords psm2l(struct dg_piece *p, struct coords pos)
{
	return sm2l(pos, p->width, p->height, p->depth, p->dir);
}

/* returns dungeon piece occupying this position */
struct dg_piece* point_occupied(struct coords pos, struct dg_list *pieces)
{
	for(struct dg_list_node *i = pieces->first; i; i = i->next) {
		struct dg_piece *t = (struct dg_piece*)i->who;
		if( pos.x >= t->pos.x && pos.x < t->pos.x + t->width &&
			pos.y >= t->pos.y && pos.y < t->pos.y + t->height &&
			pos.z >= t->pos.z && pos.z < t->pos.z + t->depth) {
			return t;
		}
	}
	return NULL;
}

/* returns any piece that intersects this area */
struct dg_piece* intersected_piece(struct level *l, struct coords a, 
		struct coords b, struct dg_list *pieces)
{
	for(struct dg_list_node *i = pieces->first; i; i = i->next) {
		struct dg_piece *t = (struct dg_piece*)i->who;
		if( a.x < t->pos.x + t->width && b.x >= t->pos.x &&
			a.y < t->pos.y + t->height && b.y >= t->pos.y &&
			a.z < t->pos.z + t->depth && b.z >= t->pos.z) {
			return t;
		}
	}

	return NULL;
}

static void recalculate_parts_total_weight(struct dg_parts_array* a)
{
	a->total_weight = 0;
	for(int i = 0; i < a->length; ++i) {
		struct dg_gen_part *p = a->data[i];
		if(p->max_count == DG_ANY_COUNT || p->count < p->max_count) {
			a->total_weight += p->weight;
		}
	}
}

/* randomly rolls for a dungeon part considering their weights */
struct dg_gen_part *weighted_part_roll(struct dg_parts_array* parts)
{
	if(parts == NULL) return NULL;

	int roll = rand() % parts->total_weight;

	for(int i = 0; i < parts->length; ++i) {
		struct dg_gen_part *p = parts->data[i];
		
		if(p->max_count != DG_ANY_COUNT && p->count >= p->max_count) continue;

		if(p->weight > roll) {
			if(p->max_count != DG_ANY_COUNT && ++p->count >= p->max_count) {
				recalculate_parts_total_weight(parts);
			}
			return parts->data[i];
		}
		roll -= parts->data[i]->weight;
	}

	/* not possible if total weight is not screwed */
	return NULL;
}

/* returns direction the specified dungeon piece connection is pointing to */
static dir_t _get_conn_dir(struct dg_gen_part *t, dir_t dir, int conn)
{
	int d = SOUTH;
	if(t->conns[conn].x == 0) d = WEST;
	else if(t->conns[conn].x == t->width-1) d = EAST;
	else if(t->conns[conn].y == 0) d = NORTH;

	return rotate_dir(dir, d);
}

dir_t get_conn_dir(struct dg_piece *p, int conn_id)
{
	return _get_conn_dir(p->type, p->dir, conn_id);
}

/* attempts to randomly select a piece that will fit into the level */
struct dg_piece *select_piece(struct level* l, struct coords pos, dir_t dir, 
		struct dg_list *pieces, struct dg_parts_array* parts)
{
	int c;
	struct dg_gen_part *sp = weighted_part_roll(parts);

	/* maybe our dungeon generator only has finite parts */
	if(sp == NULL) return NULL;

	if(sp->gen_type == GEN_STATIC) {
		/* since those internal maps are oriented north, 
		 * we use any connection at the 'southern' side */
		do { 
			c = rand() % sp->max_conns; 
		} while (sp->conns[c].y != sp->height-1);
		
		/* now we calculate coords of piece's top-left corner on the level */
		struct coords cc = sm2l(sp->conns[c],
			ADJ_WIDTH(dir, sp->width, sp->height),
			ADJ_HEIGHT(dir, sp->width, sp->height), sp->depth, dir);
		
		pos.x -= cc.x;
		pos.y -= cc.y;
		pos.z -= cc.z;
		
		/* and bottom-right corner */
		struct coords pos2 = (struct coords) {
			.x = pos.x + ADJ_WIDTH(dir, sp->width, sp->height) - 1,
			.y = pos.y + ADJ_HEIGHT(dir, sp->width, sp->height) - 1,
			.z = pos.z + sp->depth - 1};

		if( !VALID_COORDS(l, pos) || !VALID_COORDS(l, pos2)) {
			return NULL;
		}

		if(intersected_piece(l, pos, pos2, pieces) == NULL) {
			return create_piece(pos, sp->width, sp->height, sp->depth, dir, sp, 
					(struct coords){ pos.x + cc.x, pos.y + cc.y, pos.z + cc.z});
		}

		return NULL;
	} else {
		return sp->gen_fptr(l, sp, pos, dir, pieces);
	}
}

/* checks if tile is part of a doorway */
int doorway_scan(struct level* l, struct coords p, dir_t d)
{
	if(!VALID_COORDS(l, p)) return 0;

	if(at(l, p.x, p.y, p.z) == '+') {
		switch(d) {
		case EAST:
		case WEST:
			if(p.y > 0 && at(l, p.x, p.y-1, p.z) != '#') return 0;
			if(p.y < (l->h-1) && at(l, p.x, p.y+1, p.z) != '#') return 0;
			return 1;
		case NORTH:
		case SOUTH:
			if(p.x > 0 && at(l, p.x-1, p.y, p.z) != '#') return 0;
			if(p.x < (l->w-1) && at(l, p.x+1, p.y, p.z) != '#') return 0;
			return 1;
		}
	}
	else if(at(l, p.x, p.y, p.z) == '#') {
		switch(d) {
		case EAST:
		case WEST:
			if(p.y > 0 && at(l, p.x, p.y-1, p.z) == '+') return 1;
			if(p.y < (l->h-1) && at(l, p.x, p.y+1, p.z) == '+') return 1;
			return 0;
		case NORTH:
		case SOUTH:
			if(p.x > 0 && at(l, p.x-1, p.y, p.z) == '+') return 1;
			if(p.x < (l->w-1) && at(l, p.x+1, p.y, p.z) == '+') return 1;
			return 0;
		}
	}
	return 0;
}

/* properly (?) seals dungeon piece connection point */
void closeoff_connection(struct level *l, struct coords p, dir_t d)
{
	switch(d)
	{
	case NORTH:
	case SOUTH:
		if(p.x > 0 && at(l, p.x-1, p.y, p.z) == ' ') at(l, p.x, p.y, p.z) == '#';
		at(l, p.x, p.y, p.z) = '#';
		if(p.x < l->w-1 && at(l, p.x+1, p.y, p.z) == ' ') at(l, p.x, p.y, p.z) == '#';
		break;
	case EAST:
	case WEST:
		if(p.y > 0 && at(l, p.x, p.y-1, p.z) == ' ') at(l, p.x, p.y, p.z) == '#';
		at(l, p.x, p.y, p.z) = '#';
		if(p.y < l->h-1 && at(l, p.x, p.y+1, p.z) == ' ') at(l, p.x, p.y, p.z) == '#';
		break;
	}
}

void openup_connection(struct level *l, struct coords p, dir_t d, char door)
{
	if(!is_floor(at(l, p.x, p.y, p.z))) at(l, p.x, p.y, p.z) = door ? '+' : '.';
	switch(d)
	{
	case NORTH:
	case SOUTH:
		if(p.x > 0 && at(l, p.x-1, p.y, p.z) == ' ') at(l, p.x, p.y, p.z) == '#';
		if(p.x < l->w-1 && at(l, p.x+1, p.y, p.z) == ' ') at(l, p.x, p.y, p.z) == '#';
		break;
	case EAST:
	case WEST:
		if(p.y > 0 && at(l, p.x, p.y-1, p.z) == ' ') at(l, p.x, p.y, p.z) == '#';
		if(p.y < l->h-1 && at(l, p.x, p.y+1, p.z) == ' ') at(l, p.x, p.y, p.z) == '#';
		break;
	}
}

void queue_piece(struct level *l, struct coords pos, dir_t d, 
		struct dg_piece *parent, struct dg_list * pieces, struct dg_list* bo,
		struct dg_parts_array *parts)
{
	struct dg_gen_part *t = parent->type;
	/* anchor coordinates of queued piece */
	struct coords anchor = (struct coords) {
		pos.x + dir_offsets[3*d],
		pos.y + dir_offsets[3*d+1],
		pos.z + dir_offsets[3*d+2] };

	if(!VALID_COORDS(l, anchor)) return;

	struct dg_piece *o = point_occupied(anchor, pieces);

	/* if there is no piece right beyond the possible connection, 
	 * we try to build one */
	if(o == NULL) {
		struct dg_piece *next = select_piece(l, anchor, d, pieces, parts);
		/* if we found a suitable dungeon part, and we have a build order list,
		 * then we actually queue it */
		if(next && bo && parts) {
			/* corridors can't have 4589 doors */
			if(t->class != GEN_CORRIDOR) {
				openup_connection(l, pos, d, '+');
			}
			else {
				openup_connection(l, pos, d, 0);
			}
			next->dir = d;
			append_list(next, pieces);
			append_list(next, bo);
			if(next->type->class == GEN_ROOM) room_count++;
		} else {
			/* otherwise we just close up the connection point */
			closeoff_connection(l, pos, d);
		}
	} else {
		/* there is a piece, we will try to open up a passage 
		 * to reduce maze-likeness of the dungeon */

		/* no operations on queued buildings */
		if(!(o->flags & DG_FLAGS_BUILT)) return;

		/* if occupied tile is in corner of that piece, we do nothing */
		if( (anchor.x == o->pos.x || anchor.x == o->pos.x + o->width-1) &&
			(anchor.y == o->pos.y || anchor.y == o->pos.y + o->height-1) &&
			(anchor.z == o->pos.z || anchor.z == o->pos.z + o->depth-1)) {
			closeoff_connection(l, pos, d);
			return;
		}

		/* if it's a wall (aka its not an already existing passage)
		 * check if there is open space beyond it */
		if(at(l, anchor.x, anchor.y, anchor.z) == '#') {
			/* the tile beyond the wall */
			struct coords pt = (struct coords) {
				anchor.x + dir_offsets[3*d],
				anchor.y + dir_offsets[3*d+1],
				anchor.z + dir_offsets[3*d+2] };

			if(!VALID_COORDS(l, pt) || 
				!is_floor(at(l, pt.x, pt.y, pt.z))) {
				return;
			}

			if(!doorway_scan(l, anchor, d)) {
				at(l, pos.x, pos.y, pos.z) = '.';
				at(l, anchor.x, anchor.y, anchor.z) = '.';
			}
		} else if(!is_floor(at(l, anchor.x, anchor.y, anchor.z))) {
			closeoff_connection(l, pos, d);
		}
	}
}

void build_piece(struct level* l, struct dg_piece *p, struct dg_list * pieces,
		struct dg_list *bo, struct dg_parts_array* parts)
{
	static int count = 0;
	size_t doff = 0;	/* depth offset for internal map selection */
	struct dg_gen_part *t = p->type;
	char ** bp = p->type->data;
	
	count++;
	
	if(t->gen_type == GEN_STATIC) {
		/* no guarantees that the piece fits in the level.
		 * select_piece() should check that */
		for(int k = 0; k < p->depth; ++k) {
			doff = p->type->height * k;
			for(int j = 0; j < p->height; ++j) {
				for(int i = 0; i < p->width; ++i) {
					switch(p->dir) {
					case NORTH:
						at(l, p->pos.x+i, p->pos.y+j, p->pos.z+k) = bp[doff + j][i];
						break;
					case EAST:
						at(l, p->pos.x+i, p->pos.y+j, p->pos.z+k) = bp[doff + p->width-1 - i][j];
						break;
					case SOUTH:
						at(l, p->pos.x+i, p->pos.y+j, p->pos.z+k) = bp[doff + p->height-1 - j][p->width-1 - i];
						break;
					case WEST:
						at(l, p->pos.x+i, p->pos.y+j, p->pos.z+k) = bp[doff + i][p->height-1 - j];
						break;
					}
				}
			}
		}

		/* we try to queue up more pieces for every possible connection
		 * or at least close them off */
		for(int i = 0; i < t->max_conns; ++i) {
			dir_t d = get_conn_dir(p, i);

			/* converting connection internal piece coords into level coords */
			struct coords lc = psm2l(p, t->conns[i]);
			lc.x += p->pos.x;
			lc.y += p->pos.y;
			lc.z += p->pos.z;

			queue_piece(l, lc, d, p, pieces, bo, parts);
		}
	} else t->build_fptr(l, p, pieces, bo, parts);
	//at(l, p->pos.x, p->pos.y, p->pos.z) = count + '0';
	p->flags |= DG_FLAGS_BUILT;
}

void gen_dungeon(struct level* l, int max_rooms, int max_pieces)
{
	struct dg_list pieces = (struct dg_list){ NULL, NULL, 0 };
	struct dg_list build_order = (struct dg_list) { NULL, NULL, 0 };
	struct dg_list next_bo = (struct dg_list) { NULL, NULL, 0 };
	struct dg_parts_array parts = load_gen_parts();

	struct dg_piece *start;

	/* start off by building a piece somewhere in the middle of the map
	 * at the lowest depth of 0 */
	int tries = 0;
	do {
		struct coords start_pos = (struct coords) {
			l->w / 2 + rand() % (l->w / 8),
			l->h / 2 + rand() % (l->h / 8),
			0
		};
		start = select_piece(l, start_pos, rand() % 4, &pieces, &parts);
		/* with extremely small maps the algorithm will fail to start */
	} while(start == NULL && tries++ < 1000);

	if(start == NULL) die("failed to start after 1000 tries!");

	/* initial piece has no 'parent' */
	start->anchor = (struct coords) { -1, -1, -1 };

	append_list(start, &pieces);
	build_piece(l, start, &pieces, &build_order, &parts);

	/* then we continue to queue, and then build pieces 
	 * until we hit the piece limit or it's not possible
	 * to build any more pieces (the build order list is exhausted/empty) */
	while(pieces.count < max_pieces && room_count < max_rooms
		&& build_order.count > 0) {
		for(struct dg_list_node* i = build_order.first; i; i = i->next) {
			build_piece(l, (struct dg_piece*)i->who, &pieces, &next_bo, &parts);
		}
		flush_list(&build_order);
		build_order = next_bo;
		next_bo = (struct dg_list) { NULL, NULL, 0 };
	}

	/* building leftover pieces without queueing up more */
	for(struct dg_list_node* i = build_order.first; i; i = i->next) {
		build_piece(l, (struct dg_piece*)i->who, &pieces, NULL, NULL);
	}
}

int main()
{
	struct level *l = read_level(0);
	srand(time(NULL));

	room_count = 0;

	gen_dungeon(l, 1000, 1000);

	write_level(l, 1);

	return EXIT_SUCCESS;
}