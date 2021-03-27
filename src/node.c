#include "node.h"
#include "util.h"

NodeDefintion node_definitions[NODE_UNLOADED] = {
	{true,  false, "#F44026", {0.0f, 0.0f, 0.0f}},
	{false, false, "",        {0.0f, 0.0f, 0.0f}},
	{true,  false, "#00CB1F", {0.0f, 0.0f, 0.0f}},
	{true,  false, "#854025", {0.0f, 0.0f, 0.0f}},
	{true,  false, "#7A7A7A", {0.0f, 0.0f, 0.0f}},
};

v3f get_node_color(NodeDefintion *def)
{
	return def->color_initialized ? def->color : (def->color = html_to_v3f(def->color_str));
}
