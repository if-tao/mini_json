#include "map.h"
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

int compare(void *first, void *second) {
	return strcmp(((Item *)first)->key, ((Item *)second)->key);
}

Map map() {
	Map res_map;
	res_map.tree = create_rb_tree();
	return res_map;
}

void add_item(Map *pmap, Item *item) {
	insert(pmap->tree, item, compare);
}

void mapshow(Map *pmap, FUNC show_item) {
	show(pmap->tree, show_item);
}

int value_compare(void *p, void *key) {
	return strcmp(((Item *)p)->key, key);
}

void *value(Map *pmap, const char *key) {
	Node *p = find(pmap->tree, value_compare, (void *)key);
	if (p == NULL) {
		// not exist
		printf("Not Exist\n");
		return "";
	}
	return ((Item *)(p->data))->value;
}

void map_clear(Map *pmap, FUNC inner_clear) {
	clear(pmap->tree, inner_clear);
}
