#include <stdlib.h>

#include "linked_lists.h"

void add_node(node *current, node *new_node)
{
	while (current->next != NULL) current = current->next;

	current->next = new_node;
}