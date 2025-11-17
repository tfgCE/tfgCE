#pragma once

#include "..\..\..\core\functionParamsStruct.h"

#include "..\..\..\framework\ai\aiMessageHandler.h"
#include "..\..\..\framework\interfaces\presenceObserver.h"
#include "..\..\..\framework\module\moduleCustom.h"

namespace TeaForGodEmperor
{
	namespace CustomModules
	{
		class HitIndicator
		: public Framework::ModuleCustom
		, public Framework::AI::IMessageHandler
		, public Framework::IPresenceObserver
		{
			FAST_CAST_DECLARE(HitIndicator);
			FAST_CAST_BASE(Framework::ModuleCustom);
			FAST_CAST_BASE(Framework::AI::IMessageHandler);
			FAST_CAST_BASE(Framework::IPresenceObserver);
			FAST_CAST_END();

			typedef Framework::ModuleCustom base;
		public:
			static Framework::RegisteredModule<Framework::ModuleCustom> & register_itself();

			HitIndicator(Framework::IModulesOwner* _owner);
			virtual ~HitIndicator();

			Colour get_global_desaturate() const { return globalDesaturate; }
			Colour get_global_desaturate_change_into() const { return globalDesaturateChangeInto; }

			struct IndicateParams
			{
				ADD_FUNCTION_PARAM(IndicateParams, Colour, colourOverride, with_colour_override);
				ADD_FUNCTION_PARAM(IndicateParams, float, delay, with_delay);
				ADD_FUNCTION_PARAM(IndicateParams, Framework::IModulesOwner*, tryBeingRelativeTo, try_being_relative_to);
				ADD_FUNCTION_PARAM_TRUE(IndicateParams, damage, is_damage);
				ADD_FUNCTION_PARAM_FALSE(IndicateParams, reversed, is_reversed);
				IndicateParams& not_damage() { damage = false; return *this; }
			};
			void indicate_relative_dir(float _damagePt, Vector3 const& _relativeDir, IndicateParams const & _params = IndicateParams());
			void indicate_relative_location(float _damagePt, Vector3 const& _locationWS, IndicateParams const& _params = IndicateParams());
			void indicate_location(float _damagePt, Vector3 const& _locationWS, IndicateParams const& _params = IndicateParams());

		public:	// Module
			override_ void reset();
			override_ void setup_with(Framework::ModuleData const * _moduleData);

			override_ void initialise();

		protected: // ModuleCustom
			implement_ void advance_post(float _deltaTime);

		public: // IMessageHandler
			implement_ void handle_message(Framework::AI::Message const & _message);

		public: // IPresenceObserver
			implement_ void on_presence_changed_room(Framework::ModulePresence* _presence, Framework::Room * _intoRoom, Transform const & _intoRoomTransform, Framework::DoorInRoom* _exitThrough, Framework::DoorInRoom* _enterThrough, Framework::DoorInRoomArrayPtr const * _throughDoors);
			implement_ void on_presence_removed_from_room(Framework::ModulePresence* _presence, Framework::Room* _room);
			implement_ void on_presence_destroy(Framework::ModulePresence* _presence);

		protected:
			Name appearanceName;
			float length = 0.1f;
			float time = 0.5f;
			float healthPulsePt = 0.0f;
			float hitImpulse = 0.0f;
			float hitImpulseTarget = 0.0f;
			float hitImpulseSustain = 0.0f;

			bool showHealth = true;
			bool showHitImpulse = true;

			Framework::AI::MindInstance* mind = nullptr;
			Framework::ModulePresence* observingPresence = nullptr;

			Colour globalDesaturate = Colour::none;
			Colour globalDesaturateChangeInto = Colour::none;

			struct Hit
			{
				Framework::ModulePresence* relativeToPresence = nullptr;
				Optional<Vector3> locRelative; // if set, loc is updated automatically every frame
				Optional<Vector3> dirRelative; // if set, dir is updated automatically every frame
				Optional<Vector3> loc;
				Optional<Vector3> dir;
				Colour colourOverride = Colour::none;
				float delay = 0.0f;
				float at = 0.0f; // goes from 0 to 1
				bool reversed = false;
			};

			Concurrency::SpinLock hitsLock = Concurrency::SpinLock(TXT("TeaForGodEmperor.CustomModules.HitIndicator.hitsLock"));
			Array<Hit> hits;

			void stop_observing_all_presence();

			void make_hit_being_relative(Hit& hit, Framework::IModulesOwner* _tryBeingRelativeTo);

		};
	};
};

