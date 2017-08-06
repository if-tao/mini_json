#include <stdbool.h>
#include "../memory/alloc.h"
enum Color{
	Red,
	Black
};
typedef enum Color Color;

struct Node;
struct RBTree;
typedef struct Node Node; 
typedef struct RBTree RBTree;
//typedef bool (*Compare)(void *first, void *second); 
typedef int (*Compare)(void *, void *); 
typedef void (*ShowValue)(void *);
struct Node{
	void *data;
	Color node_color;
	Node *left;
	Node *right;
	Node *parent;
};
Node *new_node(void *d, Node *, Node *);


struct RBTree{
	Node *root;
	Node *tail;
	void *other_info;
};

struct InsertResult{
	bool status;
	Node *pnode;
};
typedef struct InsertResult InsertResult;

RBTree *create_rb_tree();
Node *locate(RBTree *tree, void *data, Compare com_func);
bool insert(RBTree *tree, void *data, Compare com_func);
InsertResult _insert_one_node(RBTree *tree, void *data, Compare com_func);
Node *left_rotate(Node *, RBTree *);
Node *right_rotate(Node *, RBTree *);
Node *RfatherRuncle(Node *, Node *);
///////////////////////////////
bool has_parent(Node *);
bool is_red_node(Node *);
bool is_left_child(Node *, Node *);
bool is_right_child(Node *, Node *);
//////////////////////////////
void fixup(Node *, RBTree *);
void show(RBTree *, ShowValue);
void _show(Node *, Node *,ShowValue);
Node *find(RBTree *tree, Compare com_func, void *arg);

typedef void (*MemClear)(void *);
void clear(RBTree *, MemClear clear_func);
/*
delte();
*/
