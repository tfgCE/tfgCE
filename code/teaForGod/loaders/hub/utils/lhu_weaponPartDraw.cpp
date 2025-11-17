#include "lhu_weaponPartDraw.h"

#include "lhu_colours.h"
#include "lhu_grid.h"

#include "..\loaderHub.h"

#include "..\draggables\lhd_weaponPartPilgrimInventory.h"
//
#include "..\widgets\lhw_grid.h"
//
#include "..\..\..\game\persistence.h"
#include "..\..\..\tutorials\tutorialSystem.h"

#include "..\..\..\..\core\system\video\video3dPrimitives.h"

#include "..\..\..\..\framework\video\font.h"

//

using namespace Loader;

//

// shader param
DEFINE_STATIC_NAME(highlightColour);
DEFINE_STATIC_NAME(emissiveColour);
DEFINE_STATIC_NAME(desaturateColour);
DEFINE_STATIC_NAME(alphaChannel);
DEFINE_STATIC_NAME(totalColour);
DEFINE_STATIC_NAME(exmEmiColours);
DEFINE_STATIC_NAME(exmEmiPower);

//

void Utils::WeaponPartDraw::draw_grid_element(Framework::Display* _display, HubWidgets::Grid* w, Range2 const& _at, HubScreen const* screen, HubWidgets::GridDraggable const* _element, Optional<Colour> const & _overrideColour, Optional<Colour> const& _blendTotalColour)
{
	Optional<float> doubleLine;
	if (_element && _element->draggable.get())
	{
		if (auto* wppi = fast_cast<HubDraggables::WeaponPartPilgrimInventory>(_element->draggable->data.get()))
		{
			if (wppi->is_part_of_item())
			{
				doubleLine = 4.0f;
			}
		}
	}

	Utils::Grid::draw_grid_element(_display, w, _at, screen, _element, NP, _overrideColour, doubleLine, _blendTotalColour);
}

void Utils::WeaponPartDraw::draw_draggable_at(Framework::Display* _display, HubWidgets::Grid* w, Range2 const& _at, HubWidgets::GridDraggable const* _element, Context const& _context, int _flags)
{
	if (auto* wpp = fast_cast<HubInterfaces::IWeaponPartProvider>(_element->draggable->data.get()))
	{
		// spacing to border
		draw_element_at(_display, w, _element->draggable.get(), _at.expanded_by(-floor(_at.length() * 0.1f)), wpp->get_weapon_part(), _context, _flags);
	}
}

void Utils::WeaponPartDraw::draw_draggable_at(Framework::Display* _display, HubWidgets::Grid* w, Range2 const& _at, TeaForGodEmperor::WeaponPart* wp, Context const& _context, int _flags)
{
	if (wp)
	{
		// spacing to border
		draw_element_at(_display, w, nullptr, _at.expanded_by(-floor(_at.length() * 0.1f)), wp, _context, _flags);
	}
}

void Utils::WeaponPartDraw::draw_element_at(Framework::Display* _display, Loader::HubWidget * _widget, Loader::HubDraggable const* _draggable, Range2 const& _at, TeaForGodEmperor::WeaponPart const* _wp, Context const& _context, int _flags)
{
	Range2 at = _at;

	bool needsToWait = true;
	if (Meshes::Mesh3DInstance* meshInstance = (cast_to_nonconst(is_flag_set(_flags, DrawFlags::Outline) ? _wp->get_schematic_mesh_instance_outline_only() : _wp->get_schematic_mesh_instance())))
	{
		if (meshInstance->get_mesh())
		{
			auto* v3d = ::System::Video3D::get();
			System::VideoMatrixStack& modelViewStack = v3d->access_model_view_matrix_stack();
			Vector2 mat = at.centre();
			Vector2 msize = at.length();
			Vector2 ssize = TeaForGodEmperor::WeaponPart::get_schematic_size();
			float scale = min(msize.x / ssize.x, msize.y / ssize.y);
			modelViewStack.push_to_world(Transform(mat.to_vector3(), Rotator3(0.0f, 0.0f, 0.0f).to_quat(), scale).to_matrix());
			Meshes::Mesh3DRenderingContext renderingContext;
			{
				Colour highlightColour = Colour::none;// HubColours::highlight().with_set_alpha(0.5f);
				Colour totalColour = TeaForGodEmperor::TutorialSystem::adjust_colour_for_hub(Colour::white, _widget? _widget->screen : nullptr, _widget, _draggable);
				for_count(int, idx, meshInstance->get_material_instance_count())
				{
					meshInstance->access_material_instance(idx)->set_uniform(NAME(highlightColour), highlightColour.to_vector4());
					meshInstance->access_material_instance(idx)->set_uniform(NAME(totalColour), totalColour.to_vector4());
				}
			}
			meshInstance->render(v3d, renderingContext);
			modelViewStack.pop();
			needsToWait = false;
		}
	}

	if (is_flag_set(_flags, DrawFlags::WithTransferChance))
	{
		int transferChance = _wp->get_transfer_chance();

		TeaForGodEmperor::TransferWeaponPartContext twpContext;
		twpContext.with_boosts(_context.activeBoosts);
		if (_context.preDeactivationTransfer)
		{
			twpContext.for_pre_deactivation(_context.preDeactivationLevel);
		}
		transferChance = TeaForGodEmperor::WeaponPart::adjust_transfer_chance(transferChance, twpContext);

		if (auto* font = _display->get_font())
		{
			Colour colour = get_transfer_chance_colour(transferChance);

			int const BUF_SIZE = 32;
			tchar buffer[BUF_SIZE];
			tsprintf(buffer, BUF_SIZE, TXT("%i%%"), transferChance);

			font->draw_text_at(::System::Video3D::get(), buffer, colour, at.get_at(Vector2(0.5f, 0.2f)), Vector2::one, Vector2(0.5f, 0.0f));
		}
	}

	if (needsToWait && _widget)
	{
		_widget->mark_requires_rendering();
	}
}

Colour Utils::WeaponPartDraw::get_transfer_chance_colour(int transferChance)
{
	static Colour fullChanceColour = RegisteredColours::get_colour(String(TXT("weapon_part_transfer_chance_full")));
	static Colour highChanceColour = RegisteredColours::get_colour(String(TXT("weapon_part_transfer_chance_high")));
	static Colour mediumChanceColour = RegisteredColours::get_colour(String(TXT("weapon_part_transfer_chance_medium")));
	static Colour lowChanceColour = RegisteredColours::get_colour(String(TXT("weapon_part_transfer_chance_low")));
	static Colour noChanceColour = RegisteredColours::get_colour(String(TXT("weapon_part_transfer_chance_no")));

	if (transferChance <= 0) return noChanceColour; else
	if (transferChance < 40) return lowChanceColour; else
	if (transferChance < 75) return mediumChanceColour; else
	if (transferChance < 100) return highChanceColour;
	return fullChanceColour;
}
