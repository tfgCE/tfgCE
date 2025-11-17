#pragma once

#include "..\..\core\globalDefinitions.h"
#include "..\..\core\types\optional.h"

struct Name;
struct String;

namespace TeaForGodEmperor
{
	struct Energy;
	struct EnergyCoef;

	namespace BuildStatsInfo
	{
		struct Context
		{
			Optional<int> valueColumnWidth;
		};
		void add_info(REF_ String& _string, Name const& _locStr, Context const & _context = Context());
		void add_info(REF_ String& _string, String const& _value, Context const& _context = Context());
		void add_info(REF_ String& _string, Name const& _locStr, String const& _value, Context const& _context = Context());
		void add_info(REF_ String& _string, Name const& _locStr, String const& _value, String const& _penalty, Context const& _context = Context());
		void add_info(REF_ String& _string, Name const& _locStr, Optional<bool> const & _value, Context const& _context = Context());
		void add_energy_info_as_is(REF_ String& _string, Name const& _locStr, Energy const& _energy, Context const& _context = Context());
		void add_energy_info_as_is(REF_ String& _string, Name const& _locStr, Optional<Energy> const& _energy, Context const& _context = Context());
		void add_energy_info_as_is(REF_ String& _string, Name const& _locStr, Energy const& _energy, Energy const& _penalty, Context const& _context = Context());
		void add_energy_info_as_is(REF_ String& _string, Name const& _locStr, Optional<Energy> const& _energy, Optional<Energy> const& _penalty, Context const& _context = Context());
		void add_energy_info(REF_ String& _string, Name const& _locStr, Energy const& _energy, Context const& _context = Context());
		void add_energy_info(REF_ String& _string, Name const& _locStr, Optional<Energy> const& _energy, Context const& _context = Context());
		void add_energy_info(REF_ String& _string, Name const& _locStr, Energy const& _energy, Energy const& _penalty, Context const& _context = Context());
		void add_energy_info(REF_ String& _string, Name const& _locStr, Optional<Energy> const& _energy, Optional<Energy> const& _penalty, Context const& _context = Context());
		void add_energy_coef_info(REF_ String& _string, Name const& _locStr, EnergyCoef const& _energyCoef, Context const& _context = Context());
		void add_energy_coef_info(REF_ String& _string, Name const& _locStr, Optional<EnergyCoef> const& _energyCoef, Context const& _context = Context());
		void add_energy_coef_info(REF_ String& _string, Name const& _locStr, EnergyCoef const& _energyCoef, EnergyCoef const& _penalty, Context const& _context = Context());
		void add_energy_coef_info(REF_ String& _string, Name const& _locStr, Optional<EnergyCoef> const& _energyCoef, Optional<EnergyCoef> const& _penalty, Context const& _context = Context());
		void add_energy_coef_info_as_is(REF_ String& _string, Name const& _locStr, Optional<EnergyCoef> const& _energyCoef, Context const& _context = Context());
		void add_int_as_is(REF_ String& _string, Name const& _locStr, int const & _value, Context const& _context = Context());
		void add_int_as_is(REF_ String& _string, Name const& _locStr, Optional<int> const & _value, Context const& _context = Context());
		void add_float_info_as_is(REF_ String& _string, Name const& _locStr, float const& _value, Context const& _context = Context());
		void add_float_info_as_is(REF_ String& _string, Name const& _locStr, Optional<float> const& _value, Context const& _context = Context());
		void add_float_info_as_is(REF_ String& _string, Name const& _locStr, float const& _value, float const & _penalty, Context const& _context = Context());
		void add_float_info_as_is(REF_ String& _string, Name const& _locStr, Optional<float> const& _value, Optional<float>const& _penalty,Context const& _context = Context());
		void add_float_info(REF_ String& _string, Name const& _locStr, float const& _value, Context const& _context = Context());
		void add_float_info(REF_ String& _string, Name const& _locStr, Optional<float> const& _value, Context const& _context = Context());
		void add_float_info(REF_ String& _string, Name const& _locStr, float const& _value, float const & _penalty, Context const& _context = Context());
		void add_float_info(REF_ String& _string, Name const& _locStr, Optional<float> const& _value, Optional<float> const& _penalty, Context const& _context = Context());
		void add_float_coef_info(REF_ String& _string, Name const& _locStr, float const& _coef, Context const& _context = Context());
		void add_float_coef_info(REF_ String& _string, Name const& _locStr, Optional<float> const& _coef, Context const& _context = Context());
		void add_float_coef_info(REF_ String& _string, Name const& _locStr, float const& _coef, float const & _penalty, Context const& _context = Context());
		void add_float_coef_info(REF_ String& _string, Name const& _locStr, Optional<float> const& _coef, Optional<float> const & _penalty, Context const& _context = Context());
		void add_float_coef_info_as_is(REF_ String& _string, Name const& _locStr, Optional<float> const& _coef, Context const& _context = Context());
		
		void remove_trailing_end_lines(REF_ String& _string);
	};
};
