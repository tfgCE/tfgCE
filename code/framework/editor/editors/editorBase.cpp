#include "editorBase.h"

#include "..\editorContext.h"

#include "..\..\game\game.h"
#include "..\..\library\library.h"
#include "..\..\library\usedLibraryStored.inl"
#include "..\..\ui\uiCanvasInputContext.h"

#include "..\..\ui\uiCanvasButton.h"
#include "..\..\ui\uiCanvasWindow.h"

#include "..\..\ui\utils\uiUtils.h"
#include "..\..\ui\utils\uiGrid2Menu.h"
#include "..\..\ui\utils\uiTextEditWindow.h"

#include "..\..\..\core\concurrency\scopedSpinLock.h"
#include "..\..\..\core\system\video\renderTarget.h"
#include "..\..\..\core\system\video\video3d.h"
#include "..\..\..\core\system\video\video3dPrimitives.h"
#include "..\..\..\core\system\video\videoClipPlaneStackImpl.inl"

//

#ifdef AN_ALLOW_OPTIMIZE_OFF
#pragma optimize("", off)
#endif

//

using namespace Framework;

//

// ui
DEFINE_STATIC_NAME(editor_menu);
DEFINE_STATIC_NAME(editor_save);
DEFINE_STATIC_NAME(editor_saveAll);
DEFINE_STATIC_NAME(editor_assetSaveLoadMenu);
DEFINE_STATIC_NAME(editor_props_name);
DEFINE_STATIC_NAME(editor_props_tags);
DEFINE_STATIC_NAME(editor_props_customAttributes);
DEFINE_STATIC_NAME(editor_props_customAttributes_window);

//

REGISTER_FOR_FAST_CAST(Editor::Base);

Editor::Base::Base()
{
}

Editor::Base::~Base()
{
}

void Editor::Base::restore_for_use(SimpleVariableStorage const& _setup)
{

}

void Editor::Base::store_for_later(SimpleVariableStorage & _setup) const
{

}

void Editor::Base::edit(LibraryStored* _asset, String const& _filePathToSave)
{
	editedAsset = _asset;
	filePathToSave = _filePathToSave;
}

UI::Canvas* Editor::Base::get_canvas() const
{
	return editorContext ? editorContext->get_canvas() : nullptr;
}

Vector2 Editor::Base::get_name_font_scale()
{
	return Vector2(0.8f, 1.0f);
}

void Editor::Base::process_controls()
{
	UI::Canvas* canvas = get_canvas();
	if (!canvas) { return; }

	Concurrency::ScopedSpinLock lock(canvas->access_lock(), true);

	UI::CanvasInputContext cic;
	cic.set_cursor(UI::CursorIndex::Mouse, UI::CanvasInputContext::Cursor::use_mouse(canvas));

	canvas->update(cic);
}

void Editor::Base::pre_render(CustomRenderContext* _customRC)
{
}

void Editor::Base::render_main(CustomRenderContext* _customRC)
{
	System::Video3D* v3d = System::Video3D::get();
	System::RenderTarget* rtTarget = _customRC ? _customRC->sceneRenderTarget.get() : Game::get()->get_main_render_buffer();

	::System::ForRenderTargetOrNone forRT(rtTarget);
	VectorInt2 rtSize = forRT.get_size();

	forRT.bind();

	v3d->set_viewport(VectorInt2::zero, rtSize);
	v3d->set_2d_projection_matrix_ortho(rtSize.to_vector2());
	v3d->access_model_view_matrix_stack().clear();
	v3d->access_clip_plane_stack().clear();
	v3d->set_near_far_plane(0.02f, 100.0f);
	v3d->setup_for_2d_display();
	v3d->ready_for_rendering();

	v3d->clear_colour_depth_stencil(backgroundColour);

	forRT.unbind();
}

void Editor::Base::render_debug(CustomRenderContext* _customRC)
{
}

void Editor::Base::render_ui(CustomRenderContext* _customRC)
{
	UI::Canvas* canvas = get_canvas();
	if (!canvas) { return; }

	Concurrency::ScopedSpinLock lock(canvas->access_lock(), true);

	System::Video3D* v3d = System::Video3D::get();
	System::RenderTarget* rtTarget = _customRC ? _customRC->sceneRenderTarget.get() : Game::get()->get_main_render_output_render_buffer();

	::System::ForRenderTargetOrNone forRT(rtTarget);
	VectorInt2 rtTargetSize = forRT.get_size();

	if (editorContext)
	{
		editorContext->update_canvas_real_size(rtTargetSize.to_vector2());
	}	

	canvas->render_on(v3d, forRT.rt, NP);
}

void Editor::Base::setup_ui_add_to_top_name_panel(UI::Canvas* c, UI::CanvasWindow* panel, float scale)
{
	{
		auto* b = Framework::UI::CanvasButton::get()->set_caption(TXT("reload"))
#ifdef AN_STANDARD_INPUT
			->set_shortcut(System::Key::R, UI::ShortcutParams().with_ctrl())
#endif
			->set_scale(scale)->set_auto_width(c)->set_default_height(c);
		b->set_on_press([this](Framework::UI::ActContext const& _context)
			{
				if (editorContext)
				{
					editorContext->load_edited_asset();
					update_ui_highlight();
				}
			});
		panel->add(b);
	}
	{
		auto* b = Framework::UI::CanvasButton::get()->set_caption(TXT("save"))
#ifdef AN_STANDARD_INPUT
			->set_shortcut(System::Key::S, UI::ShortcutParams().with_ctrl())
#endif
			->set_scale(scale)->set_auto_width(c)->set_default_height(c);
		b->set_id(NAME(editor_save));
		b->set_on_press([this](Framework::UI::ActContext const& _context)
			{
				if (editorContext)
				{
					editorContext->save_asset();
					editorContext->save_workspace();
					update_ui_highlight();
				}
			});
		panel->add(b);
	}
	{
		auto* b = Framework::UI::CanvasButton::get()->set_caption(TXT("save all"))
#ifdef AN_STANDARD_INPUT
			->set_shortcut(System::Key::S, UI::ShortcutParams().with_ctrl().with_shift())
#endif
			->set_scale(scale)->set_auto_width(c)->set_default_height(c);
		b->set_id(NAME(editor_saveAll));
		b->set_on_press([this](Framework::UI::ActContext const& _context)
			{
				if (editorContext)
				{
					editorContext->save_all_assets();
					editorContext->save_workspace();
					update_ui_highlight();
				}
			});
		panel->add(b);
	}
	{
		auto* b = Framework::UI::CanvasButton::get()->set_caption(editedAsset.get() ? editedAsset->get_name().to_string().to_char() : TXT("--"))->set_scale(scale)->set_font_scale(get_name_font_scale())->set_auto_width(c)->set_default_height(c);
		b->set_text_only();
		panel->add(b);
	}
}

void Editor::Base::setup_ui(REF_ SetupUIContext & _setupUIContext)
{
	UI::Canvas* canvas = get_canvas();
	if (!canvas) { return; }

	float bigScale = 2.0f;
	float scale = 1.0f;

	auto* c = canvas;
	UI::ICanvasElement* & nextTo = _setupUIContext.topLeft_right;
	UI::ICanvasElement* & below = _setupUIContext.topLeft_down;

	c->clear();

	{
		auto* menu = Framework::UI::CanvasWindow::get();
		c->add(menu);
		{
			{
				auto* b = Framework::UI::CanvasButton::get()->set_caption(TXT("menu"))
#ifdef AN_STANDARD_INPUT
					->set_shortcut(System::Key::Esc)
#endif
					->set_scale(bigScale)->set_auto_width(c)->set_default_height(c);
				b->set_id(NAME(editor_menu));
				b->set_on_press([this](Framework::UI::ActContext const& _context)
					{
						if (editorContext)
						{
							editorContext->open_menu();
						}
					});
				menu->add(b);
			}

			menu->place_content_horizontally(c);
		}
		
		UI::Utils::place_on_right(c, menu, REF_ nextTo, 1.0f);
		if (!_setupUIContext.topLeft)
		{
			_setupUIContext.topLeft = nextTo;
			below = nextTo;
		}
	}

	{
		auto* menu = Framework::UI::CanvasWindow::get();
		menu->set_id(NAME(editor_assetSaveLoadMenu));
		c->add(menu);
		{
			setup_ui_add_to_top_name_panel(c, menu, scale);

			menu->place_content_horizontally(c);
		}
		
		UI::Utils::place_on_right(c, menu, REF_ nextTo, 1.0f);
	}

	{
		auto* menu = Framework::UI::CanvasWindow::get();
		c->add(menu);
		{
			{
				auto* b = Framework::UI::CanvasButton::get()->set_caption(TXT("asset details"))
#ifdef AN_STANDARD_INPUT
					->set_shortcut(System::Key::D, UI::ShortcutParams().with_ctrl())
#endif
					->set_scale(scale)->set_auto_width(c)->set_default_height(c);
				b->set_on_press([this](Framework::UI::ActContext const& _context)
					{
						show_asset_props();
					});
				menu->add(b);
			}
			menu->place_content_horizontally(c);
		}

		UI::Utils::place_below(c, menu, NAME(editor_assetSaveLoadMenu), 0.0f);
	}

	{
		auto* menu = Framework::UI::CanvasWindow::get();
		c->add(menu);
		{
			{
				auto* b = Framework::UI::CanvasButton::get()->set_caption(TXT("assets"))
#ifdef AN_STANDARD_INPUT
					->set_shortcut(System::Key::Home, UI::ShortcutParams().with_ctrl())
#endif
					->set_scale(scale)->set_auto_width(c)->set_default_height(c);
				b->set_on_press([this](Framework::UI::ActContext const& _context)
					{
						if (editorContext)
						{
							editorContext->open_asset_manager();
						}
					});
				menu->add(b);
			}
			{
				auto* b = Framework::UI::CanvasButton::get()->set_caption(TXT("prev"))
#ifdef AN_STANDARD_INPUT
					->set_shortcut(System::Key::PageUp, UI::ShortcutParams().with_ctrl())
#endif
					->set_scale(scale)->set_auto_width(c)->set_default_height(c);
				b->set_on_press([this](Framework::UI::ActContext const& _context)
					{
						if (editorContext)
						{
							editorContext->change_asset(-1);
						}
					});
				menu->add(b);
			}
			{
				auto* b = Framework::UI::CanvasButton::get()->set_caption(TXT("next"))
#ifdef AN_STANDARD_INPUT
					->set_shortcut(System::Key::PageDown, UI::ShortcutParams().with_ctrl())
#endif
					->set_scale(scale)->set_auto_width(c)->set_default_height(c);
				b->set_on_press([this](Framework::UI::ActContext const& _context)
					{
						if (editorContext)
						{
							editorContext->change_asset(1);
						}
					});
				menu->add(b);
			}
			menu->place_content_vertically(c);
		}

		UI::Utils::place_below(c, menu, REF_ below, 0.0f);

	}
}

void Editor::Base::update_ui_highlight()
{
	UI::Canvas* canvas = get_canvas();
	if (!canvas) { return; }

	auto* c = canvas;

	auto* editedAsset = get_edited_asset();

	if (auto* b = fast_cast<Framework::UI::CanvasButton>(c->find_by_id(NAME(editor_save))))
	{
		if (editorContext->is_save_required())
		{
			b->set_colour(Colour::lerp(0.25f, canvas->get_setup().colours.buttonPaper, Colour::red));
		}
		else
		{
			b->set_colour(NP);
		}
	}
	if (auto* b = fast_cast<Framework::UI::CanvasButton>(c->find_by_id(NAME(editor_saveAll))))
	{
		if (editorContext->is_save_all_required())
		{
			b->set_colour(Colour::lerp(0.25f, canvas->get_setup().colours.buttonPaper, Colour::red));
		}
		else
		{
			b->set_colour(NP);
		}
	}
	if (auto* b = fast_cast<Framework::UI::CanvasButton>(c->find_by_id(NAME(editor_props_name))))
	{
		b->set_caption(editedAsset ? editedAsset->get_name().to_string().to_char() : TXT("--"));
	}
	if (auto* b = fast_cast<Framework::UI::CanvasButton>(c->find_by_id(NAME(editor_props_tags))))
	{
		b->set_caption(editedAsset ? editedAsset->get_tags().to_string_all().to_char() : TXT("--"));
	}
	if (auto* b = fast_cast<Framework::UI::CanvasButton>(c->find_by_id(NAME(editor_props_customAttributes))))
	{
		b->set_caption(editedAsset ? Library::custom_attributes_to_string(editedAsset->get_custom_attributes()).to_char() : TXT("--"));
	}

	if (auto* w = fast_cast<Framework::UI::CanvasWindow>(c->find_by_id(NAME(editor_assetSaveLoadMenu))))
	{
		w->place_content_horizontally(c);
	}
}

void Editor::Base::add_common_props_to_asset_props(UI::Utils::Grid2Menu& menu)
{
	menu.step_2_add_text_and_button(TXT("name"), TXT("--"))
#ifdef AN_STANDARD_INPUT
		->set_shortcut(System::Key::N)
#endif
		->set_on_press([this](Framework::UI::ActContext const& _context)
			{
				if (auto* a = get_edited_asset())
				{
					UI::Utils::edit_text(_context.canvas, TXT("name"), a->get_name().to_string(), [a, this](String const& _text)
						{
							a->set_name(_text);
							update_ui_highlight();
						});
				}
			})
		->set_id(NAME(editor_props_name));
	menu.step_2_add_text_and_button(TXT("attributes"), TXT("--"))
#ifdef AN_STANDARD_INPUT
		->set_shortcut(System::Key::A)
#endif
		->set_on_press([this](Framework::UI::ActContext const& _context)
			{
				if (auto* a = get_edited_asset())
				{
					edit_custom_attributes();
				}
			})
		->set_id(NAME(editor_props_customAttributes));
	menu.step_2_add_text_and_button(TXT("tags"), TXT("--"))
#ifdef AN_STANDARD_INPUT
		->set_shortcut(System::Key::T)
#endif
		->set_on_press([this](Framework::UI::ActContext const& _context)
			{
				if (auto* a = get_edited_asset())
				{
					UI::Utils::edit_text(_context.canvas, TXT("tags"), a->get_tags().to_string_all(), [a, this](String const& _text)
						{
							a->access_tags().clear();
							a->access_tags().load_from_string(_text);
							editorContext->mark_save_required();
							update_ui_highlight();
						});
				}
			})
		->set_id(NAME(editor_props_tags));
}

void Editor::Base::edit_custom_attributes()
{
	UI::Utils::Grid2Menu menu;

	auto* c = get_canvas();

	auto* window = menu.step_1_create_window(c, 1.0f)->set_title(TXT("attributes"));
	window->set_id(NAME(editor_props_customAttributes_window));
	window->set_modal(false)->set_closable(true);
#ifdef AN_STANDARD_INPUT
	System::Key::Type keys[] = {
		System::Key::N1, System::Key::N2, System::Key::N3, System::Key::N4, System::Key::N5, System::Key::N6, System::Key::N7, System::Key::N8, System::Key::N9, System::Key::N0,
		System::Key::Q,  System::Key::W,  System::Key::E,  System::Key::R,  System::Key::T,  System::Key::Y,  System::Key::U,  System::Key::I,  System::Key::O,  System::Key::P,
		System::Key::A,  System::Key::S,  System::Key::D,  System::Key::F,  System::Key::G,  System::Key::H,  System::Key::J,  System::Key::K,  System::Key::L,
		System::Key::Z,  System::Key::X,  System::Key::C,  System::Key::V,  System::Key::B,  System::Key::N,  System::Key::M };
#endif
	for_count(int, bitIdx, Framework::Library::get_custom_attribute_info_bits_count())
	{
		auto* b = menu.step_2_add_text_and_button(Framework::Library::get_custom_attribute_info_text_for_bit(bitIdx), TXT("--"));
		if (auto* ea = fast_cast<Framework::Sprite>(get_edited_asset()))
		{
			int ca = ea->get_custom_attributes();
			b->set_caption(is_bit_set(ca, bitIdx) ? TXT("YES") : TXT("no"));
		}
#ifdef AN_STANDARD_INPUT
		b->set_shortcut(keys[bitIdx]);
#endif
		b->set_on_press([this, bitIdx, b](Framework::UI::ActContext const& _context)
			{
				if (auto* ea = fast_cast<Framework::Sprite>(get_edited_asset()))
				{
					int ca = ea->get_custom_attributes();
					if (is_bit_set(ca, bitIdx))
					{
						clear_bit(ca, bitIdx);
					}
					else
					{
						set_bit(ca, bitIdx);
					}
					b->set_caption(is_bit_set(ca, bitIdx) ? TXT("YES") : TXT("no"));
					ea->set_custom_attributes(ca);
					editorContext->mark_save_required();
					update_ui_highlight();
				}
			});
	}

	menu.step_3_add_no_button();
	menu.step_3_add_button(TXT("close"))
#ifdef AN_STANDARD_INPUT
		->set_shortcut(System::Key::Esc)
#endif
		->set_on_press([window](Framework::UI::ActContext const& _context)
			{
				window->remove();
			});

	menu.step_4_finalise(c->get_logical_size().x * 0.4f);
}