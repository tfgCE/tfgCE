#include "gameSceneLayer_uiCanvas.h"

#include "..\..\..\core\system\video\renderTarget.h"

#include "..\game.h"
#include "..\..\library\library.h"
#include "..\..\library\usedLibraryStored.inl"
#include "..\..\ui\uiCanvasInputContext.h"

//

#ifdef AN_ALLOW_OPTIMIZE_OFF
#pragma optimize("", off)
#endif

//

using namespace Framework;
using namespace GameSceneLayers;

//

REGISTER_FOR_FAST_CAST(UICanvas);

UICanvas::UICanvas()
{
	canvas = new UI::Canvas();
	canvas->set_logical_size(canvasLogicalSize);
}

UICanvas::~UICanvas()
{
	delete canvas;
}

void UICanvas::advance(Framework::GameAdvanceContext const& _advanceContext)
{
	if (!backgroundColour.is_set())
	{
		base::advance(_advanceContext);
	}
}

void UICanvas::process_controls(Framework::GameAdvanceContext const& _advanceContext)
{
	if (!backgroundColour.is_set())
	{
		base::process_controls(_advanceContext);
	}
}

void UICanvas::process_all_time_controls(Framework::GameAdvanceContext const& _advanceContext)
{
	if (!backgroundColour.is_set())
	{
		base::process_all_time_controls(_advanceContext);
	}

	Concurrency::ScopedSpinLock lock(canvas->access_lock());

	UI::CanvasInputContext cic;
	cic.set_cursor(UI::CursorIndex::Mouse, UI::CanvasInputContext::Cursor::use_mouse(canvas));

	canvas->update(cic);
}

void UICanvas::process_vr_controls(Framework::GameAdvanceContext const& _advanceContext)
{
	if (!backgroundColour.is_set())
	{
		base::process_vr_controls(_advanceContext);
	}
}

void UICanvas::prepare_to_sound(Framework::GameAdvanceContext const& _advanceContext)
{
	if (!backgroundColour.is_set())
	{
		base::prepare_to_sound(_advanceContext);
	}
}

void UICanvas::prepare_to_render(Framework::CustomRenderContext* _customRC)
{
	if (!backgroundColour.is_set())
	{
		base::prepare_to_render(_customRC);
	}
}

void UICanvas::pre_advance(Framework::GameAdvanceContext const& _advanceContext)
{
	if (!backgroundColour.is_set())
	{
		base::pre_advance(_advanceContext);
	}
}

void UICanvas::render_on_render(Framework::CustomRenderContext* _customRC)
{
	if (!backgroundColour.is_set())
	{
		base::render_on_render(_customRC);
	}
}

void UICanvas::render_after_post_process(Framework::CustomRenderContext* _customRC)
{
	if (!backgroundColour.is_set())
	{
		base::render_after_post_process(_customRC);
	}	

	Concurrency::ScopedSpinLock lock(canvas->access_lock());

	System::Video3D* v3d = System::Video3D::get();
	System::RenderTarget* rtTarget = _customRC ? _customRC->sceneRenderTarget.get() : game->get_main_render_output_render_buffer();

	::System::ForRenderTargetOrNone forRT(rtTarget);
	VectorInt2 rtTargetSize = forRT.get_size();

	canvas->set_real_size(rtTargetSize.to_vector2());
	{
		Vector2 useLogicalSize = canvasLogicalSize;
		float aspectRatio = aspect_ratio(rtTargetSize.to_vector2());
		useLogicalSize.x = useLogicalSize.y * aspectRatio;
		canvas->set_logical_size(useLogicalSize);
	}

	canvas->render_on(v3d, forRT.rt, backgroundColour);
}
