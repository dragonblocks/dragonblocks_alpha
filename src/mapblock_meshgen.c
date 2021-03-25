static bool compare_positions(void *p1, void *p2)
{
	v3s32 *pos1 = p1;
	v3s32 *pos2 = p2;
	return pos1->x == pos2->x && pos1->y == pos2->y && pos->z == pos2->z;
}

static void *mapblock_meshgen_thread(void *cliptr)
{
	Client *client = cliptr;

	for ever {
		ListPair **lptr = &client->mapblock_meshgen_queue.first;
		if (*lptr) {
			MapBlock *block = map_get_block(client->map, *(v3s32 *)(*lptr)->key, false);
			Array vertices(sizeof(GLfloat));

			ITERATE_MAPBLOCK {
				MapNode node = block->data[x][y][z];
				BlockDef *def = block->getDef();
				if (! def->drawable)
					continue;
				ivec3 bpos(x, y, z);
				vec3 pos_from_mesh_origin = vec3(bpos) - vec3(SIZE / 2 + 0.5);
				for (int facenr = 0; facenr < 6; facenr++) {
					ivec3 npos = bpos + face_dir[facenr];
					const Block *neighbor_own, *neighbor;
					neighbor_own = neighbor = getBlockNoEx(npos);
					if (! neighbor)
						neighbor = map->getBlock(pos * SIZE + npos);
					if (neighbor && ! neighbor->getDef()->drawable)
						any_drawable_block = true;
					if (! mesh_created_before)
						neighbor = neighbor_own;
					if (! mesh_created_before && ! neighbor || neighbor && ! neighbor->getDef()->drawable) {
						textures.push_back(def->tile_def.get(facenr));
						for (int vertex_index = 0; vertex_index < 6; vertex_index++) {
							for (int attribute_index = 0; attribute_index < 5; attribute_index++) {
								GLdouble value = box_vertices[facenr][vertex_index][attribute_index];
								switch (attribute_index) {
									case 0:
									value += pos_from_mesh_origin.x;
									break;
									case 1:
									value += pos_from_mesh_origin.y;
									break;
									case 2:
									value += pos_from_mesh_origin.z;
									break;
								}
								vertices.push_back(value);
							}
						}
					}
				}
			}

			client_create_mesh(client, create_mesh(vertices.ptr, vertices.siz));
			pthread_mutex_lock(client->mapblock_meshgen_mtx);
			*lptr = (*lptr)->next;
			pthread_mutex_unlock(client->mapblock_meshgen_mtx);
		} else {
			sched_yield();
		}
	}
}

void client_mapblock_changed(Client *client, v3s32 pos)
{
	v3s32 *posptr = malloc(sizeof(v3s32));
	*posptr = pos;
	pthread_mutex_lock(client->mapblock_meshgen_mtx);
	if (! list_put(&client->mapblock_meshgen_queue, posptr, NULL))
		free(posptr);
	pthread_mutex_unlock(client->mapblock_meshgen_mtx);
}
