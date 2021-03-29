#include "node.h"
#include "util.h"

NodeDefintion node_definitions[NODE_UNLOADED] = {
	{true,  false, "#991300", {0.0f, 0.0f, 0.0f}},
	{false, false, "",        {0.0f, 0.0f, 0.0f}},
	{true,  false, "#137822", {0.0f, 0.0f, 0.0f}},
	{true,  false, "#6B3627", {0.0f, 0.0f, 0.0f}},
	{true,  false, "#4F4F4F", {0.0f, 0.0f, 0.0f}},
};

v3f get_node_color(NodeDefintion *def)
{
	return def->color_initialized ? def->color : (def->color = html_to_v3f(def->color_str));
}
