#ifndef _MODEL_H_
#define _MODEL_H_

#include <dragonstd/array.h>
#include <dragonstd/list.h>
#include <GL/glew.h>
#include <GL/gl.h>
#include <linmath.h/linmath.h>
#include <stdbool.h>
#include <stddef.h>
#include "client/mesh.h"
#include "client/texture.h"
#include "types.h"

typedef struct {
	GLuint prog;
	GLint loc_transform;
} ModelShader;

typedef struct {
	ModelShader *shader;
	VertexLayout *layout;
	size_t off_tex_idx;
	Array textures;
} ModelBatch;

typedef struct ModelNode {
	char *name;
	bool visible;
	v3f32 pos, rot, scale;
	mat4x4 abs, rel;
	Array meshes;
	struct ModelNode *parent;
	List children;
} ModelNode;

typedef struct {
	Mesh *mesh;
	GLuint *textures;
	GLuint num_textures;
	ModelShader *shader;
} ModelMesh;

typedef struct {
	char *name;
	ModelNode **node;
} ModelBoneMapping;

typedef struct Model {
	ModelNode *root;
	void *extra;
	aabb3f32 box;
	f32 distance;
	struct Model *replace;
	struct {
		void (*step)(struct Model *model, f64 dtime);
		void (*delete)(struct Model *model);
	} callbacks;
	struct {
		unsigned int delete: 1;
		unsigned int wireframe: 1;
		unsigned int frustum_culling: 1;
		unsigned int transparent: 1;
	} flags;
} Model;

// initialize
void model_init();
// ded
void model_deinit();

// create empty model
Model *model_create();
// load model from file
Model *model_load(const char *path, const char *textures_path, Mesh *cube, ModelShader *shader);
// clone existing model
Model *model_clone(Model *model);
// delete model
void model_delete(Model *model);
// use this as delete callback to free all meshes associated with the model on delete
void model_free_meshes(Model *model);
// get bone locations
void model_get_bones(Model *model, ModelBoneMapping *mappings, size_t num_mappings);

// add a new mode
ModelNode *model_node_create(ModelNode *parent);
// recalculate transform matrices
void model_node_transform(ModelNode *node);
// add a mesh to model node (will be copied)
void model_node_add_mesh(ModelNode *node, const ModelMesh *mesh);
// add and delete batch (may add multiple meshes depending on available texture units)
void model_node_add_batch(ModelNode *node, ModelBatch *batch);

// create batch
ModelBatch *model_batch_create(ModelShader *shader, VertexLayout *layout, size_t off_tex_idx);
// delete batch
void model_batch_free(ModelBatch *batch);
// add a vertex to batch
void model_batch_add_vertex(ModelBatch *batch, GLuint texture, const void *vertex);

// add model to scene
void model_scene_add(Model *model);
// render scene
void model_scene_render(f64 dtime);

#endif // _MODEL_H_
