#ifndef _ITEM_H_
#define _ITEM_H_

#include <stdbool.h>
#include <stddef.h>
#include "types.h"

typedef enum {
	ITEM_UNKNOWN,
	ITEM_NONE,
	ITEM_PICKAXE,
	ITEM_AXE,
	COUNT_ITEM,
} ItemType;

typedef struct {
	ItemType type;
	u32 count; // i know future me is going to thank me for making it big
	void *data;
} ItemStack;

typedef struct {
	bool stackable;
	size_t data_size;
	void (*create)(ItemStack *stack);
	void (*delete)(ItemStack *stack);
	void (*serialize)(Blob *buffer, void *data);
	void (*deserialize)(Blob *buffer, void *data);
} ItemDef;

void item_stack_initialize(ItemStack *stack);
void item_stack_destroy(ItemStack *stack);

void item_stack_set(ItemStack *stack, ItemType type, u32 count, Blob buffer);
void item_stack_serialize(ItemStack *stack, SerializedItemStack *serialized);
void item_stack_deserialize(ItemStack *stack, SerializedItemStack *serialized);

extern ItemDef item_defs[];

#endif // _ITEM_H_
