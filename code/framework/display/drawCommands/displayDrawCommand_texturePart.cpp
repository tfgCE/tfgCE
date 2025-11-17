#include "displayDrawCommand_texturePart.h"

#include "..\display.h"

#include "..\..\pipelines\displayPipeline.h"

#include "..\..\video\texturePart.h"

#include "..\..\..\core\system\video\video3dPrimitives.h"

using namespace Framework;
using namespace DisplayDrawCommands;

DisplayDrawCommands::TexturePart::TexturePart()
{
	use_coordinates(DisplayCoordinates::Pixel);
}

bool DisplayDrawCommands::TexturePart::draw_onto(Display* _display, ::System::Video3D * _v3d, REF_ int & _drawCyclesUsed, REF_ int & _drawCyclesAvailable, REF_ DisplayDrawContext & _context) const
{
	bool result = true;
	if (texturePart)
	{
		int blockSize = 100;
		int cyclesRequiredPerBlock = 5;
		int cyclesRequired = max(1, (int)(texturePart->get_size().x * texturePart->get_size().y) * cyclesRequiredPerBlock / blockSize);
		float invCyclesRequiredFloat = 1.0f / (float)cyclesRequired;

		int willUseNow = min(_drawCyclesAvailable, cyclesRequired - _drawCyclesUsed);

		if (atParam.is_set())
		{
			TexturePartDrawingContext drawingContext;
			drawingContext.shaderProgramInstance = DisplayPipeline::get_setup_shader_program_instance(texturePart->get_texture()->get_texture()->get_texture_id(), get_use_colourise(_display), get_use_colourise_ink(_display), get_use_colourise_paper(_display));
			drawingContext.alignToPixels = true;
			drawingContext.verticalRange.max = min(1.0f, 1.0f - (float)_drawCyclesUsed * invCyclesRequiredFloat + 0.05f); // cover a bit more back to make sure we won't skip a line
			drawingContext.verticalRange.min = 1.0f - (float)(_drawCyclesUsed + willUseNow) * invCyclesRequiredFloat;
			drawingContext.limits = get_use_limits();
			Vector2 multiply = get_use_coordinates(_display) == DisplayCoordinates::Char ? _display->get_char_size() : Vector2::one;
			texturePart->draw_at(_v3d, atParam.get().to_vector2() * multiply + _display->get_left_bottom_of_screen() + get_offset() * multiply, drawingContext);
		}

		_drawCyclesUsed += willUseNow;
		_drawCyclesAvailable -= willUseNow;

		result = willUseNow <= _drawCyclesAvailable;
	}
	return result;
}

void DisplayDrawCommands::TexturePart::prepare(Display* _display)
{
	base::prepare(_display);
}
