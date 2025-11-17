#pragma once

#include "..\..\framework\module\moduleGameplay.h"

namespace TeaForGodEmperor
{
	struct Damage;
	struct DamageInfo;

	class ModuleGameplay
	: public Framework::ModuleGameplay
	{
		FAST_CAST_DECLARE(ModuleGameplay);
		FAST_CAST_BASE(Framework::ModuleGameplay);
		FAST_CAST_END();

		typedef Framework::ModuleGameplay base;
	public:
		ModuleGameplay(Framework::IModulesOwner* _owner);
		virtual ~ModuleGameplay();

	public:
		virtual bool on_death(Damage const& _damage, DamageInfo const & _damageInfo) { return true; } // continue with normal death
	};
};

