#pragma once

#include "..\..\teaEnums.h"
#include "..\..\game\energy.h"

#include "..\..\..\core\concurrency\mrswLock.h"
#include "..\..\..\core\types\hand.h"

#include "..\moduleGameplay.h"

namespace Framework
{
	struct PresencePath;
	struct RelativeToPresencePlacement;
};

namespace TeaForGodEmperor
{
	class ModuleOrbItemData;
	class ModulePilgrim;
	
	namespace OrbItemSource
	{
		enum Type
		{
			Unknown,
			Kill
		};
	};

	/**
	 *	Processing energy quantum is a bit complicated
	 *		it is being processed in two places, both in ai logic
	 *		it requires processing to start and to end - this is lock
	 *		this is because different objects may differently handle energy (some may take both ammo and health and treat it as health)
	 */
	class ModuleOrbItem
	: public ModuleGameplay
	{
		FAST_CAST_DECLARE(ModuleOrbItem);
		FAST_CAST_BASE(ModuleGameplay);
		FAST_CAST_END();

		typedef ModuleGameplay base;
	public:
		static Framework::RegisteredModule<Framework::ModuleGameplay> & register_itself();

		ModuleOrbItem(Framework::IModulesOwner* _owner);
		virtual ~ModuleOrbItem();

		void set_source(OrbItemSource::Type _source) { source = _source; }

		bool was_taken() const { return taken; }
		bool process_taken(ModulePilgrim* _byPilgrim);

	public:
#ifdef WITH_CRYSTALITE
		void set_crystalite(int _crystalite);
		bool is_crystalite() const;
#endif

	public:
		virtual void advance_post_move(float _deltaTime);

	protected: // Module
		override_ void reset();
		override_ void setup_with(Framework::ModuleData const * _moduleData);

		override_ void initialise();

	protected:
		ModuleOrbItemData const * orbItemData = nullptr;

		mutable Concurrency::MRSWLock processingLock;

		ModulePilgrim* user = nullptr;

		bool taken = false;

		OrbItemSource::Type source = OrbItemSource::Unknown;

		float lifeScale = 1.0f; // scale due to life time - it fades out at the end
		Optional<float> finalScale;

		Name orbItemType;
		
		Range addVelocityImpulseInterval = Range::zero;
		Range addVelocityImpulse = Range::zero;
		float timeToAddVelocityImpulse = 0.0f;

#ifdef WITH_CRYSTALITE
		// crystalite
		int crystalite = 1;
#endif

		void mark_taken() { taken = true; }
	};

	class ModuleOrbItemData
	: public Framework::ModuleData
	{
		FAST_CAST_DECLARE(ModuleOrbItemData);
		FAST_CAST_BASE(Framework::ModuleData);
		FAST_CAST_END();
		typedef Framework::ModuleData base;
	public:
		ModuleOrbItemData(Framework::LibraryStored* _inLibraryStored);

	protected: // ModuleData
		override_ bool read_parameter_from(IO::XML::Node const * _node, Framework::LibraryLoadingContext & _lc);

	protected:	friend class ModuleOrbItem;
		struct ScalePair
		{
			float energy = 0.0f;
			float size = 0.0f;

			bool is_valid() const { return energy > 0.0f && size >= 0.0f; }

			static int compare(void const * _a, void const * _b)
			{
				ScalePair const * a = plain_cast<ScalePair>(_a);
				ScalePair const * b = plain_cast<ScalePair>(_b);
				if (a->energy < b->energy) return A_BEFORE_B;
				if (a->energy > b->energy) return B_BEFORE_A;
				return A_AS_B;
			}
		};
		Array<ScalePair> scaling;
	};
};

