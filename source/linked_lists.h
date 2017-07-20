#pragma once

#include "3ds.h"

struct theme_data;

typedef struct
{
	struct theme_data *data;
	void *next;
} node;

void add_node(node*, node*);
