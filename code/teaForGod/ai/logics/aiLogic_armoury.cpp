#include "aiLogic_armoury.h"

#include "..\..\modules\custom\mc_itemHolder.h"
#include "..\..\modules\gameplay\modulePilgrim.h"
#include "..\..\modules\gameplay\equipment\me_gun.h"

#include "..\..\game\game.h"
#include "..\..\game\gameDirector.h"
#include "..\..\game\persistence.h"

#include "..\..\pilgrimage\pilgrimage.h"

#include "..\..\..\framework\ai\aiMessageHandler.h"
#include "..\..\..\framework\ai\aiMindInstance.h"
#include "..\..\..\framework\library\library.h"
#include "..\..\..\framework\library\usedLibraryStored.inl"
#include "..\..\..\framework\module\modulePresence.h"
#include "..\..\..\framework\modulesOwner\modulesOwnerImpl.inl"
#include "..\..\..\framework\object\actor.h"
#include "..\..\..\framework\object\object.h"
#include "..\..\..\framework\object\sceneryType.h"
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

// room tags
DEFINE_STATIC_NAME(armouryRoom);

// door tags
DEFINE_STATIC_NAME(toNextArmoury);
DEFINE_STATIC_NAME(toPrevArmoury);

// object variables
DEFINE_STATIC_NAME(armouryId);
DEFINE_STATIC_NAME(armourySlotPortAt);
DEFINE_STATIC_NAME(armouryStartsEmpty);
DEFINE_STATIC_NAME(armouryIgnoreExistingWeapons);

// global references
DEFINE_STATIC_NAME_STR(grWeaponItemTypeToUseWithWeaponParts, TXT("weapon item type to use with weapon parts"));

// ai messages
DEFINE_STATIC_NAME_STR(aim_a_startArmoury, TXT("armoury; start"));
DEFINE_STATIC_NAME_STR(aim_a_armouryEnlarged, TXT("armoury; enlarged"));
DEFINE_STATIC_NAME_STR(aim_wm_weaponMixed, TXT("weapon mixer; weapon mixed"));
DEFINE_STATIC_NAME_STR(aim_tb_objectToDestroy, TXT("trash bin; object to destroy"));
DEFINE_STATIC_NAME_STR(aim_a_weaponBought, TXT("armoury; weapon bought"));
DEFINE_STATIC_NAME(object);
DEFINE_STATIC_NAME_STR(aim_tb_destroyObject, TXT("trash bin; destroy object"));
//

bool ArmouryPOIWall::load_from_xml(IO::XML::Node const* _node)
{
	bool result = true;

	relColumn = _node->get_int_attribute(TXT("relColumn"), relColumn);
	continuousLeftSide = _node->get_bool_attribute_or_from_child_presence(TXT("continuousLeftSide"), continuousLeftSide);
	continuousRightSide = _node->get_bool_attribute_or_from_child_presence(TXT("continuousRightSide"), continuousRightSide);
	leftBottomPOI = _node->get_name_attribute(TXT("leftBottomPOI"), leftBottomPOI);
	leftTopPOI = _node->get_name_attribute(TXT("leftTopPOI"), leftTopPOI);
	rightBottomPOI = _node->get_name_attribute(TXT("rightBottomPOI"), rightBottomPOI);
	rightTopPOI = _node->get_name_attribute(TXT("rightTopPOI"), rightTopPOI);

	return result;
}

//

REGISTER_FOR_FAST_CAST(Armoury);

Armoury::Armoury(::Framework::AI::MindInstance* _mind, ::Framework::AI::LogicData const * _logicData)
: base(_mind, _logicData, execute_logic)
, armouryData(fast_cast<ArmouryData>(_logicData))
{
}

Armoury::~Armoury()
{
}

void Armoury::advance(float _deltaTime)
{
	base::advance(_deltaTime);

	if (! destroyWeapon.is_empty() && !armouryInaccessible)
	{
		auto& wimoPtr = destroyWeapon.get_first();
		if (auto* wimo = wimoPtr.get())
		{
			for_every(w, weapons)
			{
				if (w->imo == wimo)
				{
					ai_log(this, TXT("process destroy object/weapon o%p"), wimo);
					// found, remove from presence and confirm destroy
					{
						auto& p = Persistence::access_current();
						p.remove_weapon_from_armoury(w->armouryId);
					}
					weapons.remove_at(for_everys_index(w));
					{
						auto* selfImo = get_mind()->get_owner_as_modules_owner();
						an_assert(selfImo);
						if (auto* message = selfImo->get_in_world()->create_ai_message(NAME(aim_tb_destroyObject)))
						{
							message->to_sub_world(selfImo->get_in_sub_world());
							message->access_param(NAME(object)).access_as<SafePtr<Framework::IModulesOwner>>() = wimo;
						}
					}

					break;
				}
			}
		}
		destroyWeapon.pop_front();

		updatePersistence = UpdatePersistence::Explicit;
	}
}

void Armoury::learn_from(SimpleVariableStorage & _parameters)
{
	base::learn_from(_parameters);

	weaponItemType = Framework::Library::get_current()->get_global_references().get<Framework::ItemType>(NAME(grWeaponItemTypeToUseWithWeaponParts));

	armouryRemainsEmpty = _parameters.get_value<bool>(NAME(armouryStartsEmpty), false);
	armouryIgnoreExistingWeapons = _parameters.get_value<bool>(NAME(armouryIgnoreExistingWeapons), false);
}

LATENT_FUNCTION(Armoury::execute_logic)
{
	LATENT_SCOPE();

	LATENT_PARAM(::Framework::AI::MindInstance*, mind); // auto
	LATENT_PARAM(::Framework::AI::Logic*, logic); // auto
	LATENT_END_PARAMS();

	LATENT_VAR(::Framework::AI::MessageHandler, messageHandler);

	auto * self = fast_cast<Armoury>(logic);
	auto* imo = mind->get_owner_as_modules_owner();

	self->timeLeftToUpdatePersistence = max(0.0f, self->timeLeftToUpdatePersistence - LATENT_DELTA_TIME);

	LATENT_BEGIN_CODE();

	ai_log(self, TXT("armoury, hello!"));

	self->rg = imo->get_individual_random_generator();

	messageHandler.use_with(mind);
	{
		messageHandler.set(NAME(aim_a_startArmoury), [self](Framework::AI::Message const& _message)
			{
				self->armouryRemainsEmpty = false;
			}
		);
		messageHandler.set(NAME(aim_a_armouryEnlarged), [self](Framework::AI::Message const& _message)
			{
				self->armouryEnlarged = true;
			}
		);
		messageHandler.set(NAME(aim_wm_weaponMixed), [self](Framework::AI::Message const& _message)
			{
				self->updatePersistence = UpdatePersistence::Explicit;
			}
		);
		messageHandler.set(NAME(aim_a_weaponBought), [self](Framework::AI::Message const& _message)
			{
				self->updateWeaponsRequired = true;
			}
		);
		messageHandler.set(NAME(aim_tb_objectToDestroy), [self](Framework::AI::Message const& _message)
			{
				if (auto* param = _message.get_param(NAME(object)))
				{
					if (auto* objToDestroy = param->get_as<SafePtr<Framework::IModulesOwner>>().get())
					{
						Concurrency::ScopedSpinLock lock(self->destroyWeaponLock);
						self->destroyWeapon.push_back();
						self->destroyWeapon.get_last() = objToDestroy;
					}
				}
			});
	}

	if (imo)
	{
		self->walls.clear();
		self->coveredColumns = RangeInt::empty;
		self->find_walls(imo->get_presence()->get_in_room());
	}

	// create slot ports first
	{
		self->issue_create_required_slot_ports();
		while (!self->armouryInaccessible
			&& self->docsPending)
		{
			self->process_docs();
			LATENT_YIELD();
		}
	}

	while (self->armouryRemainsEmpty)
	{
		LATENT_YIELD();
	}
	
	{
		self->issue_create_weapons();
	}

	while (! self->armouryInaccessible
		&& (self->docsPending || ! self->weaponsToCreate.is_empty()))
	{
		self->process_docs();
		LATENT_YIELD();
	}

	while (true)
	{
		// armoury auto saves time-based and when an AI message to do so comes
		// for time-based only if persistence has changed it is saved
		if (self->timeLeftToUpdatePersistence <= 0.0f && !self->armouryInaccessible)
		{
			if (self->updatePersistence == UpdatePersistence::No)
			{
				self->updatePersistence = UpdatePersistence::TimeBased;
			}
			self->timeLeftToUpdatePersistence = 10.0f;
		}

		if (self->updatePersistence != UpdatePersistence::No && !self->armouryInaccessible)
		{
			bool storePersistence = self->updatePersistence == UpdatePersistence::Explicit;
			self->updatePersistence = UpdatePersistence::No;

			Game::get()->add_sync_world_job(TXT("transfer weapons to persistence"), [self, storePersistence]()
				{
					// keep weapons that are on pilgrim,
					// do not discard other weapons
					// do not drop all weapons and armoury should remain accessible
					bool storePersistenceNow = storePersistence;
					// check if we should store persistence (unless we have to store it anyway, then still call this)
					if (self->transfer_weapons_to_persistence(true, false, true))
					{
						storePersistenceNow = true;
					}
					if (storePersistenceNow)
					{
						if (auto* gd = GameDirector::get_active())
						{
							gd->mark_auto_storing_persistence_required();
						}
					}
				});
		}

		if (self->updateWeaponsRequired &&
			self->weaponsToCreate.is_empty()) // only do this if we're not creating weapons
		{
			self->updateWeaponsRequired = false;
			if (! self->armouryInaccessible)
			{
				Game::get()->add_sync_world_job(TXT("transfer weapons to persistence and issue create weapons"), [self]()
					{
						// check if we should store persistence (unless we have to store it anyway, then still call this)
						self->transfer_weapons_to_persistence(true, false, true);
						if (auto* gd = GameDirector::get_active())
						{
							gd->mark_auto_storing_persistence_required();
						}
						// existing weapons will be kept
						self->issue_create_weapons();
					});
			}
		}
		
		if (self->armouryEnlarged && !self->armouryInaccessible)
		{
			self->armouryEnlarged = false;

			if (imo)
			{
				self->walls.clear();
				self->coveredColumns = RangeInt::empty;
				self->find_walls(imo->get_presence()->get_in_room());
			}

			LATENT_YIELD();

			self->issue_create_required_slot_ports();
		}

		// if we decided to enlarge the armoury, we still have to process docs
		while (!self->armouryInaccessible
			&& (self->docsPending || ! self->weaponsToCreate.is_empty()))
		{
			self->process_docs();
			LATENT_YIELD();
		}

		LATENT_WAIT(self->rg.get_float(0.5f, 1.0f));
	}

	LATENT_ON_BREAK();
	//

	LATENT_END_CODE();
	LATENT_RETURN();
}

void Armoury::find_walls(Framework::Room* _room, int _inColumnDir)
{
	if (_room->get_tags().get_tag(NAME(armouryRoom)))
	{
		for_every(w, walls)
		{
			if (w->room == _room)
			{
				// already handled
				return;
			}
		}

		Array<ArmouryPOIWall> readWalls;
		readWalls.push_back(armouryData->mainWall);
		readWalls.push_back(armouryData->leftWall);
		readWalls.push_back(armouryData->rightWall);

		for_every(rw, readWalls)
		{
			if (_inColumnDir == 0)
			{
				rw->sortingOrder = abs(rw->relColumn);
			}
			else if (_inColumnDir > 0)
			{
				rw->sortingOrder = rw->relColumn;
			}
			else if (_inColumnDir < 0)
			{
				rw->sortingOrder = -rw->relColumn;
			}
		}

		sort(readWalls);

		struct GetLocFromPOI
		{
			static Vector3 get(Framework::Room* _room, Name const& _poi)
			{
				Framework::PointOfInterestInstance* poi;
				if (_room->find_any_point_of_interest(_poi, poi))
				{
					return poi->calculate_placement().get_translation();
				}
				error(TXT("could not find poi \"%S\" for armoury"), _poi.to_char());
				return Vector3::zero;
			}
		};

		for_every(rw, readWalls)
		{
			SlotPortWall spw;
			spw.room = _room;
			spw.leftBottomPOI = GetLocFromPOI::get(_room, rw->leftBottomPOI);
			spw.leftTopPOI = GetLocFromPOI::get(_room, rw->leftTopPOI);
			spw.rightBottomPOI = GetLocFromPOI::get(_room, rw->rightBottomPOI);
			spw.rightTopPOI = GetLocFromPOI::get(_room, rw->rightTopPOI);

			// we will use less slots than comes from the calculations
			float extraSlotNum = 0.0f;
			if (rw->continuousLeftSide) extraSlotNum += 0.5f;
			if (rw->continuousRightSide) extraSlotNum += 0.5f;

			float distance = ((spw.rightBottomPOI + spw.rightTopPOI) * 0.5f - (spw.leftBottomPOI + spw.leftTopPOI) * 0.5f).length();
			float numF = (distance / Persistence::get_weapons_in_armoury_column_spacing() - extraSlotNum);
			int num = TypeConversions::Normal::f_i_closest(numF + 1.0f - 0.3f);
			if (rw->relColumn == 0)
			{
				num = max(2, num - (num % 2));
			}
			else
			{
				num = max(2, num);
			}

			// take into account missing slots, pretend we have them all and only then move values from POIs
			{
				Vector3 l2rDir = (spw.rightBottomPOI - spw.leftBottomPOI).normal_2d();
				float actualSpacing = distance / ((float)(num - 1) + extraSlotNum);
				Vector3 cut = l2rDir * actualSpacing * 0.5f;
				if (rw->continuousLeftSide)
				{
					spw.leftBottomPOI += cut;
					spw.leftTopPOI += cut;
				}
				if (rw->continuousRightSide)
				{
					spw.rightBottomPOI -= cut;
					spw.rightTopPOI -= cut;
				}
			}

			if (coveredColumns.is_empty())
			{
				// spread in both directions
				spw.columnRange = RangeInt(-num / 2, num / 2 - 1);
			}
			else
			{
				int inDir = _inColumnDir != 0? _inColumnDir : rw->relColumn;
				if (inDir > 0)
				{
					spw.columnRange = RangeInt(coveredColumns.max + 1, coveredColumns.max + 1 + (num - 1));
				}
				else
				{
					spw.columnRange = RangeInt(coveredColumns.min - 1 - (num - 1), coveredColumns.min - 1);
				}
			}
			coveredColumns.include(spw.columnRange);
			walls.push_back(spw);
		}
	}

	for_every_ptr(door, _room->get_doors())
	{
		if (auto* r = door->get_room_on_other_side())
		{
			if (r->get_tags().get_tag(NAME(armouryRoom)))
			{
				int relColumn = 0;
				if (door->get_tags().get_tag(NAME(toNextArmoury)))
				{
					relColumn = 1;
				}
				if (door->get_tags().get_tag(NAME(toPrevArmoury)))
				{
					relColumn = -1;
				}					
				find_walls(_room);
			}
		}
	}
}

void Armoury::issue_create_required_slot_ports()
{
	auto& p = Persistence::get_current();
	RangeInt c = p.get_weapons_in_armoury_columns_range();
	int rows = Persistence::get_weapons_in_armoury_rows();

	for(int x = c.min; x <= c.max; ++ x)
	{
		for (int y = 0; y < rows; ++y)
		{
			issue_create_slot_port(VectorInt2(x, y), true);
		}
	}
}

void Armoury::issue_create_slot_port(VectorInt2 const& _at, bool _fromColumnRange)
{
	for_every(sp, slotPorts)
	{
		if (sp->at == _at)
		{
			// already issued or created
			return;
		}
	}

	auto* spst = armouryData->slotPortSceneryType.get();
	if (!spst)
	{
		return;
	}
	Framework::Room* inRoom = nullptr;
	SlotPortWall const* spw = nullptr;
	for_every(w, walls)
	{
		if (w->columnRange.does_contain(_at.x))
		{
			spw = w;
			inRoom = w->room.get();
			break;
		}
	}

	if (!inRoom || !spw)
	{
		error(TXT("not enough place for slot parts"));
		return;
	}

	RefCountObjectPtr<Framework::DelayedObjectCreation> doc(new Framework::DelayedObjectCreation());

	doc->inSubWorld = inRoom->get_in_sub_world();
	doc->imoOwner = get_mind()->get_owner_as_modules_owner();
	doc->inRoom = inRoom;
	doc->name = String::printf(TXT("slot port %ix%i"), _at.x, _at.y);
	doc->objectType = spst;
	{
		Vector2 count = (VectorInt2(spw->columnRange.length(), Persistence::get_weapons_in_armoury_rows()) - VectorInt2::one).to_vector2();
		Vector2 relAt = VectorInt2(_at.x - spw->columnRange.min, _at.y).to_vector2();

		Vector2 pt;
		pt.x = count.x > 0.0f? relAt.x / count.x : 0.5f;
		pt.y = count.y > 0.0f? relAt.y / count.y : 0.5f;

		Vector3 topEdge = lerp(pt.x, spw->leftTopPOI, spw->rightTopPOI);
		Vector3 bottomEdge = lerp(pt.x, spw->leftBottomPOI, spw->rightBottomPOI);
		Vector3 leftEdge = lerp(pt.y, spw->leftBottomPOI, spw->leftTopPOI);
		Vector3 rightEdge = lerp(pt.y, spw->rightBottomPOI, spw->rightTopPOI);

		Vector3 at = lerp(pt.y, bottomEdge, topEdge);
		Vector3 up = (topEdge - bottomEdge).normal();
		Vector3 right = (rightEdge - leftEdge).normal();

		// for slot ports we assume that up dir is where the slot port is, forward is pointing where the gun is attached to
		// so sp's forward is our up

		doc->placement = look_matrix_with_right(at, up, right).to_transform();
	}
	doc->randomGenerator = rg;
	rg.advance_seed(23480, 592);
	doc->activateImmediatelyEvenIfRoomVisible = true;
	doc->checkSpaceBlockers = false;
	doc->variables.access<VectorInt2>(NAME(armourySlotPortAt)) = _at;
	ModuleEquipments::Gun::set_common_port_values(REF_ doc->variables);
	doc->priority = 10; // important enough
	doc->devSkippable = false; // always create it

	SlotPort sp;
	sp.at = _at;
	sp.doc = doc;
	sp.temporary = !_fromColumnRange;
	slotPorts.push_back(sp);

	queue_doc(doc.get());
}

void Armoury::issue_create_weapons()
{
	auto& p = Persistence::access_current();

	ai_log(this, TXT("issue create weapons"));

	{
		Concurrency::ScopedSpinLock lock(p.access_lock(), TXT("get weapons for armoury"));
		Array<Persistence::ExistingWeaponFromArmoury> existingWeapons;
		if (!armouryIgnoreExistingWeapons)
		{
			for_every(sp, slotPorts)
			{
				if (auto * simo = sp->imo.get())
				{
					if (auto* itemHolder = simo->get_custom<CustomModules::ItemHolder>())
					{
						if (auto* heldImo = itemHolder->get_held())
						{
							for_every(ww, weapons)
							{
								if (ww->imo == heldImo)
								{
									existingWeapons.push_back();
									auto& ew = existingWeapons.get_last();
									ew.imo = heldImo;
									ew.at = sp->at;
									ew.id = ww->armouryId;
								}
							}
						}
					}
				}
			}
		}
		p.setup_weapons_in_armoury_for_game(get_mind()->get_owner_as_modules_owner(), OUT_ & existingWeapons, armouryIgnoreExistingWeapons);
		if (p.get_weapons_in_armoury().is_empty() &&
			! armouryData->defaultWeaponSetupTemplate.is_empty())
		{
			ai_log(this, TXT("no weapons, add one (default weapon setup template)"));
			// no weapons available, add one, if not empty
			WeaponSetup addWeapon(&p);
			addWeapon.setup_with_template(armouryData->defaultWeaponSetupTemplate);
			Array<WeaponPartType const*> usingWeaponParts;
			WeaponPartType::get_parts_tagged(armouryData->defaultWeaponPartTypesTagged, OUT_ usingWeaponParts);
			addWeapon.fill_with_random_parts_at(WeaponPartAddress(), true, &usingWeaponParts);
			addWeapon.resolve_library_links();
			p.add_weapon_to_armoury(addWeapon);
			p.setup_weapons_in_armoury_for_game();
		}
		weaponsToCreate.make_space_for(p.get_weapons_in_armoury().get_size());
		for_every(wia, p.get_weapons_in_armoury())
		{
			if (wia->id.is_set() && wia->at.is_set() && ! wia->alreadyExists) // if not set, don't spawn, most likely mean that the weapon is held by someone
			{
				WeaponToCreate* wtc = new WeaponToCreate(&p);
				wtc->armouryId = wia->id.get();
				wtc->at = wia->at.get();
				wtc->setup.copy_from(wia->setup);
				ai_log(this, TXT("issue create weapon at %iX%i"), wtc->at.x, wtc->at.y);
				weaponsToCreate.push_back(RefCountObjectPtr<WeaponToCreate>(wtc));
			}
		}
		weapons.clear();
		weapons.make_space_for(weaponsToCreate.get_size() + existingWeapons.get_size());
		for_every(ew, existingWeapons)
		{
			weapons.push_back();
			auto& w = weapons.get_last();
			w.imo = ew->imo;
			w.armouryId = ew->id;
		}
		for_every_ref(wtc, weaponsToCreate)
		{
			weapons.push_back();
			weapons.get_last().armouryId = wtc->armouryId;
		}
	}
}

void Armoury::queue_doc(Framework::DelayedObjectCreation* doc)
{
	++docsPending;
	Game::get()->queue_delayed_object_creation(doc);
}

void Armoury::process_docs()
{
	if (docsPending <= 0 && weaponsToCreate.is_empty())
	{
		return;
	}

	for_every(sp, slotPorts)
	{
		if (auto* doc = sp->doc.get())
		{
			if (doc->is_done())
			{
				sp->imo = doc->createdObject;
				sp->doc.clear();
				--docsPending;
				return;
			}
		}
	}

	if (!weaponsToCreate.is_empty())
	{
		auto& wtc = *weaponsToCreate.get_first().get();

		if (auto* doc = wtc.doc.get())
		{
			if (doc->is_done())
			{
				ai_log(this, TXT("weapon created"));
				Weapon w;
				w.armouryId = wtc.armouryId;
				w.imo = doc->createdObject;
				for_every(ww, weapons)
				{
					if (ww->armouryId == w.armouryId)
					{
						// store updated version, so we can track on pilgrimage switch
						*ww = w;
						break;
					}
				}
				if (auto* wimo = w.imo.get())
				{
					// attach to slot port, it should be already created
					for_every(sp, slotPorts)
					{
						if (sp->at == wtc.at)
						{
							if (auto* simo = sp->imo.get())
							{
								SafePtr<Framework::IModulesOwner> simoRef(simo);
								SafePtr<Framework::IModulesOwner> wimoRef(wimo);
								ai_log(this, TXT("attach to slot port?"));
								Game::get()->add_immediate_sync_world_job(TXT("attach weapon to slot port"),
									[simoRef, wimoRef]()
									{
										auto* simo = simoRef.get();
										auto* wimo = wimoRef.get();
										if (simo && wimo)
										{
											if (auto* itemHolder = simo->get_custom<CustomModules::ItemHolder>())
											{
												itemHolder->hold(wimo);
											}
										}
									});
								break;
							}
						}
					}
				}
				// no need to clear doc, we pop it
				--docsPending;
				weaponsToCreate.pop_front();
			}
		}
		else
		{
			bool slotPortExists = false;
			for_every(sp, slotPorts)
			{
				if (sp->at == wtc.at)
				{
					if (auto* simo = sp->imo.get())
					{
						slotPortExists = true;
						// order to create weapon
						if (auto* wit = weaponItemType.get())
						{
							auto* inRoom = simo->get_presence()->get_in_room();

							RefCountObjectPtr<Framework::DelayedObjectCreation> doc(new Framework::DelayedObjectCreation());

							doc->inSubWorld = inRoom->get_in_sub_world();
							doc->imoOwner = get_mind()->get_owner_as_modules_owner();
							doc->inRoom = inRoom;
							doc->name = String::printf(TXT("weapon %ix%i"), wtc.at.x, wtc.at.y);
							doc->objectType = wit;
							doc->randomGenerator = rg;
							rg.advance_seed(23480, 592);
							doc->activateImmediatelyEvenIfRoomVisible = true;
							doc->checkSpaceBlockers = false;
							{
								doc->variables.access<int>(NAME(armouryId)) = wtc.armouryId;
								if (auto* pi = PilgrimageInstance::get())
								{
									if (auto* p = pi->get_pilgrimage())
									{
										doc->variables.set_from(p->get_equipment_parameters());
									}
								}
								{
									todo_multiplayer_issue(TXT("we just get player here"));
									if (auto* g = Game::get_as<Game>())
									{
										if (auto* pa = g->access_player().get_actor())
										{
											if (auto* mp = pa->get_gameplay_as<ModulePilgrim>())
											{
												if (auto* pd = mp->get_pilgrim_data())
												{
													doc->variables.set_from(pd->get_equipment_parameters());
												}
											}
										}
									}
								}
								doc->variables.set_from(armouryData->equipmentParameters);
							}
							doc->priority = 10; // important enough
							doc->devSkippable = false; // always create it
							RefCountObjectPtr<WeaponToCreate> wtcRef(&wtc);
							doc->pre_setup_module_function = [wtcRef](Framework::Module* _module)
							{
								if (auto* g = fast_cast<ModuleEquipments::Gun>(_module))
								{
									// the stats differ then a bit as we have different storages
									//g->be_pilgrim_weapon();
									g->setup_with_weapon_setup(wtcRef->setup);
								}
							};

							wtc.doc = doc;
							queue_doc(doc.get());
						}
						break;
					}
				}
			}

			if (!slotPortExists)
			{
				issue_create_slot_port(wtc.at);
			}
		}
	}
}

void Armoury::on_pilgrimage_instance_switch(PilgrimageInstance const* _from, PilgrimageInstance const* _to)
{
	// determine if the armoury is in the _from's main region, act only if so

	bool process = false;

	if (auto* imo = get_mind()->get_owner_as_modules_owner())
	{
		if (auto* r = _from->get_main_region())
		{
			if (auto* inRoom = imo->get_presence()->get_in_room())
			{
				if (inRoom->is_in_region(r))
				{
					process = true;
				}
			}
		}
	}

	if (!process)
	{
		return;
	}

	// we're moving somewhere else, weapons that are on the pilgrim are considered lost
	// weapons that do not fit into the rack are considered lost as well
	transfer_weapons_to_persistence(false, true, false);
	todo_note(TXT("this means that if we enter drop capsule and exit, the weapons are lost"));

	pio_remove();
}

void Armoury::on_pilgrimage_instance_end(PilgrimageInstance const* _pilgrimage, PilgrimageResult::Type _pilgrimageResult)
{
	// determine if the armoury is in the _pilgrimage's main region, act only if so
	// note that this is exclusive with switch, we either end or switch

	bool process = false;

	if (auto* imo = get_mind()->get_owner_as_modules_owner())
	{
		if (auto* r = _pilgrimage->get_main_region())
		{
			if (auto* inRoom = imo->get_presence()->get_in_room())
			{
				if (inRoom->is_in_region(r))
				{
					process = true;
				}
			}
		}
	}

	if (!process)
	{
		return;
	}

	// we ended/interrupted, even if we died, the weapons we have on the pilgrim should remain in the armoury
	// do not discard, we may get an upgrade between
	transfer_weapons_to_persistence(true, false, false);

	pio_remove();
}

bool Armoury::transfer_weapons_to_persistence(bool _includeWeaponsWithPlayer, bool _discardInvalid, bool _keepActive)
{
	// check where each weapon is
	//	if player took it - remove it from the persistence
	//	if doesn't exist anymore - we don't know if it existed or not, keep it as it is in the persistence
	//	check where it was put and store that

	bool somethingChanged = false;

	auto& p = Persistence::access_current();

	armouryInaccessible = true;

	// all weapons present here should be created by the armoury
	// in the actual game we should never come up with own weapons
	for_every(w, weapons)
	{
		auto* imo = w->imo.get();

		if (!imo)
		{
			//p.remove_weapon_from_armoury(w->armouryId);
			// keep in the armoury - maybe it didn't spawn?
			continue;
		}
		else
		{
			Persistence::WeaponOnPilgrimInfo onPilgrimInfo;

			if (Game* g = Game::get_as<Game>())
			{
				if (auto* pa = g->access_player().get_actor())
				{
					onPilgrimInfo.fill(pa, imo);

					if (!_includeWeaponsWithPlayer)
					{
						if (onPilgrimInfo.onPilgrim)
						{
							// taken with the player
							somethingChanged |= p.remove_weapon_from_armoury(w->armouryId);
							continue;
						}
						if (pa->get_presence()->get_in_room() ==
							imo->get_presence()->get_in_room())
						{
							// the player tried to outsmart us and dropped the weapons into the transition room
							// and if not in the transition room, it should remain somewhere on the ship and therefore can be retrieved
							somethingChanged |= p.remove_weapon_from_armoury(w->armouryId);
							continue;
						}
					}
				}
			}


			Optional<VectorInt2> at;
			if (auto* simo = imo->get_presence()->get_attached_to())
			{
				if (auto* armourySlotPortAt = simo->get_variables().get_existing<VectorInt2>(NAME(armourySlotPortAt)))
				{
					at = *armourySlotPortAt;
				}
			}

			if (auto* g = imo->get_gameplay_as<ModuleEquipments::Gun>())
			{
				somethingChanged |= p.update_weapon_in_armoury(w->armouryId, g->get_weapon_setup(), at, onPilgrimInfo);
			}
			else
			{
				somethingChanged |= p.remove_weapon_from_armoury(w->armouryId);
			}
		}
	}

	if (_discardInvalid)
	{
		// find placement for weapons that are left in the armoury
		p.setup_weapons_in_armoury_for_game();
		// discard the rest
		somethingChanged |= p.discard_invalid_weapons_from_armoury();
	}

	if (_keepActive)
	{
		armouryInaccessible = false;
	}
	else
	{
		weapons.clear();
	}

	return somethingChanged;
}

//

REGISTER_FOR_FAST_CAST(ArmouryData);

ArmouryData::ArmouryData()
{
}

ArmouryData::~ArmouryData()
{
}

bool ArmouryData::load_from_xml(IO::XML::Node const * _node, ::Framework::LibraryLoadingContext & _lc)
{
	bool result = base::load_from_xml(_node, _lc);

	slotPortSceneryType.load_from_xml(_node, TXT("slotPortSceneryType"), _lc);

	equipmentParameters.load_from_xml_child_node(_node, TXT("equipmentParameters"));
	_lc.load_group_into(equipmentParameters);

	for_every(n, _node->children_named(TXT("mainWall")))
	{
		result &= mainWall.load_from_xml(n);
	}
	for_every(n, _node->children_named(TXT("leftWall")))
	{
		result &= leftWall.load_from_xml(n);
	}
	for_every(n, _node->children_named(TXT("rightWall")))
	{
		result &= rightWall.load_from_xml(n);
	}

	defaultWeaponSetupTemplate.clear();
	for_every(n, _node->children_named(TXT("defaultWeaponSetupTemplate")))
	{
		result &= defaultWeaponSetupTemplate.load_from_xml(n);
	}

	defaultWeaponPartTypesTagged.clear();
	defaultWeaponPartTypesTagged.load_from_xml_attribute_or_child_node(_node, TXT("defaultWeaponPartTypesTagged"));

	return result;
}

bool ArmouryData::prepare_for_game(::Framework::Library* _library, ::Framework::LibraryPrepareContext& _pfgContext)
{
	bool result = base::prepare_for_game(_library, _pfgContext);

	result &= slotPortSceneryType.prepare_for_game_find(_library, _pfgContext, Framework::LibraryPrepareLevel::Resolve);

	result &= defaultWeaponSetupTemplate.resolve_library_links();

	return result;
}

//

