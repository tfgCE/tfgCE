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
	class ModulePilgrim;
	
	/**
	 *	If its health reaches zero (dies), the leak bursts open and increases a variable in owner (where it is attached to)
	 */
	class ModuleCorrosionLeak
	: public ModuleGameplay
	{
		FAST_CAST_DECLARE(ModuleCorrosionLeak);
		FAST_CAST_BASE(ModuleGameplay);
		FAST_CAST_END();

		typedef ModuleGameplay base;
	public:
		static Framework::RegisteredModule<Framework::ModuleGameplay> & register_itself();

		ModuleCorrosionLeak(Framework::IModulesOwner* _owner);
		virtual ~ModuleCorrosionLeak();

		bool is_leaking() const { return isLeaking; }
		void leak_now(DamageInfo const& _damageInfo);

	protected: // Module
		override_ void reset();
		override_ void setup_with(Framework::ModuleData const * _moduleData);

		override_ void initialise();

	public: // ModuleGameplay
		override_ bool on_death(Damage const& _damage, DamageInfo const & _damageInfo);

	protected:
		bool isLeaking = false;

		Name leakVariable;
		Name leakInstigatorVariable;
	};
};

