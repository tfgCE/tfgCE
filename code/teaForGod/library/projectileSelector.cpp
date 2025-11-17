#include "projectileSelector.h"

#include "library.h"

#include "..\modules\gameplay\equipment\me_gun.h"

#include "..\..\core\other\parserUtils.h"

#include "..\..\framework\library\usedLibraryStored.inl"

//

#ifdef AN_ALLOW_OPTIMIZE_OFF
#pragma optimize("", off)
#endif

//

using namespace TeaForGodEmperor;

//

// params
DEFINE_STATIC_NAME(coreKind); // of a single projectile
DEFINE_STATIC_NAME(damage); // of a single projectile
DEFINE_STATIC_NAME(totalDamage); // of the whole shot
DEFINE_STATIC_NAME(speed); // of a single projectile
DEFINE_STATIC_NAME(spread); // general spread of a shot
DEFINE_STATIC_NAME(number); // number of projectiles in a shot
DEFINE_STATIC_NAME(specialFeature); // has special feature of given type

// operations
DEFINE_STATIC_NAME_STR(eq, TXT("="));
DEFINE_STATIC_NAME_STR(eq2, TXT("=="));
DEFINE_STATIC_NAME_STR(neq, TXT("!="));
DEFINE_STATIC_NAME_STR(neq2, TXT("<>"));
DEFINE_STATIC_NAME_STR(gt, TXT(">"));
DEFINE_STATIC_NAME_STR(ge, TXT(">="));
DEFINE_STATIC_NAME_STR(lt, TXT("<"));
DEFINE_STATIC_NAME_STR(le, TXT("<="));

//

WeaponCoreKind::Type ProjectileSelectionSource::get_core_kind() const
{
	if (gun) return gun->get_core_kind();
	if (statsME) return statsME->coreKind;
	return WeaponCoreKind::None;
}

Energy ProjectileSelectionSource::get_single_projectile_damage() const
{
	if (gun) return gun->get_projectile_damage(true);
	if (statsME) return statsME->damage / statsME->projectilesPerShot;
	return Energy::zero();
}

Energy ProjectileSelectionSource::get_total_projectile_damage() const
{
	if (gun)
	{
		Energy totalDamage;
		gun->get_projectile_damage(true, &totalDamage);
		return totalDamage;
	}
	if (statsME) return statsME->damage;
	return Energy::zero();
}

float ProjectileSelectionSource::get_projectile_speed() const
{
	if (gun) return gun->get_projectile_speed();
	if (statsME) return statsME->projectileSpeed;
	return 0.0f;
}

float ProjectileSelectionSource::get_projectile_spread() const
{
	if (gun) return gun->get_projectile_spread();
	if (statsME) return statsME->projectileSpread;
	return 0.0f;
}

int ProjectileSelectionSource::get_projectiles_per_shot() const
{
	if (gun) return gun->get_number_of_projectiles();
	if (statsME) return statsME->projectilesPerShot;
	return 1;
}

bool ProjectileSelectionSource::does_have_special_feature(Name const& _specialFeature) const
{
	if (gun)
	{
		auto const& sf = gun->get_special_features();
		for_every(s, sf)
		{
			if (s->value == _specialFeature)
			{
				return true;
			}
		}
		return false;
	}
	if (statsME) return statsME->specialFeatures.does_contain(_specialFeature);
	return false;
}

//

struct ProjectileSelectorManager
{
public:
	static void initialise_static();
	static void close_static();
	static void clear_static();
	static void make_sure_is_built();
	static void output_log();
	static Framework::TemporaryObjectType const* select_for(ProjectileSelectionSource const& _source, OUT_ ProjectileSelectionInfo& _projectileSelectionInfo);

private:
	static ProjectileSelectorManager* s_selector;

	Concurrency::MRSWLock accessLock;

	// ideally, library should not be reloaded
	Array<SafePtr<ProjectileSelector>> sortedSelectors;

	Framework::TemporaryObjectType const* internal_select_for(ProjectileSelectionSource const& _source, OUT_ ProjectileSelectionInfo& _projectileSelectionInfo);

	void log_selectors(LogInfoContext& _log, ProjectileSelector const * _for = nullptr);
	void check_for_circularities();

	void internal_make_sure_is_built();
	void internal_output_log();

};

ProjectileSelectorManager* ProjectileSelectorManager::s_selector = nullptr;

void ProjectileSelectorManager::initialise_static()
{
	// might be reloaded
	if (!s_selector)
	{
		s_selector = new ProjectileSelectorManager();
	}
}

void ProjectileSelectorManager::close_static()
{
	delete_and_clear(s_selector);
}

void ProjectileSelectorManager::clear_static()
{
	an_assert(s_selector);
	s_selector->sortedSelectors.clear();
}

Framework::TemporaryObjectType const* ProjectileSelectorManager::select_for(ProjectileSelectionSource const& _source, OUT_ ProjectileSelectionInfo& _projectileSelectionInfo)
{
	an_assert(s_selector);
	return s_selector->internal_select_for(_source, OUT_ _projectileSelectionInfo);
}

void ProjectileSelectorManager::make_sure_is_built()
{
	an_assert(s_selector);
	s_selector->internal_make_sure_is_built();
}

void ProjectileSelectorManager::output_log()
{
	an_assert(s_selector);
	s_selector->internal_output_log();
}

void ProjectileSelectorManager::internal_make_sure_is_built()
{
	if (sortedSelectors.is_empty())
	{
		accessLock.release_read(); // leave for a moment
		{
			Concurrency::ScopedMRSWLockWrite lock(accessLock);
			if (sortedSelectors.is_empty())
			{
				for_every_ref(ps, Library::get_current_as<Library>()->get_projectile_selectors().get_all_stored())
				{
					sortedSelectors.push_back(SafePtr<ProjectileSelector>(ps));
				}
				sort(sortedSelectors, ProjectileSelector::compare_safe_ptrs_by_priority);
				check_for_circularities();
			}
		}
		accessLock.acquire_read(); // get back
	}
}

void ProjectileSelectorManager::internal_output_log()
{
	internal_make_sure_is_built();
	accessLock.release_read(); // leave for a moment
	{
		LogInfoContext log;
		log.set_colour(Colour::green);
		log.log(TXT("PROJECTILE SELECTORS"));
		{
			LOG_INDENT(log);
			log.set_colour();
			log_selectors(log);
			log.output_to_output();
		}
	}
	accessLock.acquire_read(); // get back
}

Framework::TemporaryObjectType const* ProjectileSelectorManager::internal_select_for(ProjectileSelectionSource const& _source, OUT_ ProjectileSelectionInfo& _projectileSelectionInfo)
{
	Concurrency::ScopedMRSWLockRead lock(accessLock);

#ifdef AN_DEVELOPMENT
	{
		bool anySelector = false;
		for_every_ref(ps, sortedSelectors)
		{
			if (!ps)
			{
				continue;
			}
			anySelector = true;
			break;
		}
		if (!anySelector)
		{
			sortedSelectors.clear();
		}
	}
#endif

	internal_make_sure_is_built();

	ProjectileSelector const* selected = nullptr;
	bool changed = true;
	while (changed)
	{
		changed = false;
		ProjectileSelector const* alreadySelected = selected;
		for_every_ref(ps, sortedSelectors)
		{
			if (!ps)
			{
				continue;
			}

			if (ps->get_specialisation_of() == alreadySelected)
			{
				if (ps->check_conditions_for(_source))
				{
					changed = true;
					selected = ps;
					break; // we want to stop after selecting the first one (priorities!)
				}
			}
		}
	}

	if (selected)
	{
		_projectileSelectionInfo.numerousAsOne = selected->is_numerous_as_one();
		_projectileSelectionInfo.noSpread = selected->is_no_spread();
	}

	return selected ? selected->get_projectile() : nullptr;
}

void ProjectileSelectorManager::check_for_circularities()
{
	ARRAY_STACK(ProjectileSelector const*, selectorStack, sortedSelectors.get_size());
	for_every_ref(s, sortedSelectors)
	{
		selectorStack.clear();
		auto const * si = s;
		while (si)
		{
			if (selectorStack.does_contain(si))
			{
				error(TXT("circularity detected for \"%S\""), si->get_name().to_string().to_char());
				break;
			}
			if (! selectorStack.has_place_left())
			{
				error(TXT("circularity detected for \"%S\" (no duplicates, though)"), si->get_name().to_string().to_char());
				break;
			}
			selectorStack.push_back(si);
			si = si->get_specialisation_of();
		}
	}
}

void ProjectileSelectorManager::log_selectors(LogInfoContext& _log, ProjectileSelector const* _for)
{
	for_every_ref(s, sortedSelectors)
	{
		if (s->get_specialisation_of() == _for)
		{
			_log.log(TXT("%S -> %S"), s->get_name().to_string().to_char(), s->get_projectile()? s->get_projectile()->get_name().to_string().to_char() : TXT(""));
			{
				LOG_INDENT(_log);
				log_selectors(_log, s);
			}
		}
	}
}

//

REGISTER_FOR_FAST_CAST(ProjectileSelector);
LIBRARY_STORED_DEFINE_TYPE(ProjectileSelector, projectileSelector);

int ProjectileSelector::compare_safe_ptrs_by_priority(void const* _a, void const* _b)
{
	::SafePtr<ProjectileSelector> const& a = *(plain_cast<::SafePtr<ProjectileSelector>>(_a));
	::SafePtr<ProjectileSelector> const& b = *(plain_cast<::SafePtr<ProjectileSelector>>(_b));

	if (a.get() && b.get())
	{
		if (a->get_priority() > b->get_priority()) return A_BEFORE_B;
		if (a->get_priority() < b->get_priority()) return B_BEFORE_A;
		return String::compare_icase(a->get_name().get_group().to_string(), b->get_name().get_group().to_string());
	}
	return A_AS_B;
}

void ProjectileSelector::initialise_static()
{
	ProjectileSelectorManager::initialise_static();
}

void ProjectileSelector::close_static()
{
	ProjectileSelectorManager::close_static();
}

void ProjectileSelector::clear_static()
{
	ProjectileSelectorManager::clear_static();
}

void ProjectileSelector::build_for_game()
{
	ProjectileSelectorManager::make_sure_is_built();
}

void ProjectileSelector::output_projectile_selector()
{
	ProjectileSelectorManager::output_log();
}

Framework::TemporaryObjectType const * ProjectileSelector::select_for(ProjectileSelectionSource const& _source, OUT_ ProjectileSelectionInfo& _projectileSelectionInfo)
{
	return ProjectileSelectorManager::select_for(_source, OUT_ _projectileSelectionInfo);
}

ProjectileSelector::ProjectileSelector(Framework::Library* _library, Framework::LibraryName const& _name)
: base(_library, _name)
, SafeObject<ProjectileSelector>(this)
{
}

ProjectileSelector::~ProjectileSelector()
{
	make_safe_object_unavailable();
}

bool ProjectileSelector::load_from_xml(IO::XML::Node const* _node, Framework::LibraryLoadingContext& _lc)
{
	bool result = true;

	result &= projectile.load_from_xml(_node, TXT("projectile"), _lc);
	numerousAsOne = _node->get_bool_attribute_or_from_child_presence(TXT("numerousAsOne"), numerousAsOne);
	noSpread = _node->get_bool_attribute_or_from_child_presence(TXT("noSpread"), noSpread);

	priority = _node->get_int_attribute(TXT("priority"), priority);
	result &= specialisationOf.load_from_xml(_node, TXT("specialisationOf"), _lc);

	error_loading_xml_on_assert(projectile.is_name_valid(), result, _node, TXT("\"projectile\" has to be always provided"));

	for_every(node, _node->children_named(TXT("condition")))
	{
		Condition condition;
		if (condition.load_from_xml(node))
		{
			conditions.push_back(condition);
		}
		else
		{
			result = false;
		}
	}

	return result;
}

bool ProjectileSelector::prepare_for_game(Framework::Library* _library, Framework::LibraryPrepareContext& _pfgContext)
{
	bool result = true;

	result = projectile.prepare_for_game_find(_library, _pfgContext, Framework::LibraryPrepareLevel::Resolve);
	result = specialisationOf.prepare_for_game_find(_library, _pfgContext, Framework::LibraryPrepareLevel::Resolve);

	return result;
}

bool ProjectileSelector::check_conditions_for(ProjectileSelectionSource const& _source) const
{
	for_every(c, conditions)
	{
		if (!c->check_for(_source))
		{
			return false;
		}
	}
	return true;
}

//

bool ProjectileSelector::Condition::load_from_xml(IO::XML::Node const* _node)
{
	bool result = true;

	String condText = _node->get_text();

	RangeInt operatorAt = RangeInt::empty;
	{
		tchar opChars[] = { '<', '>', '=', 0 };
		int i = 0;
		while (opChars[i])
		{
			{
				int at = condText.find_first_of(opChars[i]);
				if (at != NONE)
				{
					operatorAt.include(at);
				}
			}
			{
				int at = condText.find_last_of(opChars[i]);
				if (at != NONE)
				{
					operatorAt.include(at);
				}
			}
			++i;
		}
	}

	if (operatorAt.is_empty())
	{
		error_loading_xml(_node, TXT("no operator"));
		result = false;
	}
	else
	{
		param = Name(condText.get_left(operatorAt.min).trim());
		operation = Name(condText.get_sub(operatorAt.min, operatorAt.max - operatorAt.min + 1));
		String value = condText.get_sub(operatorAt.max + 1).trim();
		if (is_energy())
		{
			energyRef = Energy::parse(value);
		}
		else if (is_float())
		{
			floatRef = ParserUtils::parse_float(value);
		}
		else if (is_int())
		{
			intRef = ParserUtils::parse_int(value);
		}
		else if (is_weapon_core_kind())
		{
			intRef = WeaponCoreKind::parse(value, WeaponCoreKind::Plasma);
		}
		else if (is_name())
		{
			nameRef = Name(value);
		}
		else
		{
			error_loading_xml(_node, TXT("doesn't know how to handle \"%S\" param"), param.to_char());
			todo_important(TXT("maybe it was not added?"));
			result = false;
		}
	}

	return result;
}

bool ProjectileSelector::Condition::is_weapon_core_kind() const
{
	return param == NAME(coreKind)
		   ;
}

bool ProjectileSelector::Condition::is_name() const
{
	return param == NAME(specialFeature)
		   ;
}

bool ProjectileSelector::Condition::is_energy() const
{
	return param == NAME(damage)
		|| param == NAME(totalDamage)
		   ;
}

bool ProjectileSelector::Condition::is_float() const
{
	return param == NAME(speed)
		|| param == NAME(spread)
		   ;
}

bool ProjectileSelector::Condition::is_int() const
{
	return param == NAME(number)
		   ;
}

template <typename Class>
static bool profile_selector_condition_compare(Class const& paramValue, Class const& refValue, Name const& operation)
{
	if (operation == NAME(eq) ||
		operation == NAME(eq2))
	{
		return paramValue == refValue;
	}
	if (operation == NAME(neq) ||
		operation == NAME(neq2))
	{
		return paramValue != refValue;
	}
	if (operation == NAME(gt))
	{
		return paramValue > refValue;
	}
	if (operation == NAME(ge))
	{
		return paramValue >= refValue;
	}
	if (operation == NAME(lt))
	{
		return paramValue < refValue;
	}
	if (operation == NAME(le))
	{
		return paramValue <= refValue;
	}
	error(TXT("not recognised operation"));
	return false;
}

bool ProjectileSelector::Condition::check_for(ProjectileSelectionSource const& _source) const
{
	if (param == NAME(coreKind))
	{
		return profile_selector_condition_compare((int)_source.get_core_kind(), intRef, operation);
	}
	if (param == NAME(damage))
	{
		return profile_selector_condition_compare(_source.get_single_projectile_damage(), energyRef, operation);
	}
	if (param == NAME(totalDamage))
	{
		return profile_selector_condition_compare(_source.get_total_projectile_damage(), energyRef, operation);
	}
	if (param == NAME(speed))
	{
		return profile_selector_condition_compare(_source.get_projectile_speed(), floatRef, operation);
	}
	if (param == NAME(spread))
	{
		return profile_selector_condition_compare(_source.get_projectile_spread(), floatRef, operation);
	}
	if (param == NAME(number))
	{
		return profile_selector_condition_compare(_source.get_projectiles_per_shot(), intRef, operation);
	}
	if (param == NAME(specialFeature))
	{
		if (operation == NAME(eq) ||
			operation == NAME(eq2))
		{
			return _source.does_have_special_feature(nameRef);
		}
		else if (operation == NAME(neq) ||
				 operation == NAME(neq2))
		{
			return ! _source.does_have_special_feature(nameRef);
		}
		else
		{
			error(TXT("can't do comparisons like that"));
			return false;
		}
	}
	return false;
}

