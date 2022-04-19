#include <stdlib.h>
#include "item.h"

void item_stack_initialize(ItemStack *stack)
{
	stack->type = ITEM_NONE;
	stack->count = 1;
	stack->data = NULL;

	if (item_defs[stack->type].create)
		item_defs[stack->type].create(stack);	
}

void item_stack_destroy(ItemStack *stack)
{
	if (item_defs[stack->type].delete)
		item_defs[stack->type].delete(stack);	

	if (stack->data) {
		free(stack->data);
		stack->data = NULL;
	}
}

void item_stack_set(ItemStack *stack, ItemType type, u32 count, Blob buffer)
{
	item_stack_destroy(stack);

	stack->type = type;
	stack->count = count;
	stack->data = item_defs[stack->type].data_size > 0 ?
		malloc(item_defs[stack->type].data_size) : NULL;

	if (item_defs[stack->type].create)
		item_defs[stack->type].create(stack);

	if (item_defs[stack->type].deserialize)
		item_defs[stack->type].deserialize(&buffer, stack->data);
}

void item_stack_serialize(ItemStack *stack, SerializedItemStack *serialized)
{
	serialized->type = stack->type;
	serialized->count = stack->count;
	serialized->data = (Blob) {0, NULL};

	if (item_defs[stack->type].serialize)
		item_defs[stack->type].serialize(&serialized->data, stack->data);
}

void item_stack_deserialize(ItemStack *stack, SerializedItemStack *serialized)
{
	ItemType type = serialized->type;

	if (type >= COUNT_ITEM)
		type = ITEM_UNKNOWN;
	
	item_stack_set(stack, type, serialized->count, serialized->data);
}

ItemDef item_defs[COUNT_ITEM] = {
	// unknown
	{
		.stackable = false,
		.data_size = 0,
		.create = NULL,
		.delete = NULL,
		.serialize = NULL,
		.deserialize = NULL,
	},
	// none
	{
		.stackable = false,
		.data_size = 0,
		.create = NULL,
		.delete = NULL,
		.serialize = NULL,
		.deserialize = NULL,
	},
	// pickaxe
	{
		.stackable = false,
		.data_size = 0,
		.create = NULL,
		.delete = NULL,
		.serialize = NULL,
		.deserialize = NULL,
	},
	// axe
	{
		.stackable = false,
		.data_size = 0,
		.create = NULL,
		.delete = NULL,
		.serialize = NULL,
		.deserialize = NULL,
	},
};
