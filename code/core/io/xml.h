#pragma once

/**
 *	XML here is not the pure XML.
 *
 *	It has some extensions, for example it allows for multiple root nodes.
 *
 *	Also it has a "commented out node". Any node that has a name starting with "." (node_commented_out_prefix) is commented out node,
 *	is still written as normal node into the file, but when processed, such nodes are ignored, treated as comments. It is enough
 *	to have prefix only in leading node: <.node>content</node> which of course breaks xml viewers
 */
#include "xmlDocument.h"
#include "xmlNode.h"
#include "xmlAttribute.h"
#include "xmlIterators.h"
