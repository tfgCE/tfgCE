#pragma once

#include "me_armable.h"

#include "..\..\..\utils\interactiveButtonHandler.h"

#include "..\..\..\..\core\math\math.h"
#include "..\..\..\..\framework\ai\aiPerceptionRequest.h"
#include "..\..\..\..\framework\module\moduleData.h"

namespace Framework
{
	class Font;
};

namespace TeaForGodEmperor
{
	class ModulePilgrim;

	namespace ModuleEquipments
	{
		class MineData;

		class Mine
		: public Armable
		{
			FAST_CAST_DECLARE(Mine);
			FAST_CAST_BASE(Armable);
			FAST_CAST_END();

			typedef Armable base;
		public:
			static Framework::RegisteredModule<Framework::ModuleGameplay> & register_itself();

			Mine(Framework::IModulesOwner* _owner);
			virtual ~Mine();

			void trigger();

		public:
			implement_ bool is_armed() const { return !safe; }
			implement_ Framework::IModulesOwner* get_armed_by() const { return armedBy.get(); }

		public: // ModuleEquipment
			override_ void advance_post_move(float _deltaTime);
			override_ void advance_user_controls();

			override_ Optional<Energy> get_primary_state_value() const;
			override_ Optional<Energy> get_energy_state_value() const { return get_primary_state_value(); }

			override_ void display_extra(::Framework::Display* _display, bool _justStarted);

		public: // ModuleGameplay
			override_ bool on_death(Damage const& _damage, DamageInfo const & _damageInfo);

		protected: // Module
			override_ void reset();
			override_ void setup_with(Framework::ModuleData const * _moduleData);

			override_ void initialise();
			override_ void activate();

		protected:
			MineData const * mineData;

			// setup
			float detonationTime = 0.25f;
			float detonationTimeFastColliding = 0.25f;
			float proximityRange = 0.8f;
			float proximitySpeed = 0.2f;
			Collision::Flags detectFlags = Collision::Flags::none();
			Collision::Flags confirmRayFlags = Collision::Flags::none();

			SafePtr<Framework::IModulesOwner> armedBy;
			InteractiveButtonHandler interactiveButtonHandler;

			Optional<bool> switchedSides;

			bool safe = true;
			bool detonated = false;

			bool collisionForArmed = false;

			Optional<float> timeToDetonate;
			Optional<float> timeToProximityCheck;

			SimpleVariableInfo allowArmingVar;
			SimpleVariableInfo armedVar;

			void explode(Framework::IModulesOwner* _instigator = nullptr);
		};

		class MineData
		: public ModuleEquipmentData
		{
			FAST_CAST_DECLARE(MineData);
			FAST_CAST_BASE(ModuleEquipmentData);
			FAST_CAST_END();

			typedef ModuleEquipmentData base;
		public:
			MineData(Framework::LibraryStored* _inLibraryStored);
			virtual ~MineData();

		public: // Framework::ModuleData
			override_ bool load_from_xml(IO::XML::Node const * _node, Framework::LibraryLoadingContext & _lc);
			override_ bool prepare_for_game(Framework::Library* _library, Framework::LibraryPrepareContext& _pfgContext);
			override_ void prepare_to_unload();
			override_ bool read_parameter_from(IO::XML::Attribute const * _attr, Framework::LibraryLoadingContext & _lc);

		private:
			bool alwaysExplodeOnDeath = false;
			Name allowArmingVar;
			Name armedVar;
			Name buttonIdVar;
			Name onButtonIdVar;
			Framework::UsedLibraryStored<Framework::Font> font;
			VectorInt2 textAt = VectorInt2(33, 54);
			VectorInt2 textScale = VectorInt2::one;

			Colour colourSafe = Colour::green;
			Colour colourArmed = Colour::red;

			Name armSound;
			Name disarmSound;

			friend class Mine;
		};
	};
};

