#include "lhc_drawingBoard.h"

#include "..\..\..\game\game.h"

#include "..\loaderHub.h"
#include "..\widgets\lhw_button.h"
#include "..\widgets\lhw_customDraw.h"

/*
#include "..\loaderHubWidget.h"
#include "lhc_message.h"

#include "..\widgets\lhw_button.h"
#include "..\widgets\lhw_text.h"

#include "..\..\..\reportBug.h"

#include "..\..\..\..\core\buildInformation.h"
#include "..\..\..\..\core\debug\debug.h"

#ifdef AN_CLANG
#include "..\..\..\..\framework\library\usedLibraryStored.inl"
#endif
*/

#include "..\..\..\..\core\system\video\renderTarget.h"
#include "..\..\..\..\core\system\video\video3dPrimitives.h"

//

#ifdef AN_ALLOW_OPTIMIZE_OFF
#pragma optimize("", off)
#endif

//

// input
DEFINE_STATIC_NAME(select);
DEFINE_STATIC_NAME(shift);

//

using namespace Loader;
using namespace HubScreens;

//

REGISTER_FOR_FAST_CAST(DrawingBoard);

DrawingBoard* DrawingBoard::show(Hub* _hub, Vector2 const& _size, Rotator3 const& _at, float _radius)
{
	Vector2 size = _size;

	::System::RenderTarget* db = nullptr;
#ifdef WITH_DRAWING_BOARD
	if (auto* game = TeaForGodEmperor::Game::get_as<TeaForGodEmperor::Game>())
	{
		db = game->get_drawing_board_rt();
	}
#endif

	if (!db)
	{
		return nullptr;
	}

	size.x = size.y * (float)db->get_size().x / (float)db->get_size().y;

	Vector2 ppa = Vector2(db->get_size().x / size.x, db->get_size().y / size.y);

	DrawingBoard* newDB = new DrawingBoard(_hub, size, _at, _radius, ppa);

	newDB->activate();
	_hub->add_screen(newDB);

	return newDB;
}

DrawingBoard::DrawingBoard(Hub* _hub, Vector2 const& _size, Rotator3 const& _at, float _radius, Vector2 const & _ppa)
: HubScreen(_hub, Name::invalid(), _size, _at, _radius, NP, NP, _ppa)
{
#ifdef WITH_DRAWING_BOARD
	if (auto* game = TeaForGodEmperor::Game::get_as<TeaForGodEmperor::Game>())
	{
		drawingBoard = game->get_drawing_board_rt();
	}
#endif

	{
		Range2 at = mainResolutionInPixels;
		RefCountObjectPtr<HubScreen> keepThis(this);
		auto* w = new HubWidgets::CustomDraw(Name::invalid(), at, [keepThis, this](Framework::Display* _display, Range2 const& _at)
			{
				if (auto* db = drawingBoard.get())
				{
					db->render(0, ::System::Video3D::get(), _at.bottom_left(), _at.length());
				}
			});

		add_widget(w);
	}

	{
		Range2 at = mainResolutionInPixels;
		at.x.min = at.x.max - 20.0f;
		at.y.min = at.y.max - 20.0f;
		RefCountObjectPtr<HubScreen> keepThis(this);
		auto* w = new HubWidgets::Button(Name::invalid(), at, String(TXT("X")));
		w->on_click = [keepThis, this](HubWidget* _widget, Vector2 const& _at, HubInputFlags::Flags _flags) { clearDrawingBoard = true; };
		add_widget(w);
	}
}

void DrawingBoard::advance(float _deltaTime, bool _beyondModal)
{
	base::advance(_deltaTime, _beyondModal);
}

void DrawingBoard::process_input(int _handIdx, Framework::GameInput const& _input, float _deltaTime)
{
	bool pressed = _input.is_button_pressed(NAME(select));
	bool shifted = _input.is_button_pressed(NAME(shift));

	if (!pressed && ! shifted)
	{
		penAt[_handIdx].clear();
		penWasAt[_handIdx].clear();
	}
	else
	{
		Vector2 nowAt = hub->get_hand((Hand::Type)_handIdx).onScreenAt;
		nowAt -= mainResolutionInPixels.bottom_left();
		nowAt /= mainResolutionInPixels.length();
		penAt[_handIdx] = nowAt;
		penShifted[_handIdx] = shifted;
	}
}

void DrawingBoard::render_display_and_ready_to_render(bool _lazyRender)
{
	if (auto* db = drawingBoard.get())
	{
		::System::Video3D* v3d = ::System::Video3D::get();

		{
			v3d->set_viewport(VectorInt2::zero, db->get_size());
			v3d->set_near_far_plane(0.02f, 100.0f);

			v3d->set_2d_projection_matrix_ortho();
			v3d->access_model_view_matrix_stack().clear();

			v3d->setup_for_2d_display();

			db->bind();
		}

		if (clearDrawingBoard)
		{
			v3d->clear_colour(Colour::black);
			clearDrawingBoard = false;

#ifdef WITH_DRAWING_BOARD
			if (auto* game = TeaForGodEmperor::Game::get_as<TeaForGodEmperor::Game>())
			{
				game->drop_drawing_board_pixels();
			}
#endif
		}
		else
		{
			Vector2 dbSize = db->get_size().to_vector2();
			for_count(int, i, Hand::MAX)
			{
				auto& pAt = penAt[i];
				auto& pWasAt = penWasAt[i];
				if (pAt.is_set())
				{
					if (pWasAt.is_set())
					{
						if (!penShifted[i])
						{
							::System::Video3DPrimitives::line_2d(Colour::white, pWasAt.get() * dbSize, pAt.get() * dbSize);
						}
						else
						{
							::System::Video3DPrimitives::fill_circle_2d(Colour::black, pAt.get() * dbSize, dbSize.length() * 0.02f);
						}
#ifdef WITH_DRAWING_BOARD
						if (auto* game = TeaForGodEmperor::Game::get_as<TeaForGodEmperor::Game>())
						{
							game->store_drawing_board_pixels();
						}
#endif
					}
					pWasAt = pAt;
				}
			}
		}
		{
			db->bind_none();
			db->resolve();
		}
	}

	base::render_display_and_ready_to_render(_lazyRender);
}
