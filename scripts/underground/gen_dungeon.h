#pragma once

#include "gen_underground.h"
#include <sys/types.h>

#define OPPOSITE_DIR(d) ((d) ^ 2)

/* 'adjusted' width and height if piece was pointing in this direction */
#define ADJ_WIDTH(d, w, h) ((d)%2 > 0?(h):(w))
#define ADJ_HEIGHT(d, w, h) ((d)%2 > 0?(w):(h))
#define roll(a, b) (rand() % (b-a+1) + (a))

#define DG_ANY_COUNT		-1

#define DG_FLAGS_NONE		0
#define DG_FLAGS_BUILT		1

typedef enum { GEN_CORRIDOR, GEN_ROOM } piece_class_t;
typedef enum { GEN_STATIC, GEN_DYNAMIC} piece_gen_t;
typedef enum { NORTH, EAST, SOUTH, WEST, UP, DOWN} dir_t;

/* maybe should be extern instead */
static const int dir_offsets[18] = {
	 0, -1,  0,	/* NORTH */
	 1,  0,  0, /* EAST */
	 0,  1,  0, /* SOUTH */
	-1,  0,  0, /* WEST */
	 0,  0, -1, /* UP */
	 0,  0,  1  /* DOWN */ };

struct dg_list;
struct dg_piece;
struct dg_gen_part;
struct dg_parts_array;

typedef struct dg_piece* (*piece_gen_fptr) (struct level*, struct dg_gen_part*, struct coords, dir_t, struct dg_list* );
typedef void (*piece_build_fptr) (struct level*, struct dg_piece* , struct dg_list*, struct dg_list*, struct dg_parts_array*);

/* generation info for specific dungeon part */
struct dg_gen_part 
{
	int width, height, depth;	/* part dimensions */
	char ** data;				/* part map: c-strings of rows */
	piece_class_t class;		/* part class: room or corridor */
	piece_gen_t gen_type;		/* type: static or dynamic */
	unsigned int weight;		/* generation weight */
	struct coords * conns;		/* array of possible connection points */
	size_t max_conns;			/* length of conns array */
	ssize_t count, max_count;	/* maximum allowed number of this part */
	piece_gen_fptr gen_fptr;	/* function ptrs for dynamic gen */
	piece_build_fptr build_fptr;/* gen fptr returns a piece if it fits */ 
};								/* build_fptr actually builds the returned
								 * dg_piece on the level and queues more pieces
								 */

/* array of dungeon parts used for generation */
struct dg_parts_array
{
	struct dg_gen_part **data;
	size_t length;
	int total_weight;
};

/* dungeon piece located on the level */
struct dg_piece
{
	struct coords pos;			/* 3d position */
	int width, height, depth;	/* dimensions */
	dir_t dir;					/* rotation direction */
	struct dg_gen_part *type;	/* dungeon part type */
	int flags;
	struct coords anchor;		/* anchor point where this piece is connected
								 * to parent dungeon piece */
};

struct dg_list
{
	struct dg_list_node *first;
	struct dg_list_node *last;
	size_t count;
};

struct dg_list_node
{
	void* who;
	struct dg_list_node *next;
};

void die(const char* reason);
struct dg_piece *create_piece(struct coords pos, int w, int h, int d, dir_t dir, struct dg_gen_part *t, struct coords anchor);
void add_gen_part(struct dg_parts_array* arr, struct dg_gen_part* p);
struct dg_gen_part* load_static_gen_part(const char *filename);
struct dg_parts_array load_gen_parts();
struct coords sm2l(struct coords pos, int w, int h, int d, dir_t dir);
struct coords psm2l(struct dg_piece *p, struct coords pos);
struct dg_piece* point_occupied(struct coords pos, struct dg_list *pieces);
struct dg_piece* intersected_piece(struct level *l, struct coords a, struct coords b, struct dg_list *pieces);
struct dg_gen_part *weighted_part_roll(struct dg_parts_array* parts);
struct dg_piece *select_piece(struct level* l, struct coords pos, dir_t dir, struct dg_list *pieces, struct dg_parts_array* parts);
dir_t get_conn_dir(struct dg_piece *p, int conn_id);
int doorway_scan(struct level* l, struct coords p, dir_t d);
void closeoff_connection(struct level *l, struct coords p, dir_t d);
void openup_connection(struct level *l, struct coords p, dir_t d, char door);
void queue_piece(struct level *l, struct coords pos, dir_t d, struct dg_piece *parent, struct dg_list * pieces, struct dg_list* bo, struct dg_parts_array* parts);
void build_piece(struct level* l, struct dg_piece *p, struct dg_list * pieces, struct dg_list *bo, struct dg_parts_array* parts);
void gen_dungeon(struct level* l, int max_rooms, int max_pieces);