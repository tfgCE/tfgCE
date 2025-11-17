#pragma once

#include "modulePilgrimData.h"

#include "..\controllable.h"
#include "..\moduleGameplay.h"

#include "..\..\teaForGod.h"

#include "..\..\pilgrim\pilgrimBlackboard.h"
#include "..\..\pilgrim\pilgrimInventory.h"
#include "..\..\pilgrim\pilgrimOverlayInfo.h"
#include "..\..\pilgrim\pilgrimSetup.h"

#include "..\..\..\core\memory\safeObject.h"
#include "..\..\..\core\physicalSensations\physicalSensations.h"
#include "..\..\..\core\types\hand.h"

#include "..\..\..\framework\appearance\socketID.h"
#include "..\..\..\framework\object\object.h"
#include "..\..\..\framework\presence\presencePath.h"

namespace Framework
{
	class GameInput;
	class ItemType;
	struct DoorInRoomArrayPtr;
	struct RelativeToPresencePlacement;
	namespace Render
	{
		class SceneBuildContext;
	};
};

namespace TeaForGodEmperor
{
	struct Energy;
	struct GameState;
	struct PilgrimGear;
	struct PilgrimHandSetup;
	struct WeaponSetup;
	class ModuleEquipment;
	class ModuleEXM;
	class ModulePilgrimData;

	namespace AI
	{
		namespace Logics
		{
			class BridgeShop;
		};
	};

	namespace CustomModules
	{
		class ItemHolder;
	};

	namespace GameScript
	{
		namespace Elements
		{
			class CreateMainEquipment;
			class MainEquipment;
			class Pilgrim;
		};
	};

	namespace ModuleEquipments
	{
		class Gun;
	};

	namespace PilgrimDisplayBlock
	{
		enum Flag
		{
			Shield = 1
		};
		typedef int Flags;
	};

	class ModulePilgrim
	: public ModuleGameplay
	, public IControllable
	{
		FAST_CAST_DECLARE(ModulePilgrim);
		FAST_CAST_BASE(ModuleGameplay);
		FAST_CAST_BASE(IControllable);
		FAST_CAST_END();

		typedef ModuleGameplay base;
	public:
		static GameState* s_hackCreatingForGameState; // an ugly hack to access the right gear when loading game state (it should be set and cleared immediately close to initialise_modules)

#ifdef AN_DEVELOPMENT_OR_PROFILER
		Concurrency::Counter beingAdvanced;
#endif

	public:
		static Framework::RegisteredModule<Framework::ModuleGameplay> & register_itself();

		static PhysicalViolence read_physical_violence_from(Framework::ModuleData const* _moduleData);
		static Energy read_hand_energy_base_max_from(Framework::ModuleData const* _moduleData);

		ModulePilgrim(Framework::IModulesOwner* _owner);
		virtual ~ModulePilgrim();

		void use(Hand::Type _hand, Framework::IModulesOwner* _item);

		void set_block_display(Hand::Type _hand, PilgrimDisplayBlock::Flag _flag, bool _block) { if (_block) set_flag(handStates[_hand].blockDisplay, _flag); else clear_flag(handStates[_hand].blockDisplay, _flag);	}

		ModulePilgrimData const * get_pilgrim_data() const { return modulePilgrimData; }

		PilgrimBlackboard const& get_pilgrim_blackboard() const { return pilgrimBlackboard; }

		PilgrimSetup const & get_pilgrim_setup() const { return pilgrimSetup; }
		bool is_up_to_date_with_pilgrim_setup() const { return upToDateWithPilgrimSetup; }
		void update_pilgrim_setup_for(Framework::IModulesOwner const * _mainEquipment, bool _updatePendingToo);
		bool check_pilgrim_setup() const; // true if match

		PilgrimSetup const& get_pending_pilgrim_setup() const { return pendingPilgrimSetup; }
		void set_pending_pilgrim_setup(PilgrimSetup const & _newSetup); // call it to allow copying
		void create_things_accordingly_to_pending_pilgrim_setup(bool _force = false); // this require to be called from async!
		
		void clear_extra_exms_in_inventory(); // removes active and passive exms from available exms in inventory if they are not installed
		void update_exms_in_inventory(); // async or sync
		void sync_recreate_exms(); // this is done in sync, quickly destroy exms that are no longer there and create new ones
		bool has_exm_equipped(Name const& _exm, Optional<Hand::Type> const & _hand = NP) const;
		ModuleEXM* get_equipped_active_exm(Hand::Type _hand);
		ModuleEXM* get_equipped_exm(Name const& _exm, Optional<Hand::Type> const& _hand = NP);
		bool does_equipped_exm_appear_active(Hand::Type _hand, OPTIONAL_ OUT_ Optional<float>* _cooldownLeftPt = nullptr); // only active may be active
		bool is_active_exm_equipped(Hand::Type _hand, Framework::IModulesOwner const* _imoEXM) const;

		bool set_pending_pockets_vertical_adjustment(float _pocketsVerticalAdjustment);
		void recreate_mesh_accordingly_to_pending_setup(bool _force = false);

		Energy calculate_items_energy(Optional<EnergyType::Type> const & _ofType = NP) const;

		bool can_consume_equipment_energy_for(Hand::Type _hand, Energy const& _amount) const; // main equipment, consumes whole amount or none
		bool consume_equipment_energy_for(Hand::Type _hand, Energy const& _amount); // main equipment, consumes whole amount or none
		bool receive_equipment_energy_for(Hand::Type _hand, Energy const& _amount); // main equipment, consumes whole amount or none

		bool does_hand_collide_with(Hand::Type _hand, Framework::ModuleCollision const * _col, OUT_ Framework::IModulesOwner** _colliderWithHealth = nullptr) const; // hand or equipment in hand or anything attached to hand

		void get_physical_violence_hand_speeds(Hand::Type _hand, OUT_ float & _minSpeed, OUT_ float & _minSpeedStrong) const;
		
		Energy get_physical_violence_damage(Hand::Type _hand, bool _strong) const;
		
		void reset_energy_state();
		void redistribute_energy_to_health(Energy const& _minHealthEnergy, bool _fillUpHealthFromRegen, bool _makeUpEnergyIfNotEnough); // this will use existing energy, if there's not enough, it will be added to make it so

		void get_physical_violence_costs(Hand::Type _hand, bool _strong, OUT_ Energy & _health, OUT_ Energy& _ammo) const;

		void log(LogInfoContext& _log);

		void lose_hand(Hand::Type _hand);

	public:
		void store_for_game_state(PilgrimGear* _gear, OUT_ REF_ bool & _atLeastHalfHealth) const;

		void add_gear_to_persistence(bool _allowDuplicates = false);

#ifdef WITH_CRYSTALITE
	public:
		void add_crystalite(int _amount);
		int get_crystalilte() const { return crystalite; }
#endif

	public:
		PilgrimInventory& access_pilgrim_inventory() { return pilgrimInventory; }
		PilgrimInventory const& get_pilgrim_inventory() const { return pilgrimInventory; }
		bool does_have_any_weapon_parts_to_transfer() const;
		void update_pilgrim_inventory_for_transfer(); // add/remove weapon parts from held
		void update_pilgrim_inventory_for_inventory(); // remove weapon parts from items
		void transfer_out_weapon_part(PilgrimInventory::ID _weaponPartID); // will remove form pilgrim inventory (and weapons) and add to persistence

	private:
		friend class GameScript::Elements::MainEquipment;
		void ex__block_releasing_equipment(Hand::Type _hand, bool _block);
		void ex__force_release(Hand::Type _hand);
		void ex__drop_energy_by(Optional<Hand::Type> const& _hand, Energy const & _dropBy); // if false, drops random
		void ex__set_energy(Optional<Hand::Type> const& _hand, Energy const & _setTo); // if false, drops random

	public:
		void setup_scene_build_context_for_tutorial_highlights(Framework::Render::SceneBuildContext& _context);

	protected: // Module
		override_ void setup_with(Framework::ModuleData const * _moduleData);
		override_ void initialise();
		override_ void activate();

	protected: // ModuleGameplay
		override_ void advance_post_move(float _deltaTime);

	public:
		Framework::IModulesOwner* get_pilgrim_keeper() const { return pilgrimKeeper.get(); }
		Framework::IModulesOwner* get_hand(Hand::Type _type) const { return (_type == Hand::Left ? leftHand.get() : rightHand.get()); }
		Transform get_hand_relative_placement_os(Hand::Type _type, bool _centre = false) const; // relative to owner
		Framework::IModulesOwner* get_rest_point(Hand::Type _type) const { return (_type == Hand::Left ? leftRestPoint.get() : rightRestPoint.get()); }
		SafePtr<Framework::IModulesOwner>& access_hand_equipment(Hand::Type _type) { return _type == Hand::Left ? leftHandEquipment : rightHandEquipment; }
		Framework::IModulesOwner* get_hand_equipment(Hand::Type _type) const { return (_type == Hand::Left ? leftHandEquipment.get() : rightHandEquipment.get()); }
		Framework::IModulesOwner* get_main_equipment(Hand::Type _type) const { return (_type == Hand::Left ? leftMainEquipment.get() : rightMainEquipment.get()); }
		bool is_main_equipment(Framework::IModulesOwner const* _imo) const { return leftMainEquipment == _imo || rightMainEquipment == _imo; }
		bool is_in_pocket(Framework::IModulesOwner const* _imo) const;
		int get_in_pocket_index(Framework::IModulesOwner const* _imo) const; // if not in pocket, returns NONE
		bool is_held_in_hand(Framework::IModulesOwner const* _imo) const { return leftHandEquipment == _imo || rightHandEquipment == _imo; }
		Optional<Hand::Type> get_helding_hand(Framework::IModulesOwner const* _imo) const { return leftHandEquipment == _imo? Optional<Hand::Type>(Hand::Left) : (rightHandEquipment == _imo? Optional<Hand::Type>(Hand::Right) : NP); }
		bool is_actively_holding_in_hand(Hand::Type _type) const;
		bool should_highlight_damaged_main_equipment(Hand::Type _hand) const { return _hand == Hand::Left ? highlightDamagedLeftMainEquipment : highlightDamagedRightMainEquipment; }

		bool is_something_moving() const; // this is for movement-based sight

		// changing _hand to left for common energy storage is handled inside
		Energy get_hand_energy_storage(Hand::Type _hand) const;
		Energy get_hand_energy_max_storage(Hand::Type _hand) const;
		void set_hand_energy_storage(Hand::Type _hand, Energy const& _energy);
		void add_hand_energy_storage(Hand::Type _hand, Energy const& _energy);

		void get_all_items(ArrayStack<Framework::IModulesOwner*>& _into) const;
		void get_all_guns(ArrayStack<ModuleEquipments::Gun*>& _into) const;

		template <typename Container>
		void get_in_pockets(REF_ Container & _container, Optional<Hand::Type> const & _side = NP, bool _orderByPocketLevel = false, bool _emptyPocketsToo = false);

		bool put_into_pocket(Framework::IModulesOwner* _object); // returns true if managed to put into a pocket

		void set_main_equipment(Hand::Type _type, Framework::IModulesOwner* _equipment);

		void drop(Framework::IModulesOwner* _item);

		bool does_want_to_hold_equipment(Hand::Type _type) const { return handStates[_type].currentState == HandState::HoldingObject; }

		bool should_put_hand_equipment_into_pocket(Hand::Type _type) const { return handStates[_type].pocketToPutInto != nullptr; }
		Framework::IModulesOwner* get_put_hand_equipment_into_item_holder(Hand::Type _type) const { return handStates[_type].putIntoItemHolder.get(); }
		
		void paralyse_hand(Hand::Type _type, bool _paralyse) { handStates[_type].shouldBeParalysed = _paralyse; }
		void idle_pose_hand(Hand::Type _type, bool _idlePose) { handStates[_type].shouldBeIdle = _idlePose; }

		float get_fake_pointing(Hand::Type _type) const { return handStates[_type].fakePointing.state; }

		Framework::SocketID const & get_energy_inhale_socket() const { return energyInhaleSocket; }

		Vector3 get_forearm_location_os(Hand::Type _hand) const;

		void set_object_to_grab_proposition(Hand::Type _hand, Framework::IModulesOwner* _objectToGrab, Framework::DoorInRoomArrayPtr const * _throughDoors = nullptr);
		Framework::IModulesOwner* get_object_to_grab(Hand::Type _hand) const { return handStates[_hand].objectToGrab.is_set()? handStates[_hand].objectToGrab.get() : handStates[_hand].objectToGrabProposition.get(); }

		Framework::IModulesOwner* get_grabbed_object(Hand::Type _hand) const { return handStates[_hand].objectGrabbed.get(); }
		Framework::PresencePath const & get_grabbed_object_path(Hand::Type _hand) const { return handStates[_hand].objectGrabbedPath; }

		Name const & get_objects_held_object_size_var_ID() const { return objectsHeldObjectSizeVarID; }

	public:
		PilgrimOverlayInfo& access_overlay_info() { return overlayInfo; }

		bool should_pilgrim_overlay_look_for_pickups() const;
		bool should_pilgrim_overlay_speak_full_health() const { return infiniteRegen.healthRate.is_zero(); }
		bool should_pilgrim_overlay_speak_full_ammo() const { return infiniteRegen.ammoRate.is_zero(); }

		void show_weapons_blocked_overlay_message();

	public: // IControllable
		implement_ void pre_process_controls();
		implement_ void process_controls(Hand::Type _hand, Framework::GameInput const& _controls, float _deltaTime);

	public: // equipment controls
		float get_controls_use_equipment(Hand::Type _hand) const { return equipmentControls.use[_hand]; }
		float get_controls_use_as_usable_equipment(Hand::Type _hand) const { return equipmentControls.useAsUsable[_hand]; }
		float get_controls_change_equipment(Hand::Type _hand) const { return equipmentControls.change[_hand]; }
		VectorInt2 const & get_controls_dpad_equipment(Hand::Type _hand) const { return equipmentControls.dPad[_hand]; }
		bool is_controls_use_equipment_pressed(Hand::Type _hand) const;
		bool has_controls_use_equipment_been_pressed(Hand::Type _hand) const;
		bool is_controls_use_as_usable_equipment_pressed(Hand::Type _hand) const;
		bool has_controls_use_as_usable_equipment_been_pressed(Hand::Type _hand) const;
		bool is_controls_use_exm_pressed(Hand::Type _hand) const;
		bool has_controls_use_exm_been_pressed(Hand::Type _hand) const;

		// not use so far?
		//bool has_controls_alt_use_equipment_been_pressed(Hand::Type _hand) const { return equipmentControls.altUse[_hand] >= CONTROLS_THRESHOLD && equipmentControlsPrev.altUse[_hand] < CONTROLS_THRESHOLD; }
		//bool has_controls_change_equipment_been_pressed(Hand::Type _hand) const { return equipmentControls.change[_hand] >= CONTROLS_THRESHOLD && equipmentControlsPrev.change[_hand] < CONTROLS_THRESHOLD; }
		//bool has_controls_dpad_equipment_been_pressed(Hand::Type _hand) const { return equipmentControls.dPad[_hand] != equipmentControlsPrev.dPad[_hand] && !equipmentControls.dPad[_hand].is_zero(); }

	public:
		bool is_aiming_at(Framework::IModulesOwner const * imo, Framework::PresencePath const * _usePath = nullptr, OUT_ float * _distance = nullptr, float _maxOffPath = 0.1f, float _maxAngleOff = 5.0f) const;
		bool is_aiming_at(Framework::IModulesOwner const * imo, Framework::RelativeToPresencePlacement const * _usePath = nullptr, OUT_ float * _distance = nullptr, float _maxOffPath = 0.1f, float _maxAngleOff = 5.0f) const;

	private:
		ModulePilgrimData const * modulePilgrimData = nullptr;

		bool pilgrimEmpty = false; // to be created empty

		bool advancedAtLeastOnce = false;

		bool lostHand[Hand::MAX];

		Optional<PhysicalSensations::Sensation::ID> physSensMoving;
		float timeSinceBaseMoving = 10.0f;

		// current one, for this game, other parts of code should only seek for lookups in it, we go with  equipment arrays
		PilgrimSetup pendingPilgrimSetup; // requested/pending - it is set by external sources. for weapon parts it is important that this and pilgrim inventory work together
		PilgrimSetup pilgrimSetup; // this is the setup currently being used
		PilgrimBlackboard pilgrimBlackboard; // everything looked up
		PilgrimInventory pilgrimInventory;
		bool upToDateWithPilgrimSetup = true; // if everything is created

		float pendingPocketsVerticalAdjustment = 0.0f;
		float pocketsVerticalAdjustment = 0.0f;

		float minPhysicalViolenceHandSpeed = 2.0f;
		float minPhysicalViolenceHandSpeedStrong = 5.0f;
		Energy physicalViolenceDamage = Energy(20);
		Energy physicalViolenceDamageStrong = Energy(40);

		SafePtr<Framework::IModulesOwner> pilgrimKeeper;

		SafePtr<Framework::IModulesOwner> leftHand;
		SafePtr<Framework::IModulesOwner> rightHand;

		SafePtr<Framework::IModulesOwner> leftHandForearmDisplay;
		SafePtr<Framework::IModulesOwner> rightHandForearmDisplay;

		float timeSinceLastPickupInHand = 10000.0f;

#ifdef WITH_CRYSTALITE
		int crystalite = 0;
#endif

		struct EnergyState
		{
			Energy energy = Energy::zero();
			Energy maxEnergy = Energy::zero();

			Energy startingEnergy = Energy(500.0f);
			Energy baseMaxEnergy = Energy(500.0f);
		};
		EnergyState leftHandEnergyStorage;
		EnergyState rightHandEnergyStorage;

		SafePtr<Framework::IModulesOwner> leftRestPoint;
		SafePtr<Framework::IModulesOwner> rightRestPoint;

		//Array<SafePtr<Framework::IModulesOwner>> equipment; // all equipment - not used yet
		// currently used
		SafePtr<Framework::IModulesOwner> leftHandEquipment; // currently in left hand
		SafePtr<Framework::IModulesOwner> rightHandEquipment; // currently in right hand

		// main for hand (attached to forearm)
		SafePtr<Framework::IModulesOwner> leftMainEquipment;
		SafePtr<Framework::IModulesOwner> rightMainEquipment;

		bool highlightDamagedLeftMainEquipment = false;
		bool highlightDamagedRightMainEquipment = false;

		bool weaponsBlocked = false;

		struct EXMs
		{
			struct EXM
			{
				SafePtr<Framework::IModulesOwner> imo;
				Name exm;

				void clear() { imo.clear(); exm = Name::invalid(); }
			};
			// exm actors, just two
			ArrayStatic<EXM, MAX_PASSIVE_EXMS> passiveEXMs;
			EXM activeEXM;
			ArrayStatic<EXM, 32> permanentEXMs; // set on left hand only

			EXMs()
			{
				SET_EXTRA_DEBUG_INFO(passiveEXMs, TXT("ModulePilgrim::EXMs.passiveEXMs"));
				SET_EXTRA_DEBUG_INFO(permanentEXMs, TXT("ModulePilgrim::EXMs.permanentEXMs"));
			}
			void cease_to_exists_and_clear();
			void create(Framework::Actor* _owner, Hand::Type _hand, bool _createHandSetup, PilgrimHandSetup const& _handSetup, PilgrimInventory const * _inventoryForPermanent);

			Framework::IModulesOwner* create_internal(Framework::Actor* _owner, Hand::Type _hand, Name const& _exm);

			void blink_on_physical_violence();
		};
		EXMs leftEXMs;
		EXMs rightEXMs;

		void update_blackboard();
		void redistribute_energy(bool _resetEnergyState = false);

	private:
		struct Pocket;

		struct Element
		{
			float state = 0.0f;
			float target = 0.0f;

			void blend_to_target(float _blendTime, float _deltaTime);
		};

		struct HandState
		{
			enum State
			{
				EmptyHand,
				ReleaseObject,
				GrabObject,
				HoldingObject
			};

			Pocket* pocketToPutInto = nullptr;
			SafePtr<Framework::IModulesOwner> putIntoItemHolder; // only if in a item holder room

			bool shouldBeParalysed = false;
			bool shouldBeIdle = false; // forced
			struct ParalysedPoseTimes
			{
				float hand = 0.0f;
				float port = 0.0f;
				float display = 0.0f;
				float restPoint = 0.0f;
				float fingers = 0.0f;

				void reset()
				{
					*this = ParalysedPoseTimes();
				}
				void update(float _deltaTime)
				{
					hand -= _deltaTime;
					port -= _deltaTime;
					display -= _deltaTime;
					restPoint -= _deltaTime;
					fingers -= _deltaTime;
				}
			};
			ParalysedPoseTimes paralysedPoseTimeTo;

			struct PhysicalViolence
			{
				Optional<float> timeSinceLastHit = 10.0f; // clears on very slow
			};
			PhysicalViolence physicalViolence;

			bool blockUsingEquipment = false;
			bool blockUsingEquipmentDueToPort = false;
			bool blockReleasingEquipment = false;
			float blockReleasingEquipmentTimeLeft = 0.0f;
			bool exBlockReleasingEquipment = false;
			bool exForceRelease = false;
			State prevState = EmptyHand;
			State currentState = EmptyHand;
			SafePtr<Framework::IModulesOwner> objectToGrab;
			Framework::PresencePath objectToGrabPath; // relative to hand!
			SafePtr<Framework::IModulesOwner> objectGrabbed;
			Framework::PresencePath objectGrabbedPath; // relative to hand!
			float overForearm = 0.0f;

			Optional<float> objectToLateGrabPropositionStillValidFor;
			SafePtr<Framework::IModulesOwner> objectToLateGrabProposition;
			Framework::PresencePath objectToLateGrabPropositionPath; // relative to hand!
			SafePtr<Framework::IModulesOwner> objectToGrabProposition;
			Framework::PresencePath objectToGrabPropositionPath; // relative to hand!

			Element holdGun;
			Element pressTrigger;
			Element awayTrigger;
			Element holdObject;
			float holdObjectSize = 0.0f;
			Element fist;
			Element grab;
			Element grabIsDial;
			Element straight;
			Element fakePointing; // as not "pointing is fake" but "to fake a pointing pose"

			Element allowHandTrackingPose; // in some cases we want to block hand tracking (paralysed, holding an object)

			Element relaxedThumb;
			Element relaxedPointer;
			Element relaxedMiddle;
			Element relaxedRing;
			Element relaxedPinky;

			Element straightThumb;
			Element straightPointer;
			Element straightMiddle;
			Element straightRing;
			Element straightPinky;

			PilgrimDisplayBlock::Flags blockDisplay = 0;
			Element port;
			Element displayNormal;
			Element displayExtra;
			Element restPoint;
			Element restPointEmpty;

			bool forceRelease = false; // used when we move too far away from grabbable
			bool autoHoldDueToShortPause = false; // will hold until grab is requested
			bool blockUsingEquipmentDueToShortPause = false;
			bool blockUsingUsableEquipmentDueToShortPause = false;
			bool blockDeployingMainEquipmentDueToShortPause = false;
			bool blockUsingEXMDueToShortPause = false;

			bool autoMainEquipmentActive; // we're in auto main equipment mode (it is in hand or was forced to go)
			
			float prevInputGrab = 0.0f;
			float prevInputRelease = 0.0f;
			float prevInputPullEnergy = 0.0f;
			float prevInputPullEnergyAlt = 0.0f;
			float prevInputDeployMainEquipment = 0.0f;
			float prevInputUseEquipment = 0.0f;
			float prevInputUseAsUsableEquipment = 0.0f;

			float inputShowOverlayInfo = 0.0f;
			Vector2 inputOverlayInfoStick = Vector2::zero;
			float inputHeadMenuRequested = 0.0f;
			float inputHeadMenuRequestedHold = 0.0f;
			float inputFist = 0.0f;
			float inputGrab = 0.0f;
			float inputPullEnergy = 0.0f; // this is normal "pull energy" that is shared with grabbing/fists, it is not allowed to be used when holding particular things etc
			float inputPullEnergyAlt = 0.0f; // this is allowed at any time but for some controllers it is less convenient
			Vector2 inputPullEnergyAltStick = Vector2::zero;
			float inputRelease = 0.0f;
			float inputSharedReleaseGrabDeploy = 0.0f; // shared input which means that we want to release a button
			float inputDeployMainEquipment = 0.0f;
			float inputAutoHold = 0.0f;
			float inputAutoDropEmptyWeapons = 0.0f;
			float inputAutoPointing = 0.0f;
			float inputAutoMainEquipment = 0.0f; // this is auto interim equipment management - hides main equipment when interim equipment available
			float inputEasyReleaseEquipment = 0.0f; // easy release (if equipment allows to)
			float inputUseEquipment = 0.0f;
			float inputUseEquipmentAway = 0.0f; // finger is away from using
			float inputUseAsUsableEquipment = 0.0f; // equipment should have an option set (inspecters)
			float inputUseEXM = 0.0f;
			float inputSeparateFingers = 0.0f;
			float inputAllowHandTrackingPose = 0.0f;
			float inputRelaxedThumb = 0.0f;
			float inputRelaxedPointer = 0.0f;
			float inputRelaxedMiddle = 0.0f;
			float inputRelaxedRing = 0.0f;
			float inputRelaxedPinky = 0.0f;
			float inputStraightThumb = 0.0f;
			float inputStraightPointer = 0.0f;
			float inputStraightMiddle = 0.0f;
			float inputStraightRing = 0.0f;
			float inputStraightPinky = 0.0f;

			SimpleVariableInfo handHoldGunVarID;
			SimpleVariableInfo handPressTriggerVarID;
			SimpleVariableInfo handAwayTriggerVarID;
			SimpleVariableInfo handHoldObjectVarID;
			SimpleVariableInfo handHoldObjectSizeVarID;
			SimpleVariableInfo handFistVarID;
			SimpleVariableInfo handGrabVarID;
			SimpleVariableInfo handGrabDialVarID;
			SimpleVariableInfo handStraightVarID;
			SimpleVariableInfo handPortActiveVarID;
			SimpleVariableInfo handDisplayActiveNormalVarID;
			SimpleVariableInfo handDisplayActiveExtraVarID;
			SimpleVariableInfo restPointHandObjectVarID;
			SimpleVariableInfo restPointEquipmentObjectVarID;
			SimpleVariableInfo restPointTVarID;
			SimpleVariableInfo restPointEmptyVarID;

			SimpleVariableInfo allowHandTrackingPoseVarID;

			SimpleVariableInfo relaxedThumbVarID;
			SimpleVariableInfo relaxedPointerVarID;
			SimpleVariableInfo relaxedMiddleVarID;
			SimpleVariableInfo relaxedRingVarID;
			SimpleVariableInfo relaxedPinkyVarID;

			SimpleVariableInfo straightThumbVarID;
			SimpleVariableInfo straightPointerVarID;
			SimpleVariableInfo straightMiddleVarID;
			SimpleVariableInfo straightRingVarID;
			SimpleVariableInfo straightPinkyVarID;

			bool headMenuRequested = false;
			bool headMenuRequestedHold = false;
			bool showOverlayInfo = false;
			bool requiresHoldWeaponPrompt = false;
			Framework::SocketID physicalViolenceDirSocket;
		};
		bool alwaysAppearToHoldGun = false;
		Name handHoldGunVarID;
		Name handPressTriggerVarID;
		Name handAwayTriggerVarID;
		Name handHoldObjectVarID;
		Name handHoldObjectSizeVarID;
		Name objectsHeldObjectSizeVarID; // just name
		Name handFistVarID;
		Name handGrabVarID;
		Name handGrabDialVarID;
		Name handStraightVarID;
		Name handPortActiveVarID;
		Name handDisplayActiveNormalVarID;
		Name handDisplayActiveExtraVarID;
		Name restPointHandObjectVarID; // this is pointer to hand object inside rest point
		Name restPointEquipmentObjectVarID; // this is pointer to the main equipment
		Name restPointTVarID;
		Name restPointEmptyVarID;
		Name allowHandTrackingPoseVarID;
		Name relaxedThumbVarID;
		Name relaxedPointerVarID;
		Name relaxedMiddleVarID;
		Name relaxedRingVarID;
		Name relaxedPinkyVarID;
		Name straightThumbVarID;
		Name straightPointerVarID;
		Name straightMiddleVarID;
		Name straightRingVarID;
		Name straightPinkyVarID;
		HandState handStates[Hand::MAX];
		float blockControlsTimeLeft = 0.0f;

		Framework::SocketID energyInhaleSocket;

		int equipmentReleasedWithoutHoldingCount = 0; // if we hold, we reset this counter
		bool equipmentReleasedWithoutHoldingShowPrompt = false;

		struct Forearm
		{
			// in pilgrim
			Framework::SocketID forearmStartSocket;
			Framework::SocketID forearmEndSocket;
			// in hand
			Framework::SocketID handOverSocket;
		};

		Forearm forearms[2];

		struct BlendTimes
		{
			float handPoseBlendTime = 0.05f;
			float fingerBlendTime = 0.05f;
			float portFoldTime = 0.2f;
			float displayFoldTime = 0.5f;
			float restPointFoldTime = 0.2f;
		};

		BlendTimes normalBlendTimes;
		BlendTimes paralysedBlendTimes;
		BlendTimes useBlendTimes;

		//
		struct EquipmentControls
		{
			float use[2];
			float useAsUsable[2];
			float change[2];
			VectorInt2 dPad[2];
		};
		EquipmentControls equipmentControls;
		EquipmentControls equipmentControlsPrev;

		struct EXMControls
		{
			float use[2];
		};
		EXMControls exmControls;
		EXMControls exmControlsPrev;

		struct EXMRules
		{
			// what is allowed
			bool activating[2];
			bool deactivating[2];
			bool selecting[2];
		};
		EXMRules exmRules;

		struct Pocket
		{
			bool isActive = true;
			int pocketLevel = 0;
			Name name;
			Framework::SocketID socket;
			Optional<Hand::Type> side;
			Element highlight;
			int materialParamIndex = 0; // index in an array
			Array<Name> objectHoldSockets; // most important and fallbacks

			void set_in_pocket(Framework::IModulesOwner* _imo);
			Framework::IModulesOwner* get_in_pocket() const { return inPocket.get(); }

			Pocket() {}
			~Pocket();
			Pocket(Pocket const & _other);
			Pocket& operator=(Pocket const & _other);

		private: // we really want to avoid anything else changing it directly
			SafePtr<Framework::IModulesOwner> inPocket;

			void setup_in_pocket_collides_with_flags_for(Framework::IModulesOwner* _imo, bool _inPocket);
		};
		Array<Pocket> pockets;
		Optional<int> pocketsMaterialIndex;
		Name pocketsMaterialParam; // array (vec4)

		PilgrimOverlayInfo overlayInfo;
		bool overlayShowsActiveEXMs = false;

		struct InfiniteRegen
		{
			Framework::Room* checkedForRoom = nullptr;
			Energy healthRate = Energy::zero();
			float healthMB = 0.0f;
			Energy ammoRate = Energy::zero();
			float ammoMB = 0.0f;
		} infiniteRegen;

		//
		String collisionFlagsForUsedEquipment;
		String collisionFlagsForUsedEquipmentNonInteractive;
		CACHED_ Optional<Collision::Flags> useCollisionFlagsForUsedEquipment;
		CACHED_ Optional<Collision::Flags> useCollisionFlagsForUsedEquipmentNonInteractive;
		String collisionFlagsForUsedEquipmentToBeKept; // these flags will be kept as they are when pushing collision flags
		CACHED_ Optional<Collision::Flags> useCollisionFlagsForUsedEquipmentToBeKept;

		template <class PathClass>
		bool is_aiming_at_impl(Framework::IModulesOwner const * imo, PathClass const * _usePath, OUT_ float * _distance, float _maxOffPath, float _maxAngleOff) const;

		void advance_hands(float _deltaTime);

		void put_into_pocket(Pocket* _pocket, Framework::IModulesOwner* _object);

		void look_up_sockets();

	private:
		void create_equipment(bool _left = true, bool _right = true); // will destroy current equipment if any and create new one

	private:
		friend class GameScript::Elements::CreateMainEquipment;

		// these do not activate created items!
		Framework::IModulesOwner* create_equipment_internal(Hand::Type _hand, WeaponSetup const& _weaponSetup);
		Framework::IModulesOwner* create_equipment_internal(Hand::Type _hand, Framework::ItemType const* _itemType);

	private:
		struct AdvanceHandContext
		{
			ModulePilgrim* modulePilgrim;
			float deltaTime;
			int handIdx;
			Hand::Type idxAsHand;
			HandState & handState;
			SafePtr<Framework::IModulesOwner>& equipment;
			SafePtr<Framework::IModulesOwner>& mainEquipment;
			bool restPointUnfoldRequired = false;
			bool gunEquipment = false;
			bool gunEquipmentToGrab = false;
			ModuleEquipment* moduleEquipment = nullptr;
			Framework::IModulesOwner* hand = nullptr;

			AdvanceHandContext(
				ModulePilgrim* _modulePilgrim,
				float _deltaTime,
				int _handIdx,
				HandState & _handState,
				SafePtr<Framework::IModulesOwner>& _equipment,
				SafePtr<Framework::IModulesOwner>& _mainEquipment
			)
			:	modulePilgrim(_modulePilgrim)
			,	deltaTime(_deltaTime)
			,	handIdx(_handIdx)
			,	idxAsHand((Hand::Type)_handIdx)
			,	handState(_handState)
			,	equipment(_equipment)
			,	mainEquipment(_mainEquipment)
			{}
			
			void advance_physical_violence();
			void advance_late_grab();
			void advance_hand_state();
			void update_pocket_to_put_into();
			void update_equipment_state();
			void update_gun_equipment();
			void choose_pose();
				void choose_pose_paralysed();
				void choose_pose_idle();
				void choose_pose_normal();
				void choose_pose_rest_point();
			void update_pose();
			void set_pose_vars();
			void update_grabable();
			
			void post_advance();

		private:
			Framework::IModulesOwner* get_hand() { if (!hand) hand = modulePilgrim->get_hand(idxAsHand); return hand; }

			CustomModules::ItemHolder* find_item_holder_to_put_into() const;

			// helper functions that setup things
			void deploy_main_equipment(HandState & handState, SafePtr<Framework::IModulesOwner>& mainEquipment);
		};

		void keep_object_attached(Framework::IModulesOwner* _object, bool _vrAttachment = false);

		bool compare_permanent_exms() const;

		friend class GameScript::Elements::Pilgrim;
		friend class AI::Logics::BridgeShop;
	};

	template <typename Container>
	void ModulePilgrim::get_in_pockets(REF_ Container & _container, Optional<Hand::Type> const & _side, bool _orderByPocketLevel, bool _emptyPocketsToo)
	{
		if (_orderByPocketLevel)
		{
			for (int pocketLevel = PilgrimBlackboard::get_pocket_level(get_owner()); pocketLevel >= 0; --pocketLevel)
			{
				for_every(pocket, pockets)
				{
					if (pocket->pocketLevel == pocketLevel)
					{
						if (!_side.is_set() || (pocket->side.is_set() && _side.get() == pocket->side.get()))
						{
							if (auto* p = pocket->get_in_pocket())
							{
								_container.push_back(p);
							}
							else if (_emptyPocketsToo)
							{
								_container.push_back(nullptr);
							}
						}
					}
				}
			}
		}
		else
		{
			for_every(pocket, pockets)
			{
				if (!_side.is_set() || (pocket->side.is_set() && _side.get() == pocket->side.get()))
				{
					if (auto* p = pocket->get_in_pocket())
					{
						_container.push_back(p);
					}
					else if (_emptyPocketsToo)
					{
						_container.push_back(nullptr);
					}
				}
			}
		}
	}
};

