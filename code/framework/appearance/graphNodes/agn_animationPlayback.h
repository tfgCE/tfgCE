#pragma once

#include "..\animationGraphNode.h"

//

namespace Framework
{
	namespace AnimationGraphNodesLib
	{
		class AnimationPlayback
		: public IAnimationGraphNode
		{
			FAST_CAST_DECLARE(AnimationPlayback);
			FAST_CAST_BASE(IAnimationGraphNode);
			FAST_CAST_END();

			typedef IAnimationGraphNode base;

		public:
			static IAnimationGraphNode* create_node();

			AnimationPlayback() {}
			virtual ~AnimationPlayback() {}

		public: // IAnimationGraphNode
			implement_ bool load_from_xml(IO::XML::Node const* _node, LibraryLoadingContext& _lc);
			implement_ bool prepare_runtime(AnimationGraphRuntime& _runtime, AnimationGraphNodeSet& _inSet);
			implement_ void prepare_instance(AnimationGraphRuntime& _runtime) const;
			implement_ void on_activate(AnimationGraphRuntime& _runtime) const;
			implement_ void on_reactivate(AnimationGraphRuntime& _runtime) const;
			implement_ void on_deactivate(AnimationGraphRuntime& _runtime) const;
			implement_ void execute(AnimationGraphRuntime& _runtime, AnimationGraphExecuteContext& _context, OUT_ AnimationGraphOutput& _output) const;

		private:
			ANIMATION_GRAPH_RUNTIME_VARIABLES_USER(AnimationPlayback);

			Name animation;
			bool looped = false;
			float playbackSpeed = 1.0f;
			Name playbackSpeedVarID;
			Optional<float> playbackPt;
			Name playbackPtVarID;
			Name syncPlaybackPtID;
			Name provideSyncPlaybackPtID;
			float syncPlaybackPtOffset = 0.0f;
		};
	};
};
