#include "aiLogic_lockerBox.h"

#include "..\..\game\game.h"
#include "..\..\modules\gameplay\modulePilgrim.h"
#include "..\..\modules\custom\mc_emissiveControl.h"
#include "..\..\modules\custom\mc_hitIndicator.h"
#include "..\..\modules\custom\mc_itemHolder.h"
#include "..\..\pilgrimage\pilgrimageInstanceOpenWorld.h"

#include "..\..\..\core\system\video\video3dPrimitives.h"

#include "..\..\..\framework\ai\aiMindInstance.h"
#include "..\..\..\framework\game\delayedObjectCreation.h"
#include "..\..\..\framework\game\gameUtils.h"
#include "..\..\..\framework\gameScript\gameScriptExecution.h"
#include "..\..\..\framework\library\library.h"
#include "..\..\..\framework\library\usedLibraryStored.inl"
#include "..\..\..\framework\module\moduleMovementSwitch.h"
#include "..\..\..\framework\module\modulePresence.h"
#include "..\..\..\framework\module\moduleSound.h"
#include "..\..\..\framework\modulesOwner\modulesOwnerImpl.inl"
#include "..\..\..\framework\object\actor.h"
#include "..\..\..\framework\world\world.h"

//

#ifdef AN_ALLOW_OPTIMIZE_OFF
#pragma optimize("", off)
#endif

//

using namespace TeaForGodEmperor;
using namespace AI;
using namespace Logics;

//

// variables
DEFINE_STATIC_NAME(wallBoxDoorOpen);
DEFINE_STATIC_NAME(wallBoxDoorOpenActual);
DEFINE_STATIC_NAME(dataCoreOut);
DEFINE_STATIC_NAME(dataCoreOutActual);
DEFINE_STATIC_NAME(lockerBoxContent);

//

REGISTER_FOR_FAST_CAST(LockerBox);

LockerBox::LockerBox(::Framework::AI::MindInstance* _mind, ::Framework::AI::LogicData const * _logicData)
: base(_mind, _logicData, execute_logic)
{
	lockerBoxData = fast_cast<LockerBoxData>(_logicData);

	openVar.set_name(NAME(wallBoxDoorOpen));
	coreOutVar.set_name(NAME(dataCoreOut));
	openActualVar.set_name(NAME(wallBoxDoorOpenActual));
	coreOutActualVar.set_name(NAME(dataCoreOutActual));
}

LockerBox::~LockerBox()
{
}

void LockerBox::advance(float _deltaTime)
{
	base::advance(_deltaTime);

	timeToCheckPlayer -= _deltaTime;

	if (timeToCheckPlayer < 0.0f)
	{
		timeToCheckPlayer = Random::get_float(0.1f, 0.3f);

		bool isPlayerInNow = false;

		{
			auto* imo = get_mind()->get_owner_as_modules_owner();
			for_every_ptr(object, imo->get_presence()->get_in_room()->get_objects())
			{
				if (object->get_gameplay_as<ModulePilgrim>())
				{
					isPlayerInNow = true;
					break;
				}
			}
		}

		if (isPlayerInNow ^ playerIsIn)
		{
			playerIsIn = isPlayerInNow;
		}
	}
}

void LockerBox::learn_from(SimpleVariableStorage & _parameters)
{
	base::learn_from(_parameters);

	{
		Framework::LibraryName lockerBoxContent = _parameters.get_value< Framework::LibraryName>(NAME(lockerBoxContent), Framework::LibraryName::invalid());
		if (lockerBoxContent.is_valid())
		{
			lockerBoxContentObjectType.set_name(lockerBoxContent);
			lockerBoxContentObjectType.find(Framework::Library::get_current());
		}
	}
}

LATENT_FUNCTION(LockerBox::execute_logic)
{
	PERFORMANCE_GUARD_LIMIT(0.005f, TXT("[ai lockerBox] execute logic"));

	LATENT_SCOPE();
	LATENT_SCOPE_INFO(TXT("execute logic"));

	LATENT_PARAM(::Framework::AI::MindInstance*, mind); // auto
	LATENT_PARAM(::Framework::AI::Logic*, logic); // auto
	LATENT_END_PARAMS();

	LATENT_VAR(bool, madeKnown);
	LATENT_VAR(bool, madeVisited);
	LATENT_VAR(Optional<VectorInt2>, cellAt);

	auto * imo = mind->get_owner_as_modules_owner();
	auto * self = fast_cast<LockerBox>(logic);

	LATENT_BEGIN_CODE();

	self->depleted = false;

	ai_log(self, TXT("locker box, hello!"));

	self->openVar.look_up<float>(imo->access_variables());
	self->openActualVar.look_up<float>(imo->access_variables());
	self->coreOutVar.look_up<float>(imo->access_variables());
	self->coreOutActualVar.look_up<float>(imo->access_variables());

	if (imo)
	{
		if (auto* piow = PilgrimageInstanceOpenWorld::get())
		{
			cellAt = piow->find_cell_at(imo);
			if (cellAt.is_set())
			{
				self->depleted = piow->is_pilgrim_device_state_depleted(cellAt.get(), imo);
				ai_log(self, self->depleted? TXT("depleted") : TXT("available"));
			}
		}
	}

	if (!self->depleted &&
		self->lockerBoxContentObjectType.get())
	{
		{
			auto* inRoom = imo->get_presence()->get_in_room();

			RefCountObjectPtr<Framework::DelayedObjectCreation> doc(new Framework::DelayedObjectCreation());

			doc->inSubWorld = inRoom->get_in_sub_world();
			doc->imoOwner = imo;
			doc->inRoom = inRoom;
			doc->objectType = self->lockerBoxContentObjectType.get();
			doc->randomGenerator = imo->get_individual_random_generator();
			doc->activateImmediatelyEvenIfRoomVisible = true;
			doc->checkSpaceBlockers = false;
			doc->priority = 10; // important enough
			doc->devSkippable = false; // always create it

			self->contentDOC = doc;

			Game::get()->queue_delayed_object_creation(doc.get());
		}

		LATENT_WAIT(self->rg.get_float(0.05f, 0.1f));

		while (self->contentDOC.get())
		{
			if (auto* doc = self->contentDOC.get())
			{
				if (doc->is_done())
				{
					SafePtr<Framework::IModulesOwner> itemRef(doc->createdObject.get());
					SafePtr<Framework::IModulesOwner> imoRef(imo);
					Game::get()->add_immediate_sync_world_job(TXT("attach weapon to slot port"),
						[itemRef, imoRef]()
						{
							if (imoRef.get() && itemRef.get())
							{
								if (auto* ih = imoRef->get_custom<CustomModules::ItemHolder>())
								{
									ih->hold(itemRef.get(), true);
								}
							}
						});
					self->contentDOC.clear();
				}
			}

			LATENT_WAIT(self->rg.get_float(0.05f, 0.1f));
		}
	}

	madeKnown = false;
	madeVisited = false;

	if (auto* piow = PilgrimageInstanceOpenWorld::get())
	{
		madeVisited = piow->is_pilgrim_device_state_visited(cellAt.get(), imo);
	}

	while (true)
	{
		self->openVar.access<float>() = (!self->depleted && self->playerIsIn) || self->coreOutActualVar.get<float>() > 0.2f ? 1.0f : 0.0f;
		self->coreOutVar.access<float>() = (!self->depleted && self->playerIsIn && self->openActualVar.get<float>() > 0.8f) ? 1.0f : 0.0f;

		if (self->playerIsIn)
		{
			if (!madeVisited)
			{
				madeVisited = true;
				if (auto* piow = PilgrimageInstanceOpenWorld::get())
				{
					piow->mark_pilgrim_device_state_visited(cellAt.get(), imo);
				}
				if (auto* ms = MissionState::get_current())
				{
					ms->visited_interface_box();
				}
			}

			if (!madeKnown)
			{
				madeKnown = true;
				if (auto* piow = PilgrimageInstanceOpenWorld::get())
				{
					piow->mark_pilgrimage_device_direction_known(imo);
				}
			}
		}

		if (!self->depleted)
		{
			if (auto* ih = imo->get_custom<CustomModules::ItemHolder>())
			{
				if (!ih->get_held())
				{
					self->depleted = true;
					ih->set_locked(true);
					if (auto* piow = PilgrimageInstanceOpenWorld::get())
					{
						if (cellAt.is_set())
						{
							piow->mark_pilgrim_device_state_depleted(cellAt.get(), imo);
							piow->mark_pilgrimage_device_direction_known(imo);
						}
					}
				}
			}
		}

		LATENT_WAIT(self->rg.get_float(0.05f, 0.2f));
	}

	LATENT_ON_BREAK();
	//

	LATENT_ON_END();
	//

	LATENT_END_CODE();
	LATENT_RETURN();
}

//

REGISTER_FOR_FAST_CAST(LockerBoxData);

bool LockerBoxData::load_from_xml(IO::XML::Node const * _node, ::Framework::LibraryLoadingContext & _lc)
{
	bool result = base::load_from_xml(_node, _lc);

	return result;
}

bool LockerBoxData::prepare_for_game(::Framework::Library* _library, ::Framework::LibraryPrepareContext& _pfgContext)
{
	bool result = base::prepare_for_game(_library, _pfgContext);

	return result;
}
