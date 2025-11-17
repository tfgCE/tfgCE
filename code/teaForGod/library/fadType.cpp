#include "fadType.h"

#include "library.h"

#include "..\game\game.h"

#include "..\..\framework\ai\aiMindInstance.h"
#include "..\..\framework\module\moduleAI.h"
#include "..\..\framework\module\moduleAppearance.h"
#include "..\..\framework\object\actor.h"

#ifdef AN_CLANG
#include "..\..\framework\library\usedLibraryStored.inl"
#endif

using namespace TeaForGodEmperor;

//

DEFINE_STATIC_NAME(fadHand);

//

REGISTER_FOR_FAST_CAST(FADType);
LIBRARY_STORED_DEFINE_TYPE(FADType, fadType);

FADType::FADType(Framework::Library * _library, Framework::LibraryName const & _name)
: base(_library, _name)
{
}

FADType::~FADType()
{
}

bool FADType::load_from_xml(IO::XML::Node const * _node, Framework::LibraryLoadingContext & _lc)
{
	bool result = base::load_from_xml(_node, _lc);

	uiName.load_from_xml_child(_node, TXT("uiName"), _lc, TXT("ui name"));
	uiDesc.load_from_xml_child(_node, TXT("uiDesc"), _lc, TXT("ui desc"));

	id = _node->get_name_attribute_or_from_child(TXT("id"), id);
	error_loading_xml_on_assert(id.is_valid(), result, _node, TXT("\"id\" is required for FADtype"));
	priority = _node->get_int_attribute_or_from_child(TXT("priority"), priority);

	result &= actorType.load_from_xml(_node, TXT("actorType"), _lc);
	result &= aiMind.load_from_xml(_node, TXT("aiMind"), _lc);

	return result;
}

bool FADType::prepare_for_game(Framework::Library* _library, Framework::LibraryPrepareContext& _pfgContext)
{
	bool result = base::prepare_for_game(_library, _pfgContext);
	
	result &= actorType.prepare_for_game_find(_library, _pfgContext, Framework::LibraryPrepareLevel::Resolve);
	result &= aiMind.prepare_for_game_find(_library, _pfgContext, Framework::LibraryPrepareLevel::Resolve);

	return result;
}

FADType const * FADType::find(Name const & _id)
{
	FADType const * best = nullptr;
	Library::get_current_as<Library>()->get_fad_types().do_for_every(
		[&best, _id](Framework::LibraryStored * _libraryStored)
		{
			if (auto const * fadType = fast_cast<FADType>(_libraryStored))
			{
				if (fadType->id == _id)
				{
					if (!best || best->priority < fadType->priority)
					{
						best = fadType;
					}
				}
			}
		});
	return best;
}

Framework::Actor * FADType::create_actor(Framework::Actor* _owner, Hand::Type _hand) const
{
	// use owner's activation group only if available, otherwise we will be depending on game's activation group
	auto* ag = _owner->get_activation_group();
	if (ag)
	{
		Game::get()->push_activation_group(ag);
	}

	Random::Generator rg = _owner->get_individual_random_generator();

	Framework::Object* fadActor = nullptr;
	if (auto* fadActorType = actorType.get())
	{
		Game::get_as<Game>()->perform_sync_world_job(TXT("create type"), [this, &fadActor, fadActorType, _owner]()
		{
			fadActor = fadActorType->create(id.to_string());
			fadActor->init(_owner->get_in_sub_world());
		});
		fadActor->set_instigator(_owner);
		fadActor->access_individual_random_generator() = rg.spawn(); // this means that we will have same random generator for every fad for same owner
		fadActor->access_variables().set_from(get_custom_parameters()); // copy params to actor to allow overriding some default parameters
		fadActor->access_variables().access<int>(NAME(fadHand)) = _hand;
		fadActor->initialise_modules([this](Framework::Module* _module)
		{
			if (aiMind.is_set())
			{
				if (auto * moduleAI = fast_cast<Framework::ModuleAI>(_module))
				{
					if (auto* mindInstance = moduleAI->get_mind())
					{
						mindInstance->use_mind(aiMind.get());
					}
				}
			}
		});
		if (auto* appearance = fadActor->get_appearance())
		{
			if (!appearance->does_use_precise_collision_bounding_box_for_frustum_check())
			{
				error(TXT("FAD requires to have precise collision bounding box for frustum check set with proper precise collision"));
			}
		}
	}

	if (ag)
	{
		Game::get()->pop_activation_group(ag);
	}

	an_assert(!fadActor || fast_cast<Framework::Actor>(fadActor));
	return fast_cast<Framework::Actor>(fadActor);
}
