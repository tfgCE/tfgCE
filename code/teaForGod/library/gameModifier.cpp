#include "gameModifier.h"

#include "..\game\game.h"
#include "..\library\library.h"
#include "..\roomGenerators\roomGenerationInfo.h"

#include "..\..\core\tags\tag.h"

#include "..\..\framework\library\usedLibraryStored.inl"

//

using namespace TeaForGodEmperor;

//

REGISTER_FOR_FAST_CAST(GameModifier);
LIBRARY_STORED_DEFINE_TYPE(GameModifier, gameModifier);

struct StaticGameModifier
{
	mutable Concurrency::SpinLock lock = Concurrency::SpinLock(TXT("TeaForGodEmperor.StaticGameModifier.lock"));
	ArrayStatic<Name, 64> all;
	Array<GameModifier*> gameModifiers;

	void add(GameModifier* _gm)
	{
		Concurrency::ScopedSpinLock l(lock, true);
		gameModifiers.push_back_unique(_gm);
		update();
	}

	void remove(GameModifier* _gm)
	{
		Concurrency::ScopedSpinLock l(lock, true);
		gameModifiers.remove(_gm);
		update();
	}
	
	void update()
	{
		Concurrency::ScopedSpinLock l(lock, true);
		all.clear();
		for_every_ptr(gm, gameModifiers)
		{
			if (gm->get_id().is_valid())
			{
				all.push_back(gm->get_id());
			}
		}
	}
	
	GameModifier const* get(Name const & _id) const
	{
		Concurrency::ScopedSpinLock l(lock, true);
		for_every_ptr(gm, gameModifiers)
		{
			if (gm->get_id() == _id)
			{
				return gm;
			}
		}
		return nullptr;
	}

	StaticGameModifier()
	{
		SET_EXTRA_DEBUG_INFO(all, TXT("StaticGameModifier.all"));
	}

};

StaticGameModifier* g_staticGameModifier = nullptr;

//

void GameModifier::initialise_static()
{
	an_assert(!g_staticGameModifier);
	g_staticGameModifier = new StaticGameModifier();
}

void GameModifier::close_static()
{
	delete_and_clear(g_staticGameModifier);
}

VectorInt2 GameModifier::get_game_modifiers_grid_size()
{
	VectorInt2 size = VectorInt2::zero;
	for_every(gm, g_staticGameModifier->all)
	{
		if (auto* gmo = get(*gm))
		{
			size.x = max(size.x, gmo->at.x + 1);
			size.y = max(size.y, gmo->at.y + 1);
		}
	}
	return size;
}

ArrayStatic<Name, 64> GameModifier::get_all_game_modifiers()
{
	an_assert(g_staticGameModifier);
	ArrayStatic<Name, 64> result; SET_EXTRA_DEBUG_INFO(result, TXT("GameModifier::get_all_game_modifiers.result"));
	for_every(gm, g_staticGameModifier->all)
	{
		if (auto* gmo = get(*gm))
		{
			if (GameDefinition::check_chosen(gmo->forGameDefinitionTagged))
			{
				result.push_back(*gm);
			}
		}
	}
	return result;
}

GameModifier const* GameModifier::get(Name const& _id)
{
	an_assert(g_staticGameModifier);
	return g_staticGameModifier->get(_id);
}

void GameModifier::apply(Tags const& _gameModifers)
{
	auto& gs = TeaForGodEmperor::GameSettings::get();

	// default settings
	GameDefinition::setup_difficulty_using_chosen();

	gs.experienceModifier = EnergyCoef::zero();

	_gameModifers.do_for_every_tag([&gs](Tag const& _tag)
		{
			if (auto* gm = GameModifier::get(_tag.get_tag()))
			{
				gm->get_difficulty_modifier().apply_to(gs.difficulty);
				gs.experienceModifier += gm->get_experience_modifier();
			}
		});

	gs.experienceModifier = max(gs.experienceModifier, -EnergyCoef::one() + EnergyCoef(0.05f));
}

GameModifier::GameModifier(Framework::Library* _library, Framework::LibraryName const& _name)
: base(_library, _name)
{
	g_staticGameModifier->add(this);
}

GameModifier::~GameModifier()
{
	if (g_staticGameModifier)
	{
		g_staticGameModifier->remove(this);
	}
}

bool GameModifier::load_from_xml(IO::XML::Node const* _node, Framework::LibraryLoadingContext& _lc)
{
	bool result = true;

	id = _node->get_name_attribute(TXT("id"), id);
	error_loading_xml_on_assert(id.is_valid(), result, _node, TXT("no \"id\" defined"));
	
	forGameDefinitionTagged.load_from_xml_attribute_or_child_node(_node, TXT("forGameDefinitionTagged"));

	if (auto* attr = _node->get_attribute(TXT("at")))
	{
		at.load_from_string(attr->get_as_string());
	}
	
	experienceModifier = EnergyCoef::load_from_attribute_or_from_child(_node, TXT("experienceModifier"), experienceModifier);

	{
		Tags gmTags;
		gmTags.load_from_xml(_node, TXT("disabledWhen"));
		disabledWhen.clear();
		gmTags.do_for_every_tag([this](Tag const& _tag)
		{
				disabledWhen.push_back_unique(_tag.get_tag());
		});
	}

	result &= texturePartOn.load_from_xml(_node, TXT("texturePartOn"), _lc);
	result &= texturePartOff.load_from_xml(_node, TXT("texturePartOff"), _lc);

	result &= difficultyModifier.load_from_xml(_node);
	disallowCrouch = _node->get_bool_attribute_or_from_child_presence(TXT("disallowCrouch"), disallowCrouch);

	lsTitle.load_from_xml_child(_node, TXT("title"), _lc, TXT("title"));
	lsDesc.load_from_xml_child(_node, TXT("desc"), _lc, TXT("desc"));

	return result;
}

bool GameModifier::prepare_for_game(Framework::Library* _library, Framework::LibraryPrepareContext& _pfgContext)
{
	bool result = true;

	result &= texturePartOn.prepare_for_game_find(_library, _pfgContext, Framework::LibraryPrepareLevel::Resolve);
	result &= texturePartOff.prepare_for_game_find(_library, _pfgContext, Framework::LibraryPrepareLevel::Resolve);

	//

	IF_PREPARE_LEVEL(_pfgContext, Framework::LibraryPrepareLevel::Resolve)
	{
		g_staticGameModifier->update();
	}

	return result;
}
