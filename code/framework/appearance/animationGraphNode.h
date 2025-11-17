#pragma once

#include "..\..\core\math\math.h"
#include "..\..\core\types\optional.h"

//

/*
	Use only plain variables!
	The order has to be the same!

	in class
		ANIMATION_GRAPH_RUNTIME_VARIABLES_USER(AnimationPlayback);

	in prepare_runtime():
		ANIMATION_GRAPH_RUNTIME_VARIABLES_START_DECLARING(AnimationPlayback);
		ANIMATION_GRAPH_RUNTIME_VARIABLE_DECLARE(float, time, 0.0f);
		ANIMATION_GRAPH_RUNTIME_VARIABLE_DECLARE(float, playbackSpeed, 0.0f);

	in execute():
		ANIMATION_GRAPH_RUNTIME_VARIABLES_START_USING(AnimationPlayback);
		ANIMATION_GRAPH_RUNTIME_VARIABLE_USE(float, time);
		ANIMATION_GRAPH_RUNTIME_VARIABLE_NOT_USED(float, playbackSpeed);
*/

#define ANIMATION_GRAPH_RUNTIME_VARIABLES_USER(Owner) RUNTIME_ int runtimeDataStartsAt_##Owner = NONE;
#define ANIMATION_GRAPH_RUNTIME_VARIABLES_START_DECLARING(Owner) this->runtimeDataStartsAt_##Owner = _runtime.runtimeData.get_size();
#define ANIMATION_GRAPH_RUNTIME_VARIABLE_DECLARE(Class, name, defValue) \
	{ \
		int currDataOffset = _runtime.runtimeData.get_size(); \
		_runtime.runtimeData.grow_size(sizeof(Class)); \
		Class& name = *(Class*)&_runtime.runtimeData[currDataOffset]; \
		name = defValue; \
	}
#define ANIMATION_GRAPH_RUNTIME_VARIABLE_DECLARE_NO_DEFAULT(Class, name) \
	{ \
		_runtime.runtimeData.grow_size(sizeof(Class)); \
	}

#define ANIMATION_GRAPH_RUNTIME_VARIABLES_START_USING(Owner) int currDataOffset = this->runtimeDataStartsAt_##Owner;
#define ANIMATION_GRAPH_RUNTIME_VARIABLE_USE(Class, name) Class & name = *(Class*)&_runtime.runtimeData[currDataOffset]; currDataOffset += sizeof(Class);
#define ANIMATION_GRAPH_RUNTIME_VARIABLE_NOT_USED(Class, name) currDataOffset += sizeof(Class);

//

namespace Meshes
{
	class Skeleton;
	struct Pose;
}

namespace Framework
{
	class AnimationGraphNodeSet;
	class ModuleAppearance;
	interface_class IAnimationGraphNode;
	interface_class IModulesOwner;
	struct AnimationGraphExecuteContext;
	struct AnimationGraphRuntime;
	struct AnimationGraphOutput;
	struct LibraryLoadingContext;

	/*
	 *	Part of animation graph, is not copied for instances (!) uses plain variables stored in _runtime.runtimeData
	 * 
	 *	Use AnimationGraphNodeWithSingleInput for a node with single child
	 */
	interface_class IAnimationGraphNode
	: public RefCountObject
	{
		FAST_CAST_DECLARE(IAnimationGraphNode);
		FAST_CAST_END();

	public:
		virtual ~IAnimationGraphNode() {}

		Name const& get_id() const { return id; }

	public:
		virtual bool load_from_xml(IO::XML::Node const* _node, LibraryLoadingContext& _lc);

		// this preparation is done once, on graph node preparation only when animation graph (library stored) is loaded and being resolved
		// prepare runtime variables, we arrange data in a huge array, this should be plain data (!)
		// prepare connections between graph nodes (at the same level!)
		virtual bool prepare_runtime(AnimationGraphRuntime& _runtime, AnimationGraphNodeSet& _inSet) { return true; }

			// this is called for every instance, to setup animations, starting values etc.
		virtual void prepare_instance(AnimationGraphRuntime & _runtime) const {}

		// these should be called whenever node is activated/deactivated
		// we should not rely on the same number as some stuff may try to activate a few times when is already deactivating
		virtual void on_activate(AnimationGraphRuntime& _runtime) const {}
		virtual void on_reactivate(AnimationGraphRuntime& _runtime) const {}
		virtual void on_deactivate(AnimationGraphRuntime& _runtime) const {}
		virtual void execute(AnimationGraphRuntime& _runtime, AnimationGraphExecuteContext& _context, OUT_ AnimationGraphOutput& _output) const {}

	protected:
		Name id;
	};	

	/*
	 *	provided for nodes to fill, some nodes may just pass it to children, some may decide to use a different pose/output and blend them
	 *	the executing graph node should have pose provided
	 */
	struct AnimationGraphOutput
	{
	public:
		AnimationGraphOutput() {}
		~AnimationGraphOutput();

		void use_pose(Meshes::Pose* _pose);
		void use_own_pose(Meshes::Skeleton const * _skeleton);
		void drop_pose();

		Meshes::Pose* access_pose() { return pose; }
		Transform & access_root_motion() { return rootMotion; }
		Transform const& get_root_motion() const { return rootMotion; }

		void blend_in(float _blendInWeight, AnimationGraphOutput const& _blendInOutput, bool _blendRootMotion = true);

	private:
		Meshes::Pose* pose = nullptr;
		bool ownPose = false;
		Transform rootMotion = Transform::identity;

		// do not implement
		AnimationGraphOutput(AnimationGraphOutput const&);
		AnimationGraphOutput & operator = (AnimationGraphOutput const&);
	};

	/*
	 *	this structure is stored for each instance, although there is one that is stored in the actual animation graph in library that is used as a template for instances
	 */
	struct AnimationGraphRuntime
	{
		ModuleAppearance* owner = nullptr;
		IModulesOwner* imoOwner = nullptr;
		Array<byte> runtimeData;

		void ready_to_execute(); // each frame

		Optional<float>& access_sync_playback(Name const& _id);
		Optional<float> const * get_prev_sync_playback(Name const& _id) const;
		Optional<float> const * get_any_set_sync_playback(Name const& _id) const; // current or prev

	private:
		struct SyncPlayback
		{
			Name id;
			Optional<float> pt;
			Optional<float> prevPt; // used if pt not available (because we start the update with something that should continue or we completely switched to something new that should continue)
		};

		Array<SyncPlayback> syncPlaybacks;
	};

	struct AnimationGraphExecuteContext
	{
		// this changes every frame
		float deltaTime = 0.0f;
		Optional<float> deltaTimeUnused; // feedback from nodes - for example if we reached the end of the animation, we want to start a next one
	};
	
	class AnimationGraphNodeSet
	{
	public:
		AnimationGraphNodeSet();
		~AnimationGraphNodeSet();

		void clear();

		bool load_from_xml(IO::XML::Node const* _node, LibraryLoadingContext& _lc);

		bool prepare_runtime(AnimationGraphRuntime& _runtime);
		void prepare_instance(AnimationGraphRuntime& _runtime) const;

		IAnimationGraphNode* find_node(Name const& _id) const;

	private:
		Array<RefCountObjectPtr<IAnimationGraphNode>> nodes;
	};

	/*
	 *	stored in graph nodes to point at the right node, just makes life easier
	 */
	struct AnimationGraphNodeInput
	{
		Name const& get_id() const { return id; }
		IAnimationGraphNode * get_node() const { return node; }

		bool load_from_xml(IO::XML::Node const* _node, tchar const * _attrName = TXT("inputNodeId"));

		bool prepare_runtime(AnimationGraphNodeSet& _inSet);

	private:
		Name id;
		IAnimationGraphNode* node = nullptr;
	};

	class AnimationGraphNodeWithSingleInput
	: public IAnimationGraphNode
	{
		FAST_CAST_DECLARE(AnimationGraphNodeWithSingleInput);
		FAST_CAST_BASE(IAnimationGraphNode);
		FAST_CAST_END();

		typedef IAnimationGraphNode base;
	public:
		virtual ~AnimationGraphNodeWithSingleInput() {}

	public:
		virtual bool load_from_xml(IO::XML::Node const* _node, LibraryLoadingContext& _lc);
		virtual bool prepare_runtime(AnimationGraphRuntime& _runtime, AnimationGraphNodeSet& _inSet);
		virtual void prepare_instance(AnimationGraphRuntime& _runtime) const;
		virtual void on_activate(AnimationGraphRuntime& _runtime) const;
		virtual void on_reactivate(AnimationGraphRuntime& _runtime) const;
		virtual void on_deactivate(AnimationGraphRuntime& _runtime) const;
		virtual void execute(AnimationGraphRuntime& _runtime, AnimationGraphExecuteContext& _context, OUT_ AnimationGraphOutput& _output) const;

	protected:
		AnimationGraphNodeInput inputNode;
	};
};
