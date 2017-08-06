#include "rbTree.h"
#include <stdio.h>
#include <assert.h>
#include <stdlib.h>



void _show(Node *p, Node *tail, ShowValue func) {
	if (p->left != tail) _show(p->left, tail, func);
	func(p->data);
	//printf("color:%d\n", p->node_color);
	//printf("====================\n");
	if (p->right != tail) _show(p->right, tail, func);

}

void show(RBTree *tree, ShowValue func) {
	// 遍历二叉树
	if (tree->root == NULL) {
		printf("empty tree");
		return ;
	}

	_show(tree->root,  tree->tail, func);
}
//node method
Node *new_node(void *d, Node *parent, Node *tail) {
	assert (d != NULL);
	Node *pres_node = (Node *)lalloc(sizeof(Node), 1);
	pres_node->data = d;
	pres_node->node_color = Red;
	pres_node->parent = parent;
	pres_node->left = pres_node->right = tail;
	return pres_node;
}

RBTree *create_rb_tree() {
	RBTree *new_tree = (RBTree *)lalloc(sizeof(RBTree), 1);
	//new_tree.root = NULL;
	new_tree->tail = (Node *)lalloc(sizeof(Node), 1);
	new_tree->tail->node_color = Black;
	//new_tree.other_info = NULL;
	assert(new_tree != NULL);
	return new_tree;
}


Node *_locate(RBTree *tree, void *data, Compare com_func) {
	Node *p = tree->root;
	Node *parent = NULL;
	assert (p != NULL);
	while (p != tree->tail) {
		int result = com_func(p->data, data);
		if (result > 0){
			parent = p;
			p = p->left;
		}
		else if (result < 0) {
			parent = p;
			p = p->right;
		}
		else {
			// nothing todo
			//p->data = data;
			// not support update operation
			parent = p;
			break;
		}
	}
	return parent;
}
InsertResult _insert_one_node(RBTree *tree, void *data, Compare com_func){
	// inner function
	InsertResult res;
	res.status = true;
	Node *p = _locate(tree, data, com_func);
	if (p == NULL) {
		res.pnode = p;
		res.status = false;
		return res;
	}
	if (com_func(p->data, data) > 0) {
		// insert left
		p->left = new_node(data, p, tree->tail);
		res.pnode = p->left;
	}
	else {
		p->right = new_node(data, p, tree->tail);
		res.pnode = p->right;
	}
	return res;
}

bool insert(RBTree *tree, void *data, Compare com_func) {
	assert (data != NULL);
	if (tree->root == NULL) {
		tree->root = new_node(data, NULL, tree->tail);
		tree->root->node_color = Black;
	}
	else {
		InsertResult res = _insert_one_node(tree, data, com_func);
		if (res.status == false) {
			return false;
		}
		else {
			fixup(res.pnode, tree);
			return true;
		}
	}
	return true;
}

/*
 * 红黑树的调整, 对称的情况只说一种
 * 1.红父红叔:父叔变色,递归上调
 * 2.红父黑叔, 同线:以父为中心右旋
 * 3.红父黑叔, 异线
 * */
Node *left_rotate(Node *p, RBTree *tree) {
	Node *left = p->left;
	Node *parent = p->parent;
	Node *pparent = parent->parent;
	//////////////////////////////
	p->left = parent;
	parent->parent = p;
	////////////////////
	parent->right = left;
	left->parent = parent;
	///////////////////////////////
	if (pparent == NULL) {
		tree->root = p;
		return p;
	}
	if (parent == pparent->left) {
		pparent->left = p;
		p->parent = pparent;
	}
	else {
		pparent->right = p;
		p->parent = pparent;
	}
	return p;
}

Node *right_rotate(Node *p, RBTree *tree) {
	Node *right = p->right;
	Node *parent = p->parent;
	Node *pparent = p->parent->parent;
	/////////////////////////////
	p->right = parent;
	parent->parent = p;
	/////////////////////////////
	parent->left = right;
	right->parent = parent;
	//////////////////////////
	if (pparent == NULL) {
		tree->root = p;
		return p;
	}
	if (pparent->left == parent) {
		pparent->left = p;
		p->parent = pparent;
	}
	else {
		pparent->right = p;
		p->parent = pparent;
	}
	return p;
}

Node *RfatherRuncle(Node *parent, Node *uncle) {
	// balance tree node and return new adjust node
	parent->node_color = Black;
	uncle->node_color = Black;
	parent->parent->node_color = Red;
	return parent->parent;
}


bool has_parent(Node *p) {
	return p != NULL && p->parent != NULL;
}

bool is_red_node(Node *p) {
	return p->node_color == Red;
}

bool is_left_child(Node *p, Node *parent) {
	return parent->left == p;
}

bool is_right_child(Node *p, Node *parent) {
	return parent->right == p;
}

void fixup(Node *p, RBTree *tree) {
	if (has_parent(p) && is_red_node(p->parent)) {
		//need fixup
		if (is_left_child(p->parent, p->parent->parent) &&
				is_red_node(p->parent->parent->right)) {
			//case 1: red father and red uncle
			p = RfatherRuncle(p->parent, p->parent->parent->right);
			tree->root->node_color = Black;
			fixup(p, tree);
		}
		else if (is_right_child(p->parent, p->parent->parent) &&
				is_red_node(p->parent->parent->left)) {
			//case 1:symmetry
			p = RfatherRuncle(p->parent, p->parent->parent->left);
			tree->root->node_color = Black;
			fixup(p, tree);
		}
		else if (is_left_child(p, p->parent) &&
				! is_red_node(p->parent->parent->right)){
			//case 2: red father and black uncle
			p  = right_rotate(p->parent, tree);
			p->node_color = Black;
			p->right->node_color = Red;
			tree->root->node_color = Black;
		}
		else if (is_right_child(p, p->parent) &&
				! is_red_node(p->parent->parent->left)) {
			//case 2:symmetry
			p = left_rotate(p->parent, tree);
			p->node_color = Black;
			p->left->node_color = Red;
			tree->root->node_color = Black;
		}
		else if (is_right_child(p, p->parent) &&
				! is_red_node(p->parent->parent->right)) {
			//case 3:
			p = left_rotate(p, tree);
			fixup(p, tree);
		}
		else if (is_left_child(p, p->parent) &&
				! is_red_node(p->parent->parent->left)) {
			p = right_rotate(p, tree);
			fixup(p, tree);
		}
	}
}


Node *find(RBTree *tree, Compare com_func, void *arg) {
	Node *p = tree->root;
	while (p != tree->tail) {
		int res = com_func(p->data, arg);
		if (res > 0){
			p = p->left;
		}
		else if (res < 0) {
			p = p->right;
		}
		else {
			return p;
		}
	}
	return NULL;
}


void _clear_node(Node *p, Node *tail, MemClear clear_func) {
	if (p->left != tail) _clear_node(p->left, tail, clear_func);
	if (p->right != tail) _clear_node(p->right, tail, clear_func);
	if (clear_func != NULL)
		clear_func(p->data);
	lfree(p);
}
void clear(RBTree *tree, MemClear clear_func) {
	Node *p = tree->root;
	if (p != NULL && p != tree->tail) _clear_node(p, tree->tail, clear_func);
	lfree(tree->tail);
	lfree(tree);
}
