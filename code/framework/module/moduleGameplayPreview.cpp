#include "moduleGameplayPreview.h"

#include "modules.h"
#include "moduleDataImpl.inl"

#include "..\game\game.h"
#include "..\library\library.h"
#include "..\library\usedLibraryStored.inl"
#include "..\modulesOwner\modulesOwner.h"
#include "..\object\actor.h"
#include "..\object\object.h"

//

#ifdef AN_ALLOW_OPTIMIZE_OFF
#pragma optimize("", off)
#endif

//

using namespace Framework;

//

DEFINE_STATIC_NAME(timeToLive);
DEFINE_STATIC_NAME(startingScale);
DEFINE_STATIC_NAME(scaleBlendTime);
DEFINE_STATIC_NAME(pulseScale);
DEFINE_STATIC_NAME(pulseScalePeriod);

//

REGISTER_FOR_FAST_CAST(ModuleGameplayPreview);

#ifdef AN_DEVELOPMENT
ArrayStatic<ModuleGameplayPreviewControllerTransform, 256> storedControllerTransforms;
#endif

void store_controller_transform(Name const& _id, Transform const& _transform)
{
#ifdef AN_DEVELOPMENT
	for_every(ct, storedControllerTransforms)
	{
		if (ct->id == _id)
		{
			ct->placement = _transform;
			return;
		}
	}
	ModuleGameplayPreviewControllerTransform ct;
	ct.id = _id;
	ct.placement = _transform;
	storedControllerTransforms.push_back(ct);
#endif
}

Optional<Transform> restore_controller_transform(Name const& _id)
{
#ifdef AN_DEVELOPMENT
	for_every(ct, storedControllerTransforms)
	{
		if (ct->id == _id)
		{
			return ct->placement;
		}
	}
#endif
	return NP;
}

static ModuleGameplay* create_module(IModulesOwner* _owner)
{
	return new ModuleGameplayPreview(_owner);
}

static Framework::ModuleData* create_module_data(::Framework::LibraryStored* _inLibraryStored)
{
	return new ModuleGameplayPreviewData(_inLibraryStored);
}

RegisteredModule<ModuleGameplay> & ModuleGameplayPreview::register_itself()
{
	return Modules::gameplay.register_module(String(TXT("preview")), create_module, create_module_data);
}

ModuleGameplayPreview::ModuleGameplayPreview(IModulesOwner* _owner)
: base(_owner)
{
}

ModuleGameplayPreview::~ModuleGameplayPreview()
{
	for_every(at, attached)
	{
		if (auto* imo = at->attached.get())
		{
			imo->be_autonomous();
			imo->cease_to_exist(true);
		}
	}
}

void ModuleGameplayPreview::setup_with(ModuleData const * _moduleData)
{
	base::setup_with(_moduleData);
	moduleGameplayPreviewData = fast_cast<ModuleGameplayPreviewData>(_moduleData);
	if (moduleGameplayPreviewData)
	{
		controllerTransforms = moduleGameplayPreviewData->controllerTransforms;
		for_every(ct, controllerTransforms)
		{
			ct->placement = restore_controller_transform(ct->id).get(ct->placement);
		}
	}
}

void ModuleGameplayPreview::initialise()
{
	base::initialise();

	if (moduleGameplayPreviewData)
	{
		if (auto* ownerAsObject = get_owner_as_object())
		{
			Game::get()->push_activation_group(ownerAsObject->get_activation_group());
			Random::Generator rg = ownerAsObject->get_individual_random_generator();

			for_every(at, moduleGameplayPreviewData->attaches)
			{
				if (at->checkVar.is_valid())
				{
					if (!ownerAsObject->get_value<bool>(at->checkVar, false))
					{
						continue;
					}
				}
				Attached toBeAttached;
				toBeAttached.id = at->id;
				IModulesOwner* toAttach = nullptr;
				if (auto* actorType = at->actorType.get())
				{
					actorType->load_on_demand_if_required();

					Game::get_as<Game>()->perform_sync_world_job(TXT("create to attach"), [&toAttach, actorType, ownerAsObject]()
					{
						toAttach = actorType->create(actorType->get_name().to_string());
						toAttach->init(ownerAsObject->get_in_sub_world());
					});
				}
				if (auto* itemType = at->itemType.get())
				{
					itemType->load_on_demand_if_required();

					Game::get_as<Game>()->perform_sync_world_job(TXT("create to attach"), [&toAttach, itemType, ownerAsObject]()
					{
						toAttach = itemType->create(itemType->get_name().to_string());
						toAttach->init(ownerAsObject->get_in_sub_world());
					});
				}
				if (toAttach)
				{
					toAttach->access_individual_random_generator() = rg.spawn();
					toAttach->access_variables().set_from(at->variables);
					if (!at->tags.is_empty())
					{
						if (auto* o = toAttach->get_as_object())
						{
							o->access_tags().set_tags_from(at->tags);
						}
					}
					for_every(cv, at->copyVariables)
					{
						if (auto* src = ownerAsObject->get_variables().find_any_existing(cv->from))
						{
							void* dest = toAttach->access_variables().access(cv->to, src->type_id());
							RegisteredType::copy(src->type_id(), dest, src->access_raw());
						}
					}
					toAttach->initialise_modules();
					toAttach->set_instigator(ownerAsObject);
					toAttach->be_non_autonomous();
					
					IModulesOwner* attachToObject = ownerAsObject;

					if (at->attachToAttachedId.is_valid())
					{
						for_every(a, attached)
						{
							if (at->attachToAttachedId == a->id)
							{
								attachToObject = a->attached.get();
							}
						}
					}

					if (auto* ap = toAttach->get_presence())
					{
						if (at->attachToSocket.is_valid())
						{
							if (at->attachViaSocket.is_valid())
							{
								ap->attach_via_socket(attachToObject, at->attachViaSocket, at->attachToSocket);
							}
							else
							{
								ap->attach_to_socket(attachToObject, at->attachToSocket);
							}
						}
						else
						{
							ap->attach_to(attachToObject);
						}
					}

					toBeAttached.attached = toAttach;
					attached.push_back(toBeAttached);
				}
			}

			Game::get()->pop_activation_group(ownerAsObject->get_activation_group());
		}
	}
}

int ModuleGameplayPreview::find_controller_transform_index(Name const& _id) const
{
	for_every(ct, controllerTransforms)
	{
		if (ct->id == _id)
		{
			return for_everys_index(ct);
		}
	}
	return NONE;
}

Name const & ModuleGameplayPreview::get_controller_transform_id(int _idx) const
{
	return controllerTransforms.is_index_valid(_idx) ? controllerTransforms[_idx].id: Name::invalid();
}

Transform const& ModuleGameplayPreview::get_controller_transform(int _idx) const
{
	return controllerTransforms.is_index_valid(_idx) ? controllerTransforms[_idx].placement : Transform::identity;
}

void ModuleGameplayPreview::set_controller_transform(int _idx, Transform const& _placement)
{
	if (controllerTransforms.is_index_valid(_idx))
	{
		auto& ct = controllerTransforms[_idx];
		ct.placement = _placement;
		store_controller_transform(ct.id, _placement);
	}
}

//

REGISTER_FOR_FAST_CAST(ModuleGameplayPreviewData);

ModuleGameplayPreviewData::ModuleGameplayPreviewData(LibraryStored* _inLibraryStored)
: base(_inLibraryStored)
{
}

bool ModuleGameplayPreviewData::read_parameter_from(IO::XML::Node const* _node, LibraryLoadingContext& _lc)
{
	bool result = true;

	if (_node->get_name() == TXT("controllerTransform"))
	{
		ModuleGameplayPreviewControllerTransform ct;
		ct.id = _node->get_name_attribute_or_from_child(TXT("id"), ct.id);
		error_loading_xml_on_assert(ct.id.is_valid(), result, _node, TXT("no \"id\" provided"));
		ct.placement.load_from_xml_child_node(_node, TXT("placement"));
		
		controllerTransforms.push_back(ct);
	}
	else if (_node->get_name() == TXT("attach"))
	{
		attaches.push_back(ModuleGameplayPreviewAttach());
		ModuleGameplayPreviewAttach &at = attaches.get_last();
		at.id = _node->get_name_attribute_or_from_child(TXT("id"), at.id);
		at.checkVar = _node->get_name_attribute(TXT("checkVar"));
		at.actorType.load_from_xml(_node, TXT("actorType"), _lc);
		at.itemType.load_from_xml(_node, TXT("itemType"), _lc);
		error_loading_xml_on_assert(at.actorType.is_name_valid() || at.itemType.is_name_valid(), result, _node, TXT("no \"actorType\" nor \"itemType\" provided"));
		at.attachToAttachedId = _node->get_name_attribute_or_from_child(TXT("attachToAttachedId"), at.attachToAttachedId);
		at.attachToSocket = _node->get_name_attribute(TXT("attachToSocket"));
		at.attachViaSocket = _node->get_name_attribute(TXT("attachViaSocket"));
		at.tags.load_from_xml_attribute_or_child_node(_node, TXT("tags"));
		at.variables.load_from_xml_child_node(_node, TXT("parameters"));
		at.variables.load_from_xml_child_node(_node, TXT("variables"));
		_lc.load_group_into(at.variables);
		for_every(n, _node->children_named(TXT("copyVariable")))
		{
			ModuleGameplayPreviewAttach::CopyVariable cv;
			cv.from = n->get_name_attribute_or_from_child(TXT("name"), cv.from);
			cv.to = n->get_name_attribute_or_from_child(TXT("name"), cv.from);
			cv.from = n->get_name_attribute_or_from_child(TXT("from"), cv.from);
			cv.to = n->get_name_attribute_or_from_child(TXT("to"), cv.to);
			cv.to = n->get_name_attribute_or_from_child(TXT("as"), cv.to);
			at.copyVariables.push_back(cv);
		}
	}
	else
	{
		result &= base::read_parameter_from(_node, _lc);
	}
	return result;
}

bool ModuleGameplayPreviewData::prepare_for_game(Library* _library, LibraryPrepareContext& _pfgContext)
{
	bool result = base::prepare_for_game(_library, _pfgContext);

	for_every(at, attaches)
	{
		result &= at->actorType.prepare_for_game_find(_library, _pfgContext, LibraryPrepareLevel::Resolve);
		result &= at->itemType.prepare_for_game_find(_library, _pfgContext, LibraryPrepareLevel::Resolve);
	}

	return result;
}

void ModuleGameplayPreviewData::prepare_to_unload()
{
	base::prepare_to_unload();

	controllerTransforms.clear();
	attaches.clear();
}
