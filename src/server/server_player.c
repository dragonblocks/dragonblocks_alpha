#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <dragonstd/list.h>
#include "server/database.h"
#include "server/server_config.h"
#include "server/server_map.h"
#include "server/server_player.h"
#include "perlin.h"
#include "day.h"
#include "util.h"

static bool shutting_down = false;
static pthread_rwlock_t shutting_down_lock;

static List players;
static pthread_rwlock_t players_lock;

static List names;
static pthread_rwlock_t names_lock;

static u64 next_id = 1;

static bool list_compare_u64(u64 *p1, u64 *p2)
{
	return *p1 == *p2;
}

static bool get_lock(pthread_rwlock_t *lock, bool write)
{
	pthread_rwlock_rdlock(&shutting_down_lock);
	if (shutting_down) {
		pthread_rwlock_unlock(&shutting_down_lock);
		return false;
	}

	if (write)
		pthread_rwlock_wrlock(lock);
	else
		pthread_rwlock_rdlock(lock);

	pthread_rwlock_unlock(&shutting_down_lock);
	return true;
}

void server_player_init()
{
	pthread_rwlock_init(&shutting_down_lock, NULL);

	players = list_create((void *) &list_compare_u64);
	pthread_rwlock_init(&players_lock, NULL);

	names = list_create(&list_compare_string);
	pthread_rwlock_init(&names_lock, NULL);
}

// list_clear_func callback used on server shutdown to disconnect all clients properly
static void list_disconnect_player(void *key, unused void *value, unused void *arg)
{
	ServerPlayer *player = key;

	pthread_t recv_thread = player->peer->recv_thread;
	server_player_disconnect(player);
	pthread_join(recv_thread, NULL);
}

void server_player_deinit()
{
	pthread_rwlock_wrlock(&shutting_down_lock);
	shutting_down = true;

	pthread_rwlock_wrlock(&players_lock);
	pthread_rwlock_wrlock(&names_lock);
	pthread_rwlock_unlock(&shutting_down_lock);

	list_clear_func(&players, &list_disconnect_player, NULL);
	list_clear(&names);

	pthread_rwlock_destroy(&players_lock);
	pthread_rwlock_destroy(&names_lock);
	pthread_rwlock_destroy(&shutting_down_lock);
}

void server_player_add(DragonnetPeer *peer)
{
	ServerPlayer *player = malloc(sizeof *player);

	player->id = next_id++;
	player->peer = peer;
	pthread_rwlock_init(&player->ref, NULL);
	player->auth = false;
	player->name = dragonnet_addr_str(peer->raddr);
	pthread_rwlock_init(&player->auth_lock, NULL);
	player->pos = (v3f64) {0.0f, 0.0f, 0.0f};
	pthread_rwlock_init(&player->pos_lock, NULL);

	printf("Connected %s\n", player->name);

	// accept thread is joined before shutdown, we are guaranteed to obtain the lock
	pthread_rwlock_wrlock(&players_lock);

	list_put(&players, &player->id, player);
	peer->extra = player;

	pthread_rwlock_unlock(&players_lock);
}

void server_player_remove(DragonnetPeer *peer)
{
	ServerPlayer *player = peer->extra;

	// only (this) recv thread will modify the auth or name fields, no rdlocks needed

	if (get_lock(&players_lock, true)) {
		list_delete(&players, &player->id);
		pthread_rwlock_unlock(&players_lock);

		printf("Disconnected %s\n", player->name);
	}

	if (player->auth && get_lock(&names_lock, true)) {
		list_delete(&names, player->name);
		pthread_rwlock_unlock(&names_lock);
	}

	pthread_rwlock_wrlock(&player->ref);

	free(player->name);

	pthread_rwlock_destroy(&player->ref);
	pthread_rwlock_destroy(&player->auth_lock);
	pthread_rwlock_destroy(&player->pos_lock);

	free(player);
}

u64 server_player_find(char *name)
{
	if (! get_lock(&names_lock, false))
		return 0;

	u64 *id = list_get(&names, name);
	return id ? *id : 0;
}

ServerPlayer *server_player_grab(u64 id)
{
	if (! id)
		return NULL;

	if (! get_lock(&players_lock, false))
		return NULL;

	ServerPlayer *player = list_get(&players, &id);
	if (player)
		pthread_rwlock_rdlock(&player->ref);

	pthread_rwlock_unlock(&players_lock);

	return player;
}

void server_player_drop(ServerPlayer *player)
{
	pthread_rwlock_unlock(&player->ref);
}

bool server_player_auth(ServerPlayer *player, char *name)
{
	if (! get_lock(&names_lock, true))
		return false;

	pthread_rwlock_wrlock(&player->auth_lock);
	pthread_rwlock_wrlock(&player->pos_lock);

	bool success = list_put(&names, name, &player->id);

	dragonnet_peer_send_ToClientAuth(player->peer, &(ToClientAuth) {
		.success = success,
	});

	if (success) {
		printf("Authentication %s: %s -> %s\n", success ? "success" : "failure", player->name, name);

		free(player->name);
		player->name = name;
		player->auth = true;

		if (! database_load_player(player->name, &player->pos)) {
			player->pos = (v3f64) {0.0, server_map.spawn_height + 0.5, 0.0};
			database_create_player(player->name, player->pos);
		}

		dragonnet_peer_send_ToClientInfo(player->peer, &(ToClientInfo) {
			.seed = seed,
			.simulation_distance = server_config.simulation_distance,
		});

		dragonnet_peer_send_ToClientTimeOfDay(player->peer, &(ToClientTimeOfDay) {
			.time_of_day = get_time_of_day(),
		});

		server_player_send_pos(player);
	}

	pthread_rwlock_unlock(&player->pos_lock);
	pthread_rwlock_unlock(&player->auth_lock);
	pthread_rwlock_unlock(&names_lock);

	return success;
}

void server_player_disconnect(ServerPlayer *player)
{
	dragonnet_peer_shutdown(player->peer);
}

void server_player_send_pos(ServerPlayer *player)
{
	dragonnet_peer_send_ToClientPos(player->peer, & (ToClientPos) {
		.pos = player->pos,
	});
}

void server_player_iterate(void (cb)(ServerPlayer *, void *), void *arg)
{
	if (! get_lock(&players_lock, false))
		return;

	ITERATE_LIST(&players, pair) {
		ServerPlayer *player = pair->value;

		pthread_rwlock_rdlock(&player->auth_lock);
		if (player->auth)
			cb(player, arg);
		pthread_rwlock_unlock(&player->auth_lock);
	}

	pthread_rwlock_unlock(&players_lock);
}

/*
229779
373875
374193
110738
390402
357272
390480

(these are only the wholesome ones)
*/
