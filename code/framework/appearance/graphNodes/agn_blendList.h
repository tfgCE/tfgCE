#pragma once

#include "..\animationGraphNode.h"

//

namespace Framework
{
	namespace AnimationGraphNodesLib
	{
		class BlendList
		: public IAnimationGraphNode
		{
			FAST_CAST_DECLARE(BlendList);
			FAST_CAST_BASE(IAnimationGraphNode);
			FAST_CAST_END();

			typedef IAnimationGraphNode base;

		public:
			static IAnimationGraphNode* create_node();

			BlendList() {}
			virtual ~BlendList() {}

		public: // IAnimationGraphNode
			implement_ bool load_from_xml(IO::XML::Node const* _node, LibraryLoadingContext& _lc);
			implement_ bool prepare_runtime(AnimationGraphRuntime& _runtime, AnimationGraphNodeSet& _inSet);
			implement_ void prepare_instance(AnimationGraphRuntime& _runtime) const;
			implement_ void on_activate(AnimationGraphRuntime& _runtime) const;
			implement_ void on_reactivate(AnimationGraphRuntime& _runtime) const;
			implement_ void on_deactivate(AnimationGraphRuntime& _runtime) const;
			implement_ void execute(AnimationGraphRuntime& _runtime, AnimationGraphExecuteContext& _context, OUT_ AnimationGraphOutput& _output) const;

		private:
			ANIMATION_GRAPH_RUNTIME_VARIABLES_USER(BlendList);

			Name blendVarID;
			Array<AnimationGraphNodeInput> inputs;
			bool circular = false;

			float blendTime = 0.0f;
			bool blendLineary = false;

			void update_curr_blend(AnimationGraphRuntime& _runtime, bool _justActivated, float _deltaTime) const;
			void update_inputs(AnimationGraphRuntime& _runtime, REF_ int & _activeInputHi, REF_ int & _activeInputLo, int _inputHi, int _inputLo) const;
		};
	};
};
