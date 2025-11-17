#pragma once

#include "..\..\core\containers\map.h"
#include "..\..\core\types\name.h"
#include "..\..\core\other\factoryOfNamed.h"

namespace Framework
{
	interface_class IAnimationGraphNode;
	/**
	 *	Function that can register animation graph nodes by name and allows their creation.
	 */
	class AnimationGraphNodes
	: private FactoryOfNamed<IAnimationGraphNode, Name>
	{
		typedef FactoryOfNamed<IAnimationGraphNode, Name> base;
	public:
		static void initialise_static();
		static void close_static();

		static void register_animation_graph_node(Name const & _name, Create _create) { base::add(_name, _create); }
		static IAnimationGraphNode* create_node(Name const & _name) { return base::create(_name); }
	};
};
