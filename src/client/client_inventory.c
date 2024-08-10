#include <stdatomic.h>
#include <stdio.h>
#include <stdlib.h>
#include "client/client.h"
#include "client/client_config.h"
#include "client/client_inventory.h"
#include "client/client_item.h"
#include "client/gui.h"
#include "client/opengl.h"
#include "client/frustum.h"
#include "client/light.h"
#include "client/shader.h"
#include "common/inventory.h"

// for future use
#define INV_SIZE_ARMOR 4
#define	INVENTORY_ARMOR (INVENTORY_MAIN+1)

static struct {
	GLuint shader_prog;
	GLint loc_VP;
	GLint loc_depthOffset;
	ModelShader model_shader;
	LightShader light_shader;
} wield;

static struct {
	GUIElement *root;
	Texture *texture;

	GUIElement *hands[INV_SIZE_HANDS];
	GUIElement *main[INV_SIZE_MAIN];
	GUIElement *armor[INV_SIZE_ARMOR]; // for future use

	GUIElement *selected;
	v2f32 selected_scale;
	float selected_time;

	atomic_flag synced;
} menu;

// loc helpers

static InventoryLocation *loc_new(u16 l, size_t n)
{
	InventoryLocation *p = malloc(sizeof *p);
	*p = (InventoryLocation) { l, n };
	return p;
}

static GUIElement **loc_slot(InventoryLocation *loc)
{
	switch (loc->list) {
		case INVENTORY_HANDS: return &menu.hands[loc->slot];
		case INVENTORY_MAIN: return &menu.main[loc->slot];
		case INVENTORY_ARMOR: return &menu.armor[loc->slot];
	}
	return NULL;
}

static ItemType loc_item(InventoryLocation *loc)
{
	ClientEntity *entity = client_player_entity_local();
	if (!entity)
		return ITEM_NONE;

	ClientPlayerData *data = entity->extra;
	pthread_mutex_lock(&data->mtx_inv);

	ItemType item = ITEM_NONE;
	switch (loc->list) {
		case INVENTORY_HANDS: item = data->inventory.hands[loc->slot].type; break;
		case INVENTORY_MAIN: item = data->inventory.main[loc->slot].type; break;
	}

	pthread_mutex_unlock(&data->mtx_inv);
	refcount_drp(&entity->rc);

	return item;
}

static const char *list_name(u16 l)
{
	switch (l) {
		case INVENTORY_HANDS: return "hands";
		case INVENTORY_MAIN: return "main";
		case INVENTORY_ARMOR: return "armor";
	}
	return NULL;
}

// menu helpers

static void menu_slot_update(GUIElement *elem, ItemType item)
{
	float time = elem == menu.selected ? menu.selected_time : 0.0;
	GUIElement *mesh_elem = ((GUIElement **) elem->children.ptr)[0];

	ClientItemDef *def = &client_item_def[item];
	mesh_elem->def.mesh = client_item_mesh(item);

	v3s32 size = v3s32_add(v3s32_sub(def->mesh_extents.max, def->mesh_extents.min), (v3s32) {1,1,1});
	v3s32 center = v3s32_add(def->mesh_extents.min, def->mesh_extents.max);
	float scale = 1.0/s32_max(size.x, s32_max(size.y, size.z));

	mat4x4 trans;
	mat4x4_identity(trans);

	mat4x4_scale_aniso(trans, trans, scale, scale, scale);
	mat4x4_rotate_Y(trans, trans, M_PI*time);
	mat4x4_rotate_Z(trans, trans, M_PI*0.8);
	mat4x4_rotate_Y(trans, trans, M_PI*(0.5+def->inventory_rot));

	mat4x4 tmp;
	mat4x4_translate(tmp, -center.x/2.0, -center.y/2.0, -center.z/2.0);
	mat4x4_mul(trans, trans, tmp);

	mat4x4_dup(mesh_elem->def.mesh_transform, trans);

	gui_transform(elem);
}

static void menu_unselect()
{
	if (!menu.selected) return;
	GUIElement *elem = menu.selected;
	menu.selected = NULL;

	elem->def.scale = menu.selected_scale;
	menu_slot_update(elem, loc_item(elem->user));
}

static void menu_slot_click(GUIElement *element, bool right)
{
	if (right) return;

	InventoryLocation *loc = element->user;
	if (menu.selected) {
		InventoryLocation *other = menu.selected->user;

		printf("[info] inventory swap (%s %zu) with (%s %zu)\n",
			list_name(loc->list), (size_t) loc->slot, list_name(other->list), (size_t) other->slot);

		dragonnet_peer_send_ToServerInventorySwap(client, &(ToServerInventorySwap) {
			.locations = { *loc, *other },
		});

		menu_unselect();
	} else {
		menu.selected_scale = element->def.scale;
		element->def.scale.x *= 1.1;
		element->def.scale.y *= 1.1;
		menu.selected_time = 0.0;
		gui_transform(menu.selected = element);
	}
}

static void menu_slot_add(u16 list, size_t slot, f32 x, f32 y, f32 w, f32 h)
{
	GUIElement *ui = gui_add(menu.root, (GUIElementDef) {
		.pos = { x, y },
		.scale = { w, h },
		.align = { 0.5, 0.5 },
		.scale_type = SCALE_PARENT,
		.image = menu.texture,
		.on_click = menu_slot_click,
	});

	gui_add(ui, (GUIElementDef) {
		.pos = { 0.5, 0.5 },
		.scale = { 1.0, 1.0 },
		.scale_type = SCALE_PARENT,
	});

	*loc_slot(ui->user = loc_new(list, slot)) = ui;
}

static void menu_sync()
{
	if (!atomic_flag_test_and_set(&menu.synced))
		return;

	ClientEntity *entity = client_player_entity_local();
	if (!entity)
		return;

	ClientPlayerData *data = entity->extra;
	pthread_mutex_lock(&data->mtx_inv);

	for (size_t i = 0; i < INV_SIZE_HANDS; i++)
		menu_slot_update(menu.hands[i], data->inventory.hands[i].type);
	for (size_t i = 0; i < INV_SIZE_MAIN; i++)
		menu_slot_update(menu.main[i], data->inventory.main[i].type);

	pthread_mutex_unlock(&data->mtx_inv);
	refcount_drp(&entity->rc);
}

static void menu_set_open(bool open)
{
	menu.root->visible = open;
	if (!open)
		menu_unselect();
}

static void menu_update(float dtime)
{
	if (menu.selected) {
		menu.selected_time += dtime;
		if (menu.root->visible)
			menu_slot_update(menu.selected, loc_item(menu.selected->user));
	}

	menu_sync();
}

static void menu_init()
{
	const float aspect_ratio = 1.0/0.6;

	menu.root = gui_add(NULL, (GUIElementDef) {
		.pos = { 0.5, 0.5 },
		.z_index = 0.6,
		.scale = { 0.85, 0.85 },
		.align = { 0.5, 0.5 },
		.scale_type = SCALE_RATIO,
		.ratio = aspect_ratio,
		.bg_color = { (float) 0x33/0xff, (float) 0x41/0xff, (float) 0x82/0xff, 1.0 },
	});
	menu.root->visible = false;

	menu.texture = texture_load(ASSET_PATH "textures/inventory/slot.png", false);

	menu_slot_add(INVENTORY_HANDS, 0,
		0.025+0.05*1.5, 0.675+1.5*0.05*aspect_ratio,
		0.15, 0.15 * aspect_ratio);

	menu_slot_add(INVENTORY_HANDS, 1,
		0.8+0.025+0.05*1.5, 0.675+1.5*0.05*aspect_ratio,
		0.15, 0.15 * aspect_ratio);

	for (size_t yi = 0; yi < 4; yi++)
	for (size_t xi = 0; xi < 6; xi++) {
		menu_slot_add(INVENTORY_MAIN, yi*6+xi,
			0.2 + 0.1 * xi + 0.05, 0.3 + (0.1 * yi + 0.05) * aspect_ratio,
			0.1, 0.1 * aspect_ratio);
	}

	for (size_t i = 0; i < INV_SIZE_ARMOR; i++) {
		menu_slot_add(INVENTORY_ARMOR, i,
			0.25 + 0.125 * i + 0.05, (0.025 + 0.05*1.25) * aspect_ratio,
			0.125, 0.125 * aspect_ratio);
	}
}

static void menu_desync()
{
	atomic_flag_clear(&menu.synced);
}

// wield helpers

static void wield_depth_offset(f32 offset)
{
	glProgramUniform1f(wield.shader_prog, wield.loc_depthOffset, offset);
}

static void wield_init()
{
	char *shader_def;
	asprintf(&shader_def, "#define VIEW_DISTANCE %lf\n", client_config.view_distance);
	wield.shader_prog = shader_program_create(ASSET_PATH "shaders/3d/item", shader_def);
	free(shader_def);

	wield.loc_VP = glGetUniformLocation(wield.shader_prog, "VP");
	wield.loc_depthOffset = glGetUniformLocation(wield.shader_prog, "depthOffset");

	wield.model_shader.prog = wield.shader_prog;
	wield.model_shader.loc_transform = glGetUniformLocation(wield.shader_prog, "model"); GL_DEBUG

	wield.light_shader.prog = wield.shader_prog;
	light_shader_locate(&wield.light_shader);

	wield_depth_offset(0.0f);
}

static void wield_init_hand(ModelNode *hand)
{
	if (hand)
		model_node_add_mesh(hand, &(ModelMesh) {
			.mesh = NULL,
			.textures = NULL,
			.num_textures = 0,
			.shader = &wield.model_shader,
		});
}

static void wield_update_hand(ModelNode *hand, ModelNode *arm, ItemType item)
{
	Mesh *mesh = client_item_mesh(item);

	if (hand)
		((ModelMesh *) hand->meshes.ptr)[0].mesh = mesh;

	if (arm) {
		arm->rot.x = mesh ? -M_PI / 8.0 : 0.0;
		model_node_transform(arm);
	}
}

static void wield_update()
{
	glProgramUniformMatrix4fv(wield.shader_prog, wield.loc_VP, 1, GL_FALSE, frustum[0]); GL_DEBUG
	light_shader_update(&wield.light_shader);
}

static void wield_deinit()
{
	glDeleteProgram(wield.shader_prog); GL_DEBUG
}

// public api

void client_inventory_init()
{
	wield_init();
	menu_init();
}

void client_inventory_deinit()
{
	wield_deinit();
}

void client_inventory_set_open(bool open)
{
	menu_set_open(open);
}

void client_inventory_update(float dtime)
{
	wield_update();
	menu_update(dtime);
}

void client_inventory_depth_offset(f32 offset)
{
	wield_depth_offset(offset);
}

void client_inventory_init_player(ClientEntity *entity)
{
	ClientPlayerData *data = entity->extra;

	pthread_mutex_init(&data->mtx_inv, NULL);

	for (size_t i = 0; i < INV_SIZE_HANDS; i++)
		item_stack_initialize(&data->inventory.hands[i]);
	for (size_t i = 0; i < INV_SIZE_MAIN; i++)
		item_stack_initialize(&data->inventory.main[i]);

	wield_init_hand(data->bones.hand_left);
	wield_init_hand(data->bones.hand_right);
}

void client_inventory_deinit_player(ClientEntity *entity)
{
	ClientPlayerData *data = entity->extra;

	pthread_mutex_destroy(&data->mtx_inv);

	for (size_t i = 0; i < INV_SIZE_HANDS; i++)
		item_stack_destroy(&data->inventory.hands[i]);
	for (size_t i = 0; i < INV_SIZE_MAIN; i++)
		item_stack_destroy(&data->inventory.main[i]);
}

void client_inventory_update_player(__attribute__((unused)) void *peer, ToClientPlayerInventory *pkt)
{
	ClientEntity *entity = client_player_entity(pkt->id);
	if (!entity)
		return;

	ClientPlayerData *data = entity->extra;
	pthread_mutex_lock(&data->mtx_inv);

	for (size_t i = 0; i < INV_SIZE_HANDS; i++)
		item_stack_deserialize(&data->inventory.hands[i], &pkt->hands[i]);
	for (size_t i = 0; i < INV_SIZE_MAIN; i++)
		item_stack_deserialize(&data->inventory.main[i], &pkt->main[i]);

	wield_update_hand(data->bones.hand_left,  data->bones.arm_left,  data->inventory.hands[0].type);
	wield_update_hand(data->bones.hand_right, data->bones.arm_right, data->inventory.hands[1].type);

	pthread_mutex_unlock(&data->mtx_inv);

	if (entity->data.type == ENTITY_LOCALPLAYER)
		menu_desync();

	refcount_drp(&entity->rc);
}
