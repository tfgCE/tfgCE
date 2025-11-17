#include "weaponPart.h"

#include "game.h"
#include "persistence.h"

#include "..\library\library.h"
#include "..\overlayInfo\elements\oie_text.h"
#include "..\schematics\schematic.h"
#include "..\schematics\schematicLine.h"
#include "..\tutorials\tutorialHubId.h"
#include "..\utils\buildStatsInfo.h"

#include "..\..\framework\debug\previewGame.h"
#include "..\..\framework\library\usedLibraryStored.inl"

#include "..\..\core\other\parserUtils.h"

//

#ifdef AN_ALLOW_OPTIMIZE_OFF
#pragma optimize("", off)
#endif

#ifdef AN_DEVELOPMENT
//#define AN_TEST_LAYOUT
#endif

//

using namespace TeaForGodEmperor;

//

// slot connectors

// features/stats
DEFINE_STATIC_NAME(chamber);
DEFINE_STATIC_NAME(chamberNeg);
DEFINE_STATIC_NAME(baseDamage);
DEFINE_STATIC_NAME(damage);
DEFINE_STATIC_NAME(damageNeg);
DEFINE_STATIC_NAME(armourPiercing);
DEFINE_STATIC_NAME(storage);
DEFINE_STATIC_NAME(storageOutputSpeed);
DEFINE_STATIC_NAME(magazine);
DEFINE_STATIC_NAME(magazineCooldown);
DEFINE_STATIC_NAME(magazineCooldownNeg);
DEFINE_STATIC_NAME(magazineOutputSpeed);
DEFINE_STATIC_NAME(continuousFire);
DEFINE_STATIC_NAME(projectileSpeed);
DEFINE_STATIC_NAME(projectileSpread);
DEFINE_STATIC_NAME(projectileNoSpread);
DEFINE_STATIC_NAME(projectileNegSpread);
DEFINE_STATIC_NAME(multipleProjectiles);
DEFINE_STATIC_NAME(singleProjectile);
DEFINE_STATIC_NAME(antiDeflection);
DEFINE_STATIC_NAME(maxDistCoef);
DEFINE_STATIC_NAME(maxDistCoefNeg);
DEFINE_STATIC_NAME(kineticEnergyCoef);
DEFINE_STATIC_NAME(confuse);
DEFINE_STATIC_NAME(mediCoef);
DEFINE_STATIC_NAME(mediCoefNeg);

// dedicated
DEFINE_STATIC_NAME(muzzle);
DEFINE_STATIC_NAME(supercharger);
DEFINE_STATIC_NAME(retaliator);
DEFINE_STATIC_NAME(devastator);

// localised strings
DEFINE_STATIC_NAME_STR(lsCharsDamage, TXT("chars; damage"));
DEFINE_STATIC_NAME_STR(lsCharsAmmo, TXT("chars; ammo"));
DEFINE_STATIC_NAME_STR(lsCharsHealth, TXT("chars; health"));
DEFINE_STATIC_NAME_STR(lsMixerStatsChassis, TXT("weapon part mixer stats; chassis"));
DEFINE_STATIC_NAME_STR(lsMixerStatsMuzzle, TXT("weapon part mixer stats; muzzle"));
DEFINE_STATIC_NAME_STR(lsMixerStatsNode, TXT("weapon part mixer stats; node"));

// shared
	DEFINE_STATIC_NAME(value);
	DEFINE_STATIC_NAME(total);
	DEFINE_STATIC_NAME(cost);
	DEFINE_STATIC_NAME(part);
	DEFINE_STATIC_NAME(projectileCount);
	DEFINE_STATIC_NAME(nodeCount);
	DEFINE_STATIC_NAME(coef);
DEFINE_STATIC_NAME_STR(lsEquipmentDamaged, TXT("pi.ov.in; gun; damaged"));
DEFINE_STATIC_NAME_STR(lsEquipmentDamagedReplace, TXT("pi.ov.in; gun; damaged replace"));
DEFINE_STATIC_NAME_STR(lsEquipmentBaseDamage, TXT("pi.ov.in; gun; base damage"));
DEFINE_STATIC_NAME_STR(lsEquipmentDamage, TXT("pi.ov.in; gun; damage"));
DEFINE_STATIC_NAME_STR(lsEquipmentDamageBoost, TXT("pi.ov.in; gun; damage boost"));
DEFINE_STATIC_NAME_STR(lsEquipmentCost, TXT("pi.ov.in; gun; cost"));
DEFINE_STATIC_NAME_STR(lsEquipmentMagazine, TXT("pi.ov.in; gun; magazine"));
DEFINE_STATIC_NAME_STR(lsEquipmentArmourPiercing, TXT("pi.ov.in; gun; armour piercing"));
DEFINE_STATIC_NAME_STR(lsEquipmentAntiDeflection, TXT("pi.ov.in; gun; anti deflection"));
DEFINE_STATIC_NAME_STR(lsEquipmentMaxDist, TXT("pi.ov.in; gun; max dist"));
DEFINE_STATIC_NAME_STR(lsEquipmentKineticEnergyCoef, TXT("pi.ov.in; gun; kinetic energy coef"));
DEFINE_STATIC_NAME_STR(lsEquipmentConfuse, TXT("pi.ov.in; gun; confuse"));
DEFINE_STATIC_NAME_STR(lsEquipmentMediCoef, TXT("pi.ov.in; gun; medi coef"));
DEFINE_STATIC_NAME_STR(lsEquipmentContinuousFire, TXT("pi.ov.in; gun; continuous fire"));
DEFINE_STATIC_NAME_STR(lsEquipmentRoundsPerMinute, TXT("pi.ov.in; gun; rounds per minute"));
DEFINE_STATIC_NAME_STR(lsEquipmentRoundsPerMinuteClass, TXT("pi.ov.in; gun; rounds per minute; class"));
DEFINE_STATIC_NAME_STR(lsEquipmentRoundsPerMinuteClassAdj, TXT("pi.ov.in; gun; rounds per minute; class; adj"));
DEFINE_STATIC_NAME_STR(lsEquipmentRoundsPerMinuteMagazineClass, TXT("pi.ov.in; gun; rounds per minute; magazine class"));
DEFINE_STATIC_NAME_STR(lsEquipmentProjectileCount, TXT("pi.ov.in; gun; projectile count"));
DEFINE_STATIC_NAME_STR(lsEquipmentProjectileSpeed, TXT("pi.ov.in; gun; projectile speed"));
DEFINE_STATIC_NAME_STR(lsEquipmentProjectileSpread, TXT("pi.ov.in; gun; projectile spread"));

// colours
DEFINE_STATIC_NAME(pilgrim_overlay_weaponPart_damaged)

// text colours
DEFINE_STATIC_NAME(core);
DEFINE_STATIC_NAME(damaged);

// system tags
DEFINE_STATIC_NAME(lowGraphics);

//

float get_mul_rgb()
{
	float mulrgb = 0.65f;
	if (::System::Core::get_system_tags().get_tag(NAME(lowGraphics)))
	{
		// no bloom
		mulrgb = 0.85f;
	}
	return mulrgb;
}
//

bool WeaponPartAddress::operator==(Array<Name> const& _stack) const
{
	if (is_loose_part())
	{
		return false;
	}
	if (_stack.get_size() != at.get_size())
	{
		return false;
	}
	for_count(int, i, at.get_size())
	{
		if (_stack[i] != at[i])
		{
			return false;
		}
	}
	return true;
}

bool WeaponPartAddress::operator==(WeaponPartAddress const& _other) const
{
	if (is_loose_part() != _other.is_loose_part())
	{
		return false;
	}
	if (is_loose_part())
	{
		return loosePartIdx == _other.loosePartIdx;
	}
	else
	{
		return at == _other.at;
	}
}

void WeaponPartAddress::set(String const& _addr)
{
	at.clear();
	List<String> tokens;
	String splitter(TXT(":"));
	_addr.split(splitter, tokens);
	for_every(t, tokens)
	{
		at.push_back(Name(*t));
		if (at.get_space_left() == 0)
		{
			error_stop(TXT("the weapon part address is not long enough"));
			break;
		}
	}
}

String WeaponPartAddress::to_string() const
{
	String addr;
	for_every(a, at)
	{
		if (!addr.is_empty())
		{
			addr += TXT(":");
		}
		addr += a->to_string();
	}
	return addr;
}

void WeaponPartAddress::add(Name const & _id)
{
	at.push_back(_id);
}

int WeaponPartAddress::as_index_number() const
{
	int result = 0;
	for_every(a, at)
	{
		for_every(ch, a->to_string().get_data())
		{
			if (*ch == 0)
			{
				break;
			}
			int shifted = (result << 3);
			int hi = (shifted & 0xffff0000);
			int low = (shifted & 0x0000ffff);
			result = low + (hi >> 16) + (int)*ch;
		}
		result += for_everys_index(a);
	}
	return result;
}

//--

bool UsedWeaponPart::load_from_xml(Persistence* _persistence, IO::XML::Node const* _node)
{
	bool result = true;

	at.set(_node->get_string_attribute(TXT("at")));

	part = WeaponPart::create_load_from_xml(_persistence, _node);

	if (!part.get())
	{
		error_loading_xml(_node, TXT("could not load weapon part setup"));
		result = false;
	}

	return result;
}

bool UsedWeaponPart::save_to_xml(IO::XML::Node* _node) const
{
	bool result = true;

	_node->set_attribute(TXT("at"), at.to_string());

	if (part.get())
	{
		result &= part->save_to_xml(_node);
	}

	return result;
}

bool UsedWeaponPart::resolve_library_links()
{
	bool result = true;

	if (part.get())
	{
		result &= part->resolve_library_links();
	}
	else
	{
		error(TXT("no part for UsedWeaponPart"));
		result = false;
	}

#ifdef AN_DEVELOPMENT_OR_PROFILER
	if (Framework::Library::may_ignore_missing_references())
	{
		result = true;
	}
#endif
		
	return result;
}

//--

bool WeaponPart::is_same_content(WeaponPart const* _a, WeaponPart const* _b)
{
	if (_a == _b)
	{
		return true;
	}
	if (!_a || !_b)
	{
		return false;
	}
	return	_a->weaponPartType == _b->weaponPartType
		&&  _a->rgIndex == _b->rgIndex
		&&  _a->nonRandomised == _b->nonRandomised
		;
}

Vector2 WeaponPart::get_schematic_size()
{
	return Vector2::one * 64.0f;
}

Vector2 WeaponPart::get_container_schematic_size()
{
	return Vector2(2.0f, 1.0f) * 256.0f;
}

WeaponPart::WeaponPart()
: SafeObject<WeaponPart>(this)
{
	rgIndex = Random::next_int();
}

WeaponPart::~WeaponPart()
{
	make_safe_object_unavailable();
}

WeaponPart* WeaponPart::create_instance_of(Persistence* _forPersistence, WeaponPartType const* _weaponPartType, bool _nonRandomised, EnergyCoef const & _damaged, bool _defaultMeshParams, Optional<int> const& _rgIndex)
{
	an_assert(_weaponPartType);
	WeaponPart* wp = new WeaponPart();
	wp->weaponPartType = _weaponPartType;
	wp->persistence = _forPersistence;
	wp->nonRandomised = _nonRandomised;
	wp->damaged = _damaged;
	wp->defaultMeshParams = _defaultMeshParams;
	if (_rgIndex.is_set())
	{
		wp->rgIndex = _rgIndex.get();
	}

	wp->be_instance(_nonRandomised, _defaultMeshParams);

	wp->make_sure_schematic_mesh_exists();

	return wp;
}

WeaponPart* WeaponPart::create_instance_of(Persistence* _forPersistence, Framework::LibraryName const& _weaponPartType, bool _nonRandomised, EnergyCoef const& _damaged, bool _defaultMeshParams, Optional<int> const& _rgIndex)
{
	WeaponPart* wp = new WeaponPart();
	wp->weaponPartType.set_name(_weaponPartType);
	wp->persistence = _forPersistence;
	wp->nonRandomised = _nonRandomised;
	wp->damaged = _damaged;
	wp->defaultMeshParams = _defaultMeshParams;
	if (_rgIndex.is_set())
	{
		wp->rgIndex = _rgIndex.get();
	}

	wp->resolve_library_links();

	wp->make_sure_schematic_mesh_exists();

	return wp;
}

WeaponPart* WeaponPart::create_copy_for_different_persistence(Persistence* _forPersistence, Optional<bool> const& _nonRandomised, Optional<EnergyCoef> const& _damaged, Optional<bool> const & _defaultMeshParams, Optional<bool> const & _forStoring) const
{
	an_assert(persistence != _forPersistence);

	WeaponPart* wp = new WeaponPart();
	wp->persistence = _forPersistence;
	wp->rgIndex = rgIndex;
	wp->nonRandomised = _nonRandomised.get(nonRandomised);
	wp->damaged = _damaged.get(damaged);
	wp->defaultMeshParams = _defaultMeshParams.get(defaultMeshParams);
	wp->weaponPartType = weaponPartType;

	if (wp->weaponPartType.is_set())
	{
		wp->be_instance(nonRandomised, defaultMeshParams);
	}
	else
	{
		wp->resolve_library_links();
	}

	if (!_forStoring.get(false))
	{
		wp->make_sure_schematic_mesh_exists();
	}

	return wp;
}

WeaponPart* WeaponPart::create_load_from_xml(Persistence* _forPersistence, IO::XML::Node const* _node)
{
	WeaponPart* wp = new WeaponPart();
	wp->persistence = _forPersistence;

	if (!wp->load_from_xml(_node))
	{
		error_loading_xml(_node, TXT("couldn't load weapon part"));
		return nullptr;
	}

	wp->resolve_library_links();

	wp->make_sure_schematic_mesh_exists();

	return wp;
}

bool WeaponPart::check_against(IO::XML::Node const* _node)
{
	bool result = true;

	WeaponPart* loaded = new WeaponPart();
	
	if (loaded->load_from_xml(_node))
	{
		result &= rgIndex == loaded->rgIndex;
		result &= nonRandomised == loaded->nonRandomised;
		result &= damaged == loaded->damaged;
		result &= defaultMeshParams == loaded->defaultMeshParams;
		result &= weaponPartType.get_name() == loaded->weaponPartType.get_name();
	}
	else
	{
		result = false;
	}

	delete loaded;

	return result;
}

#define COMPARE(what) (_a.what == _b.what)
bool WeaponPart::compare(WeaponPart const & _a, WeaponPart const & _b)
{
	return COMPARE(rgIndex)
		&& COMPARE(nonRandomised)
		&& COMPARE(damaged)
		&& COMPARE(defaultMeshParams)
		&& COMPARE(weaponPartType);
}
#undef COMPARE

bool WeaponPart::load_from_xml(IO::XML::Node const* _node)
{
	bool result = true;

	rgIndex = _node->get_int_attribute(TXT("rgIndex"), rgIndex);
	nonRandomised = _node->get_bool_attribute(TXT("nonRandomised"), nonRandomised);
	damaged = EnergyCoef::load_from_attribute_or_from_child(_node, TXT("damaged"), EnergyCoef::zero());
	defaultMeshParams = _node->get_bool_attribute(TXT("defaultMeshParams"), defaultMeshParams);
	weaponPartType.load_from_xml(_node, TXT("weaponPartType"));

	return result;
}

bool WeaponPart::save_to_xml(IO::XML::Node* _node) const
{
	bool result = true;

	_node->set_int_attribute(TXT("rgIndex"), rgIndex);
	_node->set_bool_attribute(TXT("nonRandomised"), nonRandomised);
	if (!damaged.is_zero())
	{
		_node->set_attribute(TXT("damaged"), damaged.as_string_percentage());
	}
	_node->set_bool_attribute(TXT("defaultMeshParams"), defaultMeshParams);
	_node->set_attribute(TXT("weaponPartType"), weaponPartType.get_name().to_string());

	return result;
}

bool WeaponPart::resolve_library_links()
{
	bool result = true;

	if (!weaponPartType.is_set())
	{
		MEASURE_PERFORMANCE(resolvingLibraryLinksForWeaponPart);

		weaponPartType.find_may_fail(Library::get_current());
		if (weaponPartType.is_set())
		{
			be_instance(nonRandomised, defaultMeshParams);
		}
	}

#ifdef AN_DEVELOPMENT_OR_PROFILER
	if (Framework::Library::may_ignore_missing_references())
	{
		result = true;
	}
#endif

	return result;
}

int WeaponPart::adjust_transfer_chance(int _transferChance, TransferWeaponPartContext const & _context)
{
	int boosts = _context.activeBoosts;
	while (boosts > 0)
	{
		--boosts;
		_transferChance = _transferChance + 50;
	}

	if (_context.preDeactivationLevel.is_set())
	{
		int preDeactivationLevel = _context.preDeactivationLevel.get();
		while (preDeactivationLevel >= 0)
		{
			--preDeactivationLevel;
			_transferChance = _transferChance - 5;
			if (_transferChance > 1)
			{
				_transferChance = _transferChance * 5 / 7;
			}
		}
	}

	int lowThreshold = 30;
	if (_transferChance < lowThreshold)
	{
		_transferChance = lowThreshold + (_transferChance - lowThreshold) * 6 / 7;
	}
	int highThreshold = 80;
	if (_transferChance > highThreshold)
	{
		_transferChance = highThreshold + (_transferChance - highThreshold) * 2 / 7;
	}

	_transferChance = clamp(_transferChance, 0, 100); // clamp to sane values

	return _transferChance;
}

#define RG(offset) \
	rg.set_seed(rgIndex, offset);

#define COPY_AFFECTION(what) \
	if (weaponPartType->what.how != WeaponStatAffection::Unknown) \
	{ \
		what.how = weaponPartType->what.how; \
	}

#define SET_RANDOM(what) \
	if (! weaponPartType->what.value.is_empty()) \
	{ \
		what.value = weaponPartType->what.value.get(rg, _nonRandomised); \
		COPY_AFFECTION(what); \
	}

#define SET_OPTIONAL(what) \
	if (weaponPartType->what.value.is_set()) \
	{ \
		what = weaponPartType->what.value.get(); \
		COPY_AFFECTION(what); \
	}

#define SET_RANDOM_VAR_IF(TYPE) \
	if (varInfo->type_id() == type_id<Random::Number<TYPE>>()) \
	{ \
		meshGenerationParameters.access<TYPE>(varInfo->get_name()) = varInfo->get<Random::Number<TYPE>>().get(rg, _nonRandomised); \
	}

void WeaponPart::be_instance(bool _nonRandomised, bool _defaultMeshParams)
{
	an_assert(weaponPartType.is_set());
	
	nonRandomised = _nonRandomised;
	defaultMeshParams = _defaultMeshParams;

	Random::Generator rg;
	
	// when release, don't change these RG numbers, rather add new ones

	RG(123); transferChance = rg.get_int_from_range(30, 120);

	RG( 1); SET_OPTIONAL(coreKind);
	RG( 2); SET_RANDOM(chamber);
	RG( 3); SET_RANDOM(chamberCoef);
	RG(25); SET_RANDOM(baseDamageBoost);
	RG( 4); SET_RANDOM(damageCoef);
	RG( 5); SET_RANDOM(damageBoost);
	RG( 6); //SET_RANDOM(continuousDamagePart);
	RG( 7); SET_RANDOM(armourPiercing);
	RG( 8); SET_RANDOM(storage);
	RG( 9); SET_RANDOM(storageOutputSpeed);
	RG(10); SET_RANDOM(storageOutputSpeedAdd);
	RG(24); SET_RANDOM(storageOutputSpeedAdj);
	RG(11); SET_RANDOM(magazine);
	RG(12); SET_RANDOM(magazineCooldown);
	RG(13); SET_RANDOM(magazineCooldownCoef);
	RG(14); SET_RANDOM(magazineOutputSpeed);
	RG(15); SET_RANDOM(magazineOutputSpeedAdd);
	RG(16); SET_OPTIONAL(continuousFire);
	RG(17); SET_RANDOM(projectileSpeed);
	RG(18); SET_RANDOM(projectileSpread);
	RG(19); SET_RANDOM(projectileSpeedCoef);
	RG(20); SET_RANDOM(projectileSpreadCoef);
	RG(21); SET_RANDOM(numberOfProjectiles);
	RG(22); SET_RANDOM(antiDeflection);
	RG(23); SET_RANDOM(maxDistCoef);
	RG(26); SET_RANDOM(kineticEnergyCoef);
	RG(27); SET_RANDOM(confuse);
	RG(28); SET_RANDOM(mediCoef);
	// 29

	if (!weaponPartType->projectileColours.is_empty())
	{
		RG(800);
		projectileColour = weaponPartType->projectileColours[rg.get_int(weaponPartType->projectileColours.get_size())];
	}
	else
	{
		projectileColour.clear();
	}
	{
		RG(1000);
		weaponPartType->process_whole_mesh_generation_parameters(rg, defaultMeshParams, [this, _nonRandomised](SimpleVariableStorage const& _params, Random::Generator const& _rg)
		{
			Random::Generator rg = _rg;
			// copy variables but if there is any random number, make it actual number
			for_every(varInfo, _params.get_all())
			{
				SET_RANDOM_VAR_IF(int)
				else SET_RANDOM_VAR_IF(float)
				else SET_RANDOM_VAR_IF(Energy)
				else SET_RANDOM_VAR_IF(EnergyCoef)
				{	// just copy
					meshGenerationParameters.set(*varInfo);
				}
			}
		});
	}
}

int WeaponPart::get_mesh_generation_parameters_priority() const
{
	return weaponPartType.get() ? weaponPartType->wholeMeshGenerationParametersPriority : 0;
}

void WeaponPart::mark_in_current_use()
{
	inCurrentUse = true;
	make_sure_schematic_mesh_exists();
}

void WeaponPart::clear_in_current_use()
{
	inCurrentUse = false;
	make_sure_schematic_mesh_exists();
}

void WeaponPart::make_sure_schematic_mesh_exists()
{
#ifdef AN_DEVELOPMENT_OR_PROFILER
	if (Framework::is_preview_game())
	{
		return;
	}
#endif

	Concurrency::ScopedSpinLock lock(schematicCreationLock);

	if (schematic.get())
	{
		return;
	}
	if (!weaponPartType.get())
	{
		return;
	}
	if (creatingSchematic.increase() == 1) // incremented just once
	{
		if (auto* g = Game::get())
		{
			if (g->is_performing_async_job_on_this_thread())
			{
				// if we're already in an async thread, do the mesh immediately
				build_schematic_mesh();
				build_container_schematic_mesh();
			}
			else
			{
				// top priority as they're pretty quick (unless we are doing one long async job)
				RefCountObjectPtr<WeaponPart> thisKeeper(this);
				Game::get()->add_async_world_job_top_priority(TXT("create schematic for weapon part"), [thisKeeper, this]()
				{
					build_schematic_mesh();
					build_container_schematic_mesh();
				});
			}
		}
	}
}

struct WeaponPartSchematicFeature
{
	Name stat;
	float importance;
	int lines;

	WeaponPartSchematicFeature() {}
	WeaponPartSchematicFeature(Name const& _stat, float _importance) : stat(_stat), importance(_importance) {}

	static int compare(void const* _a, void const* _b)
	{
		WeaponPartSchematicFeature const* a = plain_cast<WeaponPartSchematicFeature>(_a);
		WeaponPartSchematicFeature const* b = plain_cast<WeaponPartSchematicFeature>(_b);
		if (a->importance > b->importance) return A_BEFORE_B;
		if (a->importance < b->importance) return B_BEFORE_A;
		return A_AS_B;
	}

	static void store(ArrayStack<WeaponPartSchematicFeature>& _in, Name const& _stat, float _importance)
	{
		for_every(f, _in)
		{
			if (f->stat == _stat)
			{
				f->importance += _importance;
				return;
			}
		}
		_in.push_back(WeaponPartSchematicFeature(_stat, _importance));
	}

	static void store(ArrayStack<WeaponPartSchematicFeature>& _in, WeaponPartStatInfo<float> const& _what, Name const& _asPositive, float _asPositiveRef, Name const & _asNegative = Name::invalid(), float _asNegativeRef = 0.0f)
	{
		if (!_what.value.is_set())
		{
			return;
		}

		if (_what.value.get() > 0.0f)
		{
			store(_in, _asPositive, _asPositiveRef != 0.0f? _what.value.get() / _asPositiveRef : 1.0f);
		}
		else if (_what.value.get() < 0.0f && _asNegative.is_valid())
		{
			store(_in, _asNegative, _asNegativeRef != 0.0f? _what.value.get() / _asNegativeRef : 1.0f);
		}
	}

	static void store(ArrayStack<WeaponPartSchematicFeature>& _in, WeaponPartStatInfo<int> const& _what, Name const& _asPositive, float _asPositiveRef)
	{
		if (!_what.value.is_set())
		{
			return;
		}

		if (_what.value.get() > 0)
		{
			store(_in, _asPositive, _asPositiveRef != 0.0f? (float)_what.value.get() / _asPositiveRef : 1.0f);
		}
	}

	static void store(ArrayStack<WeaponPartSchematicFeature>& _in, WeaponPartStatInfo<bool> const& _what, Name const& _stat, float _importance)
	{
		if (!_what.value.is_set())
		{
			return;
		}

		if (_what.value.get())
		{
			store(_in, _stat, _importance);
		}
	}

	static void store(ArrayStack<WeaponPartSchematicFeature>& _in, WeaponPartStatInfo<Energy> const& _what, Name const& _asPositive, Energy const & _asPositiveRef, Name const & _asNegative = Name::invalid(), Energy const & _asNegativeRef = Energy::zero())
	{
		if (!_what.value.is_set())
		{
			return;
		}

		if (_what.value.get().is_positive())
		{
			store(_in, _asPositive, ! _asPositiveRef.is_zero()? _what.value.get().as_float() / _asPositiveRef.as_float() : 1.0f);
		}
		else if (_what.value.get().is_negative() && _asNegative.is_valid())
		{
			store(_in, _asNegative, ! _asNegativeRef.is_zero()? _what.value.get().as_float() / _asNegativeRef.as_float() : 1.0f);
		}
	}

	static void store(ArrayStack<WeaponPartSchematicFeature>& _in, WeaponPartStatInfo<EnergyCoef> const& _what, Name const& _asPositive, EnergyCoef const & _asPositiveRef, Name const & _asNegative = Name::invalid(), EnergyCoef const & _asNegativeRef = EnergyCoef::zero())
	{
		if (!_what.value.is_set())
		{
			return;
		}

		if (_what.value.get().is_positive())
		{
			store(_in, _asPositive, ! _asPositiveRef.is_zero()? _what.value.get().as_float() / _asPositiveRef.as_float() : 1.0f);
		}
		else if (_what.value.get().is_negative() && _asNegative.is_valid())
		{
			store(_in, _asNegative, ! _asNegativeRef.is_zero()? _what.value.get().as_float() / _asNegativeRef.as_float() : 1.0f);
		}
	}
};

TutorialHubId WeaponPart::get_tutorial_hub_id() const
{
	TutorialHubId tutorialHubId;
	if (get_weapon_part_type())
	{
		tutorialHubId.set(String::printf(TXT("WeaponPart:%S"), get_weapon_part_type()->get_name().to_string().to_char()));
	}
	return tutorialHubId;
}

Optional<Range2> WeaponPart::get_container_schematic_slot_at(WeaponPartAddress const& _address) const
{
	Concurrency::ScopedSpinLock lock(schematicCreationLock);

	if (containerSchematicSlotsAreAccessible)
	{
		for_every(slot, containerSchematicSlots)
		{
			if (slot->address == _address)
			{
				return slot->at;
			}
		}
	}
	return NP;
}

void WeaponPart::add_container_schematic_slot(Name const& slotId, Vector2 const& _at, Random::Generator const& rg, WeaponPartType::Type _weaponPartType, WeaponPartAddress const& _address)
{
	SchematicSlot slot;
	slot.slotId = slotId;
	slot.at = Range2::empty;
	slot.at.include(_at);
	slot.rg = rg;
	slot.weaponPartType = _weaponPartType;
	slot.address = _address;
	containerSchematicSlots.push_back(slot);
}

#define maybe_round

void WeaponPart::build_container_schematic_mesh()
{
	ASSERT_ASYNC_OR_LOADING_OR_PREVIEW;
	an_assert(weaponPartType.get());

	containerSchematic.clear();
	containerSchematicSlotsAreAccessible = false;

	if (weaponPartType->get_type() == WeaponPartType::GunChassis)
	{
		Range2 range;
		{
			Vector2 size = get_container_schematic_size();
			range.x.min = - size.x * 0.5f;
			range.x.max =   size.x * 0.5f;
			range.y.min = - size.y * 0.5f;
			range.y.max =   size.y * 0.5f;
		}

		// setup slots
		{
			containerSchematicSlots.clear();

			Random::Generator rg;
			rg.set_seed(get_rand_index(), 0);
			WeaponPartAddress chassisAddress;
			chassisAddress.add(get_weapon_part_type()->get_plug().id);
			add_container_schematic_slot(Name::invalid(), Vector2::half, rg, WeaponPartType::GunChassis, chassisAddress);

			for_every(slot, get_weapon_part_type()->get_slots())
			{
				rg.advance_seed(2350, 261);
				Vector2 at = Vector2(rg.get_float(0.1f, 0.9f), rg.get_float(0.1f, 0.9f));
				WeaponPartAddress address = chassisAddress;
				address.add(slot->id);
				add_container_schematic_slot(slot->id, at, rg, slot->connectType, address);
			}

			Random::Generator mainRG = containerSchematicSlots.get_first().rg;

			// arrange them, all at once
			{
				Random::Generator rg;
				rg = mainRG;

#ifdef AN_TEST_LAYOUT
				if (!dv.get())
				{
					dv = new DebugVisualiser(String(TXT("setup slots")));
					dv->activate();
				}
#endif
				Range2 useRange = range; // use full range! we should not depend on mixing font size into the size

				float size = get_schematic_size().x;
				float halfSize = maybe_round(size * 0.5f);
				Range2 validRange = useRange.expanded_by(Vector2(-halfSize * 1.1f));

				// move all slots
				ARRAY_STACK(Vector2, moveSlots, containerSchematicSlots.get_size());
				moveSlots.set_size(containerSchematicSlots.get_size());

				float separation = size * 1.1f;
				bool isOk = false;
				int tryIdx = 0;
				int slotStrategy = 1;

				// give them sizes
				for_every(slot, containerSchematicSlots)
				{
					Range2 r = Range2::empty;
					r.include(slot->at.centre());
					r.expand_by(Vector2::one * size * 0.5f);
					slot->at = r;
				}

				while (!isOk && tryIdx < 300)
				{
					++tryIdx;

					for_every(moveSlot, moveSlots)
					{
						*moveSlot = Vector2::zero;
					}

					isOk = true;

					if ((tryIdx % 40) == 0)
					{
						slotStrategy = mod(slotStrategy + 1, 4);
					}

					for_every(s, containerSchematicSlots)
					{
						if (s->weaponPartType == WeaponPartType::GunChassis)
						{
							continue;
						}

						Vector2 move = Vector2::zero;
						Vector2 cat = s->at.centre();

						float moveAwayChassisCoef = 0.05f;
						float moveAwayCoef = 0.4f;
						float stayInsideCoef = 0.15f;

						for_every(a, containerSchematicSlots)
						{
							if (a != s)
							{
								if (a->weaponPartType == WeaponPartType::GunChassis)
								{
									Vector2 chAt = a->at.centre();
									Vector2 dist2 = cat - chAt;
									if (abs(dist2.x) < separation &&
										abs(dist2.y) < separation)
									{
										move += sign(dist2.x) * separation * clamp(1.0f - abs(dist2.x) / separation, 0.0f, 1.0f) * moveAwayChassisCoef * Vector2::xAxis;
										move += sign(dist2.y) * separation * clamp(1.0f - abs(dist2.y) / separation, 0.0f, 1.0f) * moveAwayChassisCoef * Vector2::yAxis;
									}
								}
								else
								{
									Vector2 atAt = a->at.centre();
									Vector2 dist2 = cat - atAt;
									if (abs(dist2.x) < separation &&
										abs(dist2.y) < separation)
									{
										if (slotStrategy == 0)
										{
											move += sign(dist2.x) * separation * clamp(1.0f - abs(dist2.x) / separation, 0.0f, 1.0f) * moveAwayCoef * Vector2::xAxis;
											move += sign(dist2.y) * separation * clamp(1.0f - abs(dist2.y) / separation, 0.0f, 1.0f) * moveAwayCoef * Vector2::yAxis;
										}
										else if (slotStrategy == 2)
										{
											move += sign(dist2.x) * separation * clamp(abs(dist2.x) / separation, 0.0f, 1.0f) * moveAwayCoef * Vector2::xAxis;
											move += sign(dist2.y) * separation * clamp(abs(dist2.y) / separation, 0.0f, 1.0f) * moveAwayCoef * Vector2::yAxis;
										}
										else
										{
											Vector2 imp(1.0f, 1.0f);
											if (abs(dist2.x) > abs(dist2.y)) imp = Vector2(1.0f, 0.3f);
											if (abs(dist2.y) > abs(dist2.x)) imp = Vector2(0.3f, 1.0f);
											move += imp.x * sign(dist2.x) * separation * clamp(abs(dist2.x) / separation, 0.0f, 1.0f) * moveAwayCoef * Vector2::xAxis;
											move += imp.y * sign(dist2.y) * separation * clamp(abs(dist2.y) / separation, 0.0f, 1.0f) * moveAwayCoef * Vector2::yAxis;
										}
									}
								}
							}
						}

						if (cat.x > validRange.x.max) move.x += stayInsideCoef * (validRange.x.max - cat.x);
						if (cat.x < validRange.x.min) move.x += stayInsideCoef * (validRange.x.min - cat.x);
						if (cat.y > validRange.y.max) move.y += stayInsideCoef * (validRange.y.max - cat.y);
						if (cat.y < validRange.y.min) move.y += stayInsideCoef * (validRange.y.min - cat.y);

						if (!move.is_zero())
						{
							isOk = false;
						}

						if (tryIdx > 50 && (tryIdx % 20) == 0)
						{
							if (!move.is_zero())
							{	// random fluctuation - stir a bit
								move.x += rg.get_float(-0.15f, 0.15f) * size;
								move.y += rg.get_float(-0.15f, 0.15f) * size;
								if (move.x != 0.0f && abs(move.x) < 1.0f)
								{
									move.x = sign(move.x);
								}
								if (move.y != 0.0f && abs(move.y) < 1.0f)
								{
									move.y = sign(move.y);
								}
							}
						}

						Vector2& moveSlot = moveSlots[for_everys_index(s)];

						moveSlot = move;
					}

#ifdef AN_TEST_LAYOUT
					dv->start_gathering_data();
					dv->clear();
#endif

#ifdef AN_TEST_LAYOUT
					dv->add_range2(validRange.expanded_by(Vector2(halfSize)), Colour::black);
					dv->add_range2(chassisSlot.at, Colour::green);
#endif

					for_every(a, containerSchematicSlots)
					{
						Vector2 moveSlot = moveSlots[for_everys_index(a)];
						if (moveSlot.x != 0.0f && abs(moveSlot.x) < 1.0f)
						{
							moveSlot.x = sign(moveSlot.x);
						}
						if (moveSlot.y != 0.0f && abs(moveSlot.y) < 1.0f)
						{
							moveSlot.y = sign(moveSlot.y);
						}

						a->at.x.min += moveSlot.x;
						a->at.x.max += moveSlot.x;
						a->at.y.min += moveSlot.y;
						a->at.y.max += moveSlot.y;

						a->at = Range2(maybe_round(a->at.centre())).expanded_by(Vector2(halfSize));

#ifdef AN_TEST_LAYOUT
						dv->add_range2(a->at, Colour::blue);
						dv->add_arrow(a->at.centre() - moveSlot, a->at.centre(), Colour::red);
#endif
					}
#ifdef AN_TEST_LAYOUT
					if (!isOk)
					{
						dv->add_text(Vector2::zero, String(TXT("repeat")), Colour::red);
						dv->add_text(validRange.centre(), String::printf(TXT("%i:%i"), tryIdx, slotStrategy), Colour::red, validRange.length().length() * 0.2f);
					}
#endif

#ifdef AN_TEST_LAYOUT
					dv->end_gathering_data();
					dv->show_and_wait_for_key();
#endif
				}
			}
		}

		// build schematic
		{
			containerSchematic = new TeaForGodEmperor::Schematic();

			// background lines
			{
				Range2 useRange = range;

				float detailWidth = min(range.x.length(), range.y.length()) * 0.01f;
				Colour detailColour = RegisteredColours::get_colour(String(TXT("weapon_part_schematic_background"))).mul_rgb(get_mul_rgb());
				Random::Generator rg;
				rg.set_seed(get_rand_index(), 536234);
				{
					Vector2 point00 = useRange.centre();
					for_count(int, i, rg.get_int_from_range(4, 10))
					{
						TeaForGodEmperor::SchematicLine* line = new TeaForGodEmperor::SchematicLine();
						line->set_sort_priority(-10);
						line->set_line_width(detailWidth);
						line->set_colour(detailColour);
						Vector2 dir = rg.get_chance(0.7f) ? Vector2::yAxis : Vector2::xAxis;
						Vector2 start = useRange.bottom_left();
						Range limits;
						float endAt;
						float border;
						float minBorderOffset;
						minBorderOffset = detailWidth * 0.0f;
						if (dir.x > dir.y)
						{
							start.x += minBorderOffset;
							start.y = rg.get_float(useRange.y.get_at(0.2f), useRange.y.get_at(0.8f));
							limits.min = useRange.y.get_at(0.1f);
							limits.max = useRange.y.get_at(0.9f);
							endAt = useRange.x.max - minBorderOffset;
							border = useRange.x.length() * 0.1f;
						}
						else
						{
							start.y += minBorderOffset;
							start.x = rg.get_float(useRange.x.get_at(0.2f), useRange.x.get_at(0.8f));
							limits.min = useRange.x.get_at(0.1f);
							limits.max = useRange.x.get_at(0.9f);
							endAt = useRange.y.max - minBorderOffset;
							border = useRange.y.length() * 0.1f;
						}
						Vector2 point = start;
						float& v = dir.x > dir.y ? point.x : point.y;
						float& o = dir.x > dir.y ? point.y : point.x; // limited by limits

						line->add(point - point00);
						v += rg.get_float(border, border * 3.0f);
						line->add(point - point00);
						while (v < endAt)
						{
							float offDir = rg.get_chance(0.5f) ? 0.0f : (rg.get_bool() ? -1.0f : 1.0f);
							float goFor = rg.get_float(border * 2.0f, min(border * 5.0f, endAt - border - v));
							if (v > endAt - border * 2.0f)
							{
								// end!
								offDir = 0.0f;
								goFor = endAt - v;
							}
							if (offDir != 0.0f)
							{
								float wouldGoFor = offDir < 0.0f ? min(goFor, o - limits.min) : min(goFor, limits.max - o);
								if (wouldGoFor < border)
								{
									offDir = 0.0f;
								}
								else
								{
									goFor = wouldGoFor;
								}
							}

							o += offDir * goFor;

							v += goFor;
							line->add(point - point00);
						}
						containerSchematic->add(line);
						if (rg.get_chance(0.3f))
						{
							float sep = detailWidth * 2.0f;
							float off = sep;
							for_count(int, c, rg.get_int_from_range(1, 3))
							{
								TeaForGodEmperor::SchematicLine* comp = new TeaForGodEmperor::SchematicLine();
								comp->be_companion_of(*line, off);
								containerSchematic->add(comp);
								off += sep;
							}
						}
					}
				}
			}

			// slot lines
			{
				SchematicSlot& ch = containerSchematicSlots.get_first();
				Vector2 centre = ch.at.centre();
				Vector2 point00 = centre;
				//float detailWidth = ch.at.length().length() * 0.07f;
				float detailWidth = min(range.x.length(), range.y.length()) * 0.02f;
				Colour detailColour = RegisteredColours::get_colour(String(TXT("weapon_part_schematic_detail"))).mul_rgb(get_mul_rgb());

				for_every(slot, containerSchematicSlots)
				{
					{
						Vector2 slotAt = slot->at.centre();
						if (slotAt != centre)
						{
							TeaForGodEmperor::SchematicLine* line = new TeaForGodEmperor::SchematicLine();
							line->set_line_width(detailWidth);
							line->set_colour(detailColour);

							Vector2 zero = centre;
							Vector2 start = centre;

							{
								Vector2 toSlot = (slotAt - centre).normal() * ch.at.length().length();
								zero = zero + toSlot * 0.15f;
								start = start + toSlot * 0.4f;
							}

							Vector2 mid = zero;
							{
								mid = start;
								Vector2 zeroToStart = start - zero;
								if (abs(zeroToStart.x) >= abs(zeroToStart.y))
								{
									mid.x = zero.x + sign(zeroToStart.x) * abs(zeroToStart.y);
								}
								else
								{
									mid.y = zero.y + sign(zeroToStart.y) * abs(zeroToStart.x);
								}
							}

							Vector2 diff = slotAt - start;
							float minDiff = min(abs(diff.x), abs(diff.y));
							float maxDiff = max(abs(diff.x), abs(diff.y));

							Random::Generator rg;
							rg.set_seed(get_rand_index(), 1282752 + 340 * for_everys_index(slot));

							float initialStraight = rg.get_float(0.0f, maxDiff - minDiff);
							if (initialStraight < detailWidth * 10.0f)
							{
								initialStraight = 0.0f;
							}
							float endStraight = maxDiff - (initialStraight + minDiff);

							Vector2 s0 = start;
							Vector2 s1 = slotAt;
							if (abs(diff.x) >= abs(diff.y))
							{
								s0.x += sign(diff.x) * initialStraight;
								s1.x -= sign(diff.x) * endStraight;
							}
							else
							{
								s0.y += sign(diff.y) * initialStraight;
								s1.y -= sign(diff.y) * endStraight;
							}

							line->add(zero - point00);
							if (mid != zero)
							{
								line->add(mid - point00);
							}
							line->add(start - point00);
							if (s0 != start)
							{
								line->add(s0 - point00);
							}
							line->add(s1 - point00);
							if (s1 != slotAt)
							{
								line->add(slotAt - point00);
							}

							containerSchematic->add(line);
						}
					}
				}
			}

			containerSchematic->build_mesh();
		}
	}
	visibleContainerSchematic = containerSchematic;
	containerSchematicSlotsAreAccessible = true;
}


void WeaponPart::build_schematic_mesh()
{
	ASSERT_ASYNC_OR_LOADING_OR_PREVIEW;
	an_assert(weaponPartType.get());

	Random::Generator rg;
	RG(149852);

	// create it offline and plug in only when finalised
	RefCountObjectPtr<Schematic> sch;
	sch = new Schematic();

	Vector2 size = get_schematic_size();
	Vector2 halfSize = size * 0.5f;
	float minSize = min(size.x, size.y);

	float detailWidth = minSize * 0.04f;
	float outlineWidth = minSize * 0.06f;
	//float halfNarrowSafeRegionPt = 0.5f;
	float halfSafeRegionPt = 0.8f;

	float mulrgb = get_mul_rgb();

	Colour interiorColour				= RegisteredColours::get_colour(String(TXT("weapon_part_schematic_interior"))).mul_rgb(mulrgb);
	Colour outlineColour				= RegisteredColours::get_colour(String(TXT("weapon_part_schematic_outline"))).mul_rgb(mulrgb);
	Colour detailColour					= RegisteredColours::get_colour(String(TXT("weapon_part_schematic_detail"))).mul_rgb(mulrgb);
	Colour plasmaColour					= RegisteredColours::get_colour(String(TXT("weapon_part_schematic_plasma"))).mul_rgb(mulrgb);
	Colour corrosionColour				= RegisteredColours::get_colour(String(TXT("weapon_part_schematic_corrosion"))).mul_rgb(mulrgb);
	Colour lightningColour				= RegisteredColours::get_colour(String(TXT("weapon_part_schematic_lightning"))).mul_rgb(mulrgb);
	Colour chassisColour				= RegisteredColours::get_colour(String(TXT("weapon_part_schematic_chassis"))).mul_rgb(mulrgb);
	Colour chamberColour				= RegisteredColours::get_colour(String(TXT("weapon_part_schematic_chamber"))).mul_rgb(mulrgb);
	Colour storageColour				= RegisteredColours::get_colour(String(TXT("weapon_part_schematic_storage"))).mul_rgb(mulrgb);
	Colour storageSpeedColour			= RegisteredColours::get_colour(String(TXT("weapon_part_schematic_storage_speed"))).mul_rgb(mulrgb);
	Colour magazineColour				= RegisteredColours::get_colour(String(TXT("weapon_part_schematic_magazine"))).mul_rgb(mulrgb);
	Colour magazineSpeedColour			= RegisteredColours::get_colour(String(TXT("weapon_part_schematic_magazine_speed"))).mul_rgb(mulrgb);
	Colour magazineCooldownColour		= RegisteredColours::get_colour(String(TXT("weapon_part_schematic_magazine_cool"))).mul_rgb(mulrgb);
	Colour damageColour					= RegisteredColours::get_colour(String(TXT("weapon_part_schematic_damage"))).mul_rgb(mulrgb);
	Colour specialColour				= RegisteredColours::get_colour(String(TXT("weapon_part_schematic_special"))).mul_rgb(mulrgb);
	Colour armourPiercingColour			= RegisteredColours::get_colour(String(TXT("weapon_part_schematic_armour_pierce"))).mul_rgb(mulrgb);
	Colour continuousFireColour			= RegisteredColours::get_colour(String(TXT("weapon_part_schematic_cont_fire"))).mul_rgb(mulrgb);
	Colour projectileColour				= RegisteredColours::get_colour(String(TXT("weapon_part_schematic_projectile"))).mul_rgb(mulrgb);
	Colour multipleProjectilesColour	= RegisteredColours::get_colour(String(TXT("weapon_part_schematic_multiple_proj"))).mul_rgb(mulrgb);
	Colour singleProjectileColour		= RegisteredColours::get_colour(String(TXT("weapon_part_schematic_single_proj"))).mul_rgb(mulrgb);
	Colour projectileSpreadColour		= RegisteredColours::get_colour(String(TXT("weapon_part_schematic_proj_spread"))).mul_rgb(mulrgb);
	Colour projectileNoSpreadColour		= RegisteredColours::get_colour(String(TXT("weapon_part_schematic_proj_no_spread"))).mul_rgb(mulrgb);
	Colour projectileNegSpreadColour	= RegisteredColours::get_colour(String(TXT("weapon_part_schematic_proj_neg_spread"))).mul_rgb(mulrgb);
	Colour projectileSpeedColour		= RegisteredColours::get_colour(String(TXT("weapon_part_schematic_proj_speed"))).mul_rgb(mulrgb);
	
	if (!get_damaged().is_zero())
	{
		Colour damagedColour = RegisteredColours::get_colour(String(TXT("weapon_part_schematic_damaged"))).mul_rgb(mulrgb); 

		outlineColour				= damagedColour;
		detailColour				= damagedColour;
		plasmaColour				= damagedColour;
		corrosionColour				= damagedColour;
		lightningColour				= damagedColour;
		chassisColour				= damagedColour;
		chamberColour				= damagedColour;
		storageColour				= damagedColour;
		storageSpeedColour			= damagedColour;
		magazineColour				= damagedColour;
		magazineSpeedColour			= damagedColour;
		magazineCooldownColour		= damagedColour;
		damageColour				= damagedColour;
		specialColour				= damagedColour;
		armourPiercingColour		= damagedColour;
		continuousFireColour		= damagedColour;
		projectileColour			= damagedColour;
		multipleProjectilesColour	= damagedColour;
		singleProjectileColour		= damagedColour;
		projectileSpreadColour		= damagedColour;
		projectileNoSpreadColour	= damagedColour;
		projectileNegSpreadColour	= damagedColour;
		projectileSpeedColour		= damagedColour;
	}

	// add content

	float centreSizePt = 0.3f;
	float centreSize = centreSizePt * minSize;
	float halfCentreSize = centreSize * 0.5f;
	Vector2 centreAt = Vector2::zero;
	{
		float offPt = 1.0f;
		if (weaponPartType->get_type() == WeaponPartType::GunMuzzle)
		{
			centreAt.x = 0.0f; // has to be centred
			centreAt.y = rg.get_float(-halfSize.y + centreSize * offPt, -halfCentreSize);
		}
		else if (weaponPartType->get_type() == WeaponPartType::GunNode)
		{
			float const mid = 0.1f;
			offPt = 0.1f;
			centreAt.x = 0.0f; // has to be centred
			centreAt.y = rg.get_float(-halfSafeRegionPt * halfSize.y * mid + centreSize * offPt, halfSafeRegionPt * halfSize.y * mid - centreSize * offPt);
		}
		else if (weaponPartType->get_type() == WeaponPartType::GunChassis)
		{
			float const mid = 0.5f;
			centreAt.x = rg.get_float(-halfSafeRegionPt * halfSize.x * mid + centreSize * offPt, halfSafeRegionPt * halfSize.x * mid - centreSize * offPt);
			centreAt.y = rg.get_float(-halfSafeRegionPt * halfSize.y * mid + centreSize * offPt, halfSafeRegionPt * halfSize.y * mid - centreSize * offPt);
		}
	}

	// add lines from centre to sides

	Name dedicatedSchematic = weaponPartType->get_dedicated_schematic();

	ARRAY_STACK(WeaponPartSchematicFeature, features, 32);
	{
		WeaponPartSchematicFeature::store(features, chamber, NAME(chamber), Energy(20.0f));
		WeaponPartSchematicFeature::store(features, chamberCoef, NAME(chamber), EnergyCoef(0.2f), NAME(chamberNeg), EnergyCoef(-0.2f));
		WeaponPartSchematicFeature::store(features, baseDamageBoost, NAME(damage), Energy(2.0f), NAME(damageNeg), Energy(-2.0f));
		WeaponPartSchematicFeature::store(features, damageBoost, NAME(damage), Energy(10.0f), NAME(damageNeg), Energy(-10.0f));
		WeaponPartSchematicFeature::store(features, damageCoef, NAME(damage), EnergyCoef(0.2f), NAME(damageNeg), EnergyCoef(-0.2f));
		WeaponPartSchematicFeature::store(features, armourPiercing, NAME(armourPiercing), EnergyCoef(0.25f));
		WeaponPartSchematicFeature::store(features, storage, NAME(storage), Energy(200.0f));
		WeaponPartSchematicFeature::store(features, storageOutputSpeed, NAME(storageOutputSpeed), Energy(30.0f));
		WeaponPartSchematicFeature::store(features, storageOutputSpeedAdd, NAME(storageOutputSpeed), Energy(20.0f));
		WeaponPartSchematicFeature::store(features, storageOutputSpeedAdj, NAME(storageOutputSpeed), EnergyCoef(0.5f));
		WeaponPartSchematicFeature::store(features, magazine, NAME(magazine), Energy(40.0f));
		WeaponPartSchematicFeature::store(features, magazineOutputSpeed, NAME(magazineOutputSpeed), Energy(70.0f));
		WeaponPartSchematicFeature::store(features, magazineOutputSpeedAdd, NAME(magazineOutputSpeed), Energy(70.0f));
		WeaponPartSchematicFeature::store(features, magazineCooldown, NAME(magazineCooldown), 0.3f, NAME(magazineCooldownNeg), -0.3f);
		WeaponPartSchematicFeature::store(features, magazineCooldownCoef, NAME(magazineCooldown), 0.2f, NAME(magazineCooldownNeg), -0.2f);
		WeaponPartSchematicFeature::store(features, continuousFire, NAME(continuousFire), 1.0f);
		if (weaponPartType->get_type() != WeaponPartType::GunChassis)
		{
			WeaponPartSchematicFeature::store(features, projectileSpeed, NAME(projectileSpeed), 50.0f);
			WeaponPartSchematicFeature::store(features, projectileSpeedCoef, NAME(projectileSpeed), 0.2f);
			if (projectileSpread.value.is_set())
			{
				if (projectileSpread.value.get() == 0.0f)
				{
					WeaponPartSchematicFeature::store(features, NAME(projectileNoSpread), 10.0f);
				}
				else if (projectileSpread.value.get() > 0.0f)
				{
					WeaponPartSchematicFeature::store(features, projectileSpread, NAME(projectileSpread), 4.0f);
				}
				else
				{
					WeaponPartSchematicFeature::store(features, projectileSpread, NAME(projectileNegSpread), 4.0f);
				}
			}
			WeaponPartSchematicFeature::store(features, projectileSpreadCoef, NAME(projectileSpread), 0.3f, NAME(projectileNegSpread), -0.3f);
			if (numberOfProjectiles.value.is_set())
			{
				if (numberOfProjectiles.value.get() > 1)
				{
					WeaponPartSchematicFeature::store(features, NAME(multipleProjectiles), 1.0f);
				}
				else
				{
					WeaponPartSchematicFeature::store(features, NAME(singleProjectile), 1.0f);
				}
			}
		}
		WeaponPartSchematicFeature::store(features, antiDeflection, NAME(antiDeflection), 0.5f);
		WeaponPartSchematicFeature::store(features, maxDistCoef, NAME(maxDistCoef), 0.2f, NAME(maxDistCoefNeg), -0.2f);
		WeaponPartSchematicFeature::store(features, kineticEnergyCoef, NAME(kineticEnergyCoef), 1.5f);
		WeaponPartSchematicFeature::store(features, confuse, NAME(confuse), 1.5f);
		WeaponPartSchematicFeature::store(features, mediCoef, NAME(mediCoef), EnergyCoef(0.2f), NAME(mediCoefNeg), EnergyCoef(-0.1f));

		// we need to have at least one feature
		if (features.is_empty() && dedicatedSchematic.is_valid())
		{
			WeaponPartSchematicFeature::store(features, dedicatedSchematic, 4.0f);
		}

		sort(features);
	}

	RG(834597);
	Array<SchematicLine*> lines;
	int mainLinesRequired = max(2, rg.get_int_from_range(-1, 2) + features.get_size() * 3 / 5);
	int linesRequired = 0;
	for_every(feature, features)
	{
		feature->lines = max(1, TypeConversions::Normal::f_i_closest(floor(feature->importance)));
		if (feature->stat == NAME(projectileNoSpread) ||
			feature->stat == NAME(projectileNegSpread))
		{
			feature->lines = 1;
		}
		linesRequired += feature->lines;
	}
	int mainLinesCount = 0;
	while (mainLinesCount < mainLinesRequired || lines.get_size() < linesRequired)
	{
		RG(98797 + mainLinesCount);

		SchematicLine* line = new SchematicLine();
		line->set_line_width(detailWidth);
		line->set_colour(detailColour);

		Vector2 at = centreAt;
		line->add(at);
		Vector2 dir = Vector2::yAxis.rotated_by_angle(45.0f * (float)rg.get_int(8));
		while (at.x >= -halfSize.x && at.x <= halfSize.x && at.y >= -halfSize.y && at.y <= halfSize.y)
		{
			float dist = (0.05f + 0.05f * (float)rg.get_int_from_range(1, 5)) * minSize;
			if (! dir.is_almost_zero())
			{
				float maxSize = max(abs(dir.x), abs(dir.y));
				dir *= 1.0f / maxSize; // to align grid
			}
			at += dir * dist;
			dir = dir.rotated_by_angle(45.0f * (float)rg.get_int_from_range(-1, 1));
			Vector2 safeDir = Vector2::yAxis;
			Vector2 relAt = at - centreAt;
			if (relAt.x > 0.0f && abs(relAt.x) <= abs(relAt.y))
			{
				dir.x = max(0.0f, dir.x);
				safeDir = Vector2::xAxis;
			}
			if (relAt.x < 0.0f && abs(relAt.x) <= abs(relAt.y))
			{
				dir.x = min(0.0f, dir.x);
				safeDir = -Vector2::xAxis;
			}
			if (relAt.y > 0.0f && abs(relAt.y) <= abs(relAt.x))
			{
				dir.y = max(0.0f, dir.y);
				safeDir = Vector2::yAxis;
			}
			if (relAt.y < 0.0f && abs(relAt.y) <= abs(relAt.x))
			{
				dir.y = min(0.0f, dir.y);
				safeDir = -Vector2::yAxis;
			}
			if (dir.is_almost_zero())
			{
				dir = safeDir;
			}
			dir = dir.normal();
			line->add(at);
		}
		lines.push_back(line);
		sch->add(line);
		++mainLinesCount;

		float offsets[] = { 1.0f, -1.0f, 2.0f, -2.0f, 3.0f, -3.0f };
		for_count(int, i, rg.get_int_from_range(0, 2))
		{
			SchematicLine* comp = new SchematicLine();
			comp->be_companion_of(*line, detailWidth * 2.0f * (float)offsets[i]);
			lines.push_back(comp);
			sch->add(comp);
		}
	}

	Name centreFeature;

	// add centre
	if (weaponPartType->get_type() == WeaponPartType::GunCore && coreKind.value.is_set())
	{
		if (coreKind.value.get() == WeaponCoreKind::Plasma)
		{
			{
				// diamond
				SchematicLine* line = new SchematicLine();
				line->set_sort_priority(20);
				line->set_line_width(outlineWidth);
				line->set_colour(outlineColour);
				line->set_looped();
				line->set_filled(plasmaColour);
				float sideF = halfCentreSize * 1.5f;
				line->add(centreAt + Vector2(0.0f, sideF));
				line->add(centreAt + Vector2(sideF, 0.0f));
				line->add(centreAt + Vector2(0.0f, -sideF));
				line->add(centreAt + Vector2(-sideF, 0.0f));
				sch->cut_inner_lines_with(*line);
				sch->add(line);
			}
		}
		else if (coreKind.value.get() == WeaponCoreKind::Corrosion)
		{
			{
				// hex
				SchematicLine* line = new SchematicLine();
				line->set_sort_priority(20);
				line->set_line_width(outlineWidth);
				line->set_colour(outlineColour);
				line->set_looped();
				line->set_filled(corrosionColour);
				float sideF = halfCentreSize * 1.5f;
				float sideC = sideF * 0.3f;
				line->add(centreAt + Vector2(sideC, sideF));
				line->add(centreAt + Vector2(sideF, 0.0f));
				line->add(centreAt + Vector2(sideC, -sideF));
				line->add(centreAt + Vector2(-sideC, -sideF));
				line->add(centreAt + Vector2(-sideF, 0.0f));
				line->add(centreAt + Vector2(-sideC, sideF));
				sch->cut_inner_lines_with(*line);
				sch->add(line);
			}
		}
		else if (coreKind.value.get() == WeaponCoreKind::Lightning)
		{
			{
				// two triangles - lightning
				for_count(int, t, 2)
				{
					float sideT = t == 0 ? 1.0f : -1.0f;
					SchematicLine* line = new SchematicLine();
					line->set_sort_priority(20);
					line->set_line_width(outlineWidth);
					line->set_colour(outlineColour);
					line->set_looped();
					line->set_filled(lightningColour);
					float sideF = halfCentreSize * 1.5f;
					float sideC = sideF * 0.6f;
					float sideCH = sideF * 0.3f;
					line->add(centreAt + sideT * Vector2(sideCH, 0.0f));
					line->add(centreAt + sideT * Vector2(-sideF, 0.0f));
					line->add(centreAt + sideT * Vector2(sideC, sideF));
					line->add(centreAt + sideT * Vector2(sideCH, 0.0f));
					sch->cut_inner_lines_with(*line);
					sch->add(line);
				}
			}
		}
		else
		{
			todo_implement
		}
	}
	else if (weaponPartType->get_type() == WeaponPartType::GunChassis)
	{
		float radius = halfCentreSize * 1.0f;
		{
			SchematicLine* line = new SchematicLine();
			line->set_sort_priority(20);
			line->set_line_width(outlineWidth);
			line->set_colour(outlineColour);
			line->set_looped();
			line->set_filled(chassisColour);
			int aCount = 24;
			for (int a = 0; a < aCount; ++a)
			{
				line->add(centreAt + (Vector2::yAxis * radius).rotated_by_angle(360.0f * ((float)a / (float)aCount)));
			}
			sch->cut_inner_lines_with(*line);
			sch->add(line);
		}
		{
			int nCount = weaponPartType->get_slot_count(WeaponPartType::GunNode);
			if (nCount > 0)
			{
				float anRadius = nCount == 1? 0.0f : radius;
				float startingAngle = 0.0f;
				if (nCount == 2)
				{
					startingAngle = rg.get_chance(0.8f) ? 90.0f : 0.0f;
				}
				if (nCount == 3)
				{
					startingAngle = rg.get_chance(0.6f) ? 0.0f : 180.0f;
				}
				if (nCount == 4)
				{
					startingAngle = rg.get_chance(0.6f) ? 45.0f : 0.0f;
				}
				for (int n = 0; n < nCount; ++n)
				{
					SchematicLine* line = new SchematicLine();
					line->set_sort_priority(21);
					line->set_line_width(outlineWidth);
					line->set_colour(outlineColour);
					line->set_outline(false);
					line->set_looped();
					line->set_filled(outlineColour);
					float nRadius = radius * 0.2f;
					Vector2 at = centreAt + (Vector2::yAxis * anRadius * 0.35f).rotated_by_angle(startingAngle + 360.0f * ((float)n / (float)nCount));
					int aCount = 12;
					for (int a = 0; a < aCount; ++a)
					{
						line->add(at + (Vector2::yAxis * nRadius).rotated_by_angle(360.0f * ((float)a / (float)aCount)));
					}
					sch->cut_inner_lines_with(*line);
					sch->add(line);
				}
			}
		}
	}
	else if (weaponPartType->get_type() != WeaponPartType::GunCore)
	{
		int const highestPriority = 3;
		int const priorityDedicated = 3;
		int const priorityA = 2;
		int const priorityB = 1;
		int const priorityC = 0;
		for (int priority = highestPriority; priority >= 0; priority--)
		{
			for_every(feature, features)
			{
				// (do) = dedicated only
				// magazine, storage
				if ((dedicatedSchematic == NAME(magazine) && priority == priorityDedicated) ||
					(dedicatedSchematic == NAME(storage) && priority == priorityDedicated) ||
					(feature->stat == NAME(magazine) && priority == priorityA) ||
					(feature->stat == NAME(storage) && priority == priorityC))
				{
					centreFeature = priority == priorityDedicated? dedicatedSchematic : feature->stat;
					{
						// rectangle
						float sideC = halfCentreSize * 0.6f;
						float sideF = halfCentreSize * 1.1f;
						{
							SchematicLine* line = new SchematicLine();
							line->set_sort_priority(20);
							line->set_line_width(outlineWidth);
							line->set_colour(outlineColour);
							line->set_looped();
							line->set_filled(feature->stat == NAME(magazine)? magazineColour : storageColour);
							line->add(centreAt + Vector2(sideC, sideF));
							line->add(centreAt + Vector2(sideC, -sideF));
							line->add(centreAt + Vector2(-sideC, -sideF));
							line->add(centreAt + Vector2(-sideC, sideF));
							sch->cut_inner_lines_with(*line);
							sch->add(line);
						}
						// crossed
						{
							SchematicLine* line = new SchematicLine();
							line->set_sort_priority(21);
							line->set_line_width(outlineWidth);
							line->set_colour(outlineColour);
							line->add(centreAt + Vector2(-sideC, -sideC));
							line->add(centreAt + Vector2(sideC, sideC));
							sch->add(line);
						}
					}
					break;
				}
				// magazine output speed, storage output speed
				if ((dedicatedSchematic == NAME(magazineOutputSpeed) && priority == priorityDedicated) ||
					(dedicatedSchematic == NAME(storageOutputSpeed) && priority == priorityDedicated) ||
					(feature->stat == NAME(magazineOutputSpeed) && priority == priorityB) ||
					(feature->stat == NAME(storageOutputSpeed) && priority == priorityB))
				{
					centreFeature = priority == priorityDedicated ? dedicatedSchematic : feature->stat;
					{
						// rectangle
						float sideC = halfCentreSize * 0.6f;
						float sideF = halfCentreSize * 1.1f;
						{
							SchematicLine* line = new SchematicLine();
							line->set_sort_priority(20);
							line->set_line_width(outlineWidth);
							line->set_colour(outlineColour);
							line->set_looped();
							line->set_filled(dedicatedSchematic == NAME(magazineOutputSpeed) || feature->stat == NAME(magazine) ? magazineColour : storageColour);
							line->add(centreAt + Vector2(sideC, sideF));
							line->add(centreAt + Vector2(sideC, -sideF));
							line->add(centreAt + Vector2(-sideC, -sideF));
							line->add(centreAt + Vector2(-sideC, sideF));
							sch->cut_inner_lines_with(*line);
							sch->add(line);
						}
						// /\ mark
						{
							SchematicLine* line = new SchematicLine();
							line->set_sort_priority(21);
							line->set_line_width(outlineWidth);
							line->set_colour(outlineColour);
							line->add(centreAt + Vector2(-sideC, -sideC * 0.5f));
							line->add(centreAt + Vector2(0.0f, sideC * 0.5f));
							line->add(centreAt + Vector2(sideC, -sideC * 0.5f));
							sch->add(line);
						}
					}
					break;
				}
				// supercharger
				if ((dedicatedSchematic == NAME(supercharger) && priority == priorityDedicated))
				{
					centreFeature = priority == priorityDedicated ? dedicatedSchematic : feature->stat;
					{
						// rectangle (a bit bigger)
						float sideC = halfCentreSize * 0.8f;
						float sideF = halfCentreSize * 1.5f;
						{
							SchematicLine* line = new SchematicLine();
							line->set_sort_priority(20);
							line->set_line_width(outlineWidth);
							line->set_colour(outlineColour);
							line->set_looped();
							line->set_filled(dedicatedSchematic == NAME(magazineOutputSpeed) || feature->stat == NAME(magazine) ? magazineColour : storageColour);
							line->add(centreAt + Vector2(sideC, sideF));
							line->add(centreAt + Vector2(sideC, -sideF));
							line->add(centreAt + Vector2(-sideC, -sideF));
							line->add(centreAt + Vector2(-sideC, sideF));
							sch->cut_inner_lines_with(*line);
							sch->add(line);
						}
						// double /\ mark
						float marksAt[] = {  sideF * 0.3f,
											-sideF * 0.4f };
						for_count(int, m, 2)
						{
							SchematicLine* line = new SchematicLine();
							line->set_sort_priority(21);
							line->set_line_width(outlineWidth);
							line->set_colour(outlineColour);
							line->add(centreAt + Vector2(-sideC,marksAt[m] -sideC * 0.5f));
							line->add(centreAt + Vector2(0.0f, marksAt[m] + sideC * 0.5f));
							line->add(centreAt + Vector2(sideC, marksAt[m] -sideC * 0.5f));
							sch->add(line);
						}
					}
					break;
				}
				// retaliator
				if ((dedicatedSchematic == NAME(retaliator) && priority == priorityDedicated))
				{
					centreFeature = priority == priorityDedicated ? dedicatedSchematic : feature->stat;
					{
						// wide cross
						float sideC = halfCentreSize * 1.0f;
						float sideF = halfCentreSize * 1.3f;
						{
							SchematicLine* line = new SchematicLine();
							line->set_sort_priority(20);
							line->set_line_width(outlineWidth);
							line->set_colour(outlineColour);
							line->set_looped();
							line->set_filled(damageColour);
							line->add(centreAt + Vector2( sideC,  sideF));
							line->add(centreAt + Vector2( sideC,  sideC));
							line->add(centreAt + Vector2( sideF,  sideC));
							line->add(centreAt + Vector2( sideF, -sideC));
							line->add(centreAt + Vector2( sideC, -sideC));
							line->add(centreAt + Vector2( sideC, -sideF));
							line->add(centreAt + Vector2(-sideC, -sideF));
							line->add(centreAt + Vector2(-sideC, -sideC));
							line->add(centreAt + Vector2(-sideF, -sideC));
							line->add(centreAt + Vector2(-sideF,  sideC));
							line->add(centreAt + Vector2(-sideC,  sideC));
							line->add(centreAt + Vector2(-sideC,  sideF));
							line->add(centreAt + Vector2(-sideC,  sideF));
							sch->cut_inner_lines_with(*line);
							sch->add(line);
						}
						// /\ mark
						{
							SchematicLine* line = new SchematicLine();
							line->set_sort_priority(21);
							line->set_line_width(outlineWidth);
							line->set_colour(outlineColour);
							line->add(centreAt + Vector2(-sideF, -sideF * 0.5f));
							line->add(centreAt + Vector2(  0.0f,  sideF * 0.5f));
							line->add(centreAt + Vector2( sideF, -sideF * 0.5f));
							sch->add(line);
						}
					}
					break;
				}
				// devastator
				if ((dedicatedSchematic == NAME(devastator) && priority == priorityDedicated))
				{
					centreFeature = priority == priorityDedicated ? dedicatedSchematic : feature->stat;
					{
						// square with spikes at corners
						float sideC = halfCentreSize * 0.9f;
						float sideF = halfCentreSize * 1.5f;
						float sideS = halfCentreSize * 0.5f;
						float sideM = halfCentreSize * 0.6f;
						{
							SchematicLine* line = new SchematicLine();
							line->set_sort_priority(20);
							line->set_line_width(outlineWidth);
							line->set_colour(outlineColour);
							line->set_looped();
							line->set_filled(damageColour);
							line->add(centreAt + Vector2( sideF,  sideF));
							line->add(centreAt + Vector2( sideC,  sideS));
							line->add(centreAt + Vector2( sideC, -sideS));
							line->add(centreAt + Vector2( sideF, -sideF));
							line->add(centreAt + Vector2( sideS, -sideC));
							line->add(centreAt + Vector2(-sideS, -sideC));
							line->add(centreAt + Vector2(-sideF, -sideF));
							line->add(centreAt + Vector2(-sideC, -sideS));
							line->add(centreAt + Vector2(-sideC,  sideS));
							line->add(centreAt + Vector2(-sideF,  sideF));
							line->add(centreAt + Vector2(-sideS,  sideC));
							line->add(centreAt + Vector2( sideS,  sideC));
							line->add(centreAt + Vector2( sideF,  sideF));
							sch->cut_inner_lines_with(*line);
							sch->add(line);
						}
						// /\ mark
						{
							SchematicLine* line = new SchematicLine();
							line->set_sort_priority(21);
							line->set_line_width(outlineWidth);
							line->set_colour(outlineColour);
							line->add(centreAt + Vector2(-sideM, -sideM * 0.5f));
							line->add(centreAt + Vector2(  0.0f,  sideM * 0.5f));
							line->add(centreAt + Vector2( sideM, -sideM * 0.5f));
							sch->add(line);
						}
					}
					break;
				}
				// muzzles
				if ((dedicatedSchematic == NAME(muzzle) && priority == priorityDedicated) ||
					((feature->stat == NAME(singleProjectile) ||
					  feature->stat == NAME(multipleProjectiles)) && priority == priorityA))
				{
					// for this use shape + colour depending on spread
					Name useFeatureStat = feature->stat;
					if (priority == priorityDedicated)
					{
						for_every(feature, features)
						{
							if (feature->stat == NAME(singleProjectile)) { useFeatureStat = feature->stat; }
							if (feature->stat == NAME(multipleProjectiles)) { useFeatureStat = feature->stat; }
						}
					}

					Colour innerColour = singleProjectileColour;
					if (useFeatureStat == NAME(multipleProjectiles))
					{
						innerColour = multipleProjectilesColour;
					}

					for_every(feature, features)
					{
						if (feature->stat == NAME(projectileNoSpread)) { innerColour = projectileNoSpreadColour; }
						if (feature->stat == NAME(projectileNegSpread)) { innerColour = projectileNegSpreadColour; }
						if (feature->stat == NAME(projectileSpread)) { innerColour = projectileSpreadColour; }
					}

					if (useFeatureStat == NAME(singleProjectile))
					{
						centreFeature = priority == priorityDedicated ? dedicatedSchematic : feature->stat;
						{
							float sideC = halfCentreSize * 0.6f;
							float sideF = halfCentreSize * 1.0f;
							{
								SchematicLine* line = new SchematicLine();
								line->set_sort_priority(20);
								line->set_line_width(outlineWidth);
								line->set_colour(outlineColour);
								line->set_looped();
								line->set_filled(innerColour);
								line->add(centreAt + Vector2(-sideC, sideF));
								line->add(centreAt + Vector2(sideC, sideF));
								line->add(centreAt + Vector2(sideF, -sideF));
								line->add(centreAt + Vector2(-sideF, -sideF));
								sch->cut_inner_lines_with(*line);
								sch->add(line);
							}
						}
						break;
					}
					if (useFeatureStat == NAME(multipleProjectiles))
					{
						centreFeature = priority == priorityDedicated ? dedicatedSchematic : feature->stat;
						{
							float sideF = halfCentreSize * 1.0f;
							{
								SchematicLine* line = new SchematicLine();
								line->set_sort_priority(20);
								line->set_line_width(outlineWidth);
								line->set_colour(outlineColour);
								line->set_looped();
								line->set_filled(innerColour);
								line->add(centreAt + Vector2(-sideF, sideF));
								line->add(centreAt + Vector2(sideF, sideF));
								line->add(centreAt + Vector2(sideF, -sideF));
								line->add(centreAt + Vector2(-sideF, -sideF));
								sch->cut_inner_lines_with(*line);
								sch->add(line);
							}
							// divided in 2
							{
								SchematicLine* line = new SchematicLine();
								line->set_sort_priority(21);
								line->set_line_width(outlineWidth);
								line->set_colour(outlineColour);
								line->add(centreAt + Vector2(0.0f, -sideF));
								line->add(centreAt + Vector2(0.0f, sideF));
								sch->add(line);
							}
						}
						break;
					}
				}
				// armour piercing
				if ((dedicatedSchematic == NAME(armourPiercing) && priority == priorityDedicated) ||
					(feature->stat == NAME(armourPiercing) && priority == priorityA))
				{
					centreFeature = priority == priorityDedicated ? dedicatedSchematic : feature->stat;
					{
						// cut-in triangle
						float sideC = halfCentreSize * 0.8f;
						float sideF = halfCentreSize * 1.1f;
						{
							SchematicLine* line = new SchematicLine();
							line->set_sort_priority(20);
							line->set_line_width(outlineWidth);
							line->set_colour(outlineColour);
							line->set_looped();
							line->set_filled(damageColour);
							line->add(centreAt + Vector2(0.0f, sideF));
							line->add(centreAt + Vector2(sideC, -sideF));
							line->add(centreAt + Vector2(0.0f, -sideF * 0.3f));
							line->add(centreAt + Vector2(-sideC, -sideF));
							sch->cut_inner_lines_with(*line);
							sch->add(line);
						}
					}
					break;
				}
				// continuous fire
				if ((dedicatedSchematic == NAME(continuousFire) && priority == priorityDedicated) ||
					(feature->stat == NAME(continuousFire) && priority == priorityA))
				{
					centreFeature = priority == priorityDedicated ? dedicatedSchematic : feature->stat;
					{
						// hexagon with line inside
						float sideC = halfCentreSize * 0.6f;
						float sideS = halfCentreSize * 0.4f;
						float sideF = halfCentreSize * 1.0f;
						{
							SchematicLine* line = new SchematicLine();
							line->set_sort_priority(20);
							line->set_line_width(outlineWidth);
							line->set_colour(outlineColour);
							line->set_looped();
							line->set_filled(damageColour);
							line->add(centreAt + Vector2(0.0f, sideF));
							line->add(centreAt + Vector2(sideC, sideS));
							line->add(centreAt + Vector2(sideC, -sideS));
							line->add(centreAt + Vector2(0.0f, -sideF));
							line->add(centreAt + Vector2(-sideC, -sideS));
							line->add(centreAt + Vector2(-sideC, sideS));
							sch->cut_inner_lines_with(*line);
							sch->add(line);
						}
						{
							SchematicLine* line = new SchematicLine();
							line->set_sort_priority(21);
							line->set_line_width(outlineWidth);
							line->set_colour(outlineColour);
							line->add(centreAt + Vector2(0.0f, sideS));
							line->add(centreAt + Vector2(0.0f, -sideS));
							sch->add(line);
						}
					}
					break;
				}
				// damage
				if ((dedicatedSchematic == NAME(damage) && priority == priorityDedicated) ||
					(feature->stat == NAME(damage) && priority == priorityC))
				{
					centreFeature = priority == priorityDedicated ? dedicatedSchematic : feature->stat;
					{
						// triangle
						float sideC = halfCentreSize * 0.8f;
						float sideF = halfCentreSize * 1.1f;
						{
							SchematicLine* line = new SchematicLine();
							line->set_sort_priority(20);
							line->set_line_width(outlineWidth);
							line->set_colour(outlineColour);
							line->set_looped();
							line->set_filled(damageColour);
							line->add(centreAt + Vector2(0.0f, sideF));
							line->add(centreAt + Vector2(sideC, -sideF));
							line->add(centreAt + Vector2(-sideC, -sideF));
							sch->cut_inner_lines_with(*line);
							sch->add(line);
						}
					}
					break;
				}
				// damage
				if ((dedicatedSchematic == NAME(baseDamage) && priority == priorityDedicated) ||
					(feature->stat == NAME(baseDamage) && priority == priorityC))
				{
					centreFeature = priority == priorityDedicated ? dedicatedSchematic : feature->stat;
					{
						// triangle with rectangle below
						float sideC = halfCentreSize * 0.8f;
						float offset = halfCentreSize * 0.2f;
						float sideF = halfCentreSize * 1.7f + offset;
						float sideF0 = -halfCentreSize * 0.3f + offset;
						float sideF1 = -halfCentreSize * 1.0f + offset;
						float sideF2 = -halfCentreSize * 1.7f + offset;
						{
							SchematicLine* line = new SchematicLine();
							line->set_sort_priority(20);
							line->set_line_width(outlineWidth);
							line->set_colour(outlineColour);
							line->set_looped();
							line->set_filled(damageColour);
							line->add(centreAt + Vector2(0.0f, sideF));
							line->add(centreAt + Vector2(sideC, sideF0));
							line->add(centreAt + Vector2(-sideC, sideF0));
							sch->cut_inner_lines_with(*line);
							sch->add(line);
						}
						{
							SchematicLine* line = new SchematicLine();
							line->set_sort_priority(20);
							line->set_line_width(outlineWidth);
							line->set_colour(outlineColour);
							line->set_looped();
							line->set_filled(damageColour);
							line->add(centreAt + Vector2(-sideC, sideF1));
							line->add(centreAt + Vector2( sideC, sideF1));
							line->add(centreAt + Vector2( sideC, sideF2));
							line->add(centreAt + Vector2(-sideC, sideF2));
							sch->cut_inner_lines_with(*line);
							sch->add(line);
						}
					}
					break;
				}
				// kinetic energy coef
				if ((dedicatedSchematic == NAME(kineticEnergyCoef) && priority == priorityDedicated) ||
					(feature->stat == NAME(kineticEnergyCoef) && priority == priorityC))
				{
					centreFeature = priority == priorityDedicated ? dedicatedSchematic : feature->stat;
					{
						//   .
						//  / \
						// /   \
						// `\ /`
						//	 v
						float side = halfCentreSize * 1.0f;
						float topBottom = halfCentreSize * 1.3f;
						float mid = halfCentreSize * 0.3f;
						float sideI = side * ((topBottom - mid) / (topBottom + mid));
						{
							SchematicLine* line = new SchematicLine();
							line->set_sort_priority(20);
							line->set_line_width(outlineWidth);
							line->set_colour(outlineColour);
							line->set_looped();
							line->set_filled(specialColour);
							line->add(centreAt + Vector2(0.0f, topBottom));
							line->add(centreAt + Vector2(side, -mid));
							line->add(centreAt + Vector2(sideI, -mid));
							line->add(centreAt + Vector2(0.0f, -topBottom));
							line->add(centreAt + Vector2(-sideI, -mid));
							line->add(centreAt + Vector2(-side, -mid));
							sch->cut_inner_lines_with(*line);
							sch->add(line);
						}
					}
					break;
				}
				// confuse
				if ((dedicatedSchematic == NAME(confuse) && priority == priorityDedicated) ||
					(feature->stat == NAME(confuse) && priority == priorityC))
				{
					centreFeature = priority == priorityDedicated ? dedicatedSchematic : feature->stat;
					{
						// square with snail inside
						float big = halfCentreSize * 1.3f;
						float moveIn = halfCentreSize * 0.4f;
						{
							SchematicLine* line = new SchematicLine();
							line->set_sort_priority(20);
							line->set_line_width(outlineWidth);
							line->set_colour(outlineColour);
							line->set_looped();
							line->set_filled(specialColour);
							line->add(centreAt + Vector2(0.0f, big));
							line->add(centreAt + Vector2(big, 0.0f));
							line->add(centreAt + Vector2(0.0f, -big));
							line->add(centreAt + Vector2(-big, 0.0f));
							sch->cut_inner_lines_with(*line);
							sch->add(line);
						}
						{
							SchematicLine* line = new SchematicLine();
							line->set_sort_priority(21);
							line->set_line_width(outlineWidth * 0.3f);
							line->set_colour(outlineColour);
							float t = big;
							t = big - moveIn - moveIn;
							line->add(centreAt + Vector2(0.0f, -t));
							line->add(centreAt + Vector2(-t, 0.0f));
							line->add(centreAt + Vector2(0.0f, t));
							line->add(centreAt + Vector2(t, 0.0f));
							line->add(centreAt + Vector2(t * 0.5f, -t * 0.5f));
							line->add(centreAt);
							sch->add(line);
						}
					}
					break;
				}
				// mediCoef
				if ((dedicatedSchematic == NAME(mediCoef) && priority == priorityDedicated) ||
					(feature->stat == NAME(mediCoef) && priority == priorityC))
				{
					centreFeature = priority == priorityDedicated ? dedicatedSchematic : feature->stat;
					{
						// cross
						float big = halfCentreSize * 1.3f;
						float in = big / 3.0f;
						{
							SchematicLine* line = new SchematicLine();
							line->set_sort_priority(20);
							line->set_line_width(outlineWidth);
							line->set_colour(outlineColour);
							line->set_looped();
							line->set_filled(specialColour);
							line->add(centreAt + Vector2(in, big));
							line->add(centreAt + Vector2(in, in));
							line->add(centreAt + Vector2(big, in));
							line->add(centreAt + Vector2(big, -in));
							line->add(centreAt + Vector2(in, -in));
							line->add(centreAt + Vector2(in, -big));
							line->add(centreAt + Vector2(-in, -big));
							line->add(centreAt + Vector2(-in, -in));
							line->add(centreAt + Vector2(-big, -in));
							line->add(centreAt + Vector2(-big, in));
							line->add(centreAt + Vector2(-in, in));
							line->add(centreAt + Vector2(-in, big));
							sch->cut_inner_lines_with(*line);
							sch->add(line);
						}
					}
					break;
				}
				// projectile speed
				if ((dedicatedSchematic == NAME(projectileSpeed) && priority == priorityDedicated) ||
					(feature->stat == NAME(projectileSpeed) && priority == priorityC))
				{
					centreFeature = priority == priorityDedicated ? dedicatedSchematic : feature->stat;
					{
						// blunt arrow
						float sideC = halfCentreSize * 0.5f;
						float sideF = halfCentreSize * 1.1f;
						{
							SchematicLine* line = new SchematicLine();
							line->set_sort_priority(20);
							line->set_line_width(outlineWidth);
							line->set_colour(outlineColour);
							line->set_looped();
							line->set_filled(projectileSpeedColour);
							line->add(centreAt + Vector2(  0.0f,  sideF));
							line->add(centreAt + Vector2( sideC,  sideF - sideC * 2.0f));
							line->add(centreAt + Vector2( sideC, -sideF));
							line->add(centreAt + Vector2(-sideC, -sideF));
							line->add(centreAt + Vector2(-sideC,  sideF - sideC * 2.0f));
							line->add(centreAt + Vector2(  0.0f,  sideF));
							sch->cut_inner_lines_with(*line);
							sch->add(line);
						}
					}
					break;
				}
				if (priority == priorityDedicated)
				{
					// we do the feature pass once for priority dedicated
					break;
				}
			}
			if (centreFeature.is_valid())
			{
				break;
			}
		}
		if (!centreFeature.is_valid())
		{
			// just a plain square
			float sideF = halfCentreSize * 1.0f;
			{
				SchematicLine* line = new SchematicLine();
				line->set_sort_priority(20);
				line->set_line_width(outlineWidth);
				line->set_colour(outlineColour);
				line->set_looped();
				line->add(centreAt + Vector2(-sideF, sideF));
				line->add(centreAt + Vector2(sideF, sideF));
				line->add(centreAt + Vector2(sideF, -sideF));
				line->add(centreAt + Vector2(-sideF, -sideF));
				sch->cut_inner_lines_with(*line);
				sch->add(line);
			}
		}
	}

	// colour lines
	if (!lines.is_empty())
	{
		RG(865974);
		ARRAY_STACK(SchematicLine*, colourLines, lines.get_size());
		colourLines.copy_from(lines);
		for (int featureLine = 0;; ++featureLine)
		{
			bool anythingAdded = false;
			for_every(feature, features)
			{
				// include centre feature
				if (featureLine >= feature->lines)
				{
					continue;
				}
				Optional<Colour> colour;
				Optional<Colour> halfColour;
				float const neg = 0.5f;
				if (feature->stat == NAME(magazine))
				{
					colour = magazineColour;
				}
				if (feature->stat == NAME(magazineOutputSpeed))
				{
					colour = magazineColour;
					halfColour = magazineSpeedColour;
				}
				if (feature->stat == NAME(magazineCooldown)) { colour = magazineCooldownColour; halfColour = magazineColour; }
				if (feature->stat == NAME(magazineCooldownNeg)) { colour = magazineCooldownColour.mul_rgb(neg); halfColour = magazineColour; }
				if (feature->stat == NAME(chamber)) { colour = chamberColour; }
				if (feature->stat == NAME(storage)) { colour = storageColour; }
				if (feature->stat == NAME(storageOutputSpeed)) { colour = storageColour; halfColour = storageSpeedColour; }
				if (feature->stat == NAME(damage)) { colour = damageColour; }
				if (feature->stat == NAME(damageNeg)) { colour = damageColour.mul_rgb(neg); }
				if (feature->stat == NAME(armourPiercing)) { colour = armourPiercingColour;  halfColour = damageColour; }
				if (feature->stat == NAME(continuousFire)) { colour = continuousFireColour;  halfColour = damageColour; }
				if (feature->stat == NAME(multipleProjectiles)) { colour = multipleProjectilesColour; }
				if (feature->stat == NAME(singleProjectile)) { colour = singleProjectileColour; }
				if (feature->stat == NAME(projectileNoSpread)) { colour = projectileColour; halfColour = projectileNoSpreadColour; }
				if (feature->stat == NAME(projectileNegSpread)) { colour = projectileColour; halfColour = projectileNegSpreadColour; }
				if (feature->stat == NAME(projectileSpread)) { colour = projectileColour; halfColour = projectileSpreadColour; }
				if (feature->stat == NAME(projectileSpeed)) { colour = projectileSpeedColour; halfColour = projectileSpreadColour; }
				if (feature->stat == NAME(supercharger)) { colour = storageColour; halfColour = chamberColour; }
				if (feature->stat == NAME(retaliator)) { colour = damageColour; }
				if (colour.is_set())
				{
					if (colourLines.is_empty())
					{
						warn(TXT("not enough lines to have all features"));
						break;
					}
					anythingAdded = true;
					int clIdx = rg.get_int(colourLines.get_size());
					colourLines[clIdx]->set_colour(colour.get(), halfColour);
					colourLines[clIdx]->set_sort_priority(10);
					colourLines.remove_fast_at(clIdx);
				}
			}
			if (!anythingAdded)
			{
				break;
			}
		}
	}

	RG(806547);

	// the outline should come at the end and should clip all other lines
	{
		SchematicLine* outline = nullptr;
		if (weaponPartType->get_type() == WeaponPartType::GunChassis)
		{
			SchematicLine* line = new SchematicLine();
			line->set_sort_priority(20);
			line->set_line_width(outlineWidth);
			line->set_colour(outlineColour);
			line->set_looped();
			float sideF = 1.0f;
			float rt = min(1.0f, rg.get_float(0.5f, 1.2f)) * halfSafeRegionPt;
			float lt = min(1.0f, rg.get_float(0.5f, 1.2f)) * halfSafeRegionPt;
			float rb = min(1.0f, rg.get_float(0.5f, 1.2f)) * halfSafeRegionPt;
			float lb = min(1.0f, rg.get_float(0.5f, 1.2f)) * halfSafeRegionPt;
			line->add(halfSize * Vector2(-lt, sideF));
			line->add(halfSize * Vector2(rt, sideF));
			line->add(halfSize * Vector2(sideF, rt));
			line->add(halfSize * Vector2(sideF, -rb));
			line->add(halfSize * Vector2(rb, -sideF));
			line->add(halfSize * Vector2(-lb, -sideF));
			line->add(halfSize * Vector2(-sideF, -lb));
			line->add(halfSize * Vector2(-sideF, lt));
			sch->cut_outer_lines_with(*line);
			sch->add(line);
			outline = line;
		}
		else if (weaponPartType->get_type() == WeaponPartType::GunNode)
		{
			SchematicLine* line = new SchematicLine();
			line->set_sort_priority(20);
			line->set_line_width(outlineWidth);
			line->set_colour(outlineColour);
			line->set_looped();
			float sideC = halfSafeRegionPt;
			float sideS = rg.get_float(0.3f, 0.8f) * halfSafeRegionPt;
			float sideF = 1.0f;
			line->add(halfSize * Vector2(-sideC, sideS));
			line->add(halfSize * Vector2(0.0f, sideF));
			line->add(halfSize * Vector2(sideC, sideS));
			line->add(halfSize * Vector2(sideC, -sideS));
			line->add(halfSize * Vector2(0.0f, -sideF));
			line->add(halfSize * Vector2(-sideC, -sideS));
			sch->cut_outer_lines_with(*line);
			sch->add(line);
			outline = line;
		}
		else if (weaponPartType->get_type() == WeaponPartType::GunCore)
		{
			SchematicLine* line = new SchematicLine();
			line->set_sort_priority(20);
			line->set_line_width(outlineWidth);
			line->set_colour(outlineColour);
			line->set_looped();
			float sideF = 1.0f;
			line->add(halfSize * Vector2(0.0f, sideF));
			line->add(halfSize * Vector2(sideF, 0.0f));
			line->add(halfSize * Vector2(0.0f, -sideF));
			line->add(halfSize * Vector2(-sideF, 0.0f));
			sch->cut_outer_lines_with(*line);
			sch->add(line);
			outline = line;
		}
		else if (weaponPartType->get_type() == WeaponPartType::GunMuzzle)
		{
			SchematicLine* line = new SchematicLine();
			line->set_sort_priority(20);
			line->set_line_width(outlineWidth);
			line->set_colour(outlineColour);
			line->set_looped();
			float sideC = halfSafeRegionPt * rg.get_float(0.8f, 1.0f);
			float sideB = halfSafeRegionPt * min(1.0f, rg.get_float(0.5f, 1.2f));
			float sideN = halfSafeRegionPt * rg.get_float(0.05f, 0.25f);
			float sideF = 1.0f;
			line->add(halfSize * Vector2(-sideN, sideF));
			line->add(halfSize * Vector2(sideN, sideF));
			line->add(halfSize * Vector2(sideC, -sideB));
			line->add(halfSize * Vector2(sideC, -sideF));
			line->add(halfSize * Vector2(-sideC, -sideF));
			line->add(halfSize * Vector2(-sideC, -sideB));
			sch->cut_outer_lines_with(*line);
			sch->add(line);
			outline = line;
		}
		if (outline)
		{
			SchematicLine* background = new SchematicLine();
			background->add_from(outline);
			background->full_schematic_only();
			background->set_outline(false);
			background->set_looped(true);
			background->set_filled(interiorColour);
			background->set_sort_priority(-100);
			sch->add(background);
		}
	}

	sch->build_mesh();
	schematic = sch;
	visibleSchematic = sch;
}

#define ADD_A_CHAR() \
	{ \
		if (rg.get_chance(breakerChance)) \
		{ \
			result += breakers[rg.get_int(breakers.get_length())]; \
			breakerChance = 0.0f; \
		} \
		if (breakerChance <= 0.0f) \
		{ \
			breakerChance += 0.05f; \
		} \
		else \
		{ \
			breakerChance = (breakerChance + 0.01f) * 1.5f; \
		} \
		result += chars[rg.get_int(chars.get_length())]; \
	}

#define ADD_CHAR(_what) \
	if (_what.value.is_set()) ADD_A_CHAR();

String WeaponPart::build_ui_name() const
{
	String result;
	String chars(TXT("0123456789abcdef"));
	String breakers(TXT("/-."));

	Random::Generator rg;
	rg.set_seed(rgIndex, 8979394);

	float breakerChance = rg.get_float(-0.1f, 0.0f);

	for_count(int, i, 2)
	{
		ADD_A_CHAR();
	}
	ADD_CHAR(coreKind);
	ADD_CHAR(chamber);
	ADD_CHAR(chamberCoef);
	ADD_CHAR(baseDamageBoost);
	ADD_CHAR(damageCoef);
	ADD_CHAR(damageBoost);
	ADD_CHAR(armourPiercing);
	ADD_CHAR(storage);
	ADD_CHAR(storageOutputSpeed);
	ADD_CHAR(storageOutputSpeedAdd);
	ADD_CHAR(storageOutputSpeedAdj);
	ADD_CHAR(magazine);
	ADD_CHAR(magazineCooldown);
	ADD_CHAR(magazineCooldownCoef);
	ADD_CHAR(magazineOutputSpeed);
	ADD_CHAR(magazineOutputSpeedAdd);
	ADD_CHAR(continuousFire);
	ADD_CHAR(projectileSpeed);
	ADD_CHAR(projectileSpread);
	ADD_CHAR(projectileSpeedCoef);
	ADD_CHAR(projectileSpreadCoef);
	ADD_CHAR(numberOfProjectiles);
	ADD_CHAR(antiDeflection);
	ADD_CHAR(maxDistCoef);
	ADD_CHAR(kineticEnergyCoef);
	ADD_CHAR(confuse);
	ADD_CHAR(mediCoef);

	return result;
}

#define WITH_PENALTY_COMBINED(type, rawStat, adjustedStat) \
	adjustedStat

#define WITH_PENALTY(type, rawStat, adjustedStat) \
	rawStat, rawStat.is_set()? Optional<type>(adjustedStat.get() - rawStat.get()) : NP

String WeaponPart::build_use_info() const
{
	String result;

	BuildStatsInfo::remove_trailing_end_lines(result);

	return result;
}

ArrayStatic<Name, 8> const & WeaponPart::get_special_features() const
{
	if (auto* wpt = weaponPartType.get())
	{
		return wpt->specialFeatures;
	}
	else
	{
		static ArrayStatic<Name, 8> noSpecialFeatures; SET_EXTRA_DEBUG_INFO(noSpecialFeatures, TXT("WeaponPart::get_special_features.noSpecialFeatures"));
		return noSpecialFeatures;
	}
}

Framework::LocalisedString const& WeaponPart::get_stat_info() const
{
	if (auto* wpt = weaponPartType.get())
	{
		return wpt->statInfo;
	}
	else
	{
		return Framework::LocalisedString::none;
	}
}

Framework::LocalisedString const& WeaponPart::get_part_only_stat_info() const
{
	if (auto* wpt = weaponPartType.get())
	{
		return wpt->partOnlyStatInfo;
	}
	else
	{
		return Framework::LocalisedString::none;
	}
}

void WeaponPart::build_overlay_stats_info(OUT_ List<StatInfo> & _statsInfo, OPTIONAL_ OUT_ OverlayInfo::TextColours* _textColours) const
{
	_statsInfo.clear();

	an_assert_log_always(get_weapon_part_type());
	if (!get_weapon_part_type())
	{
		return;
	}

	WeaponCoreModifiers const* weaponCoreModifiers = nullptr;
	String fullCore;
	if (coreKind.value.is_set())
	{
		if (auto* gd = GameDefinition::get_chosen())
		{
			if ((weaponCoreModifiers = gd->get_weapon_core_modifiers()))
			{
				if (auto* fc = weaponCoreModifiers->get_for_core(coreKind.value.get()))
				{
					if (_textColours && fc->overlayInfoColour.is_set())
					{
						_textColours->add(NAME(core), fc->overlayInfoColour.get());
						fullCore = TXT("#core#");
					}
				}
			}
		}
		_statsInfo.push_back(StatInfo());
		_statsInfo.get_last().text = (fullCore + (LOC_STR(WeaponCoreKind::to_localised_string_id_long(coreKind.value.get())) + String::space() + LOC_STR(WeaponCoreKind::to_localised_string_id_icon(coreKind.value.get()))).trim());
	}
	else
	{
		if (get_weapon_part_type()->get_type() == WeaponPartType::GunChassis)
		{
			int nodeCount = weaponPartType->get_slot_count(WeaponPartType::GunNode);
			_statsInfo.push_back(StatInfo());
			_statsInfo.get_last().text = (Framework::StringFormatter::format_sentence_loc_str(NAME(lsMixerStatsChassis), Framework::StringFormatterParams()
				.add(NAME(nodeCount), nodeCount)));
		}
		else if (get_weapon_part_type()->get_type() == WeaponPartType::GunMuzzle)
		{
			_statsInfo.push_back(StatInfo());
			_statsInfo.get_last().text = (LOC_STR(NAME(lsMixerStatsMuzzle)));
		}
		else if (get_weapon_part_type()->get_type() == WeaponPartType::GunNode)
		{
			_statsInfo.push_back(StatInfo());
			_statsInfo.get_last().text = (LOC_STR(NAME(lsMixerStatsNode)));
		}
	}

	if (!damaged.is_zero())
	{
		String damagedPrefix;
		if (_textColours)
		{
			Colour tc = RegisteredColours::get_colour(NAME(pilgrim_overlay_weaponPart_damaged));
			if (!tc.is_none())
			{
				damagedPrefix = String(TXT("#damaged#"));
				_textColours->add(NAME(damaged), tc);
			}
		}
		_statsInfo.push_back(StatInfo());
		_statsInfo.get_last().text = (damagedPrefix + Framework::StringFormatter::format_sentence_loc_str(NAME(lsEquipmentDamagedReplace), Framework::StringFormatterParams()
			.add(NAME(value), damaged.as_string_percentage())));
	}

	// here are only stats coming directly from the part
	{
		bool partOnlyStatInfoAdded = false;
		{
			auto & v = get_part_only_stat_info();
			if (v.is_valid() &&
				!v.get().is_empty())
			{
				partOnlyStatInfoAdded = true;
				_statsInfo.push_back(StatInfo());
				_statsInfo.get_last().text = (v.get());
			}
		}

		if (!partOnlyStatInfoAdded)
		{
			auto& v = get_stat_info();
			if (v.is_valid() &&
				!v.get().is_empty())
			{
				_statsInfo.push_back(StatInfo());
				_statsInfo.get_last().text = (v.get());
			}
		}
	}

	bool damageIsBaseValue = get_weapon_part_type()->get_type() == WeaponPartType::GunChassis;
	Energy damage = Energy::zero();
	Energy baseDamageBoost = Energy::zero();
	Energy damageBoost = Energy::zero();
	Energy cost = Energy::zero();
	EnergyCoef damageCoef = EnergyCoef::zero();
	EnergyCoef costCoef = EnergyCoef::zero();
	{
		auto v = get_chamber().value;
		if (v.is_set())
		{
			damage += v.get();
			cost += v.get();
		}
	}
	{
		auto v = get_chamber_coef().value;
		if (v.is_set())
		{
			damageCoef += v.get();
			costCoef += v.get();
		}
	}
	{
		auto v = get_base_damage_boost().value;
		if (v.is_set())
		{
			baseDamageBoost += v.get();
		}
	}
	{
		auto v = get_damage_boost().value;
		if (v.is_set())
		{
			damageBoost += v.get();
		}
	}
	{
		auto v = get_damage_coef().value;
		if (v.is_set())
		{
			damageCoef += v.get();
		}
	}

	if (!baseDamageBoost.is_zero())
	{
		String v;
		if (!baseDamageBoost.is_zero())
		{
			v += String::space() + LOC_STR(NAME(lsCharsDamage)) + baseDamageBoost.as_string_relative();
		}
		if (!v.is_empty())
		{
			_statsInfo.push_back(StatInfo());
			_statsInfo.get_last().text = (Framework::StringFormatter::format_sentence_loc_str(NAME(lsEquipmentBaseDamage), Framework::StringFormatterParams()
				.add(NAME(damage), v.trim())));
		}
	}
	if (!damage.is_zero() || ! damageCoef.is_zero())
	{
		String v;
		if (!damage.is_zero())
		{
			if (damageIsBaseValue)
			{
				v += String::space() + LOC_STR(NAME(lsCharsDamage)) + damage.as_string();
			}
			else
			{
				v += String::space() + LOC_STR(NAME(lsCharsDamage)) + damage.as_string_relative();
			}
		}
		if (!damageCoef.is_zero())
		{
			v += String::space() + damageCoef.as_string_percentage_relative();
		}
		if (!v.is_empty())
		{
			_statsInfo.push_back(StatInfo());
			_statsInfo.get_last().text = (Framework::StringFormatter::format_sentence_loc_str(NAME(lsEquipmentDamage), Framework::StringFormatterParams()
				.add(NAME(damage), v.trim())));
		}
	}
	if (!damageBoost.is_zero())
	{
		String v;
		if (!damageBoost.is_zero())
		{
			v += String::space() + LOC_STR(NAME(lsCharsDamage)) + damageBoost.as_string_relative();
		}
		if (!v.is_empty())
		{
			_statsInfo.push_back(StatInfo());
			_statsInfo.get_last().text = (Framework::StringFormatter::format_sentence_loc_str(NAME(lsEquipmentDamageBoost), Framework::StringFormatterParams()
				.add(NAME(damage), v.trim())));
		}
	}
	if (!cost.is_zero() || !costCoef.is_zero())
	{
		String v;
		if (!cost.is_zero())
		{
			v += String::space() + LOC_STR(NAME(lsCharsAmmo)) + cost.as_string_relative();
		}
		if (!costCoef.is_zero())
		{
			v += String::space() + costCoef.as_string_percentage_relative();
		}
		_statsInfo.push_back(StatInfo());
		_statsInfo.get_last().text = (Framework::StringFormatter::format_sentence_loc_str(NAME(lsEquipmentCost), Framework::StringFormatterParams()
			.add(NAME(cost), v.trim())));
	}

	Energy rpmClassDiv = Energy(1.0f);
	{
		auto v = get_storage_output_speed();
		if (v.value.is_set())
		{
			_statsInfo.push_back(StatInfo());
			_statsInfo.get_last().text = (Framework::StringFormatter::format_sentence_loc_str(NAME(lsEquipmentRoundsPerMinuteClass), Framework::StringFormatterParams()
				.add(NAME(value), (v.value.get() / rpmClassDiv).as_string(0))));
		}
	}
	{
		auto v = get_storage_output_speed_add();
		if (v.value.is_set())
		{
			String symbol(v.value.get().is_negative()? TXT("-") : TXT("+"));
			_statsInfo.push_back(StatInfo());
			_statsInfo.get_last().text = (Framework::StringFormatter::format_sentence_loc_str(NAME(lsEquipmentRoundsPerMinuteClass), Framework::StringFormatterParams()
				.add(NAME(value), symbol + (abs(v.value.get()) / rpmClassDiv).as_string(0))));
		}
	}
	{
		auto v = get_storage_output_speed_adj();
		if (v.value.is_set())
		{
			_statsInfo.push_back(StatInfo());
			_statsInfo.get_last().text = (Framework::StringFormatter::format_sentence_loc_str(NAME(lsEquipmentRoundsPerMinuteClassAdj), Framework::StringFormatterParams()
				.add(NAME(value), (abs(v.value.get())).as_string_percentage(0))));
		}
	}

	{
		auto v = get_magazine();
		if (v.value.is_set())
		{
			_statsInfo.push_back(StatInfo());
			_statsInfo.get_last().text = (Framework::StringFormatter::format_sentence_loc_str(NAME(lsEquipmentMagazine), Framework::StringFormatterParams()
				.add(NAME(value), LOC_STR(NAME(lsCharsAmmo)) + v.value.get().as_string_relative())));
		}
	}

	{
		auto v = get_magazine_output_speed();
		if (v.value.is_set())
		{
			_statsInfo.push_back(StatInfo());
			_statsInfo.get_last().text = (Framework::StringFormatter::format_sentence_loc_str(NAME(lsEquipmentRoundsPerMinuteMagazineClass), Framework::StringFormatterParams()
				.add(NAME(value), (v.value.get() / rpmClassDiv).as_string(0))));
		}
	}
	{
		auto v = get_magazine_output_speed_add();
		if (v.value.is_set())
		{
			String symbol(v.value.get().is_negative()? TXT("-") : TXT("+"));
			_statsInfo.push_back(StatInfo());
			_statsInfo.get_last().text = (Framework::StringFormatter::format_sentence_loc_str(NAME(lsEquipmentRoundsPerMinuteMagazineClass), Framework::StringFormatterParams()
				.add(NAME(value), symbol + (abs(v.value.get()) / rpmClassDiv).as_string(0))));
		}
	}
	{
		auto& v = continuousFire;
		if (v.value.is_set() && v.value.get())
		{
			_statsInfo.push_back(StatInfo());
			_statsInfo.get_last().text = (LOC_STR(NAME(lsEquipmentContinuousFire)));
		}
	}

	if (get_weapon_part_type()->get_type() != WeaponPartType::GunChassis)
	{
		{
			auto& v = numberOfProjectiles;
			if (v.value.is_set())
			{
				_statsInfo.push_back(StatInfo());
				_statsInfo.get_last().text = (Framework::StringFormatter::format_sentence_loc_str(NAME(lsEquipmentProjectileCount), Framework::StringFormatterParams()
					.add(NAME(projectileCount), v.value.get())));
			}
		}
		{
			auto v = get_projectile_speed();
			if (v.value.is_set())
			{
				String value;
				{
					float speed = v.value.get();
					String symbol(get_weapon_part_type()->get_type() != WeaponPartType::GunMuzzle ? (speed >= 0.0f ? TXT("+") : TXT("-")) : (speed >= 0.0f ? TXT("") : TXT("-")));
					MeasurementSystem::Type ms = TeaForGodEmperor::PlayerSetup::get_current().get_preferences().get_measurement_system();
					if (ms == MeasurementSystem::Imperial)
					{
						value = symbol + MeasurementSystem::as_feet_speed(abs(speed));
					}
					if (ms == MeasurementSystem::Metric)
					{
						value = symbol + MeasurementSystem::as_meters_speed(abs(speed), NP, nullptr, 0);
					}
				}
				_statsInfo.push_back(StatInfo());
				_statsInfo.get_last().text = (Framework::StringFormatter::format_sentence_loc_str(NAME(lsEquipmentProjectileSpeed), Framework::StringFormatterParams()
					.add(NAME(value), value)));
			}
		}
		{
			auto v = get_projectile_speed_coef();
			if (v.value.is_set() && v.value.get() != 1.0f)
			{
				_statsInfo.push_back(StatInfo());
				_statsInfo.get_last().text = (Framework::StringFormatter::format_sentence_loc_str(NAME(lsEquipmentProjectileSpeed), Framework::StringFormatterParams()
					.add(NAME(value), String::printf(TXT("%c%.0f%%"), v.value.get() >= 0.0f ? '+' : '-', 100.0f * v.value.get()))));
			}
		}
		{
			auto v = get_projectile_spread();
			if (v.value.is_set())
			{
				String value;
				{
					float spread = v.value.get();
					String symbol(get_weapon_part_type()->get_type() != WeaponPartType::GunMuzzle ? (spread >= 0.0f ? TXT("+") : TXT("-")) : (spread >= 0.0f ? TXT("") : TXT("-")));
					value = symbol + String::printf(TXT("%.0f%S"), abs(spread), String::degree().to_char());
				}
				_statsInfo.push_back(StatInfo());
				_statsInfo.get_last().text = (Framework::StringFormatter::format_sentence_loc_str(NAME(lsEquipmentProjectileSpread), Framework::StringFormatterParams()
					.add(NAME(value), value)));
			}
		}
		{
			auto v = get_projectile_spread_coef();
			if (v.value.is_set())
			{
				_statsInfo.push_back(StatInfo());
				_statsInfo.get_last().text = (Framework::StringFormatter::format_sentence_loc_str(NAME(lsEquipmentProjectileSpread), Framework::StringFormatterParams()
					.add(NAME(value), String::printf(TXT("%c%.0f%%"), v.value.get() >= 0.0f ? '+' : '-', 100.0f * abs(v.value.get())))));
			}
		}
	}
	{
		auto v = get_armour_piercing();
		if (v.value.is_set())
		{
			_statsInfo.push_back(StatInfo());
			_statsInfo.get_last().text = (Framework::StringFormatter::format_sentence_loc_str(NAME(lsEquipmentArmourPiercing), Framework::StringFormatterParams()
				.add(NAME(value), !v.value.get().is_one() ? v.value.get().as_string_percentage(0) : String::empty())).trim());
		}
	}
	{
		auto v = get_anti_deflection();
		if (v.value.is_set())
		{
			_statsInfo.push_back(StatInfo());
			_statsInfo.get_last().text = (Framework::StringFormatter::format_sentence_loc_str(NAME(lsEquipmentAntiDeflection), Framework::StringFormatterParams()
				.add(NAME(value), v.value.get() < 1.0f ? String::printf(TXT("%.0f%%"), 100.0f * v.value.get()) : String::empty())).trim());
		}
	}
	{
		auto v = get_max_dist_coef();
		if (v.value.is_set())
		{
			_statsInfo.push_back(StatInfo());
			_statsInfo.get_last().text = (Framework::StringFormatter::format_sentence_loc_str(NAME(lsEquipmentMaxDist), Framework::StringFormatterParams()
				.add(NAME(value), String::printf(TXT("%c%.0f%%"), v.value.get() >= 0.0f ? '+' : '-', 100.0f * abs(v.value.get())))));
		}
	}
	{
		auto v = get_kinetic_energy_coef();
		if (v.value.is_set())
		{
			_statsInfo.push_back(StatInfo());
			_statsInfo.get_last().text = (Framework::StringFormatter::format_sentence_loc_str(NAME(lsEquipmentKineticEnergyCoef), Framework::StringFormatterParams()
				.add(NAME(total), String::empty())
				.add(NAME(value), String::printf(TXT("%c%.2f"), v.value.get() >= 0.0f ? '+' : '-', (v.value.get() * WEAPON_NODE_KINETIC_ENERGY_IMPULSE_SHOW)))
			));
		}
	}
	{
		auto v = get_confuse();
		if (v.value.is_set())
		{
			_statsInfo.push_back(StatInfo());
			_statsInfo.get_last().text = (Framework::StringFormatter::format_sentence_loc_str(NAME(lsEquipmentConfuse), Framework::StringFormatterParams()
				.add(NAME(value), String::printf(TXT("%c%S"), v.value.get() >= 0.0f ? '+' : '-', ParserUtils::float_to_string_auto_decimals(v.value.get(), 2).to_char()))
			));
		}
	}
	{
		auto v = get_medi_coef();
		if (v.value.is_set())
		{
			_statsInfo.push_back(StatInfo());
			_statsInfo.get_last().text = (Framework::StringFormatter::format_sentence_loc_str(NAME(lsEquipmentMediCoef), Framework::StringFormatterParams()
				.add(NAME(value), v.value.get().as_string_percentage_relative())
			));
		}
	}

	if (coreKind.value.is_set())
	{
		if (weaponCoreModifiers)
		{
			List<String> info;
			weaponCoreModifiers->build_overlay_stats_info(coreKind.value.get(), info, fullCore);
			for_every(i, info)
			{
				_statsInfo.push_back(StatInfo());
				_statsInfo.get_last().text = *i;
			}
		}
	}

	return;
}
