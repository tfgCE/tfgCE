#pragma once

#include "..\moduleGameplay.h"

#include "..\..\game\damage.h"

#include "..\..\..\framework\library\usedLibraryStored.h"

namespace Framework
{
	class DoorInRoom;
	class TemporaryObjectType;
};

namespace TeaForGodEmperor
{
	class ModulePilgrim;
	class ModuleExplosionData;

	namespace CustomModules
	{
		class Health;
	};

	class ModuleExplosion
	: public ModuleGameplay
	{
		FAST_CAST_DECLARE(ModuleExplosion);
		FAST_CAST_BASE(ModuleGameplay);
		FAST_CAST_END();

		typedef ModuleGameplay base;
	public:
		static Framework::RegisteredModule<Framework::ModuleGameplay> & register_itself();

		ModuleExplosion(Framework::IModulesOwner* _owner);
		virtual ~ModuleExplosion();

		Damage& access_damage() { return damage; }
		ContinuousDamage& access_continuous_damage() { return continuousDamage; }
		Range& access_range() { return range; }

		void force_full_damage_for(Framework::IModulesOwner* _imo);

	public: // Module
		override_ void setup_with(Framework::ModuleData const * _moduleData);
		override_ void reset();
		override_ void activate();

		override_ void advance_post_move(float _deltaTime);

	protected:
		struct CollectedToDamage
		{
			Framework::IModulesOwner const * object;
			Framework::Room const * inRoom; // if no hit, we should end in same room
			CustomModules::Health* health;
			Transform placementWS; // relative to starting room
			Vector3 locationWS; // relative to starting room
			Vector3 hitRelativeDir;
			bool forceDamage = false;
			bool occluded = false;
		};
		void collect_to_damage(Vector3 loc, Framework::Room* inRoom, float distanceLeft, ArrayStack<CollectedToDamage> & collected, Framework::DoorInRoom* throughDoor = nullptr, Transform const & roomTransform = Transform::identity);

		void do_damage(Vector3 loc, Framework::Room* inRoom);

	private:
		ModuleExplosionData const * moduleExplosionData = nullptr;

		ArrayStatic<SafePtr<Framework::IModulesOwner>, 16> forcedFullDamageFor;

		bool doExplosion = false;

		bool cantKillPilgrim = false;

		float impulseMassBase = 120.0f;
		float impulseSpeed = 1.0f;
		Range impulseMassCoefRange = Range(0.0f, 1.5f);

		float confussionDuration = 0.0f;

		Range range = Range(2.0f); // min is below which whole damage is taken
		Damage damage;
		ContinuousDamage continuousDamage;

		String explosionCollisionFlags;
		Optional<Collision::Flags> useExplosionCollisionFlags;
	};

	class ModuleExplosionData
	: public Framework::ModuleData
	{
		FAST_CAST_DECLARE(ModuleExplosionData);
		FAST_CAST_BASE(Framework::ModuleData);
		FAST_CAST_END();
		typedef Framework::ModuleData base;
	public:
		ModuleExplosionData(Framework::LibraryStored* _inLibraryStored);

	protected: // Framework::ModuleData
		override_ bool load_from_xml(IO::XML::Node const * _node, Framework::LibraryLoadingContext & _lc);
		override_ bool read_parameter_from(IO::XML::Attribute const * _attr, Framework::LibraryLoadingContext & _lc);
		override_ bool prepare_for_game(Framework::Library* _library, Framework::LibraryPrepareContext& _pfgContext);

	private:
		friend class ModuleExplosion;

		Range range = Range(2.0f); // min is below which whole damage is taken
	};
};

