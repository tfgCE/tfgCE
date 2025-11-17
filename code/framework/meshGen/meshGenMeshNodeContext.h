#pragma once

#include "..\..\core\wheresMyPoint\iWMPOwner.h"

#include "meshGenGenerationContext.h"

namespace IO
{
	namespace XML
	{
		class Node;
	};
};

namespace Framework
{
	namespace MeshGeneration
	{
		class MeshNodeContext
		: public WheresMyPoint::IOwner
		{
			FAST_CAST_DECLARE(MeshNodeContext);
			FAST_CAST_BASE(WheresMyPoint::IOwner);
			FAST_CAST_END();

			typedef WheresMyPoint::IOwner base;
		public:
			MeshNodeContext(GenerationContext & _generationContext, ElementInstance & _elementInstance, MeshNode * _meshNode);
			~MeshNodeContext();

			GenerationContext & access_generation_context() { return generationContext; }
			ElementInstance & access_element_instance() { return elementInstance; }
			MeshNode * access_mesh_node() { return meshNode.get(); }
			MeshNode const * get_mesh_node() const { return meshNode.get(); }

		public: // WheresMyPoint::IOwner
			override_ bool store_value_for_wheres_my_point(Name const & _byName, WheresMyPoint::Value const & _value);
			override_ bool restore_value_for_wheres_my_point(Name const & _byName, OUT_ WheresMyPoint::Value & _value) const;
			override_ bool store_convert_value_for_wheres_my_point(Name const& _byName, TypeId _to);
			override_ bool rename_value_forwheres_my_point(Name const& _from, Name const& _to);
			override_ bool store_global_value_for_wheres_my_point(Name const & _byName, WheresMyPoint::Value const & _value);
			override_ bool restore_global_value_for_wheres_my_point(Name const & _byName, OUT_ WheresMyPoint::Value & _value) const;
			override_ bool get_point_for_wheres_my_point(Name const & _byName, OUT_ Vector3 & _point) const;

			override_ IOwner* get_wmp_owner() { return &elementInstance; }

		private:
			GenerationContext & generationContext;
			ElementInstance & elementInstance;
			MeshNodePtr meshNode;
		};
	};
};
