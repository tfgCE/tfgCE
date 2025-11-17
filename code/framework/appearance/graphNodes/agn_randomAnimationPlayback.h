#pragma once

#include "..\animationGraphNode.h"

//

namespace Framework
{
	namespace AnimationGraphNodesLib
	{
		class RandomAnimationPlayback
		: public IAnimationGraphNode
		{
			FAST_CAST_DECLARE(RandomAnimationPlayback);
			FAST_CAST_BASE(IAnimationGraphNode);
			FAST_CAST_END();

			typedef IAnimationGraphNode base;

		public:
			static IAnimationGraphNode* create_node();

			RandomAnimationPlayback() {}
			virtual ~RandomAnimationPlayback() {}

		public: // IAnimationGraphNode
			implement_ bool load_from_xml(IO::XML::Node const* _node, LibraryLoadingContext& _lc);
			implement_ bool prepare_runtime(AnimationGraphRuntime& _runtime, AnimationGraphNodeSet& _inSet);
			implement_ void prepare_instance(AnimationGraphRuntime& _runtime) const;
			implement_ void on_activate(AnimationGraphRuntime& _runtime) const;
			implement_ void on_deactivate(AnimationGraphRuntime& _runtime) const;
			implement_ void execute(AnimationGraphRuntime& _runtime, AnimationGraphExecuteContext& _context, OUT_ AnimationGraphOutput& _output) const;

		private:
			ANIMATION_GRAPH_RUNTIME_VARIABLES_USER(RandomAnimationPlayback);

			struct Animation
			{
				Name animation;
				float probCoef = 1.0f;
			};
			Array<Animation> animations;

			bool looped = false; // if not looped, will switch between anims
			Name provideSyncPlaybackPtID;
		};
	};
};
