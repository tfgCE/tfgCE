#pragma once

#include "..\library\libraryStored.h"

#include "animationGraphNode.h"

namespace Framework
{
	class AnimationGraph
	: public LibraryStored
	{
		FAST_CAST_DECLARE(AnimationGraph);
		FAST_CAST_BASE(LibraryStored);
		FAST_CAST_END();
		LIBRARY_STORED_DECLARE_TYPE();

		typedef LibraryStored base;
	public:
		AnimationGraph(Library* _library, LibraryName const& _name);
		virtual ~AnimationGraph();

	public:
		void prepare_instance(AnimationGraphRuntime& _runtime, ModuleAppearance* _owner);

		// check animation graph node, uses external runtime!
		void on_activate(AnimationGraphRuntime& _runtime);
		void on_reactivate(AnimationGraphRuntime& _runtime); // can be still active but we reentered it
		void on_deactivate(AnimationGraphRuntime& _runtime);
		void execute(AnimationGraphRuntime& _runtime, AnimationGraphExecuteContext& _context, OUT_ AnimationGraphOutput& _output);

	public: // LibraryStored
#ifdef AN_DEVELOPMENT
		override_ void ready_for_reload();
#endif
		override_ void prepare_to_unload();

		override_ bool load_from_xml(IO::XML::Node const* _node, LibraryLoadingContext& _lc);
		override_ bool prepare_for_game(Library* _library, LibraryPrepareContext& _pfgContext);

		override_ void log(LogInfoContext& _context) const;

	private:
		// check animation graph node
		bool prepare_runtime();

	private:
		AnimationGraphNodeSet nodeSet;
		AnimationGraphNodeInput inputNode;

		AnimationGraphRuntime runtime;
	};
};
