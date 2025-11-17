#pragma once

#include "..\..\..\framework\ai\aiLogicData.h"
#include "..\..\..\framework\ai\aiLogicWithLatentTask.h"
#include "..\..\..\framework\game\delayedObjectCreation.h"
#include "..\..\..\framework\game\gameInputProxy.h"
#include "..\..\..\framework\text\localisedString.h"

#include "..\..\..\core\memory\refCountObject.h"

#include "..\..\game\weaponSetup.h"
#include "..\..\pilgrimage\pilgrimageInstanceObserver.h"

namespace Framework
{
	class Display;
	class DisplayText;
	class ItemType;
	class SceneryType;
	interface_class IModulesOwner;
};

namespace TeaForGodEmperor
{
	namespace AI
	{
		namespace Logics
		{
			class ArmouryData;

			struct ArmouryPOIWall
			{
				int relColumn = 0; // relative column order to room centre (central wall is at 0, left is at -1, right is at 1)
				bool continuousLeftSide = false;
				bool continuousRightSide = false;
				Name leftBottomPOI;
				Name leftTopPOI;
				Name rightBottomPOI;
				Name rightTopPOI;

				RUNTIME_ int sortingOrder = 0; // the lower the sooner

				bool load_from_xml(IO::XML::Node const* _node);

				static int compare(void const* _a, void const* _b)
				{
					ArmouryPOIWall const & a = *plain_cast<ArmouryPOIWall>(_a);
					ArmouryPOIWall const & b = *plain_cast<ArmouryPOIWall>(_b);
					if (a.sortingOrder < b.sortingOrder) return A_BEFORE_B;
					if (a.sortingOrder > b.sortingOrder) return B_BEFORE_A;
					return A_AS_B;
				}
			};

			/**
			 *	Creates slots for weapons and actual weapons
			 *	Manages weapons for persistence
			 *	
			 *	For getting back to the ship, mission state's end moves all weapons into persistence.
			 *	When the armoury kicks in, it checks for any already existing weapons to avoid their creation.
			 */
			class Armoury
			: public ::Framework::AI::LogicWithLatentTask
			, public IPilgrimageInstanceObserver
			{
				FAST_CAST_DECLARE(Armoury);
				FAST_CAST_BASE(::Framework::AI::LogicWithLatentTask);
				FAST_CAST_BASE(IPilgrimageInstanceObserver);
				FAST_CAST_END();

				typedef ::Framework::AI::LogicWithLatentTask base;
			public:
				static ::Framework::AI::Logic* create_ai_logic(::Framework::AI::MindInstance* _mind, ::Framework::AI::LogicData const * _logicData) { return new Armoury(_mind, _logicData); }

			public:
				Armoury(::Framework::AI::MindInstance* _mind, ::Framework::AI::LogicData const * _logicData);
				virtual ~Armoury();

			public: // Logic
				implement_ void advance(float _deltaTime);

				implement_ void learn_from(SimpleVariableStorage & _parameters);

			protected:
				implement_ void on_pilgrimage_instance_switch(PilgrimageInstance const* _from, PilgrimageInstance const* _to);
				implement_ void on_pilgrimage_instance_end(PilgrimageInstance const* _pilgrimage, PilgrimageResult::Type _pilgrimageResult);

			private:
				static LATENT_FUNCTION(execute_logic);

				static LATENT_FUNCTION(main_page);

			private:
				ArmouryData const * armouryData = nullptr;
				Random::Generator rg;

				Framework::UsedLibraryStored<Framework::ItemType> weaponItemType;

				bool armouryRemainsEmpty = false;
				bool armouryIgnoreExistingWeapons = false;
				bool armouryInaccessible = false;
				bool armouryEnlarged = false;
				
				enum UpdatePersistence
				{
					No,
					TimeBased,
					Explicit
				};
				UpdatePersistence updatePersistence = UpdatePersistence::No;
				float timeLeftToUpdatePersistence = 0.0f;

				bool updateWeaponsRequired = false;

				struct Weapon
				{
					int armouryId; // should always be set, persistence should set armouryId for everything
					SafePtr<Framework::IModulesOwner> imo; // if empty, means it hasn't spawned yet
				};
				Array<Weapon> weapons; // these are weapons already created
				
				struct WeaponToCreate
				: public RefCountObject
				{
					int armouryId; // as above
					bool discard = false;
					VectorInt2 at; // where will it be put?
					RefCountObjectPtr<Framework::DelayedObjectCreation> doc;
					WeaponSetup setup;

					WeaponToCreate(Persistence* _persistence) : setup(_persistence) {}
				};
				Array<RefCountObjectPtr<WeaponToCreate>> weaponsToCreate; // will be used to update in "weapons"

				struct SlotPort
				{
					VectorInt2 at; // armoury coordinate
					bool temporary = false; // extra slot port that should be removed when not holding anything
					bool wasHoldingSomething = false;
					SafePtr<Framework::IModulesOwner> imo;
					RefCountObjectPtr<Framework::DelayedObjectCreation> doc;
				};
				Array<SlotPort> slotPorts;

				struct SlotPortWall
				{
					RangeInt columnRange = RangeInt::empty;
					SafePtr<Framework::Room> room;
					Vector3 leftBottomPOI;
					Vector3 leftTopPOI;
					Vector3 rightBottomPOI;
					Vector3 rightTopPOI;
				};
				Array<SlotPortWall> walls;
				RangeInt coveredColumns;

				Concurrency::SpinLock destroyWeaponLock;
				Array<SafePtr<Framework::IModulesOwner>> destroyWeapon;

				int docsPending = 0;

				void find_walls(Framework::Room* _room, int _inColumnDir = 0); // _inColumnDir - in which direction are we increasing columns, to sort walls appropriately

				void issue_create_required_slot_ports(); // all in valid range
				void issue_create_slot_port(VectorInt2 const & _at, bool _fromColumnRange = false);

				void issue_create_weapons(); // all in valid range

				void queue_doc(Framework::DelayedObjectCreation* doc);
				void process_docs();
			
				bool transfer_weapons_to_persistence(bool _includeWeaponsWithPlayer, bool _discardInvalid, bool _keepActive); // return true if something has changed
			};

			class ArmouryData
			: public ::Framework::AI::LogicData
			{
				FAST_CAST_DECLARE(ArmouryData);
				FAST_CAST_BASE(::Framework::AI::LogicData);
				FAST_CAST_END();

				typedef ::Framework::AI::LogicData base;
			public:
				static ::Framework::AI::LogicData* create_ai_logic_data() { return new ArmouryData(); }

				ArmouryData();
				virtual ~ArmouryData();

			public: // LogicData
				virtual bool load_from_xml(IO::XML::Node const * _node, ::Framework::LibraryLoadingContext & _lc);
				virtual bool prepare_for_game(::Framework::Library* _library, ::Framework::LibraryPrepareContext& _pfgContext);

			private:
				Framework::UsedLibraryStored<Framework::SceneryType> slotPortSceneryType;
				
				SimpleVariableStorage equipmentParameters;

				ArmouryPOIWall mainWall;
				ArmouryPOIWall leftWall;
				ArmouryPOIWall rightWall;

				WeaponSetupTemplate defaultWeaponSetupTemplate; // will be added to the armoury if no weapon available
				TagCondition defaultWeaponPartTypesTagged; // used to fill with random parts

				friend class Armoury;
			};
		};
	};
};