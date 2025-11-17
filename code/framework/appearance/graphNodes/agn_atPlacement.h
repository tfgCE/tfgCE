#pragma once

#include "..\animationGraphNode.h"
#include "..\controllers\ac_atPlacement.h"

//

namespace Framework
{
	namespace AnimationGraphNodesLib
	{
		class AtPlacement
		: public AnimationGraphNodeWithSingleInput
		{
			FAST_CAST_DECLARE(AtPlacement);
			FAST_CAST_BASE(AnimationGraphNodeWithSingleInput);
			FAST_CAST_END();

			typedef AnimationGraphNodeWithSingleInput base;

		public:
			static IAnimationGraphNode* create_node();

			AtPlacement() {}
			virtual ~AtPlacement() {}

		public: // IAnimationGraphNode
			implement_ bool load_from_xml(IO::XML::Node const* _node, LibraryLoadingContext& _lc);
			implement_ bool prepare_runtime(AnimationGraphRuntime& _runtime, AnimationGraphNodeSet& _inSet);
			implement_ void prepare_instance(AnimationGraphRuntime& _runtime) const;
			implement_ void on_activate(AnimationGraphRuntime& _runtime) const;
			implement_ void on_reactivate(AnimationGraphRuntime& _runtime) const;
			implement_ void on_deactivate(AnimationGraphRuntime& _runtime) const;
			implement_ void execute(AnimationGraphRuntime& _runtime, AnimationGraphExecuteContext& _context, OUT_ AnimationGraphOutput& _output) const;

		private:
			ANIMATION_GRAPH_RUNTIME_VARIABLES_USER(AtPlacement);

			Name bone;
			Transform placement = Transform::identity;
			AppearanceControllersLib::AtPlacementType::Type placementType = AppearanceControllersLib::AtPlacementType::BS;
		};
	};
};
