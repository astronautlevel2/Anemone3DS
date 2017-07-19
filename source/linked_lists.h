#pragma once

typedef struct
{
	void *data;
	void *next;
} node;

void add_node(node*, node*);