#include "aiManagerLogic_airshipProxiesManager.h"

#include "aiManagerLogic_airshipsManager.h"

#include "..\..\..\game\game.h"
#include "..\..\..\modules\moduleMovementAirshipProxy.h"

#include "..\..\..\..\framework\ai\aiMindInstance.h"
#include "..\..\..\..\framework\library\library.h"
#include "..\..\..\..\framework\module\moduleAI.h"
#include "..\..\..\..\framework\module\moduleAppearance.h"
#include "..\..\..\..\framework\modulesOwner\modulesOwner.h"
#include "..\..\..\..\framework\world\room.h"

//

#ifdef AN_ALLOW_OPTIMIZE_OFF
#pragma optimize("", off)
#endif

//

using namespace TeaForGodEmperor;
using namespace AI;
using namespace Logics;
using namespace Managers;

//

// parameters
DEFINE_STATIC_NAME(inRoom);
DEFINE_STATIC_NAME(airshipProxySceneryType);
DEFINE_STATIC_NAME(autoMeshGenerate);

//

REGISTER_FOR_FAST_CAST(AirshipProxiesManager);

AirshipProxiesManager::AirshipProxiesManager(::Framework::AI::MindInstance* _mind, ::Framework::AI::LogicData const * _logicData)
: base(_mind, _logicData, execute_logic)
, airshipProxiesManagerData(fast_cast<AirshipProxiesManagerData>(_logicData))
{
}

AirshipProxiesManager::~AirshipProxiesManager()
{
}

LATENT_FUNCTION(AirshipProxiesManager::execute_logic)
{
	LATENT_SCOPE();

	LATENT_PARAM(::Framework::AI::MindInstance*, mind); // auto
	LATENT_PARAM(::Framework::AI::Logic*, logic); // auto

	LATENT_END_PARAMS();

	LATENT_VAR(SafePtr<Framework::IModulesOwner>, airshipManagerIMO);
	LATENT_VAR(bool, skipBeingProxy);

	auto * self = fast_cast<AirshipProxiesManager>(logic);
	auto * imo = mind->get_owner_as_modules_owner();

	LATENT_BEGIN_CODE();

	// wait a bit to make sure POIs are there
	LATENT_WAIT(1.0f);

	// get information where it does work
	self->inRoom = imo->get_variables().get_value<SafePtr<Framework::Room>>(NAME(inRoom), SafePtr<Framework::Room>());

	skipBeingProxy = false;
	if (auto* r = self->inRoom.get())
	{
		if (!self->airshipProxiesManagerData->roomTagged.check(r->get_tags()))
		{
			skipBeingProxy = true;
		}
	}

	if (skipBeingProxy)
	{
		LATENT_WAIT(Random::get_float(1.0f, 5.0f));
	}
	else
	{
		if (auto* piow = PilgrimageInstanceOpenWorld::get())
		{
			self->mapCell = piow->find_cell_at(self->inRoom.get()).get(VectorInt2::zero);
		}

		// find airship manager in utility rooms
		{
			PilgrimageInstance::for_our_utility_rooms(imo,
				[&airshipManagerIMO](Framework::Room* _room)
				{
					if (!airshipManagerIMO.is_set())
					{
						for_every_ptr(object, _room->get_objects())
						{
							if (auto* ai = object->get_ai())
							{
								if (auto* m = ai->get_mind())
								{
									if (auto* aml = fast_cast<AirshipsManager>(m->get_logic()))
									{
										airshipManagerIMO = object;
										break;
									}
								}
							}
						}
					}
				});
		}

		while (true)
		{
			if (auto* doc = self->spawningAirshipDOC.get())
			{
				MEASURE_PERFORMANCE(airshipProxiesManager_handleDoneDOC);
				if (doc->is_done())
				{
					Airship airship;
					airship.airship = doc->createdObject.get();
					if (airship.airship.is_set())
					{
						if (auto* map = fast_cast<ModuleMovementAirshipProxy>(airship.airship->get_movement()))
						{
							if (auto* airshipsManager = Framework::AI::Logic::get_logic_as<AirshipsManager>(airshipManagerIMO.get()))
							{
								if (auto* airshipSource = airshipsManager->get_airship(self->airships.get_size()))
								{
									map->set_movement_source(airshipSource);
								}
							}
						}
						self->airships.push_back(airship);
					}
					self->spawningAirshipDOC.clear();
				}
			}
			if (!self->spawningAirshipDOC.is_set())
			{
				scoped_call_stack_info_str_printf(TXT("check for more airships (imo o%p)"), airshipManagerIMO.get());
				if (auto* airshipsManager = Framework::AI::Logic::get_logic_as<AirshipsManager>(airshipManagerIMO.get()))
				{
					scoped_call_stack_info_str_printf(TXT("check for more airships (logic l%p)"), airshipsManager);
					scoped_call_stack_info_str_printf(TXT("current count: %i"), self->airships.get_size());
					if (auto* airship = airshipsManager->get_airship(self->airships.get_size()))
					{
						MEASURE_PERFORMANCE(airshipProxiesManager_DOCForNextAirship);
						auto* usingMesh = airship->get_appearance()->get_mesh();
						Framework::ObjectType const * proxyType = nullptr;
						if (auto* ao = airship->get_as_object())
						{
							if (auto* airshipType = ao->get_object_type())
							{
								if (airshipType != self->cachedProxyTypeFor)
								{
									self->cachedProxyType = nullptr;
									self->cachedProxyTypeFor = airshipType;

									if (auto * proxyLibName =  airshipType->get_custom_parameters().get_existing<Framework::LibraryName>(NAME(airshipProxySceneryType)))
									{
										self->cachedProxyType = Framework::Library::get_current()->get_scenery_types().find_may_fail(*proxyLibName);
									}
								}
								proxyType = self->cachedProxyType;
							}
						}
						if (proxyType)
						{
							Framework::DelayedObjectCreation* doc = new Framework::DelayedObjectCreation();
							doc->activateImmediatelyEvenIfRoomVisible = true;
							doc->wmpOwnerObject = imo;
							doc->inRoom = self->inRoom.get();
							doc->name = TXT("airship proxy");
							doc->objectType = cast_to_nonconst(proxyType);
							doc->placement = Transform(Vector3(0.0f, 0.0f, -1000.0f), Quat::identity); // well below the ground, will be teleported to appropriate location
							doc->randomGenerator = airship->get_individual_random_generator();
							doc->priority = 100;
							doc->checkSpaceBlockers = false;
							if (usingMesh)
							{
								doc->pre_setup_module_function = [usingMesh](Framework::Module* _module)
								{
									if (auto* a = fast_cast<Framework::ModuleAppearance>(_module))
									{
										// use provided mesh
										a->get_owner()->access_variables().access<bool>(NAME(autoMeshGenerate)) = false;
										a->use_mesh(usingMesh);
									}
								};
							}

							self->spawningAirshipDOC = doc;

							Game::get()->queue_delayed_object_creation(doc);
						}
					}
				}
			}
			LATENT_WAIT(Random::get_float(0.1f, 0.5f));
		}
	}

	LATENT_ON_BREAK();
	//

	LATENT_ON_END();
	//

	LATENT_END_CODE();
	LATENT_RETURN();
}

void AirshipProxiesManager::advance(float _deltaTime)
{
	base::advance(_deltaTime);
}

//

REGISTER_FOR_FAST_CAST(AirshipProxiesManagerData);

AirshipProxiesManagerData::AirshipProxiesManagerData()
{
}

AirshipProxiesManagerData::~AirshipProxiesManagerData()
{
}

bool AirshipProxiesManagerData::load_from_xml(IO::XML::Node const * _node, ::Framework::LibraryLoadingContext & _lc)
{
	bool result = base::load_from_xml(_node, _lc);

	roomTagged.load_from_xml_attribute_or_child_node(_node, TXT("roomTagged"));

	return result;
}

bool AirshipProxiesManagerData::prepare_for_game(::Framework::Library* _library, ::Framework::LibraryPrepareContext& _pfgContext)
{
	bool result = base::prepare_for_game(_library, _pfgContext);

	return result;
}
