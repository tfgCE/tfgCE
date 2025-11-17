#pragma once	

#include "..\..\core\containers\arrayStatic.h"
#include "..\..\core\memory\safeObject.h"
#include "..\..\core\types\optional.h"
#include "..\..\core\types\string.h"

namespace TeaForGodEmperor
{
	class WeaponPart;

	namespace WeaponStatAffection // load from xml as "affects"
	{
		enum Type
		{
			Unknown,
			IncBetter, // increasing and better
			IncMuchBetter,
			DecWorse, // decreasing and worse
			DecMuchWorse,
			DecBetter,
			DecMuchBetter,
			IncWorse,
			IncMuchWorse,
			Set,
			Special,
		};

		inline Type parse(String const & _text)
		{
			if (_text == TXT("increasing better")) return IncBetter;
			if (_text == TXT("increasing much better")) return IncMuchBetter;
			if (_text == TXT("increasing worse")) return IncWorse;
			if (_text == TXT("increasing much worse")) return IncMuchWorse;
			if (_text == TXT("decreasing better")) return DecBetter;
			if (_text == TXT("decreasing much better")) return DecMuchBetter;
			if (_text == TXT("decreasing worse")) return DecWorse;
			if (_text == TXT("decreasing much worse")) return DecMuchWorse;
			if (_text == TXT("inc better")) return IncBetter;
			if (_text == TXT("inc much better")) return IncMuchBetter;
			if (_text == TXT("inc worse")) return IncWorse;
			if (_text == TXT("inc much worse")) return IncMuchWorse;
			if (_text == TXT("dec better")) return DecBetter;
			if (_text == TXT("dec much better")) return DecMuchBetter;
			if (_text == TXT("dec worse")) return DecWorse;
			if (_text == TXT("dec much worse")) return DecMuchWorse;
			if (_text == TXT("special")) return Special;
			if (_text == TXT("set")) return Set;
			error(TXT("affection not recognised \"%S\""), _text.to_char());
			return Unknown;
		}

		inline Type negate_effect(Type _t) // if was increasing, will be still increasing but will switch from better to worse
		{
			if (_t == IncBetter) return DecWorse;
			if (_t == IncMuchBetter) return DecMuchWorse;
			if (_t == IncWorse) return DecBetter;
			if (_t == IncMuchWorse) return DecMuchBetter;
			if (_t == DecBetter) return IncWorse;
			if (_t == DecMuchBetter) return IncMuchWorse;
			if (_t == DecWorse) return IncBetter;
			if (_t == DecMuchWorse) return IncMuchBetter;
			return _t;
		}

		void initialise_static();
		void ready_to_use();
		void close_static();


		String const& to_ui_symbol(Type _t); // symbol
	};

	struct WeaponsStatAffectedByPart
	{
		WeaponPart const * weaponPart = nullptr; // just reference
		WeaponStatAffection::Type how = WeaponStatAffection::Unknown;

		WeaponsStatAffectedByPart() {}
		~WeaponsStatAffectedByPart() {}

		bool operator == (WeaponsStatAffectedByPart const& _other) const { return weaponPart == _other.weaponPart; } // same part should not affect same stat in two ways
		bool operator != (WeaponsStatAffectedByPart const& _other) const { return !(operator==(_other)); }
	};

	template <typename Class>
	struct WeaponStatInfo
	{
		Class value;
		ArrayStatic<WeaponsStatAffectedByPart, 8> affectedBy;
		ArrayStatic<WeaponsStatAffectedByPart, 8> notAffectedBy; // if was cleared by "setting/maxing"

		WeaponStatInfo() {}
		WeaponStatInfo(Class const& _value) : value(_value) {}
		~WeaponStatInfo() {}

		WeaponStatInfo& operator=(Class const& _value) { value = _value; return *this; }
		WeaponStatInfo& operator=(WeaponStatInfo const& _other) { value = _other.value; affectedBy = _other.affectedBy; notAffectedBy = _other.notAffectedBy; return *this; }

		void reset();

		void clear_affection();
		void move_to_not_affected_by();
		void add_affection(WeaponPart const* _part, WeaponStatAffection::Type _how);

		bool operator == (WeaponStatInfo const& _other) const { return value == _other.value && affectedBy == _other.affectedBy && notAffectedBy == _other.notAffectedBy; }
		bool operator != (WeaponStatInfo const& _other) const { return !(operator==(_other)); }
	};

	template <typename Class>
	struct WeaponPartStatInfo
	{
		Optional<Class> value;
		WeaponStatAffection::Type how = WeaponStatAffection::Unknown;

		WeaponPartStatInfo() {}
		WeaponPartStatInfo(Class const& _value) : value(_value) {}
		~WeaponPartStatInfo() {}

		bool load_from_xml(IO::XML::Node const* _node, tchar const* _value);

		void set_auto_positive_is_better();
		void set_auto_negative_is_better();
		void set_auto_coef_more_than_1_is_better();
		void set_auto_coef_less_than_1_is_better();
		void set_auto_set();
		void set_auto_special();
	};

	template <typename Class>
	struct WeaponPartTypeStatInfo
	{
		Class value;
		WeaponStatAffection::Type how = WeaponStatAffection::Unknown;

		WeaponPartTypeStatInfo() {}
		WeaponPartTypeStatInfo(Class const& _value) : value(_value) {}
		~WeaponPartTypeStatInfo() {}

		void set_auto_positive_is_better();
		void set_auto_negative_is_better();
		void set_auto_coef_more_than_1_is_better();
		void set_auto_coef_less_than_1_is_better();
		void set_auto_set();
		void set_auto_special();
	};
};

