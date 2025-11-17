#pragma once

#include "..\..\core\math\math.h"
#include "..\..\core\random\randomNumber.h"

class SimpleVariableStorage;

namespace Random
{
	class Generator;
};

namespace Framework
{
	class Module;
	class ModuleData;
};

namespace TeaForGodEmperor
{
	class PhysicalMaterial;

	struct EnergyCoef;
	struct EnergyRange;

	struct Energy
	{
		static Energy s_zero;
		static Energy s_one;
		static const int decimals = 2;
		static const int factor = 100;

		static Energy smallest() { return Energy::pure(1); }

		static Energy const & zero() { return s_zero; }
		static Energy const & one() { return s_one; }
		static Energy half_of(Energy const & _value) { return pure(_value.energy / 2); }
		static bool is_zero(Energy const & _value) { return _value.is_zero(); }
		static Energy get_random(Random::Generator & _generator, Energy const & _min, Energy const & _max);
		static Energy parse_from(String const & _string, Energy const & _defValue) { return _string.is_empty()? _defValue : parse(_string); }

		static Energy pure(int _energy) { Energy e; e.energy = _energy; return e; }
		int get_pure() const { return energy; }

		Energy() {}
		explicit Energy(int _main, int _decimals = 0) : energy(_main * factor + (_main >= 0? abs(_decimals) : -abs(_decimals))) {}
		explicit Energy(float _f);
		static Energy positive(int _main, int _decimals) { return pure(abs(_main) * factor + abs(_decimals)); }
		static Energy negative(int _main, int _decimals) { return pure(-abs(_main) * factor - abs(_decimals)); }

		bool is_zero() const { return energy == 0; }
		bool is_negative() const { return energy < 0; }
		bool is_positive() const { return energy > 0; }

		Energy operator - () const { return pure(-energy); }
		Energy operator + (Energy const & _other) const { return pure(energy + _other.energy); }
		Energy operator - (Energy const & _other) const { return pure(energy - _other.energy); }
		Energy operator * (Energy const & _other) const { return pure(energy * _other.energy / factor); }
		Energy operator / (Energy const & _other) const { return pure(energy * factor / _other.energy); }
		Energy operator * (int _mul) const { return pure(energy * _mul); }
		Energy operator / (int _div) const { return pure(energy / _div); }

		Energy operator += (Energy const & _other) { energy += _other.energy; return *this; }
		Energy operator -= (Energy const & _other) { energy -= _other.energy; return *this; }

		inline bool operator == (Energy const & _other) const { return energy == _other.energy; }
		inline bool operator != (Energy const & _other) const { return energy != _other.energy; }
		inline bool operator < (Energy const & _other) const { return energy < _other.energy; }
		inline bool operator <= (Energy const & _other) const { return energy <= _other.energy; }
		inline bool operator > (Energy const & _other) const { return energy > _other.energy; }
		inline bool operator >= (Energy const & _other) const { return energy >= _other.energy; }

		bool load_from_xml(IO::XML::Node const* _node) { return load_from_xml(_node, nullptr); }
		bool load_from_xml(IO::XML::Attribute const* _attr);
		bool load_from_xml(IO::XML::Node const * _node, tchar const * _attr);
		bool load_from_attribute_or_from_child(IO::XML::Node const* _node, tchar const* _name);

		static Energy load_from_attribute_or_from_child(IO::XML::Node const* _node, tchar const* _name, Energy const& _default);

		static bool can_get_from_module_data(Framework::Module * _module, Framework::ModuleData const * _moduleData, Name const & _name);
		static Energy get_from_module_data(Framework::Module * _module, Framework::ModuleData const * _moduleData, Name const & _name, Energy const & _default = Energy());
		static Energy get_from_storage(SimpleVariableStorage const & _storage, Name const & _name, Energy const & _default = Energy());
		static Energy parse(tchar const * _txt) { return parse(String(_txt)); }
		static Energy parse(String const & _txt);

		Energy timed(float _deltaTime, REF_ float & _missingBit) const; // missing bit is part that didn't make it fully, will be counted in next call
		Energy adjusted(EnergyCoef const & _coef) const; // * _coef
		Energy adjusted_rev_one(EnergyCoef const & _coef) const; // * (1.0f - _coef)
		Energy adjusted_clamped(EnergyCoef const & _coef) const; // * clamp(_coef, 0.0f, 1.0f)
		Energy adjusted_inv(EnergyCoef const & _coef) const; // / _coef
		Energy adjusted_plus_one(EnergyCoef const& _coef) const; // * (1.0 + _coef)
		Energy adjusted_plus_one_inv(EnergyCoef const& _coef) const; // / (1.0 + _coef)
		Energy mul(float _coef) const; // to avoid direct operations with float 
		Energy mul_with_min(float _coef, Energy const & _minValue) const; // to avoid direct operations with float if >= minValue, won't get below it
		Energy div(float _coef) const; // to avoid direct operations with float 
		Energy div_safe(float _coef) const; // to avoid direct operations with float 
		float div_to_float(Energy const& _other) const { return _other.energy == 0? 0.0f : (float)energy / (float)_other.energy; }
		EnergyCoef div_to_coef(Energy const& _other) const;
		Energy halved() const { return pure(energy / 2); }
		Energy floored() const { return pure(energy > 0? (energy - (energy % factor)) : (energy + (-energy % factor))); }
		Energy decimals_only() const { return pure(energy % factor); }

		EnergyCoef as_part_of(Energy const& _other, bool _clampTo01 = false) const;

		String as_string(int _decimals = -1) const;
		String as_string_relative(int _decimals = -1) const;
		String as_string_auto_decimals() const;
		float as_float() const { return (float)energy / (float)factor; }
		int as_int() const { return energy / factor; }

		String format(String const & _format) const;

	private:
		friend struct EnergyCoef;

		int energy = 0;
	};

	struct EnergyCoef
	{
		static EnergyCoef s_zero;
		static EnergyCoef s_half;
		static EnergyCoef s_one;
		static EnergyCoef s_onePercent;

		static const int decimals = 4;
		static const int factor = 10000;

		static EnergyCoef const& zero() { return s_zero; }
		static EnergyCoef const& half() { return s_half; }
		static EnergyCoef const& one() { return s_one; }
		static EnergyCoef const& one_percent() { return s_onePercent; }
		static EnergyCoef percent(int _percent) { return s_onePercent * _percent; }

		static EnergyCoef half_of(EnergyCoef const& _value) { return pure(_value.coef / 2); }
		static bool is_zero(EnergyCoef const& _value) { return _value.is_zero(); }
		static EnergyCoef get_random(Random::Generator& _generator, EnergyCoef const& _min, EnergyCoef const& _max);
		static EnergyCoef parse_from(String const& _string, EnergyCoef const& _defValue) { return _string.is_empty() ? _defValue : parse(_string); }

		static EnergyCoef positive(int _main, int _decimals) { return pure(abs(_main) * factor + abs(_decimals)); }
		static EnergyCoef negative(int _main, int _decimals) { return pure(-abs(_main) * factor - abs(_decimals)); }

		static EnergyCoef pure(int _coef) { EnergyCoef e; e.coef = _coef; return e; }
		int get_pure() const { return coef; }

		EnergyCoef() {}
		EnergyCoef(int _main, int _decimals = 0) : coef(_main * factor + (_main >= 0 ? abs(_decimals) : -abs(_decimals))) {}
		explicit EnergyCoef(float _f);

		EnergyCoef operator - () const { return pure(-coef); }
		EnergyCoef operator + (EnergyCoef const& _other) const { return pure(coef + _other.coef); }
		EnergyCoef operator - (EnergyCoef const& _other) const { return pure(coef - _other.coef); }
		EnergyCoef operator * (EnergyCoef const& _other) const { return pure(coef * _other.coef / factor); }
		EnergyCoef operator / (EnergyCoef const& _other) const { return pure(coef * factor / _other.coef); }
		EnergyCoef operator * (int _mul) const { return pure(coef * _mul); }
		EnergyCoef operator / (int _div) const { return pure(coef / _div); }
		EnergyCoef operator * (float _val) const { return pure((int)((float)coef * _val)); }

		EnergyCoef operator += (EnergyCoef const& _other) { coef += _other.coef; return *this; }
		EnergyCoef operator -= (EnergyCoef const& _other) { coef -= _other.coef; return *this; }
		bool operator < (EnergyCoef const& _other) const { return coef < _other.coef; }
		bool operator <= (EnergyCoef const& _other) const { return coef <= _other.coef; }
		bool operator > (EnergyCoef const& _other) const { return coef > _other.coef; }
		bool operator >= (EnergyCoef const& _other) const { return coef >= _other.coef; }
		bool operator == (EnergyCoef const& _other) const { return coef == _other.coef; }
		bool operator != (EnergyCoef const& _other) const { return coef != _other.coef; }

		EnergyCoef mul(float _coef) const { return *this * _coef; } // consistent with energy

		EnergyCoef adjusted(EnergyCoef const& _coef) const { return *this * _coef; }
		EnergyCoef adjusted_clamped(EnergyCoef const& _coef) const { return *this * clamp(_coef, EnergyCoef::zero(), EnergyCoef::one()); }
		EnergyCoef adjusted_to_one(EnergyCoef const& _coef) const { return *this + (EnergyCoef::one() - *this) * _coef; } // this + (1.0f - this) * _coef

		float adjust_plus_one(float _value) const { return (*this + one()).as_float() * _value; }

		bool is_zero() const { return coef == 0; }
		bool is_one() const { return *this == s_one; }
		bool is_negative() const { return coef < 0; }
		bool is_positive() const { return coef > 0; }

		float as_float() const { return (float)coef / (float)factor; }
		int as_int() const { return coef / factor; }
		int as_int_percentage() const { return (coef * 100) / factor; }

		bool load_from_xml(IO::XML::Node const* _node) { return load_from_xml(_node, nullptr); }
		bool load_from_xml(IO::XML::Attribute const* _attr);
		bool load_from_xml(IO::XML::Node const * _node, tchar const * _attr, bool _requiresPercentageSign = false);
		bool load_from_attribute_or_from_child(IO::XML::Node const* _node, tchar const* _name, bool _requiresPercentageSign = false);

		String as_string(int _decimals = -1) const;
		String as_string_auto_decimals() const;

		String as_string_percentage(int _decimals = 0) const;
		String as_string_percentage_relative(int _decimals = 0) const;
		String as_string_percentage_auto_decimals() const;

		static EnergyCoef load_from_attribute_or_from_child(IO::XML::Node const * _node, tchar const * _name, EnergyCoef const & _default = EnergyCoef(1), bool _requiresPercentageSign = false);
		static EnergyCoef get_from_module_data(Framework::Module * _module, Framework::ModuleData const * _moduleData, Name const & _name, EnergyCoef const & _default = EnergyCoef(1), bool _requiresPercentageSign = false);
		static EnergyCoef get_from_storage(SimpleVariableStorage const & _storage, Name const& _name, EnergyCoef  const& _default = EnergyCoef());
		static EnergyCoef parse(tchar const * _txt, bool _requiresPercentageSign = false) { return parse(String(_txt), _requiresPercentageSign); }
		static EnergyCoef parse(String const & _txt, bool _requiresPercentageSign = false);

	private:
		friend struct Energy;

		int coef = 0;
	};

	struct EnergyRange
	{
		static EnergyRange empty;

		Energy min;
		Energy max;

		EnergyRange() {}
		explicit EnergyRange(Energy const & _at) : min(_at), max(_at) {}
		explicit EnergyRange(Energy const & _min, Energy const & _max) : min(_min), max(_max) {}

		static EnergyRange parse_from(String const& _string, EnergyRange const& _defValue) { return _string.is_empty() ? _defValue : parse(_string); }

		bool is_empty() const { return min > max; }
		bool does_contain(Energy const & _e) const { return _e >= min && _e <= max; }
		bool does_contain_min_exc_max_inc(Energy const & _e) const { return (min == max? _e >= min : _e > min) && _e <= max; } // min exclusive, max inclusive

		Energy get_random() const;
		Energy get_random(Random::Generator & rg) const;

		static bool can_get_from_module_data(Framework::Module* _module, Framework::ModuleData const* _moduleData, Name const& _name);
		static EnergyRange get_from_module_data(Framework::Module* _module, Framework::ModuleData const* _moduleData, Name const& _name, EnergyRange const& _default = EnergyRange());
		static EnergyRange get_from_storage(SimpleVariableStorage const & _storage, Name const& _name, EnergyRange const& _default = EnergyRange());

		bool load_from_xml(IO::XML::Node const * _node, tchar const * _attr);
		bool load_from_string(String const & _string);
		bool load_from_xml(IO::XML::Node const* _node) { return load_from_xml(_node, nullptr); }
		bool load_from_xml(IO::XML::Attribute const* _attr) { return load_from_string(_attr->get_as_string()); }

		static EnergyRange parse(tchar const* _txt) { return parse(String(_txt)); }
		static EnergyRange parse(String const& _txt);

		String as_string(int _decimals = -1) const;
	};

	struct EnergyCoefRange
	{
		static EnergyCoefRange empty;

		EnergyCoef min;
		EnergyCoef max;

		EnergyCoefRange() {}
		explicit EnergyCoefRange(EnergyCoef const & _at) : min(_at), max(_at) {}
		explicit EnergyCoefRange(EnergyCoef const & _min, EnergyCoef const & _max) : min(_min), max(_max) {}

		static EnergyCoefRange parse_from(String const& _string, EnergyCoefRange const& _defValue) { return _string.is_empty() ? _defValue : parse(_string); }

		bool is_empty() const { return min > max; }
		bool does_contain(EnergyCoef const & _e) const { return _e >= min && _e <= max; }
		bool does_contain_min_exc_max_inc(EnergyCoef const & _e) const { return (min == max? _e >= min : _e > min) && _e <= max; } // min exclusive, max inclusive

		EnergyCoef get_random() const;
		EnergyCoef get_random(Random::Generator& rg) const;

		bool load_from_xml(IO::XML::Node const * _node, tchar const * _attr);
		bool load_from_string(String const & _string);

		static EnergyCoefRange parse(tchar const* _txt) { return parse(String(_txt)); }
		static EnergyCoefRange parse(String const& _txt);

		String as_string(int _decimals = -1) const;
		String as_string_percentage(int _decimals = 0) const;
	};
};

DECLARE_REGISTERED_TYPE(TeaForGodEmperor::Energy);
DECLARE_REGISTERED_TYPE(TeaForGodEmperor::EnergyCoef);
DECLARE_REGISTERED_TYPE(TeaForGodEmperor::EnergyRange);
DECLARE_REGISTERED_TYPE(TeaForGodEmperor::EnergyCoefRange);

template <>
inline TeaForGodEmperor::Energy mod_raw<TeaForGodEmperor::Energy>(TeaForGodEmperor::Energy const _a, TeaForGodEmperor::Energy const _b)
{
	return TeaForGodEmperor::Energy::pure(_a.get_pure() % _b.get_pure());
}

template <>
inline bool load_value_from_xml<TeaForGodEmperor::Energy>(REF_ TeaForGodEmperor::Energy & _a, IO::XML::Node const * _node, tchar const * _valueName)
{
	if (_node)
	{
		if (auto * attr = _node->get_attribute(_valueName))
		{
			_a.load_from_xml(_node, _valueName);
			return true;
		}
		if (auto * node = _node->first_child_named(_valueName))
		{
			return _a.load_from_xml(node);
		}
	}
	return false;
}

template <>
inline bool load_value_from_xml<TeaForGodEmperor::EnergyCoef>(REF_ TeaForGodEmperor::EnergyCoef & _a, IO::XML::Node const * _node, tchar const * _valueName)
{
	if (_node)
	{
		if (auto * attr = _node->get_attribute(_valueName))
		{
			_a.load_from_xml(_node, _valueName);
			return true;
		}
		if (auto * node = _node->first_child_named(_valueName))
		{
			return _a.load_from_xml(node);
		}
	}
	return false;
}

template <>
inline bool load_value_from_xml<TeaForGodEmperor::EnergyRange>(REF_ TeaForGodEmperor::EnergyRange & _a, IO::XML::Node const * _node, tchar const * _valueName)
{
	if (_node)
	{
		if (auto * attr = _node->get_attribute(_valueName))
		{
			_a.load_from_xml(_node, _valueName);
			return true;
		}
		if (auto * node = _node->first_child_named(_valueName))
		{
			return _a.load_from_string(node->get_text());
		}
	}
	return false;
}

template <>
inline void set_to_default<TeaForGodEmperor::Energy>(TeaForGodEmperor::Energy& _object)
{
	_object = TeaForGodEmperor::Energy::zero();
}

template <>
inline void set_to_default<TeaForGodEmperor::EnergyRange>(TeaForGodEmperor::EnergyRange& _object)
{
	_object = TeaForGodEmperor::EnergyRange::empty;
}

template <>
inline void set_to_default<TeaForGodEmperor::EnergyCoef>(TeaForGodEmperor::EnergyCoef& _object)
{
	_object = TeaForGodEmperor::EnergyCoef::zero();
}

//

template <>
inline TeaForGodEmperor::EnergyCoef mod_raw<TeaForGodEmperor::EnergyCoef>(TeaForGodEmperor::EnergyCoef const _a, TeaForGodEmperor::EnergyCoef const _b)
{
	return TeaForGodEmperor::EnergyCoef::pure(_a.get_pure() % _b.get_pure());
}

//

DECLARE_REGISTERED_TYPE(Random::Number<TeaForGodEmperor::Energy>);
DECLARE_REGISTERED_TYPE(Random::Number<TeaForGodEmperor::EnergyCoef>);

//

// specialisations
template<>
class Random::NumberUtilsFor<TeaForGodEmperor::Energy>
: public TeaForGodEmperor::Energy
{};

template<>
class Random::NumberUtilsFor<TeaForGodEmperor::EnergyCoef>
: public TeaForGodEmperor::EnergyCoef
{};

template<>
inline TeaForGodEmperor::Energy half_of(TeaForGodEmperor::Energy const _value)
{
	return TeaForGodEmperor::Energy::half_of(_value);
}

template <>
inline TeaForGodEmperor::Energy default_value<TeaForGodEmperor::Energy>()
{
	return TeaForGodEmperor::Energy::zero();
}

template <>
inline TeaForGodEmperor::EnergyRange default_value<TeaForGodEmperor::EnergyRange>()
{
	return TeaForGodEmperor::EnergyRange::empty;
}
