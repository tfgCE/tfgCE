#include "animationSet.h"

#include "..\..\core\mesh\animationSet.h"
#include "..\..\core\serialisation\importerHelper.h"

#include "..\library\library.h"
#include "..\library\usedLibraryStored.inl"
#include "..\module\moduleAppearance.h"

//

#ifdef AN_ALLOW_OPTIMIZE_OFF
#pragma optimize("", off)
#endif

//

using namespace Framework;

//

REGISTER_FOR_FAST_CAST(Framework::AnimationSet);
LIBRARY_STORED_DEFINE_TYPE(AnimationSet, animationSet);

AnimationSet::AnimationSet(Library * _library, LibraryName const & _name)
: LibraryStored(_library, _name)
, animationSet(nullptr)
{
}

AnimationSet::~AnimationSet()
{
}

void AnimationSet::prepare_to_unload()
{
	base::prepare_to_unload();
	delete_and_clear(animationSet);
}

#ifdef AN_DEVELOPMENT
void AnimationSet::ready_for_reload()
{
	base::ready_for_reload();

	if (animationSet)
	{
		animationSet->clear();
	}
}
#endif

struct ::Framework::AnimationSetImporter
: public ImporterHelper<Meshes::AnimationSet, Meshes::AnimationSetImportOptions>
{
	typedef ImporterHelper<Meshes::AnimationSet, Meshes::AnimationSetImportOptions> base;

	AnimationSetImporter(Framework::AnimationSet* _animationSet)
	: base(_animationSet->animationSet)
	{}

protected:
	override_ void create_object()
	{
		object = new Meshes::AnimationSet();
	}

	override_ void delete_object()
	{
		delete_and_clear(object);
	}

	override_ void import_object()
	{
		object = Meshes::AnimationSet::importer().import(importFromFileName, importOptions);
	}
};

bool AnimationSet::load_from_xml(IO::XML::Node const * _node, LibraryLoadingContext & _lc)
{
	make_sure_animation_set_exists();

	bool result = LibraryStored::load_from_xml(_node, _lc);

	{
		AnimationSetImporter animationSetImporter(this);
		result &= animationSetImporter.setup_from_xml(_node, _lc.get_library_source() == LibrarySource::Assets);
		if (!animationSetImporter.import())
		{
#ifdef AN_OUTPUT_IMPORT_ERRORS
			error_loading_xml(_node, TXT("can't import"));
#endif
			result = false;
		}
	}

	return result;
}

bool AnimationSet::prepare_for_game(Library* _library, LibraryPrepareContext& _pfgContext)
{
	bool result = LibraryStored::prepare_for_game(_library, _pfgContext);

	return result;
}

void AnimationSet::log(LogInfoContext & _context) const
{
	base::log(_context);

	if (animationSet)
	{
		animationSet->log(_context);
	}
}

void AnimationSet::make_sure_animation_set_exists()
{
	if (!animationSet)
	{
		animationSet = new Meshes::AnimationSet();
	}
}

//--

void AnimationLookUp::clear()
{
	animationId = Name::invalid();
	animation = nullptr;
}

bool AnimationLookUp::look_up(ModuleAppearance const* _appearance, Name const& _animation)
{
	for_every_ref(animSet, _appearance->get_animation_sets())
	{
		auto* aSet = animSet->get_animation_set();
		int animIdx = aSet->find_animation_index(_animation);
		if (animIdx != NONE)
		{
			animationId = _animation;
			animation = &aSet->get_animation(animIdx);

			return true;
		}
	}

	animationId = Name::invalid();
	animation = nullptr;

	return false;
}
