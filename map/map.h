#ifndef _MAP_H__
#define _MAP_H__
#include "rbTree.h"

//定义基本的map一个元素的结构
struct Item{
	char *key;
	void *value;
};
typedef struct Item Item;

// compare函数是为了实现两个比较的
// show_item是展示其中一个元素的
// inner_clear是清除一个元素的内存
// 由于红黑树中存储的是void *所以在这里需要提供这么三个函数
int compare(void *, void *);
//需要用户自己提供的函数
//构造一个元素key:value
//Item *new_item(const char*, void *, int size);

struct Map{
	RBTree *tree;
};
typedef struct Map Map;
typedef void (*FUNC)(void *);

//构造一个map
Map map();
//将元素加入map中
void add_item(Map *pmap, Item *);
//获取key对应value
void *value(Map *, const char *);
//展示map中的数据
void mapshow(Map *pmap, FUNC show_item);
//清除map所占用的内存
void map_clear(Map *, FUNC);

#endif //_MAP_H__
