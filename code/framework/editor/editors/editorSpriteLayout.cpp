#include "editorSpriteLayout.h"

#include "editorBaseImpl.inl"
#include "..\editorContext.h"

#include "..\..\ui\uiCanvasInputContext.h"

#include "..\..\ui\uiCanvasButton.h"
#include "..\..\ui\uiCanvasWindow.h"

#include "..\..\ui\utils\uiUtils.h"
#include "..\..\ui\utils\uiGrid2Menu.h"
#include "..\..\ui\utils\uiListWindow.h"
#include "..\..\ui\utils\uiListWindowImpl.inl"
#include "..\..\ui\utils\uiQuestionWindow.h"
#include "..\..\ui\utils\uiTextEditWindow.h"

#include "..\..\video\colourPalette.h"
#include "..\..\video\sprite.h"

#include "..\..\library\library.h"
#include "..\..\library\usedLibraryStored.inl"

#include "..\..\..\core\debug\debugRenderer.h"
#include "..\..\..\core\system\input.h"
#include "..\..\..\core\system\core.h"
#include "..\..\..\core\other\parserUtils.h"

//

#ifdef AN_ALLOW_OPTIMIZE_OFF
#pragma optimize("", off)
#endif

#define HIGHLIGHT_EDITED_ENTRY_TIME 0.2f
#define HIGHLIGHT_EDITED_ENTRY_COLOUR Colour::orange

#define IGNORE_ZERO_OFFSET true

#define CONTENT_ACCESS SpriteLayoutContentAccess

//

using namespace Framework;

//

DEFINE_STATIC_NAME(chooseLibraryStored_includeSprite);

// ui
DEFINE_STATIC_NAME(editorSpriteLayout_props);
DEFINE_STATIC_NAME(editorSpriteLayout_entryInfo);

//

//--

REGISTER_FOR_FAST_CAST(Editor::SpriteLayout);

Editor::SpriteLayout::SpriteLayout()
{
}

Editor::SpriteLayout::~SpriteLayout()
{
}

void Editor::SpriteLayout::edit(LibraryStored* _asset, String const& _filePathToSave)
{
	base::edit(_asset, _filePathToSave);
}


void Editor::SpriteLayout::restore_for_use(SimpleVariableStorage const& _setup)
{
	base::restore_for_use(_setup);
	update_ui_highlight();
}

void Editor::SpriteLayout::store_for_later(SimpleVariableStorage& _setup) const
{
	base::store_for_later(_setup);
}

void Editor::SpriteLayout::process_controls()
{
	base::process_controls();

	if (auto* vm = fast_cast<Framework::SpriteLayout>(get_edited_asset()))
	{
		if (CONTENT_ACCESS::get_edited_entry_index(vm) == NONE &&
			! vm->get_entries().is_empty())
		{
			CONTENT_ACCESS::set_edited_entry_index(vm, 0);
		}
	}

	highlightEditedEntryFor = max(0.0f, highlightEditedEntryFor - ::System::Core::get_raw_delta_time());

	UI::Canvas* canvas = get_canvas();
	if (!canvas) { return; }

	// manage gizmos
	//
	
	// active sprite entry
	{
		Framework::SpriteLayout::Entry * activeEntry = nullptr;
		auto* vm = fast_cast<Framework::SpriteLayout>(get_edited_asset());
		if (vm)
		{
			activeEntry = CONTENT_ACCESS::access_edited_entry(vm);
		}
		if (activeEntry)
		{
			if (transformGizmos.forEntry != activeEntry)
			{
				remove_transform_gizmos();
			}

			// create if required
			if (!transformGizmos.offsetGizmo.is_in_use())
			{
				transformGizmos.offsetGizmo.atWorld = activeEntry->at.to_vector2() * (activeEntry->sprite.get()? activeEntry->sprite->get_pixel_size() : Vector2::one);
				transformGizmos.offsetGizmo.fromWorld = Vector2::zero;
				transformGizmos.forEntry = activeEntry;
			}

			// update
			bool didSomethingChange = false;
			{
				// update main offset gizmo now
				transformGizmos.offsetGizmo.update(this);

				{
					VectorInt2& offset = activeEntry->at;
					VectorInt2 prevOffset = offset;
					offset = TypeConversions::Normal::f_i_closest(transformGizmos.offsetGizmo.atWorld / (activeEntry->sprite.get() ? activeEntry->sprite->get_pixel_size() : Vector2::one));
					didSomethingChange |= offset != prevOffset;
				}
			}

			if (didSomethingChange)
			{
				do_operation(Operation::ChangedEntryContent);
			}
		}
		else
		{
			remove_transform_gizmos();
		}
	}
}

bool Editor::SpriteLayout::on_hold(int _cursorIdx, int _buttonIdx, Vector2 const& _rayStart)
{
	update_cursor_ray(_cursorIdx, _rayStart);
	auto& cr = cursorRays[_cursorIdx];
	if (cr.entryIdx.is_set())
	{
		if (auto* vm = fast_cast<Framework::SpriteLayout>(get_edited_asset()))
		{
			CONTENT_ACCESS::set_edited_entry_index(vm, cr.entryIdx.get());
			highlightEditedEntryFor = HIGHLIGHT_EDITED_ENTRY_TIME;
			do_operation(Operation::ChangedEntry);
			return true;
		}
	}

	return base::on_hold(_cursorIdx, _buttonIdx, _rayStart);
}

bool Editor::SpriteLayout::on_held(int _cursorIdx, int _buttonIdx, Vector2 const& _rayAt, Vector2 const& _movedBy, Vector2 const& _cursorMovedBy2DNormalised)
{
	if (cursorRays.is_index_valid(_cursorIdx))
	{
		auto& cr = cursorRays[_cursorIdx];
		if (cr.entryIdx.is_set())
		{
			if (auto* vm = fast_cast<Framework::SpriteLayout>(get_edited_asset()))
			{
				if (auto* e = CONTENT_ACCESS::access_entry(vm, cr.entryIdx.get()))
				{
					if (auto* s = e->sprite.get())
					{
						cr.movedByAccumulated += _movedBy;
						Vector2 inPixels = cr.movedByAccumulated / s->get_pixel_size();
						VectorInt2 movedByPixels = TypeConversions::Normal::f_i_closest(inPixels);
						if (!movedByPixels.is_zero())
						{
							cr.movedByAccumulated -= movedByPixels.to_vector2() * s->get_pixel_size();
							e->at += movedByPixels;
							remove_transform_gizmos();
							do_operation(Operation::ChangedEntryContent);
						}
						return true;
					}
				}
			}
		}
	}

	return base::on_held(_cursorIdx, _buttonIdx, _rayAt, _movedBy, _cursorMovedBy2DNormalised);
}

void Editor::SpriteLayout::update_cursor_ray(int _cursorIdx, Vector2 const& _rayStart)
{
	if (!cursorRays.is_index_valid(_cursorIdx))
	{
		cursorRays.set_size(max(cursorRays.get_size(), _cursorIdx + 1));
	}

	auto& cr = cursorRays[_cursorIdx];

	if (hoveringOverAnyGizmo)
	{
		// clear and ignore
		cr = CursorRay();
		return;
	}

	if (cr.rayStart.is_set() && cr.rayStart == _rayStart)
	{
		// already cached
		return;
	}

	cr.hitPixel = false;
	cr.entryIdx.clear();
	cr.rayStart = _rayStart;

	if (auto* vm = fast_cast<Framework::SpriteLayout>(get_edited_asset()))
	{
		for_every_reverse(e, vm->get_entries())
		{
			if (auto* s = e->sprite.get())
			{
				Vector2 rayStart = _rayStart - e->at.to_vector2() * s->get_pixel_size();
				Framework::SpritePixelHitInfo hitInfo;
				if (SpriteContentAccess::ray_cast(s, rayStart, hitInfo, true, IGNORE_ZERO_OFFSET))
				{
					cr.hitPixel = true;
					cr.entryIdx = for_everys_index(e);
					cr.cellCoord = hitInfo.hit;
					break;
				}
			}
		}
	}
}

void Editor::SpriteLayout::do_operation(Operation _operation)
{
	bool didSomething = false;
	bool removeGizmos = false;

	if (auto* vm = fast_cast<Framework::SpriteLayout>(get_edited_asset()))
	{
		if (_operation == Operation::AddEntry)
		{
			cursorRays.clear();
			UI::Utils::choose_library_stored<Framework::Sprite>(get_canvas(), NAME(chooseLibraryStored_includeSprite), TXT("include sprite"), lastSpriteIncluded.get(), 1.0f,
				[this](Framework::Sprite* _chosen)
				{
					if (auto* vm = fast_cast<Framework::SpriteLayout>(get_edited_asset()))
					{
						CONTENT_ACCESS::add_entry(vm, _chosen);
						lastSpriteIncluded = _chosen;
						highlightEditedEntryFor = HIGHLIGHT_EDITED_ENTRY_TIME;
					}
				});
			removeGizmos = true;
		}
		if (_operation == Operation::DuplicateEntry)
		{
			cursorRays.clear();
			CONTENT_ACCESS::duplicate_edited_entry(vm);
			didSomething = true;
			removeGizmos = true;
		}
		if (_operation == Operation::RemoveEntry)
		{
			cursorRays.clear();
			CONTENT_ACCESS::remove_edited_entry(vm);
			didSomething = true;
			removeGizmos = true;
		}
		if (_operation == Operation::ChangedEntry)
		{
			didSomething = true;
		}
		if (_operation == Operation::ChangedEntryContent)
		{
			didSomething = true;
		}
		if (_operation == Operation::MovedEntry)
		{
			cursorRays.clear();
			didSomething = true;
			removeGizmos = true;
		}
	}

	if (removeGizmos)
	{
		remove_transform_gizmos();
	}

	if (didSomething)
	{
		if (editorContext)
		{
			if (_operation != Operation::ChangedEntry)
			{
				editorContext->mark_save_required();
			}
			update_ui_highlight();
		}
	}
}

Range2 Editor::SpriteLayout::get_grid_range() const
{
	if (auto* vm = fast_cast<Framework::SpriteLayout>(get_edited_asset()))
	{
		Range2 r = Range2::empty;
		for_every(e, vm->get_entries())
		{
			if (auto* s = e->sprite.get())
			{
				RangeInt2 er = SpriteContentAccess::get_combined_range(s);
				er.offset(e->at);
				er.expand_by(VectorInt2::one * 4);
				r.include(s->to_full_range(er));
			}
		}


		return r;
	}

	return base::get_grid_range();
}

Vector2 Editor::SpriteLayout::get_grid_step() const
{
	if (auto* vm = fast_cast<Framework::SpriteLayout>(get_edited_asset()))
	{
		if (auto* e = CONTENT_ACCESS::access_edited_entry(vm))
		{
			if (auto* s = e->sprite.get())
			{
				return s->get_pixel_size();
			}
		}
	}

	return base::get_grid_step();
}

Vector2 Editor::SpriteLayout::get_grid_zero_at() const
{
	return base::get_grid_zero_at();
}

float Editor::SpriteLayout::get_grid_alpha(float _v, float _step) const
{
	float a = 0.2f;
	int i = TypeConversions::Normal::f_i_closest(round_to(_v, _step) / _step);
	if ((i % 8) == 0)
	{
		a = max(a, 1.0f);
	}
	if ((i % 4) == 0)
	{
		a = max(a, 0.75f);
	}
	if ((i % 2) == 0)
	{
		a = max(a, 0.5f);
	}
	return a;
}

void Editor::SpriteLayout::render_debug(CustomRenderContext* _customRC)
{
	debug_gather_in_renderer();

	debug_gather_restore();

	base::render_debug(_customRC);
}

void Editor::SpriteLayout::render_edited_asset()
{
	if (auto* vm = fast_cast<Framework::SpriteLayout>(editedAsset.get()))
	{
		Framework::SpriteLayoutContentAccess::RenderContext context;
		if (highlightEditedEntryFor > 0.0f)
		{
			context.editedEntryColour = HIGHLIGHT_EDITED_ENTRY_COLOUR;
		}
		context.ignoreZeroOffset = IGNORE_ZERO_OFFSET;
		CONTENT_ACCESS::render(vm, context);
	}
	else
	{
		base::render_edited_asset();
	}
}

void Editor::SpriteLayout::setup_ui(REF_ SetupUIContext& _setupUIContext)
{
	base::setup_ui(REF_ _setupUIContext);

	UI::Canvas* canvas = get_canvas();
	if (!canvas) { return; }
	auto* c = canvas;

	Framework::UI::ICanvasElement* & below = _setupUIContext.topRight_down;

	// top right corner
	{
		{
			auto* menu = Framework::UI::CanvasWindow::get();
			menu->set_title(TXT("tools"))->set_closable(false)->set_movable(false);
			c->add(menu);
			{
				{
					auto* b = Framework::UI::CanvasButton::get()->set_caption(String(TXT("move up")))
#ifdef AN_STANDARD_INPUT
						->set_shortcut(System::Key::UpArrow, UI::ShortcutParams().with_ctrl().handle_hold_as_presses())
#endif
						->set_auto_width(c)->set_default_height(c);
					b->set_on_press([this](Framework::UI::ActContext const& _context)
						{
							if (auto* vm = fast_cast<Framework::SpriteLayout>(get_edited_asset()))
							{
								CONTENT_ACCESS::move_edited_entry(vm, 1);
							}
							do_operation(Operation::ChangedEntryContent);
						});
					menu->add(b);
				}
				{
					auto* b = Framework::UI::CanvasButton::get()->set_caption(String(TXT("move down")))
#ifdef AN_STANDARD_INPUT
						->set_shortcut(System::Key::DownArrow, UI::ShortcutParams().with_ctrl().handle_hold_as_presses())
#endif
						->set_auto_width(c)->set_default_height(c);
					b->set_on_press([this](Framework::UI::ActContext const& _context)
						{
							if (auto* vm = fast_cast<Framework::SpriteLayout>(get_edited_asset()))
							{
								CONTENT_ACCESS::move_edited_entry(vm, -1);
							}
							do_operation(Operation::ChangedEntryContent);
						});
					menu->add(b);
				}
				{
					auto* b = Framework::UI::CanvasButton::get()->set_caption(String(TXT("next/up")))
#ifdef AN_STANDARD_INPUT
						->set_shortcut(System::Key::UpArrow, UI::ShortcutParams().handle_hold_as_presses())
#endif
						->set_auto_width(c)->set_default_height(c);
					b->set_on_press([this](Framework::UI::ActContext const& _context)
						{
							if (auto* vm = fast_cast<Framework::SpriteLayout>(get_edited_asset()))
							{
								CONTENT_ACCESS::set_edited_entry_index(vm, CONTENT_ACCESS::get_edited_entry_index(vm) + 1);
								highlightEditedEntryFor = HIGHLIGHT_EDITED_ENTRY_TIME;
							}
							do_operation(Operation::ChangedEntry);
						});
					menu->add(b);
				}
				{
					auto* b = Framework::UI::CanvasButton::get()->set_caption(String(TXT("prev/down")))
#ifdef AN_STANDARD_INPUT
						->set_shortcut(System::Key::DownArrow, UI::ShortcutParams().handle_hold_as_presses())
#endif
						->set_auto_width(c)->set_default_height(c);
					b->set_on_press([this](Framework::UI::ActContext const& _context)
						{
							if (auto* vm = fast_cast<Framework::SpriteLayout>(get_edited_asset()))
							{
								CONTENT_ACCESS::set_edited_entry_index(vm, CONTENT_ACCESS::get_edited_entry_index(vm) - 1);
								highlightEditedEntryFor = HIGHLIGHT_EDITED_ENTRY_TIME;
							}
							do_operation(Operation::ChangedEntry);
						});
					menu->add(b);
				}
				{
					auto* b = Framework::UI::CanvasButton::get()->set_caption(String(TXT("add")))
#ifdef AN_STANDARD_INPUT
						->set_shortcut(System::Key::A)
#endif
						->set_auto_width(c)->set_default_height(c);
					b->set_on_press([this](Framework::UI::ActContext const& _context)
						{
							do_operation(Operation::AddEntry);
						});
					menu->add(b);
				}
				{
					auto* b = Framework::UI::CanvasButton::get()->set_caption(String(TXT("remove")))
#ifdef AN_STANDARD_INPUT
						->set_shortcut(System::Key::R)
#endif
						->set_auto_width(c)->set_default_height(c);
					b->set_on_press([this](Framework::UI::ActContext const& _context)
						{
							do_operation(Operation::RemoveEntry);
						});
					menu->add(b);
				}
				{
					auto* b = Framework::UI::CanvasButton::get()->set_caption(String(TXT("duplicate")))
#ifdef AN_STANDARD_INPUT
						->set_shortcut(System::Key::D)
#endif
						->set_auto_width(c)->set_default_height(c);
					b->set_on_press([this](Framework::UI::ActContext const& _context)
						{
							do_operation(Operation::DuplicateEntry);
						});
					menu->add(b);
				}
				{
					auto* b = Framework::UI::CanvasButton::get()->set_caption(String(TXT("tags")))
#ifdef AN_STANDARD_INPUT
						->set_shortcut(System::Key::T)
#endif
						->set_auto_width(c)->set_default_height(c);
					b->set_on_press([this](Framework::UI::ActContext const& _context)
						{
							if (auto* vm = fast_cast<Framework::SpriteLayout>(get_edited_asset()))
							{
								if (auto* e = CONTENT_ACCESS::access_edited_entry(vm))
								{
									UI::Utils::edit_text(_context.canvas, TXT("tags"), e->tags.to_string_all(),
										[e, this](String const& _text)
										{
											e->tags.clear();
											e->tags.load_from_string(_text);
											do_operation(Operation::ChangedEntryContent);
										});
								}
							}
						});
					menu->add(b);
				}

				//menu->place_content_horizontally(c);
				menu->place_content_on_grid(c, VectorInt2(2, 0));
			}
			UI::Utils::place_below(c, menu, REF_ below, 1.0f);

			if (!_setupUIContext.topRight)
			{
				_setupUIContext.topRight = below;
				_setupUIContext.topRight_left = below;
			}
		}
	}
}

void Editor::SpriteLayout::remove_transform_gizmos()
{
	transformGizmos = TransformGizmos(); // will clear all
}

void Editor::SpriteLayout::show_asset_props()
{
	auto* c = get_canvas();

	if (auto* e = c->find_by_id(NAME(editorSpriteLayout_props)))
	{
		e->remove();
		return;
	}

	if (auto* vm = fast_cast<Framework::SpriteLayout>(get_edited_asset()))
	{
		UI::Utils::Grid2Menu menu;

		auto* window = menu.step_1_create_window(c, 1.0f)->set_title(TXT("sprite layout props"));
		window->set_id(NAME(editorSpriteLayout_props));
		window->set_modal(false)->set_closable(true);
		add_common_props_to_asset_props(menu);
		menu.step_3_add_no_button();
		menu.step_3_add_button(TXT("close"))
#ifdef AN_STANDARD_INPUT
			->set_shortcut(System::Key::Esc)
#endif
			->set_on_press([window](Framework::UI::ActContext const& _context)
				{
					window->remove();
				});

		menu.step_4_finalise(c->get_logical_size().x * 0.5f);

		update_ui_highlight();
	}
}

void Editor::SpriteLayout::update_ui_highlight()
{
	UI::Canvas* canvas = get_canvas();
	if (!canvas) { return; }
	auto* c = canvas;

	if (auto* b = fast_cast<Framework::UI::CanvasButton>(c->find_by_id(NAME(editorSpriteLayout_entryInfo))))
	{
		String spriteName;
		String tags;
		if (auto* vm = fast_cast<Framework::SpriteLayout>(get_edited_asset()))
		{
			if (auto* e = CONTENT_ACCESS::access_edited_entry(vm))
			{
				if (auto* s = e->sprite.get())
				{
					spriteName = s->get_name().to_string();
				}
				tags = e->tags.to_string_all();
			}
		}
		b->set_caption(String::printf(TXT("%S / %S"), spriteName.to_char(), tags.to_char()));
		b->set_auto_width(canvas);
		b->set_default_height(canvas);
	}

	base::update_ui_highlight();
}

void Editor::SpriteLayout::setup_ui_add_to_top_name_panel(UI::Canvas* c, UI::CanvasWindow* panel, float scale)
{
	base::setup_ui_add_to_top_name_panel(c, panel, scale);

	{
		auto* b = Framework::UI::CanvasButton::get();
		b->set_caption(TXT("/"));
		b->set_scale(scale)->set_auto_width(c)->set_default_height(c);
		b->set_font_scale(get_name_font_scale());
		b->set_text_only();
		panel->add(b);
	}
	{
		auto* b = Framework::UI::CanvasButton::get();
		b->set_id(NAME(editorSpriteLayout_entryInfo));
		{
			String spriteName;
			String tags;
			if (auto* vm = fast_cast<Framework::SpriteLayout>(get_edited_asset()))
			{
				if (auto* e = CONTENT_ACCESS::access_edited_entry(vm))
				{
					if (auto* s = e->sprite.get())
					{
						spriteName = s->get_name().to_string();
					}
					tags = e->tags.to_string_all();
				}
			}
			b->set_caption(String::printf(TXT("%S / %S"), spriteName.to_char(), tags.to_char()));
		}
		b->set_scale(scale)->set_auto_width(c)->set_default_height(c);
		b->set_font_scale(get_name_font_scale());
		b->set_text_only();
		panel->add(b);
	}
}

