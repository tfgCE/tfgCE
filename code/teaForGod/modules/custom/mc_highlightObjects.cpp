#include "mc_highlightObjects.h"

#include "..\gameplay\modulePilgrim.h"

#include "..\..\game\gameDirector.h"
#include "..\..\game\playerSetup.h"
#include "..\..\game\screenShotContext.h"

#include "..\..\..\framework\game\bullshotSystem.h"
#include "..\..\..\framework\library\library.h"
#include "..\..\..\framework\library\usedLibraryStored.inl"
#include "..\..\..\framework\module\moduleDataImpl.inl"
#include "..\..\..\framework\module\modules.h"
#include "..\..\..\framework\render\sceneBuildContext.h"

//

#ifdef AN_ALLOW_OPTIMIZE_OFF
#pragma optimize("", off)
#endif

//

using namespace TeaForGodEmperor;
using namespace CustomModules;

//

// params
DEFINE_STATIC_NAME(leftHandMaterial);
DEFINE_STATIC_NAME(rightHandMaterial);

// bullshot system allowances
DEFINE_STATIC_NAME_STR(bsDisableHighlights, TXT("disable highlights"));

// reasons
DEFINE_STATIC_NAME(grab);
DEFINE_STATIC_NAME(damaged);

//

REGISTER_FOR_FAST_CAST(HighlightObjects);

static Framework::ModuleCustom* create_module(Framework::IModulesOwner* _owner)
{
	return new HighlightObjects(_owner);
}

static Framework::ModuleData* create_module_data(::Framework::LibraryStored* _inLibraryStored)
{
	return new HighlightObjectsData(_inLibraryStored);
}

Framework::RegisteredModule<Framework::ModuleCustom> & HighlightObjects::register_itself()
{
	return Framework::Modules::custom.register_module(String(TXT("highlightObjects")), create_module, create_module_data);
}

HighlightObjects::HighlightObjects(Framework::IModulesOwner* _owner)
: base(_owner)
{
}

HighlightObjects::~HighlightObjects()
{
}

void HighlightObjects::setup_with(Framework::ModuleData const * _moduleData)
{
	base::setup_with(_moduleData);
	moduleData = fast_cast<HighlightObjectsData>(_moduleData);
}

void HighlightObjects::reset()
{
	base::reset();

	mark_requires(all_customs__advance_post);
}

void HighlightObjects::initialise()
{
	base::initialise();

	mark_requires(all_customs__advance_post);
}

void HighlightObjects::advance_post(float _deltaTime)
{
	// by default blend out all
	for_count(int, othIdx, MaxObjectsToHighlight)
	{
		objectsToHighlight[othIdx].target = 0.0f;
	}
	if (PlayerSetup::get_current().get_preferences().highlightInteractives &&
		(! GameDirector::get() || ! GameDirector::get()->are_weapons_blocked())
#ifdef AN_ALLOW_BULLSHOTS
		 && ! Framework::BullshotSystem::is_setting_active(NAME(bsDisableHighlights))
#endif
		)
	{
		if (auto* pilgrim = get_owner()->get_gameplay_as<ModulePilgrim>())
		{
			for_count(int, hand, Hand::MAX)
			{
				for_count(int, highlightIdx, 3)
				{
					// 0 hand equipment being put in a pocket or object to grab
					// 1 damaged weapon on the forearm
					// 2 hand to take the damaged weapon off the forearm

					Framework::IModulesOwner* highlightImo = nullptr;
					float highlightImoTarget = 0.0f;
					Name highlightReason;
					Framework::Material* highlightMaterial = nullptr;
					if (highlightIdx == 0)
					{
						if (auto* eq = pilgrim->get_hand_equipment((Hand::Type)hand))
						{
							if (pilgrim->should_put_hand_equipment_into_pocket((Hand::Type)hand))
							{
								highlightImo = eq;
								highlightImoTarget = 0.2f;
								highlightReason = NAME(grab);
							}
							else if (auto* ih = pilgrim->get_put_hand_equipment_into_item_holder((Hand::Type)hand))
							{
								highlightImo = ih;
								highlightImoTarget = 0.2f;
								highlightReason = NAME(grab);
							}
						}
						else
						{
							if (auto* imo = pilgrim->get_object_to_grab((Hand::Type)hand))
							{
								if (pilgrim->get_main_equipment((Hand::Type)hand) != imo)
								{
									highlightImo = imo;
									highlightImoTarget = 1.0f;
									highlightReason = NAME(grab);
								}
							}
						}
						highlightMaterial = moduleData->handMaterials[hand].get();
					}
					else if (highlightIdx == 1)
					{
						if (pilgrim->should_highlight_damaged_main_equipment((Hand::Type)hand))
						{
							highlightImo = pilgrim->get_main_equipment((Hand::Type)hand);
							highlightImoTarget = 1.0f;
							highlightReason = NAME(damaged);
							highlightMaterial = moduleData->damagedMaterial.get();
						}
					}
					else if (highlightIdx == 2)
					{
						if (pilgrim->should_highlight_damaged_main_equipment((Hand::Type)hand))
						{
							highlightImo = pilgrim->get_hand(Hand::other_hand((Hand::Type)hand));
							highlightImoTarget = 1.0f;
							highlightReason = NAME(damaged);
							highlightMaterial = moduleData->damagedMaterial.get();
						}
					}

					if (highlightImo && highlightMaterial)
					{
						ArrayStatic<Framework::IModulesOwner*, 6> imos;
						imos.push_back(highlightImo);
						for(int i = 0; i < imos.get_size(); ++ i)
						{
							auto* imo = imos[i];
							Concurrency::ScopedSpinLock lock(imo->get_presence()->access_attached_lock());
							for_every_ptr(attached, imo->get_presence()->get_attached())
							{
								if (imos.has_place_left())
								{
									imos.push_back(attached);
								}
								else
								{
									break;
								}
							}
						}
						for_every_ptr(imo, imos)
						{
							for_count(int, othIdx, MaxObjectsToHighlight)
							{
								auto& oth = objectsToHighlight[othIdx];
								if (oth.imo.is_set() &&
									oth.imo == imo &&
									oth.hand == hand &&
									oth.reason == highlightReason)
								{
									oth.target = highlightImoTarget;
									break;
								}
								else if (!oth.imo.is_set())
								{
									if (auto* appearance = imo->get_appearance())
									{
										if (auto* mesh = appearance->get_mesh())
										{
											if (auto* iMesh = mesh->get_mesh())
											{
												if (auto* mat = highlightMaterial)
												{
													oth.hand = (Hand::Type)hand;
													oth.imo = imo;
													oth.reason = highlightReason;
													oth.materialInstance.set_material(mat->get_material(), iMesh->get_material_usage());
													oth.state = 0.0f;
													oth.target = highlightImoTarget;
													oth.individualOffset = Random::get_float(0.0f, 1000.0f);
													if (moduleData &&
														moduleData->instanceOffsetParamName.is_valid())
													{
														oth.materialInstance.set_uniform(moduleData->instanceOffsetParamName, oth.individualOffset);
													}
													break;
												}
											}
										}
									}
								}
							}
						}
					}
				}
			}
		}
	}
	for_count(int, othIdx, MaxObjectsToHighlight)
	{
		auto & oth = objectsToHighlight[othIdx];
		oth.state = blend_to_using_speed_based_on_time(oth.state, oth.target, magic_number 0.1f, _deltaTime);
		if (oth.state == 0.0f && oth.target == 0.0f)
		{
			oth.imo.clear();
		}
		else if (moduleData)
		{
			oth.materialInstance.set_uniform(moduleData->highlightParamName, oth.state);
		}
	}
}

void HighlightObjects::setup_scene_build_context(Framework::Render::SceneBuildContext& _context)
{
	if (ScreenShotContext::get_active())
	{
		// don't render highlights on screenshots
		return;
	}
	for_count(int, othIdx, MaxObjectsToHighlight)
	{
		auto & oth = objectsToHighlight[othIdx];
		if (oth.imo.is_set())
		{
			_context.set_additional_render_of(Framework::Render::SceneBuildContext::AdditionalRenderOfParams().for_imo(oth.imo.get()).use_material(&oth.materialInstance));
		}
	}
}

//

REGISTER_FOR_FAST_CAST(HighlightObjectsData);

HighlightObjectsData::HighlightObjectsData(Framework::LibraryStored* _inLibraryStored)
: base(_inLibraryStored)
{

}

HighlightObjectsData::~HighlightObjectsData()
{
}

bool HighlightObjectsData::read_parameter_from(IO::XML::Attribute const * _attr, Framework::LibraryLoadingContext & _lc)
{
	if (_attr->get_name() == TXT("leftHandMaterial"))
	{
		bool result = true;
		result &= handMaterials[Hand::Left].load_from_xml(_attr, _lc);
		return result;
	}
	else if (_attr->get_name() == TXT("rightHandMaterial"))
	{
		bool result = true;
		result &= handMaterials[Hand::Right].load_from_xml(_attr, _lc);
		return result;
	}
	else if (_attr->get_name() == TXT("damagedMaterial"))
	{
		bool result = true;
		result &= damagedMaterial.load_from_xml(_attr, _lc);
		return result;
	}
	else if (_attr->get_name() == TXT("highlightParamName"))
	{
		highlightParamName = _attr->get_as_name();
		return true;
	}
	else if (_attr->get_name() == TXT("instanceOffsetParamName"))
	{
		instanceOffsetParamName = _attr->get_as_name();
		return true;
	}	
	else
	{
		return base::read_parameter_from(_attr, _lc);
	}
}

bool HighlightObjectsData::prepare_for_game(Framework::Library* _library, Framework::LibraryPrepareContext& _pfgContext)
{
	bool result = base::prepare_for_game(_library, _pfgContext);

	for_count(int, i, Hand::MAX)
	{
		result &= handMaterials[i].prepare_for_game_find(_library, _pfgContext, Framework::LibraryPrepareLevel::Resolve);
	}
	result &= damagedMaterial.prepare_for_game_find(_library, _pfgContext, Framework::LibraryPrepareLevel::Resolve);

	if (!highlightParamName.is_valid())
	{
		error(TXT("highlightParamName not set"));
		result = false;
	}

	return result;
}

void HighlightObjectsData::prepare_to_unload()
{
	base::prepare_to_unload();
	handMaterials[Hand::Left].clear();
	handMaterials[Hand::Right].clear();
	damagedMaterial.clear();
}
