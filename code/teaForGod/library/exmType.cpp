#include "exmType.h"

#include "library.h"

#include "..\game\game.h"
#include "..\game\persistence.h"

#include "..\modules\gameplay\modulePilgrim.h"
#include "..\modules\gameplay\moduleEXM.h"

#include "..\pilgrim\pilgrimOverlayInfo.h"

#include "..\tutorials\tutorialHubId.h"

#include "..\utils\variablesStringFormatterParamsProvider.h"

#include "..\..\framework\ai\aiMindInstance.h"
#include "..\..\framework\module\moduleAI.h"
#include "..\..\framework\module\moduleAppearance.h"
#include "..\..\framework\object\actor.h"

#ifdef AN_CLANG
#include "..\..\framework\library\usedLibraryStored.inl"
#endif

//

#ifdef AN_ALLOW_OPTIMIZE_OFF
#pragma optimize("", off)
#endif

//

using namespace TeaForGodEmperor;

//

DEFINE_STATIC_NAME(exmHand);

// exm ids - passive
DEFINE_STATIC_NAME_STR(exmWeaponCollector, TXT("p/wpcl"));
DEFINE_STATIC_NAME_STR(exmLongRangeAutomap, TXT("p/lram"));
DEFINE_STATIC_NAME_STR(exmEnergyDispenserDrain, TXT("p/eddr"));
DEFINE_STATIC_NAME_STR(exmUpgradeMachineRefill, TXT("p/umrf"));
DEFINE_STATIC_NAME_STR(exmEnergyDispatcher, TXT("p/engd"));
DEFINE_STATIC_NAME_STR(exmSuperHealthStorage, TXT("p/shst"));
DEFINE_STATIC_NAME_STR(exmInstantWeaponCharge, TXT("p/iwc"));
DEFINE_STATIC_NAME_STR(exmVampireFists, TXT("p/vmpf"));
DEFINE_STATIC_NAME_STR(exmBeggarFists, TXT("p/bgrf"));
DEFINE_STATIC_NAME_STR(exmProjectileEnergyAbsorber, TXT("p/peab"));
DEFINE_STATIC_NAME_STR(exmEnergyRetainer, TXT("p/enre"));
DEFINE_STATIC_NAME_STR(exmAnotherChance, TXT("p/anch"));

// exm ids - active
DEFINE_STATIC_NAME_STR(exmInvestigator, TXT("a/inst"));
DEFINE_STATIC_NAME_STR(exmReroll, TXT("a/rrll"));

// exm ids - integral
DEFINE_STATIC_NAME_STR(exmAntiMotionSickness, TXT("i/ams"));

//

Name const& EXMID::Passive::weapon_collector() { return NAME(exmWeaponCollector); }
Name const& EXMID::Passive::long_range_automap() { return NAME(exmLongRangeAutomap); }
Name const& EXMID::Passive::energy_dispenser_drain() { return NAME(exmEnergyDispenserDrain); }
Name const& EXMID::Passive::upgrade_machine_refill() { return NAME(exmUpgradeMachineRefill); }
Name const& EXMID::Passive::energy_dispatcher() { return NAME(exmEnergyDispatcher); }
Name const& EXMID::Passive::super_health_storage() { return NAME(exmSuperHealthStorage); }
Name const& EXMID::Passive::instant_weapon_charge() { return NAME(exmInstantWeaponCharge); }
Name const& EXMID::Passive::vampire_fists() { return NAME(exmVampireFists); }
Name const& EXMID::Passive::beggar_fists() { return NAME(exmBeggarFists); }
Name const& EXMID::Passive::projectile_energy_absorber() { return NAME(exmProjectileEnergyAbsorber); }
Name const& EXMID::Passive::energy_retainer() { return NAME(exmEnergyRetainer); }
Name const& EXMID::Passive::another_chance() { return NAME(exmAnotherChance); }

Name const& EXMID::Active::investigator() { return NAME(exmInvestigator); }
Name const& EXMID::Active::reroll() { return NAME(exmReroll); }

Name const& EXMID::Integral::anti_motion_sickness() { return NAME(exmAntiMotionSickness); }

//

REGISTER_FOR_FAST_CAST(EXMType);
LIBRARY_STORED_DEFINE_TYPE(EXMType, exmType);

EXMType::EXMType(Framework::Library * _library, Framework::LibraryName const & _name)
: base(_library, _name)
{
}

EXMType::~EXMType()
{
}

bool EXMType::load_from_xml(IO::XML::Node const * _node, Framework::LibraryLoadingContext & _lc)
{
	bool result = base::load_from_xml(_node, _lc);

	uiName.load_from_xml_child(_node, TXT("uiName"), _lc, TXT("ui name"));
	uiFullName.load_from_xml_child(_node, TXT("uiFullName"), _lc, TXT("ui full name"));
	uiDesc.load_from_xml_child(_node, TXT("uiDesc"), _lc, TXT("ui desc"));
	uiInputTip.load_from_xml_child(_node, TXT("uiInputTip"), _lc, TXT("ui input tip"));

	id = _node->get_name_attribute_or_from_child(TXT("id"), id);
	error_loading_xml_on_assert(id.is_valid(), result, _node, TXT("\"id\" is required for EXMtype"));

	result &= actorType.load_from_xml(_node, TXT("actorType"), _lc);
	result &= aiMind.load_from_xml(_node, TXT("aiMind"), _lc);

	isPermanent = _node->get_bool_attribute_or_from_child_presence(TXT("permanent"), isPermanent);
	isActive = _node->get_bool_attribute_or_from_child_presence(TXT("active"), isActive);
	isIntegral = _node->get_bool_attribute_or_from_child_presence(TXT("integral"), isIntegral);
	canBeActivatedAtAnyTime = _node->get_bool_attribute_or_from_child_presence(TXT("canBeActivatedAtAnyTime"), canBeActivatedAtAnyTime);
	canBeActivatedAtAnyTime = ! _node->get_bool_attribute_or_from_child_presence(TXT("cantBeActivatedAtAnyTime"), ! canBeActivatedAtAnyTime);
	
	level = _node->get_int_attribute(TXT("level"), level);

	result &= lineModel.load_from_xml(_node, TXT("lineModel"), _lc);
	result &= lineModelForDisplay.load_from_xml(_node, TXT("lineModelForDisplay"), _lc);

	permanentPrerequisites.clear();
	for_every(node, _node->children_named(TXT("permanentPrerequisite")))
	{
		Name exm = node->get_name_attribute(TXT("id"));
		if (exm.is_valid())
		{
			permanentPrerequisites.push_back(exm);
		}
	}
	permanentLimit = _node->get_int_attribute_or_from_child(TXT("permanentLimit"), permanentLimit);

	return result;
}

bool EXMType::prepare_for_game(Framework::Library* _library, Framework::LibraryPrepareContext& _pfgContext)
{
	bool result = base::prepare_for_game(_library, _pfgContext);
	
	result &= actorType.prepare_for_game_find(_library, _pfgContext, Framework::LibraryPrepareLevel::Resolve);
	result &= aiMind.prepare_for_game_find(_library, _pfgContext, Framework::LibraryPrepareLevel::Resolve);
	result &= lineModel.prepare_for_game_find(_library, _pfgContext, Framework::LibraryPrepareLevel::Resolve);
	result &= lineModelForDisplay.prepare_for_game_find(_library, _pfgContext, Framework::LibraryPrepareLevel::Resolve);

	return result;
}

Array<Name> EXMType::get_all()
{
	Array<Name> exms;
	Library::get_current_as<Library>()->get_exm_types().do_for_every(
		[&exms](Framework::LibraryStored* _libraryStored)
		{
			if (auto const* exmType = fast_cast<EXMType>(_libraryStored))
			{
				exms.push_back(exmType->id);
			}
		});
	return exms;
}

Array<Name> EXMType::get_all(TagCondition const& _tagCondition, Optional<RangeInt> const& _allowedEXMLevels)
{
	Array<Name> exms;
	Library::get_current_as<Library>()->get_exm_types().do_for_every(
		[&exms, &_tagCondition, &_allowedEXMLevels](Framework::LibraryStored* _libraryStored)
		{
			if (auto const* exmType = fast_cast<EXMType>(_libraryStored))
			{
				if (_tagCondition.check(_libraryStored->get_tags()))
				{
					if (!_allowedEXMLevels.is_set() ||
						_allowedEXMLevels.get().does_contain(exmType->get_level()))
					{
						exms.push_back(exmType->id);
					}
				}
			}
		});
	return exms;
}

Array<EXMType*> EXMType::get_all_exms(TagCondition const& _tagCondition, Optional<RangeInt> const& _allowedEXMLevels)
{
	Array<EXMType*> exms;
	Library::get_current_as<Library>()->get_exm_types().do_for_every(
		[&exms, &_tagCondition, &_allowedEXMLevels](Framework::LibraryStored* _libraryStored)
		{
			if (auto * exmType = fast_cast<EXMType>(_libraryStored))
			{
				if (_tagCondition.is_empty() || _tagCondition.check(_libraryStored->get_tags()))
				{
					if (!_allowedEXMLevels.is_set() ||
						_allowedEXMLevels.get().is_empty() ||
						_allowedEXMLevels.get().does_contain(exmType->get_level()))
					{
						exms.push_back(exmType);
					}
				}
			}
		});
	return exms;
}

EXMType const* EXMType::find(Name const& _id)
{
	EXMType const* best = nullptr;
	Library::get_current_as<Library>()->get_exm_types().do_for_every(
		[&best, _id](Framework::LibraryStored* _libraryStored)
		{
			if (auto const* exmType = fast_cast<EXMType>(_libraryStored))
			{
				if (exmType->id == _id)
				{
					// priority system was here
					if (!best)
					{
						best = exmType;
					}
				}
			}
		});
	return best;
}

Framework::Actor * EXMType::create_actor(Framework::Actor* _owner, Hand::Type _hand) const
{
	// use owner's activation group only if available, otherwise we will be depending on game's activation group
	auto* ag = _owner->get_activation_group();
	if (ag)
	{
		Game::get()->push_activation_group(ag);
	}

	Random::Generator rg = _owner->get_individual_random_generator();

	Framework::Object* exmActor = nullptr;
	if (auto* exmActorType = actorType.get())
	{
		exmActorType->load_on_demand_if_required();

		Game::get_as<Game>()->perform_sync_world_job(TXT("create exm"), [this, &exmActor, exmActorType, _owner]()
		{
			exmActor = exmActorType->create(id.to_string());
			exmActor->init(_owner->get_in_sub_world());
		});
		exmActor->set_instigator(_owner);
		exmActor->access_individual_random_generator() = rg.spawn(); // this means that we will have same random generator for every exm for same owner
		exmActor->access_variables().set_from(get_custom_parameters()); // copy params to actor to allow overriding some default parameters
		exmActor->access_variables().access<int>(NAME(exmHand)) = _hand;
		exmActor->initialise_modules([this](Framework::Module* _module)
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
		if (auto* appearance = exmActor->get_appearance())
		{
			if (!appearance->does_use_precise_collision_bounding_box_for_frustum_check())
			{
				error(TXT("EXM requires to have precise collision bounding box for frustum check set with proper precise collision"));
			}
		}
		if (auto * mexm = exmActor->get_gameplay_as<ModuleEXM>())
		{
			mexm->set_hand(_hand);
			mexm->set_exm_type(this);
		}
		else
		{
			warn(TXT("exm \"%S\" does not have an EXM module"), exmActor->get_name().to_char());
		}
	}

	if (ag)
	{
		Game::get()->pop_activation_group(ag);
	}

	an_assert(!exmActor || fast_cast<Framework::Actor>(exmActor));
	return fast_cast<Framework::Actor>(exmActor);
}

void EXMType::request_input_tip(ModulePilgrim* mp, Hand::Type heldByHand) const
{
	PilgrimOverlayInfo::ShowTipParams params;
	params.with_time_to_deactivate_tip(10.0f);
	params.for_hand(heldByHand);
	if (uiInputTip.is_valid())
	{
		params.with_text(uiInputTip.get());
	}
	mp->access_overlay_info().show_tip(PilgrimOverlayInfoTip::InputActivateEXM, params);
}

String EXMType::get_ui_desc() const
{
	VariablesStringFormatterParamsProvider vsfpp(get_custom_parameters());
	return Framework::StringFormatter::format_loc_str(uiDesc, Framework::StringFormatterParams().add_params_provider(&vsfpp));
}

bool EXMType::does_player_have_equipped(Name const& _exm)
{
	todo_multiplayer_issue(TXT("we just get player here"));
	if (auto* g = Game::get_as<Game>())
	{
		if (auto* pa = g->access_player().get_actor())
		{
			if (auto* mp = pa->get_gameplay_as<ModulePilgrim>())
			{
				if (mp->has_exm_equipped(NAME(exmWeaponCollector)))
				{
					return true;
				}
			}
		}
	}
	return false;
}