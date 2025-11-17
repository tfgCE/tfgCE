#pragma once

#include "..\..\core\mesh\animationSet.h"

#include "..\library\libraryStored.h"

namespace Framework
{
	struct AnimationSetImporter;
	class Animation;
	class ModuleAppearance;

	PLAIN_ struct AnimationLookUp
	{
		Meshes::Animation const* get() const { return animation; }
		bool is_valid() const { return animation; }

		void clear();
		bool look_up(ModuleAppearance const* _appearance, Name const& _animation);

	private:
		Name animationId;
		Meshes::Animation const* animation = nullptr;
	};

	class AnimationSet
	: public LibraryStored
	{
		FAST_CAST_DECLARE(AnimationSet);
		FAST_CAST_BASE(LibraryStored);
		FAST_CAST_END();
		LIBRARY_STORED_DECLARE_TYPE();

		typedef LibraryStored base;
	public:
		AnimationSet(Library * _library, LibraryName const & _name);
		virtual ~AnimationSet();

		Meshes::AnimationSet* get_animation_set() { return animationSet; }

	public: // LibraryStored
#ifdef AN_DEVELOPMENT
		override_ void ready_for_reload();
#endif
		override_ void prepare_to_unload();

		override_ bool load_from_xml(IO::XML::Node const * _node, LibraryLoadingContext & _lc);
		override_ bool prepare_for_game(Library* _library, LibraryPrepareContext& _pfgContext);

		override_ void log(LogInfoContext & _context) const;

	private:
		Meshes::AnimationSet* animationSet = nullptr;

		void make_sure_animation_set_exists();

		friend struct AnimationSetImporter;
	};
};
