#pragma once

#include "theme.h"

typedef struct
{
	theme *data;
	void *next;
} node;

void add_node(node*, node*);