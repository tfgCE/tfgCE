#include "lhs_test.h"

#include "..\loaderHub.h"
#include "..\loaderHubScreen.h"

#include "..\widgets\lhw_customDraw.h"
#include "..\widgets\lhw_list.h"

#include "..\..\..\game\game.h"
#include "..\..\..\library\library.h"
#include "..\..\..\modules\gameplay\modulePilgrim.h"
#include "..\..\..\modules\gameplay\equipment\me_gun.h"
#include "..\..\..\schematics\schematic.h"
#include "..\..\..\schematics\schematicLine.h"

#include "..\..\..\..\framework\object\actor.h"
#include "..\..\..\..\framework\video\font.h"

#ifdef AN_CLANG
#include "..\..\..\..\framework\library\usedLibraryStored.inl"
#endif

//

DEFINE_STATIC_NAME(test);

//

using namespace Loader;
using namespace HubScenes;

//

class SomeData
: public Loader::IHubDraggableData
{
	FAST_CAST_DECLARE(SomeData);
	FAST_CAST_BASE(Loader::IHubDraggableData);
	FAST_CAST_END();
public:
	String desc;
};
REGISTER_FOR_FAST_CAST(SomeData);

//

REGISTER_FOR_FAST_CAST(Test);

Test::Test()
{
}

Array<RefCountObjectPtr<SomeData>>* testArray0 = nullptr;
Array<RefCountObjectPtr<SomeData>>* testArray1 = nullptr;

void Test::on_activate(HubScene* _prev)
{
	base::on_activate(_prev);

	get_hub()->allow_to_deactivate_with_loader_immediately(false);

	get_hub()->deactivate_all_screens();

	HubScreen* screen;
	Vector2 requiredSize = Vector2(600.0f, 300.0f);
	Range2 wholeScreen(requiredSize);
	{
		Vector2 ppa = HubScreen::s_pixelsPerAngle;
		requiredSize.y += 4.0f;
		requiredSize /= ppa;
		screen = new HubScreen(get_hub(), NAME(test), requiredSize, get_hub()->get_background_dir_or_main_forward(), get_hub()->get_radius() * 0.7f, NP, NP, ppa);
		screen->activate();
		screen->followYawDeadZoneByBorder = HubScreen::DEFAULT_YAW_DEAD_ZONE_BY_BORDER * 2.0f;
		get_hub()->add_screen(screen);
	}

	if (0)
	{
		testArray0 = new Array<RefCountObjectPtr<SomeData>>();
		testArray1 = new Array<RefCountObjectPtr<SomeData>>();
		for_count(int, i, 20)
		{
			SomeData* sd = new SomeData();
			sd->desc = String::printf(TXT("%i"), i);
			testArray0->push_back(RefCountObjectPtr<SomeData>(sd));
		}

		for_count(int, i, 2)
		{
			Vector2 beAt(screen->mainResolutionInPixels.x.centre() - 20.0f * 8.0f - 15.0f, screen->mainResolutionInPixels.y.min + 8.0f);
			beAt.x += (float)i * (30.0f * 8.0f);
			auto* w = new HubWidgets::List(Name::invalid(), Range2(Range(beAt.x, beAt.x + 20.0f * 8.0f), Range(beAt.y, beAt.y + 20.0f * 8.0f)));
			w->be_draggable();
			//w->elementSize = Vector2(16.0f, 16.0f);
			if (i == 0)
			{
				w->elementSize = Vector2(48.0f, 48.0f);
				w->be_grid();
				w->can_drop_discard = [](Loader::HubDraggable const* _draggable) { return true; };
			}
			else
			{
				w->elementSize = Vector2(16.0f, 16.0f);
				for_every_ref(sd, *testArray0)
				{
					w->add(sd);
				}
				w->should_stay_on_drag = [](Loader::HubDraggable const* _draggable) { return true; };
			}
			w->draw_element = [w](Framework::Display* _display, Range2 const& _at, Loader::HubDraggable const* _element)
			{
				if (auto* sd = fast_cast<SomeData>(_element->data.get()))
				{
					if (auto* font = _display->get_font())
					{
						font->draw_text_at(::System::Video3D::get(), sd->desc.to_char(), w->hub->is_selected(w, _element) ? HubColours::selected_highlighted() : HubColours::text(), _at.centre(), Vector2::one, Vector2::half);
					}
				}
			};
			w->can_hover = [](Loader::HubDraggable const* _element, Vector2 const& _at) { return fast_cast<SomeData>(_element->data.get()) != nullptr; };
			w->can_drop = [](Loader::HubDraggable const* _element, Vector2 const& _at) { return fast_cast<SomeData>(_element->data.get()) != nullptr; };
			Array<RefCountObjectPtr<SomeData>>* testArray = i == 0 ? testArray0 : testArray1;
			w->on_drag_done = [w, testArray]()
			{
				if (HubWidgets::List* _list = fast_cast<HubWidgets::List>(w))
				{
					testArray->clear();
					for_every_ref(element, _list->elements)
					{
						if (element->from == _list)
						{
							if (auto* sd = fast_cast<SomeData>(element->data.get()))
							{
								testArray->push_back(RefCountObjectPtr<SomeData>(sd));
							}
						}
					}
				}
			};

			screen->add_widget(w);
		}
	}
	if (0)
	{
		RefCountObjectPtr<TeaForGodEmperor::Schematic> schematic;
		schematic = new TeaForGodEmperor::Schematic();

		{
			TeaForGodEmperor::SchematicLine* line = new TeaForGodEmperor::SchematicLine();
			line->set_line_width(10.0f);
			line->set_colour(Colour::green);
			line->set_looped();
			line->add(Vector2(-80.0f, -80.0f));
			line->add(Vector2(-80.0f, 80.0f));
			line->add(Vector2(80.0f, 80.0f));
			line->add(Vector2(80.0f, -80.0f));
			schematic->add(line);
		}

		{
			TeaForGodEmperor::SchematicLine* line = new TeaForGodEmperor::SchematicLine();
			line->set_sort_priority(10);
			line->set_line_width(10.0f);
			line->set_colour(Colour::red, Colour::yellow);
			line->set_looped();
			line->add(Vector2(-60.0f, -60.0f));
			line->add(Vector2(-30.0f, 60.0f));
			line->add(Vector2(180.0f, 20.0f));
			line->add(Vector2(60.0f, -30.0f));
			//schematic->cut_outer_lines_with(*line);
			schematic->add(line);
		}

		schematic->build_mesh();

		auto* w = new HubWidgets::CustomDraw(Name::invalid(), wholeScreen, nullptr);
		w->custom_draw = [w, schematic](Framework::Display* _display, Range2 const& _at)
		{
			if (Meshes::Mesh3DInstance* meshInstance = (cast_to_nonconst(&schematic->get_mesh_instance())))
			{
				auto* v3d = ::System::Video3D::get();
				System::VideoMatrixStack& modelViewStack = v3d->access_model_view_matrix_stack();
				Vector2 at = Vector2(300.0, 200.0f);
				modelViewStack.push_to_world(Transform(at.to_vector3(), Rotator3(0.0f, 0.0f, 0.0f).to_quat()).to_matrix());
				Meshes::Mesh3DRenderingContext renderingContext;
				meshInstance->render(v3d, renderingContext);
				modelViewStack.pop();

				w->mark_requires_rendering();
			}
		};
		screen->add_widget(w);
	}
	if (1)
	{
		if (auto* game = TeaForGodEmperor::Game::get_as<TeaForGodEmperor::Game>())
		{
			if (auto* pa = game->access_player().get_actor())
			{
				if (auto* mp = pa->get_gameplay_as<TeaForGodEmperor::ModulePilgrim>())
				{
					Array<RefCountObjectPtr<TeaForGodEmperor::WeaponPart>> weaponParts;
					for_count(int, handIdx, Hand::MAX)
					{
						if (auto* gunObject = mp->get_main_equipment(Hand::Type(handIdx)))
						{
							if (auto* gun = (gunObject->get_gameplay_as<TeaForGodEmperor::ModuleEquipments::Gun>()))
							{
								for_every(part, gun->get_weapon_setup().get_parts())
								{
									if (part->part.get())
									{
										//weaponParts.push_back(part->part);
									}
								}
							}
						}
					}
					{
						for_count(int, i, 18)
						{
							int idx = Random::get_int(TeaForGodEmperor::Library::get_current_as<TeaForGodEmperor::Library>()->get_weapon_part_types().get_size());
							auto* wpt = TeaForGodEmperor::Library::get_current_as<TeaForGodEmperor::Library>()->get_weapon_part_types().get_all_stored()[idx].get();
							if (wpt->can_be_chosen_randomly())
							{
								RefCountObjectPtr<TeaForGodEmperor::WeaponPart> wp;
								wp = TeaForGodEmperor::WeaponPart::create_instance_of(&TeaForGodEmperor::Persistence::access_current(), wpt);
								weaponParts.push_back(wp);
							}
							else
							{
								--i;
							}
						}
					}
					for_every_ref(part, weaponParts)
					{
						part->make_sure_schematic_mesh_exists();
					}
					auto* w = new HubWidgets::CustomDraw(Name::invalid(), wholeScreen, nullptr);
					w->custom_draw = [w, weaponParts](Framework::Display* _display, Range2 const& _at)
					{
						Vector2 wpSize = Vector2(80.0f, 80.0f);
						float spacing = 20.0f;
						Vector2 at = _display->get_left_bottom_of_screen();
						at.x += spacing * 0.5f;
						at.y += spacing * 0.5f;
						for_every_ref(part, weaponParts)
						{
							if (at.y >= _display->get_right_top_of_screen().y - wpSize.y - spacing * 0.5f)
							{
								at.x += wpSize.x + spacing;
								at.y = _display->get_left_bottom_of_screen().y;
								at.y += spacing * 0.5f;
							}

							if (Meshes::Mesh3DInstance* meshInstance = (cast_to_nonconst(part->get_schematic_mesh_instance())))
							{
								auto* v3d = ::System::Video3D::get();
								System::VideoMatrixStack& modelViewStack = v3d->access_model_view_matrix_stack();
								Vector2 mat = at + wpSize * 0.5f;
								modelViewStack.push_to_world(Transform(mat.to_vector3(), Rotator3(0.0f, 0.0f, 0.0f).to_quat(), wpSize.x / TeaForGodEmperor::WeaponPart::get_schematic_size().x).to_matrix());
								Meshes::Mesh3DRenderingContext renderingContext;
								meshInstance->render(v3d, renderingContext);
								modelViewStack.pop();

								w->mark_requires_rendering();
							}

							at.y += wpSize.y + spacing;
						}
					};
					screen->add_widget(w);
				}
			}
		}
	}
}

void Test::on_update(float _deltaTime)
{
	base::on_update(_deltaTime);
}
