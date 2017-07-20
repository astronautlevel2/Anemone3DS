#pragma once

#include "3ds.h"
#include "theme.h"

typedef struct
{
	theme_data *data;
	void *next;
} node;

void add_node(node*, node*);
