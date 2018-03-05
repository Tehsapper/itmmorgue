#pragma once

#include "gen_underground.h"
#include "gen_dungeon.h"

extern struct dg_gen_part xbone_room;
extern struct dg_gen_part stairwell;
extern struct dg_gen_part simple_room;
extern struct dg_gen_part column_room;

struct dg_piece* simple_room_gen(struct level* l, struct dg_gen_part *p, struct coords a, dir_t dir, struct dg_list * pieces);
void simple_room_build(struct level* l, struct dg_piece* p, struct dg_list* pieces, struct dg_list* build_order, struct dg_parts_array* parts);
void column_room_build(struct level* l, struct dg_piece* p, struct dg_list* pieces, struct dg_list* build_order, struct dg_parts_array* parts);