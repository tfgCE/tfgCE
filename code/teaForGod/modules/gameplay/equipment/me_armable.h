#pragma once

#include "..\..\gameplay\moduleEquipment.h"

namespace TeaForGodEmperor
{
	class ModulePilgrim;

	namespace ModuleEquipments
	{
		class Armable
		: public ModuleEquipment
		{
			FAST_CAST_DECLARE(Armable);
			FAST_CAST_BASE(ModuleEquipment);
			FAST_CAST_END();

			typedef ModuleEquipment base;
		public:
			Armable(Framework::IModulesOwner* _owner);
			virtual ~Armable();

		public:
			virtual bool is_armed() const = 0;
			virtual Framework::IModulesOwner* get_armed_by() const = 0;
		};
	};
};

