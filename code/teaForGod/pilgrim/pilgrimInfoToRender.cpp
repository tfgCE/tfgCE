#include "pilgrimInfoToRender.h"

#include "..\library\exmType.h"
#include "..\modules\custom\health\mc_health.h"
#include "..\modules\gameplay\moduleEXM.h"
#include "..\modules\gameplay\modulePilgrim.h"
#include "..\modules\gameplay\equipment\me_gun.h"

#include "..\..\core\system\video\renderTarget.h"
#include "..\..\core\system\video\video3dPrimitives.h"

#include "..\..\framework\library\library.h"
#include "..\..\framework\library\usedLibraryStored.inl"
#include "..\..\framework\modulesOwner\modulesOwner.h"
#include "..\..\framework\time\time.h"

#ifdef AN_CLANG
#include "..\..\framework\modulesOwner\modulesOwnerImpl.inl"
#endif

//

#ifdef AN_ALLOW_OPTIMIZE_OFF
#pragma optimize("", off)
#endif

//

using namespace TeaForGodEmperor;

//

DEFINE_STATIC_NAME_STR(fontBigNumbers, TXT("pilgrim info; font big numbers"));
DEFINE_STATIC_NAME_STR(fontMidNumbers, TXT("pilgrim info; font mid numbers"));
DEFINE_STATIC_NAME_STR(fontText, TXT("pilgrim info; font text"));
DEFINE_STATIC_NAME_STR(leftHand, TXT("pilgrim info; left hand"));
DEFINE_STATIC_NAME_STR(rightHand, TXT("pilgrim info; right hand"));
//
DEFINE_STATIC_NAME_STR(healthColour, TXT("handHealth"));
DEFINE_STATIC_NAME_STR(inHandColour, TXT("handOtherEquipment"));
DEFINE_STATIC_NAME_STR(weaponColour, TXT("handWeapon"));
DEFINE_STATIC_NAME_STR(weaponInterimColour, TXT("handWeaponInterim"));
DEFINE_STATIC_NAME_STR(exmColour, TXT("exm_selected"));
DEFINE_STATIC_NAME_STR(exmActiveColour, TXT("exm_active"));
DEFINE_STATIC_NAME_STR(statsInfoDiffPlusColour, TXT("statsInfoDiffPlus"));
DEFINE_STATIC_NAME_STR(statsInfoDiffMinusColour, TXT("statsInfoDiffMinus"));

//

void PilgrimInfoToRender::VariableValue::advance(float _deltaTime)
{
	highlight = blend_to_using_time(highlight, 0.0f, 0.3f, _deltaTime);
}

//

void PilgrimInfoToRender::EquipmentInfo::clear()
{
	equipment.clear();
	ammo = -1;
	magazineAmmo = -1;
}

void PilgrimInfoToRender::EquipmentInfo::clear_highlight()
{
	ammo.clear_highlight();
	magazineAmmo.clear_highlight();
}

void PilgrimInfoToRender::EquipmentInfo::update(::Framework::IModulesOwner* _equipment, Energy const& _availableEnergyInMainStorage, bool _inUse)
{
	if (!_equipment)
	{
		clear();
		return;
	}

	if (auto * g = _equipment->get_gameplay_as<ModuleEquipments::Gun>())
	{
		ammo = g->calculate_total_ammo(_availableEnergyInMainStorage);
		magazineAmmo = _inUse? g->calculate_ammo_in_chamber_and_magazine() : -1;
	}
	else
	{
		ammo = -1;
		magazineAmmo = -1;
	}

	if (equipment != _equipment)
	{
		ammo.clear_highlight();
		magazineAmmo.clear_highlight();
	}
	equipment = _equipment;
}

void PilgrimInfoToRender::EquipmentInfo::advance(float _deltaTime)
{
	ammo.advance(_deltaTime);
	magazineAmmo.advance(_deltaTime);
}

//

void PilgrimInfoToRender::Hand::clear()
{
	energy = 0;
	mainEquipment.clear();
	inHandEquipment.clear();
}

void PilgrimInfoToRender::Hand::clear_highlight()
{
	energy.clear_highlight();
	mainEquipment.clear_highlight();
	inHandEquipment.clear_highlight();
}

void PilgrimInfoToRender::Hand::update(::Framework::IModulesOwner* _pilgrim, ::Hand::Type _hand)
{
	if (auto * p = _pilgrim->get_gameplay_as<ModulePilgrim>())
	{
		Energy e = p->get_hand_energy_storage(_hand);
		energy = e.as_int();
		auto* me = p->get_main_equipment(_hand);
		auto* he = p->get_hand_equipment(_hand);
		mainEquipment.update(me, e, me != nullptr && me == he);
		inHandEquipment.update(he, e, he != nullptr);
	}
	else
	{
		energy = 0;
		mainEquipment.clear();
		inHandEquipment.clear();
	}
}

void PilgrimInfoToRender::Hand::advance(float _deltaTime)
{
	energy.advance(_deltaTime);
	mainEquipment.advance(_deltaTime);
	inHandEquipment.advance(_deltaTime);
}

//

void PilgrimInfoToRender::Info::clear()
{
	pilgrim.clear();

	health = 0;
	healthTotal = 0;

	for_count(int, h, ::Hand::MAX)
	{
		hand[h].clear();
	}
}

void PilgrimInfoToRender::Info::clear_highlight()
{
	health.clear_highlight();
	healthTotal.clear_highlight();

	for_count(int, h, ::Hand::MAX)
	{
		hand[h].clear_highlight();
	}
}

void PilgrimInfoToRender::Info::update(::Framework::IModulesOwner* _pilgrim)
{
	if (!_pilgrim)
	{
		clear();
		return;
	}

	if (auto * h = _pilgrim->get_custom<CustomModules::Health>())
	{
		health = h->get_health().as_int();
		healthTotal = h->calculate_total_energy_available(EnergyType::Health).as_int();
	}
	else
	{
		health = 0;
		healthTotal = 0;
	}

	for_count(int, h, ::Hand::MAX)
	{
		hand[h].update(_pilgrim, (::Hand::Type)h);
	}

	if (pilgrim != _pilgrim)
	{
		health.clear_highlight();
		healthTotal.clear_highlight();
	}
	pilgrim = _pilgrim;
}

void PilgrimInfoToRender::Info::advance(float _deltaTime)
{
	health.advance(_deltaTime);
	healthTotal.advance(_deltaTime);

	for_count(int, h, ::Hand::MAX)
	{
		hand[h].advance(_deltaTime);
	}
}

//

void PilgrimInfoToRender::clear_highlight()
{
	curr.clear_highlight();
}

void PilgrimInfoToRender::advance(::Framework::IModulesOwner* _pilgrim, float _deltaTime)
{
	curr.advance(_deltaTime);
	update(_pilgrim);
}

void PilgrimInfoToRender::update(::Framework::IModulesOwner* _pilgrim)
{
	curr.update(_pilgrim);
}

void PilgrimInfoToRender::update_assets()
{
	if (! fontBigNumbers.is_set())
	{
		fontBigNumbers = Framework::Library::get_current()->get_global_references().get<Framework::Font>(NAME(fontBigNumbers), true);
	}
	if (!fontMidNumbers.is_set())
	{
		fontMidNumbers = Framework::Library::get_current()->get_global_references().get<Framework::Font>(NAME(fontMidNumbers), true);
	}
	if (! fontText.is_set())
	{
		fontText = Framework::Library::get_current()->get_global_references().get<Framework::Font>(NAME(fontText), true);
	}
	if (! leftHand.is_set())
	{
		leftHand = Framework::Library::get_current()->get_global_references().get<Framework::TexturePart>(NAME(leftHand), true);
	}
	if (! rightHand.is_set())
	{
		rightHand = Framework::Library::get_current()->get_global_references().get<Framework::TexturePart>(NAME(rightHand), true);
	}
}

#define SCALE_TWEAK 0.7f

float PilgrimInfoToRender::get_best_border_width(Vector2 const & _size)
{
	return _size.x * 0.2f * SCALE_TWEAK;
}

#define COLOUR(valueVar, colour) \
		(valueVar.highlight >= 0.0f? \
				Colour::lerp(valueVar.highlight, colour, goingUp) : \
				Colour::lerp(-valueVar.highlight, colour, goingDown))

void PilgrimInfoToRender::render(::System::RenderTarget* _rt, float _borderWidth)
{
	if (!isVisible)
	{
		return;
	}

	an_assert(_rt);

	auto* v3d = ::System::Video3D::get();

	// ready to render to main render target
	v3d->setup_for_2d_display();

	v3d->set_viewport(VectorInt2::zero, _rt->get_size());
	v3d->set_near_far_plane(0.02f, 100.0f);

	v3d->set_2d_projection_matrix_ortho();
	v3d->access_model_view_matrix_stack().clear();

	_rt->bind();

	update_assets();

	if (fontText.is_set() &&
		fontBigNumbers.is_set() &&
		fontMidNumbers.is_set() &&
		leftHand.is_set() && leftHand->is_valid() &&
		rightHand.is_set() && rightHand->is_valid())
	{
		Vector2 size = _rt->get_size().to_vector2();
		float scale = (size.x / (12.0f * 40.0f)) * (1.0f - 0.6f * (1.0f - SCALE_TWEAK));
		Vector2 s(scale, scale);

		::System::FontDrawingContext fontContext;
		Framework::TexturePartDrawingContext textureContext;
		textureContext.scale = s;

		tchar buffer[256];

		Colour healthColour = RegisteredColours::get_colour(NAME(healthColour));
		Colour inHandColour = RegisteredColours::get_colour(NAME(inHandColour));
		Colour weaponColour = RegisteredColours::get_colour(NAME(weaponColour));
		Colour weaponInterimColour = RegisteredColours::get_colour(NAME(weaponInterimColour));
		Colour goingUp = RegisteredColours::get_colour(NAME(statsInfoDiffPlusColour));
		Colour goingDown = RegisteredColours::get_colour(NAME(statsInfoDiffMinusColour));

		for_count(int, h, ::Hand::MAX)
		{
			Range2 r(h == ::Hand::Left ? Range(0.0f, _borderWidth - 1.0f) : Range(size.x - 1.0f - (_borderWidth - 1.0f), size.x - 1.0f), Range(0.0f, size.y - 1.0f));
			Vector2 at(r.x.centre(), r.y.max);

			::System::Video3DPrimitives::fill_rect_2d(Colour::blackWarm, r);

			float line = scale * 8.0f;

			at.y -= line * 3.0f * ((1.0f - SCALE_TWEAK) / 0.3f); // tweaking

			at.y -= line * 2.0f;

			{
				at.y -= 16.0f * scale;
				(h == ::Hand::Left ? leftHand.get() : rightHand.get())->draw_at(v3d, at, textureContext);
				at.y -= 16.0f * scale;
			}

			auto& handInfo = curr.hand[h];

			at.y -= line * 1.5f;

			for_count(int, ieq, 2)
			{
				Colour useColour = inHandColour;
				auto& eqInfo = ieq == 0 ? handInfo.inHandEquipment : handInfo.mainEquipment;
				auto* e = eqInfo.equipment.get();
				if (e)
				{
					auto* mg = e->get_gameplay_as<ModuleEquipments::Gun>();
					if (mg)
					{
						useColour = weaponColour;
					}
					else
					{
						useColour = weaponInterimColour;
					}
				}
				{
					at.y -= line * 1.0f;
					fontText->get_font()->draw_text_at(v3d, ieq == 0? TXT("in hand") : TXT("main"), useColour, at, s, Vector2(0.5f, 0.0f), false, fontContext);
				}
				{
					tsprintf(buffer, 255, TXT(""));
					{
						tsprintf(buffer, 255, TXT("%02i"), handInfo.energy.value);
					}
					if (ieq == 1)
					{
						fontBigNumbers->get_font()->draw_text_at(v3d, buffer, COLOUR(handInfo.energy, useColour), at, s, Vector2(0.5f, 1.0f), false, fontContext);
					}
					else
					{
						fontMidNumbers->get_font()->draw_text_at(v3d, buffer, COLOUR(handInfo.energy, useColour), at, s, Vector2(0.5f, 1.0f), false, fontContext);
					}
				}
				if (ieq == 1)
				{
					at.y -= line * 2.5f;
				}
				else
				{
					at.y -= line * 2.0f;
				}
				if (e)
				{
					auto* mg = e->get_gameplay_as<ModuleEquipments::Gun>();

					if (mg && eqInfo.ammo.value >= 0)
					{
						Vector2 tat = at;
						{
							tat.y -= line * 1.0f;
							fontText->get_font()->draw_text_at(v3d, TXT("ammo"), useColour, tat, s, Vector2(0.5f, 0.0f), false, fontContext);
						}
						if (eqInfo.ammo.value >= 0)
						{
							tsprintf(buffer, 255, TXT("%02i"), eqInfo.ammo.value);
							fontMidNumbers->get_font()->draw_text_at(v3d, buffer, COLOUR(eqInfo.ammo, useColour), tat, s, Vector2(0.5f, 1.0f), false, fontContext);
							tat.y -= line * 1.5f;
						}
						if (mg->is_using_magazine() && eqInfo.magazineAmmo.value >= 0)
						{
							tsprintf(buffer, 255, TXT("(%02i)"), eqInfo.magazineAmmo.value);
							fontText->get_font()->draw_text_at(v3d, buffer, COLOUR(eqInfo.magazineAmmo, useColour), tat, s, Vector2(0.5f, 1.0f), false, fontContext);
							tat.y -= line * 1.0f;
						}
					}
				}
				at.y -= line * 3.5f;
				at.y -= line * 1.5f;
			}

			if (h == ::Hand::Right)
			{
				{
					at.y -= line * 1.0f;
					fontText->get_font()->draw_text_at(v3d, TXT("health"), healthColour, at, s, Vector2(0.5f, 0.0f), false, fontContext);
				}
				{
					tsprintf(buffer, 255, TXT("%02i"), curr.health.value);
					fontBigNumbers->get_font()->draw_text_at(v3d, buffer, COLOUR(curr.health, healthColour), at, s, Vector2(0.5f, 1.0f), false, fontContext);
					at.y -= line * 2.0f;
				}
				at.y -= line * 0.1f;
				{
					at.y -= line * 1.0f;
					fontText->get_font()->draw_text_at(v3d, TXT("total"), healthColour, at, s, Vector2(0.5f, 0.0f), false, fontContext);
				}
				{
					tsprintf(buffer, 255, TXT("%02i"), curr.healthTotal.value);
					fontMidNumbers->get_font()->draw_text_at(v3d, buffer, COLOUR(curr.healthTotal, healthColour), at, s, Vector2(0.5f, 1.0f), false, fontContext);
					at.y -= line * 2.0f;
				}
			}
			at.y -= line * 1.0f;
		}
	}

	_rt->unbind();
}
