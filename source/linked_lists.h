#pragma once

#include "3ds.h"

typedef struct
{
	void *data;
	void *next;
} node;

void add_node(node*, node*);
