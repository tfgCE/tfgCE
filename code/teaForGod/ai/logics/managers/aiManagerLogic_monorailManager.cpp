#include "aiManagerLogic_monorailManager.h"

#include "..\..\..\game\game.h"
#include "..\..\..\modules\moduleMovementMonorailCart.h"

#include "..\..\..\..\core\random\randomUtils.h"

#include "..\..\..\..\framework\ai\aiMindInstance.h"
#include "..\..\..\..\framework\library\library.h"
#include "..\..\..\..\framework\library\usedLibraryStored.inl"
#include "..\..\..\..\framework\module\moduleAppearance.h"
#include "..\..\..\..\framework\modulesOwner\modulesOwner.h"
#include "..\..\..\..\framework\world\room.h"

//

#ifdef AN_ALLOW_OPTIMIZE_OFF
#pragma optimize("", off)
#endif

#define TELEPORT_PRIORITY_INIITAL 0
#define TELEPORT_PRIORITY_OFF_TRACK 10
#define TELEPORT_PRIORITY_ON_TRACK 20

//

using namespace TeaForGodEmperor;
using namespace AI;
using namespace Logics;
using namespace Managers;

//

// parameters
DEFINE_STATIC_NAME(inRoom);

//

REGISTER_FOR_FAST_CAST(MonorailManager);

MonorailManager::MonorailManager(::Framework::AI::MindInstance* _mind, ::Framework::AI::LogicData const * _logicData)
: base(_mind, _logicData, execute_logic)
, monorailManagerData(fast_cast<MonorailManagerData>(_logicData))
{
	mark_requires_ready_to_activate();

	random = _mind->get_owner_as_modules_owner()->get_individual_random_generator();
	random.advance_seed(2397, 9752904);

#ifdef LOG_MONORAIL_MANAGER
	output(TXT("[monorail manager] on"));
#endif
}

MonorailManager::~MonorailManager()
{
#ifdef LOG_MONORAIL_MANAGER
	output(TXT("[monorail manager] off"));
#endif
}

LATENT_FUNCTION(MonorailManager::execute_logic)
{
	LATENT_SCOPE();

	LATENT_PARAM(::Framework::AI::MindInstance*, mind); // auto
	LATENT_PARAM(::Framework::AI::Logic*, logic); // auto

	LATENT_END_PARAMS();

	auto * self = fast_cast<MonorailManager>(logic);
	auto * imo = mind->get_owner_as_modules_owner();

	LATENT_BEGIN_CODE();

	self->started = false;

	// get information where it does work
	self->inRoom = imo->get_variables().get_value<SafePtr<Framework::Room>>(NAME(inRoom), SafePtr<Framework::Room>());

	ai_log_colour(self, Colour::blue);
	ai_log(self, TXT("random \"%S\""), self->random.get_seed_string().to_char());
	ai_log_no_colour(self);

	self->started = true;

#ifdef LOG_MONORAIL_MANAGER
	output(TXT("[monorail manager] track length %.3f"), self->track.length);
#endif

	if (self->track.length == 0.0f)
	{
		self->find_track();
	}

	if (self->cartTypes.is_empty())
	{
		self->find_cart_types();
	}

	// spawn trains
	self->spawningCarts = true;
	while (self->spawningCarts)
	{
		if (self->cartDOCs.is_empty())
		{
			if (self->totalCartLength > self->track.length * (1.0f + self->monorailManagerData->extraAmountPt) ||
				self->cartTypes.is_empty())
			{
#ifdef LOG_MONORAIL_MANAGER
				output(TXT("[monorail manager] spawned carts for %.3f which is enough"), self->totalCartLength);
#endif
				self->spawningCarts = false; // end
			}
			else
			{
				// add a few carts to spawn
				for_count(int, idx, self->random.get_int_from_range(3, 7))
				{
					auto* doc = self->create_cart_doc(imo);

					doc->process(Game::get(), true);
					self->cartDOCs.push_back(RefCountObjectPtr<Framework::DelayedObjectCreation>(doc));

					Game::get()->queue_delayed_object_creation(doc);

#ifdef LOG_MONORAIL_MANAGER
					output(TXT("[monorail manager] queued cart creation"));
#endif
				}
			}
		}
		else
		{
			while (!self->cartDOCs.is_empty())
			{
				auto* doc = self->cartDOCs.get_first().get();
				if (doc->is_done())
				{
					self->handle_cart_doc(doc);
					self->cartDOCs.pop_front();
				}
				else
				{
					break;
				}
			}
		}
		LATENT_WAIT(Random::get_float(0.1f, 0.5f));
	}

	while (true)
	{
		LATENT_WAIT(Random::get_float(0.1f, 0.5f));
	}

	LATENT_ON_BREAK();
	//

	LATENT_ON_END();
	//

	LATENT_END_CODE();
	LATENT_RETURN();
}

void MonorailManager::ready_to_activate()
{
	base::ready_to_activate();

	if (auto* sample = monorailManagerData->sound.get())
	{
		sample->load_on_demand_if_required();
	}

	inRoom = get_mind()->get_owner_as_modules_owner()->get_variables().get_value<SafePtr<Framework::Room>>(NAME(inRoom), SafePtr<Framework::Room>());

	find_track();

	find_cart_types();

	if (!cartTypes.is_empty())
	{
		auto* imo = get_mind()->get_owner_as_modules_owner();
		if (imo && inRoom.get() && track.length != 0.0f)
		{
			while (totalCartLength <= track.length * (1.0f + monorailManagerData->extraAmountPt) &&
				   ! cartTypes.is_empty())
			{
				// add a few carts to spawn
				RefCountObjectPtr<Framework::DelayedObjectCreation> doc;
				doc = create_cart_doc(imo);
				doc->process(Game::get(), true);
				int cartsCount = carts.get_size();
				handle_cart_doc(doc.get());
				if (cartsCount == carts.get_size())
				{
					// something went wrong, continue with ordinary path
					break;
				}
			}

			// add as many as possible
			{
				Vector3 dir = (track.endWS - track.startWS).normal();
				Vector3 velocity = dir * monorailManagerData->track.speed;
				Vector3 furthestWS = track.endWS;
				while (true)
				{	// add new one at the end
					float currentLength = (furthestWS - track.endWS).length();
					if (currentLength >= track.length)
					{
						// enough
						break;
					}
					else
					{
						int last = NONE;
						{
							int highestId = 0;
							for_every(c, carts)
							{
								if (c->id.is_set())
								{
									if (last == NONE ||
										c->id.get() > highestId)
									{
										highestId = c->id.get();
										last = for_everys_index(c);
									}
								}
							}
						}
						ARRAY_STACK(int, available, carts.get_size());
						for_every(c, carts)
						{
							if (!c->id.is_set())
							{
								// no world active check here, we're sure they're not 
								available.push_back(for_everys_index(c));
							}
						}
						if (!available.is_empty())
						{
#ifdef LOG_MONORAIL_MANAGER
							output(TXT("[monorail manager] add one at the end (current length %.3f < track length %.3f)"), currentLength, track.length);
#endif

							int idx = available[random.get_int(available.get_size())];
							auto& c = carts[idx];
							if (last == NONE)
							{
								c.id = 0;
							}
							else
							{
								c.id = carts[last].id.get() + 1;
							}
							if (auto* mc = fast_cast<TeaForGodEmperor::ModuleMovementMonorailCart>(c.cart->get_movement()))
							{
								if (last == NONE)
								{
									mc->decouple();
								}
								else
								{
									mc->couple_forward_to(carts[last].cart.get(), monorailManagerData->carts.connectorForwardSocket, monorailManagerData->carts.connectorBackSocket);
								}
							}
							Transform placement = prepare_placement(c, furthestWS);
							c.cart->get_presence()->request_teleport_within_room(placement, Framework::ModulePresence::TeleportRequestParams().silent_teleport().with_teleport_priority(TELEPORT_PRIORITY_ON_TRACK).be_visible());
							c.cart->get_presence()->set_velocity_linear(velocity);
							furthestWS = placement.location_to_world(c.connectorBackOS);
						}
						else
						{
							break;
						}
					}
				}
			}
		}
	}
}

Framework::DelayedObjectCreation* MonorailManager::create_cart_doc(Framework::IModulesOwner* imo)
{
	Framework::DelayedObjectCreation* doc = new Framework::DelayedObjectCreation();
	if (monorailManagerData->allowSpawnImmediatelyWhenRoomVisible)
	{
		doc->priority = 1000;
	}
	doc->activateImmediatelyEvenIfRoomVisible = true;
	doc->wmpOwnerObject = imo;
	doc->inRoom = inRoom.get();
	doc->name = TXT("monorail cart");
	{
		int idx = RandomUtils::ChooseFromContainer<Array<CartType>, CartType>::choose(
			random, cartTypes, [](CartType const& _ct) { return _ct.types.is_empty() ? 0.0f : _ct.probCoef; });
		auto& ct = cartTypes[idx].types;
		doc->objectType = fast_cast<Framework::ObjectType>(ct[random.get_int(ct.get_size())]);
	}
	doc->placement = Transform(track.safeWS, Quat::identity);
	doc->randomGenerator = random.spawn();

	inRoom->collect_variables(doc->variables);

	return doc;
}

void MonorailManager::handle_cart_doc(Framework::DelayedObjectCreation* doc)
{
	if (doc->is_done())
	{
		Cart newCart;
		newCart.cart = doc->createdObject.get();
		if (newCart.cart.is_set())
		{
			if (auto* ap = newCart.cart->get_appearance())
			{
				newCart.connectorForwardOS = ap->calculate_socket_os(monorailManagerData->carts.connectorForwardSocket).get_translation();
				newCart.connectorBackOS = ap->calculate_socket_os(monorailManagerData->carts.connectorBackSocket).get_translation();
			}
			if (auto* mc = fast_cast<TeaForGodEmperor::ModuleMovementMonorailCart>(newCart.cart->get_movement()))
			{
				mc->use_track(track.startWS, track.endWS);
				mc->set_speed(monorailManagerData->track.speed);
			}
			newCart.length = (newCart.connectorBackOS - newCart.connectorForwardOS).length();
			totalCartLength += newCart.length;
			newCart.id.clear();
			carts.push_back(newCart);
			// hide it
			newCart.cart->get_presence()->request_teleport_within_room(Transform(track.safeWS, Quat::identity), Framework::ModulePresence::TeleportRequestParams().silent_teleport().with_teleport_priority(TELEPORT_PRIORITY_INIITAL).be_visible(false));
#ifdef LOG_MONORAIL_MANAGER
			output(TXT("[monorail manager] spawned cart (total length %.3f)"), totalCartLength);
#endif
		}
	}
}

void MonorailManager::find_track()
{
	if (!inRoom.get())
	{
		return;
	}
	if (auto* imo = get_mind()->get_owner_as_modules_owner())
	{
		track.startWS = Vector3::zero;
		inRoom->for_every_point_of_interest(monorailManagerData->track.startPOI, [this](Framework::PointOfInterestInstance* _fpoi)
			{
				track.startWS = _fpoi->calculate_placement().get_translation();
			});
		track.endWS = Vector3::zero;
		inRoom->for_every_point_of_interest(monorailManagerData->track.endPOI, [this](Framework::PointOfInterestInstance* _fpoi)
			{
				track.endWS = _fpoi->calculate_placement().get_translation();
			});
		if (monorailManagerData->swapEndsChance > 0.0f)
		{
			auto rg = imo->get_individual_random_generator();
			rg.advance_seed(239756, 97234);
			if (rg.get_chance(monorailManagerData->swapEndsChance))
			{
				swap(track.startWS, track.endWS);
			}
		}
		track.trackWS = Segment(track.startWS, track.endWS);
		track.length = (track.startWS - track.endWS).length();

		track.safeWS = Vector3::zero;
		inRoom->for_every_point_of_interest(monorailManagerData->track.safePOI, [this](Framework::PointOfInterestInstance* _fpoi)
			{
				track.safeWS = _fpoi->calculate_placement().get_translation();
			});

		totalCartLength = 0.0f;
	}
}

void MonorailManager::find_cart_types()
{
	cartTypes.clear();
	if (inRoom.get())
	{
		for_every(ct, monorailManagerData->carts.types)
		{
			CartType c;
			c.probCoef = ct->probCoef;
			Framework::WorldSettingConditionContext wscc(inRoom.get());
			wscc.get(Framework::Library::get_current(), Framework::SceneryType::library_type(), c.types, ct->cartType);
			cartTypes.push_back(c);
		}
	}
}

void MonorailManager::advance(float _deltaTime)
{
#ifdef LOG_MONORAIL_MANAGER
	reportManagerTime -= _deltaTime;
	if (reportManagerTime < 0.0f)
	{
		while (reportManagerTime < 0.0f)
		{
			reportManagerTime += 5.0f;
		}
		float currentLength = 0.0f;
		{
			Vector3 furthestWS = track.endWS;
			int last = NONE;
			{
				int highestId = 0;
				for_every(c, carts)
				{
					if (c->id.is_set())
					{
						if (last == NONE ||
							c->id.get() > highestId)
						{
							highestId = c->id.get();
							last = for_everys_index(c);
						}
					}
				}
			}
			if (last != NONE)
			{
				auto& c = carts[last];
				furthestWS = c.cart->get_presence()->get_placement().location_to_world(c.connectorBackOS);
			}
			currentLength = (furthestWS - track.endWS).length();
		}
		output(TXT("[monorail manager] report"));
		output(TXT("[monorail manager] track %S -> %S"), track.startWS.to_string().to_char(), track.endWS.to_string().to_char());
		output(TXT("[monorail manager] current length %.3f (track %.3f)"), currentLength, track.length);
		output(TXT("[monorail manager] carts %i"), carts.get_size());
		for_every(c, carts)
		{
			Vector3 backWS = c->cart->ai_get_placement().location_to_world(c->connectorBackOS);
			bool isActive = false;
			if (auto* wo = fast_cast<Framework::WorldObject>(c->cart.get()))
			{
				isActive = wo->is_world_active();
			}
			output(TXT("[monorail manager] [%03i][%c] cart back at %S"), c->id.get(NONE), isActive? 'A' : '-', backWS.to_string().to_char());
		}
	}
#endif

	base::advance(_deltaTime);

	if (started && ! carts.is_empty())
	{
		Vector3 dir = (track.endWS - track.startWS).normal();
		Vector3 velocity = dir * monorailManagerData->track.speed;
		{	// remove destroyed
			bool allOk = false;
			while (!allOk)
			{
				allOk = true;
				for_every(c, carts)
				{
					if (!c->cart.get())
					{
						int cIdx = for_everys_index(c);
						carts.remove_at(cIdx);
						allOk = false;
						break;
					}
				}
			}
		}
		// find first (lowest id)
		int first = NONE;
		{
			int lowestId = 0;
			for_every(c, carts)
			{
				if (c->id.is_set())
				{
					if (first == NONE ||
						c->id.get() < lowestId)
					{
						lowestId = c->id.get();
						first = for_everys_index(c);
					}
				}
			}
		}
		if (first == NONE)
		{
#ifdef LOG_MONORAIL_MANAGER
			output(TXT("[monorail manager] no first cart, choose random one and place at the track's end"));
#endif
			// add first one, anyone, at end
			first = random.get_int(carts.get_size());
			auto& c = carts[first];
			c.id = 0;
			//Vector3 forwardAt = track.endWS;
			if (auto * mc = fast_cast<TeaForGodEmperor::ModuleMovementMonorailCart>(c.cart->get_movement()))
			{
				mc->decouple();
			}
			if (monorailManagerData->allowSpawnImmediatelyWhenRoomVisible)
			{
				c.cart->get_presence()->request_teleport_within_room(prepare_placement(c, track.endWS), Framework::ModulePresence::TeleportRequestParams().silent_teleport().with_teleport_priority(TELEPORT_PRIORITY_ON_TRACK).be_visible(true));
			}
			else
			{
				c.cart->get_presence()->request_teleport_within_room(prepare_placement(c, c.cart->was_recently_seen_by_player()? track.startWS : track.endWS), Framework::ModulePresence::TeleportRequestParams().silent_teleport().with_teleport_priority(TELEPORT_PRIORITY_ON_TRACK).be_visible(true));
			}
			c.cart->get_presence()->set_velocity_linear(velocity);
		}
		{	// check if first has gone pass the end ws
			auto& c = carts[first];
			Vector3 backWS = c.cart->ai_get_placement().location_to_world(c.connectorBackOS);
			if (Vector3::dot(backWS - track.endWS, track.startWS - track.endWS) < 0.0f)
			{
				c.id.clear();
				for_every(c2, carts)
				{
					if (c2->id.is_set())
					{
						c2->id = c2->id.get() - 1;
					}
				}
				first = NONE;
#ifdef LOG_MONORAIL_MANAGER
				output(TXT("[monorail manager] cart moves past the end"));
#endif
				if (auto * mc = fast_cast<TeaForGodEmperor::ModuleMovementMonorailCart>(c.cart->get_movement()))
				{
#ifdef LOG_MONORAIL_MANAGER
					output(TXT("[monorail manager] decouple"));
#endif

					mc->decouple();
					// hide it
					c.cart->get_presence()->request_teleport_within_room(Transform(track.safeWS, Quat::identity), Framework::ModulePresence::TeleportRequestParams().silent_teleport().with_teleport_priority(TELEPORT_PRIORITY_OFF_TRACK).be_visible(false));
				}
			}
		}
		if (first != NONE)
		{
			{	// add new one at the end
				float currentLength = 0.0f;
				Vector3 furthestWS = track.endWS;
				int last = NONE;
				{
					int highestId = 0;
					for_every(c, carts)
					{
						if (c->id.is_set())
						{
							if (last == NONE ||
								c->id.get() > highestId)
							{
								highestId = c->id.get();
								last = for_everys_index(c);
							}
						}
					}
				}
				{
					auto& c = carts[last];
					furthestWS = c.cart->get_presence()->get_placement().location_to_world(c.connectorBackOS);
				}
				currentLength = (furthestWS - track.endWS).length();
				if (currentLength < track.length)
				{
					ARRAY_STACK(int, available, carts.get_size());
					for_every(c, carts)
					{
						if (!c->id.is_set())
						{
							if (auto* wo = fast_cast<Framework::WorldObject>(c->cart.get()))
							{
								if (wo->is_world_active())
								{
									available.push_back(for_everys_index(c));
								}
							}
						}
					}
					if (!available.is_empty())
					{
#ifdef LOG_MONORAIL_MANAGER
						output(TXT("[monorail manager] add one at the end (current length %.3f < track length %.3f)"), currentLength, track.length);
#endif

						int idx = available[random.get_int(available.get_size())];
						auto& c = carts[idx];
						c.id = carts[last].id.get() + 1;
						if (auto * mc = fast_cast<TeaForGodEmperor::ModuleMovementMonorailCart>(c.cart->get_movement()))
						{
							mc->couple_forward_to(carts[last].cart.get(), monorailManagerData->carts.connectorForwardSocket, monorailManagerData->carts.connectorBackSocket);
						}
						c.cart->get_presence()->request_teleport_within_room(prepare_placement(c, track.startWS), Framework::ModulePresence::TeleportRequestParams().silent_teleport().with_teleport_priority(TELEPORT_PRIORITY_ON_TRACK).be_visible(true));
						c.cart->get_presence()->set_velocity_linear(velocity);
					}
					else
					{
#ifdef LOG_MONORAIL_MANAGER
						output(TXT("[monorail manager] no cart to add at the end (carts %i)"), carts.get_size());
#endif
						an_assert(spawningCarts, TXT("[monorail manager] no available cart!"));
					}
				}
			}
		}

		// that might be not required?
		for_every(c, carts)
		{
			if (c->id.is_set())
			{
				c->cart->get_presence()->set_velocity_linear(velocity);
			}
		}

		if (auto* sample = monorailManagerData->sound.get())
		{
			if (auto* imo = get_mind()->get_owner_as_modules_owner())
			{
				if (auto* r = imo->get_presence()->get_in_room())
				{
					Optional<Transform> placement;
					{
						float at = 0.5f;
						if (!soundAlreadyAtCentre)
						{
							Range atTrack = Range::empty;
							for_every(c, carts)
							{
								Vector3 frontWS = c->cart->ai_get_placement().location_to_world(c->connectorForwardOS);
								Vector3 backWS = c->cart->ai_get_placement().location_to_world(c->connectorBackOS);
								atTrack.include(track.trackWS.get_t(frontWS));
								atTrack.include(track.trackWS.get_t(backWS));
							}
							at = lerp(0.5f, atTrack.centre(), atTrack.clamp(0.5f));
						}
						if (abs(at - 0.5f) < 0.001f)
						{
							at = 0.5f;
							soundAlreadyAtCentre = true;
						}
						if (!sound.get() ||
							!sound->is_active() ||
							soundAt != at)
						{
							soundAt = at;
							placement = look_matrix(lerp(at, track.startWS, track.endWS), track.trackWS.get_start_to_end_dir(), Vector3::zAxis).to_transform(); // up dir doesn't really matter
						}
					}

					if (placement.is_set())
					{
						if (!sound.get() ||
							! sound->is_active())
						{
							sound = r->access_sounds().play(sample, r, placement.get());
						}

						if (auto* s = sound.get())
						{
							s->place(r, placement.get());
						}
					}
				}
			}
		}
	}
}

Transform MonorailManager::prepare_placement(Cart const& _for, Vector3 const& _forwardConnectorShouldBeAtWS) const
{
	Quat orientation = look_matrix33((track.endWS - track.startWS).normal(), Vector3::zAxis).to_quat();
	Vector3 connectorForwardWS = orientation.to_world(_for.connectorForwardOS);
	Vector3 beAt = _forwardConnectorShouldBeAtWS - connectorForwardWS;
	return Transform(beAt, orientation);
}

//

REGISTER_FOR_FAST_CAST(MonorailManagerData);

MonorailManagerData::MonorailManagerData()
{
}

MonorailManagerData::~MonorailManagerData()
{
}

bool MonorailManagerData::load_from_xml(IO::XML::Node const * _node, ::Framework::LibraryLoadingContext & _lc)
{
	bool result = base::load_from_xml(_node, _lc);

	allowSpawnImmediatelyWhenRoomVisible = _node->get_bool_attribute_or_from_child_presence(TXT("allowSpawnImmediatelyWhenRoomVisible"), allowSpawnImmediatelyWhenRoomVisible);
	swapEndsChance = _node->get_float_attribute_or_from_child(TXT("swapEndsChance"), swapEndsChance);
	extraAmountPt = _node->get_float_attribute_or_from_child(TXT("extraAmountPt"), extraAmountPt);
	sound.load_from_xml_child_node(_node, TXT("sound"), TXT("sample"));

	carts.types.clear();

	for_every(node, _node->children_named(TXT("carts")))
	{
		for_every(n, node->children_named(TXT("type")))
		{
			Carts::Type t;
			if (t.cartType.load_from_xml(n))
			{
				t.probCoef = n->get_float_attribute_or_from_child(TXT("probCoef"), t.probCoef);
				carts.types.push_back(t);
			}
			else
			{
				result = false;
				error_loading_xml(_node, TXT("invalid type for cart"));
			}
		}
		for_every(n, node->children_named(TXT("connectors")))
		{
			carts.connectorForwardSocket = n->get_name_attribute(TXT("forwardSocket"), carts.connectorForwardSocket);
			carts.connectorBackSocket = n->get_name_attribute(TXT("backSocket"), carts.connectorBackSocket);
		}
	}

	for_every(node, _node->children_named(TXT("track")))
	{
		track.startPOI = node->get_name_attribute(TXT("startPOI"), track.startPOI);
		track.endPOI = node->get_name_attribute(TXT("endPOI"), track.endPOI);
		track.safePOI = node->get_name_attribute(TXT("safePOI"), track.safePOI);
		track.speed = node->get_float_attribute(TXT("speed"), track.speed);
	}

	error_loading_xml_on_assert(!carts.types.is_empty(), result, _node, TXT("no types for carts provided"));

	return result;
}

bool MonorailManagerData::prepare_for_game(::Framework::Library* _library, ::Framework::LibraryPrepareContext& _pfgContext)
{
	bool result = base::prepare_for_game(_library, _pfgContext);

	result &= sound.prepare_for_game_find(_library, _pfgContext, Framework::LibraryPrepareLevel::Resolve);

	return result;
}
