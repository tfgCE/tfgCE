#include "oie_text.h"

#include "..\overlayInfoSystem.h"

#include "..\..\game\game.h"

#include "..\..\..\core\system\core.h"
#include "..\..\..\core\system\video\video3dPrimitives.h"
#include "..\..\..\framework\video\font.h"

//

#ifdef AN_ALLOW_OPTIMIZE_OFF
#pragma optimize("", off)
#endif

//

using namespace TeaForGodEmperor;
using namespace OverlayInfo;
using namespace Elements;

//

// input
DEFINE_STATIC_NAME(scrollFaster);
DEFINE_STATIC_NAME(scrollStick);

//

Optional<Colour> OverlayInfo::TextColours::process_line(REF_ String& _line) const
{
	Optional<Colour> result;
	if (_line.get_length() > 2 && _line.get_data()[0] == '#')
	{
		int closing = _line.find_first_of('#', 1);
		if (closing != NONE)
		{
			String id = _line.get_sub(1, closing - 1);
			for_every(c, colours)
			{
				if (c->id == id.to_char())
				{
					result = c->colour;
				}
			}
			_line = _line.get_sub(closing + 1);
		}
	}

	return result;
}

void OverlayInfo::TextColours::alter_colour(REF_ Colour& _colour, Optional<Colour> const& _withColour)
{
	if (_withColour.is_set())
	{
		_colour = Colour::lerp(_withColour.get().a, _colour, _withColour.get()).with_set_alpha(_colour.a);
	}
}

//

REGISTER_FOR_FAST_CAST(Text);

void Text::update_text()
{
	if (requiresTextUpdate)
	{
		requiresTextUpdate = false;
		rawLines.clear();

		text.split(String(TXT("~")), rawLines);

		lines.clear();
		for_every(rl, rawLines)
		{
			lines.push_back(Line());
			auto& newLine = lines.get_last();
			newLine.line = *rl;
			for_every(c, colours.colours)
			{
				if (!c->id.is_valid())
				{
					newLine.colour = c->colour;
				}
			}
			auto processedColour = colours.process_line(REF_ newLine.line);
			if (processedColour.is_set())
			{
				newLine.colour = processedColour;
			}
		}

		if (maxLineWidth > 0)
		{
			for (int i = 0; i < lines.get_size(); ++i)
			{
				if (lines[i].line.get_length() > maxLineWidth)
				{
					int cutAt = lines[i].line.find_last_of(' ', maxLineWidth - 1);
					bool removeSpace = cutAt != NONE;
					if (cutAt == NONE)
					{
						cutAt = maxLineWidth;
					}
					String left = lines[i].line.get_sub(removeSpace ? cutAt + 1 : cutAt);
					lines[i].line.keep_left_inline(cutAt);
					if (!left.is_empty())
					{
						Line newLine;
						newLine.colour = lines[i].colour;
						newLine.line = left;
						lines.insert_after(lines.iterator_for(i), newLine);
					}
				}
			}
		}
	}
}

void Text::advance(OverlayInfo::System const* _system, float _deltaTime)
{
	base::advance(_system, _deltaTime);

	update_text();

	if (scrollDown.active)
	{
		Optional<float> scrollStick;
		if (auto* g = Game::get_as<Game>())
		{
			auto& input = g->access_all_time_controls_input();

			if (input.has_button_been_pressed(NAME(scrollFaster)))
			{
				scrollDown.scrollFaster = true;
			}
			if (input.has_button_been_released(NAME(scrollFaster)))
			{
				scrollDown.scrollFaster = false;
			}
			Vector2 readScrollStick = input.get_stick(NAME(scrollStick));
			if (readScrollStick.length() > 0.1f)
			{
				scrollStick = readScrollStick.y;
			}
		}

		float scrollDeltaTime = _deltaTime;
		if (scrollDown.scrollFaster)
		{
			scrollDeltaTime *= 10.0f;
		}

		if (scrollStick.is_set())
		{
			scrollDown.initialWait = 0.0f;
			scrollDown.atLine += scrollDown.speed * 4.0f * scrollDeltaTime * (-scrollStick.get());
		}

		if (scrollDown.initialWait > 0.0f)
		{
			scrollDown.initialWait -= scrollDeltaTime;
		}
		else if (!scrollStick.is_set())
		{
			scrollDown.atLine += scrollDown.speed * scrollDeltaTime;
		}
		if (!scrollDown.showLines.is_empty())
		{
			scrollDown.atLine = max(scrollDown.atLine, (float)scrollDown.showLines.max);
			float atEnd = (float)lines.get_size() + scrollDown.showLines.min;
			scrollDown.atLine = max(0.0f, min(scrollDown.atLine, atEnd));
			if (scrollDown.atLine >= atEnd)
			{
				if (scrollDown.postWait > 0.0f)
				{
					scrollDown.postWait -= scrollDeltaTime;
					if (scrollDown.postWait <= 0.0f)
					{
						deactivate();
					}
				}
			}
		}
	}
}

void Text::render(OverlayInfo::System const* _system, int _renderLayer) const
{
	if (_renderLayer != renderLayer)
	{
		return;
	}
	auto* useFont = font;
	if (!useFont)
	{
		useFont = _system->get_default_font();
	}
	if (useFont)
	{
		if (auto* f = useFont->get_font())
		{
			if (!charSize.is_set() && ! charOffset.is_zero())
			{
				charSize = f->calculate_char_size();
			}
			auto* v3d = ::System::Video3D::get();
			auto& mvs = v3d->access_model_view_matrix_stack();
			auto placement = get_placement();
			mvs.push_to_world(placement.to_matrix());
			Vector3 relAt = mvs.get_current().get_translation();
			float useScale = 1.0f;
			float useAdditionalScaleNow;
			if (letterSize.is_set())
			{
				useScale = letterSize.get() / useFont->get_height();
				useAdditionalScaleNow = useAdditionalScale.get(0.0f);
			}
			else
			{
				useScale = 0.05f / useFont->get_height();
				useAdditionalScaleNow = useAdditionalScale.get(1.0f);
			}
			useScale *= calculate_additional_scale(relAt.length(), useAdditionalScaleNow);
			useScale *= scale;
			mvs.push_to_world(Transform((useScale * charSize.get(Vector2::zero) * charOffset).to_vector3_xz(), Quat::identity).to_matrix());
			float active = get_faded_active();
			//
			float activeDueToCamera = 1.0f;
			{
				Transform cameraPlacement = _system->get_camera_placement();
				float alongDir = abs(Vector3::dot(placement.get_axis(Axis::Y), placement.get_translation() - cameraPlacement.get_translation()));
				float maxDist = min(0.3f, letterSize.get(0.05f) * 20.0f);
				activeDueToCamera *= clamp(alongDir / maxDist, 0.0f, 1.0f);
			}
			Colour useColour = colour.with_alpha(active * activeDueToCamera).mul_rgb(pulse);
			_system->apply_fade_out_due_to_tips(this, REF_ useColour); 
			::System::FontDrawingContext fdc;
			fdc.blend = forceBlend || useColour.a < 0.99f;
			fdc.smallerDecimals = smallerDecimals;
			if (backgroundColour.is_set() && ! scrollDown.active)
			{
				int lineIdx = 0;
				float fontHeight = f->get_height();
				ARRAY_STACK(Range2, textCoversLines, lines.get_size());
				for_every(line, lines)
				{
					Range2 textCovers = Range2::empty;
					if (!line->line.is_empty())
					{
						Vector2 pt = Vector2::half;
						pt.x = horizontalAlign;
						/*	this is the explanation what's going on
						{
							// [lines, va, idx]
							// [1, 0.0, 0] ->  0.0
							// [1, 0.5, 0] ->  0.5
							// [1, 1.0, 0] ->  1.0
							// [2, 0.0, 0] -> -1.0
							// [2, 0.0, 1] ->  0.0
							// [2, 0.5, 0] ->  0.0
							// [2, 0.5, 1] ->  1.0
							// [2, 1.0, 0] ->  1.0
							// [2, 1.0, 1] ->  2.0
							// [3, 0.0, 0] -> -2.0
							// [3, 0.0, 2] ->  0.0
							// [3, 0.5, 0] -> -0.5
							// [3, 0.5, 2] ->  1.5
							// [3, 1.0, 0] ->  1.0
							// [3, 1.0, 2] ->  3.0

							float wholeThingPTSize = (float)(lines.get_size())
												   + (((float)(lines.get_size() - 1)) * lineSpacing);

							// at vertical align 1.0, first line is always on top
							// at vertical align 0.0, last line is always 0.0
							float firstLineVA1 = 1.0f;
							float lastLineVA0 = 0.0f;

							float lineAdvance = 1.0f + lineSpacing;
							float currLineVA1 = firstLineVA1 + (float)lineIdx * lineAdvance;
							float currLineVA0 = lastLineVA0  - (float)(lines.get_size() - 1 - lineIdx) * lineAdvance;

							float currLine = lerp(verticalAlign, currLineVA0, currLineVA1);
							
							pt.y = currLine;
						}
						*/
						{
							float lineAdvance = 1.0f + lineSpacing;
							float currLineVA1 = 1.0f + (float)lineIdx * lineAdvance;
							float currLineVA0 = 0.0f - (float)(lines.get_size() - 1 - lineIdx) * lineAdvance;

							pt.y = lerp(verticalAlign, currLineVA0, currLineVA1);
						}
						Vector2 lineSize = f->calculate_text_size(line->line.to_char(), Vector2::one) * useScale;

						textCovers.x.include(-lineSize.x * pt.x);
						textCovers.x.include(lineSize.x * (1.0f - pt.x));
						textCovers.y.include(-lineSize.y * pt.y);
						textCovers.y.include(lineSize.y * (1.0f - pt.y));
					}
					++lineIdx;
					if (!textCovers.is_empty())
					{
						textCovers.expand_by(Vector2(0.5f, 0.1f) * (fontHeight * useScale));

						textCoversLines.push_back(textCovers);
					}
				}

				{
					Range2* tc = textCoversLines.begin();
					Range2* tn = tc;
					++tn;
					float minDist = fontHeight * useScale * 0.025f;
					float halfMinDist = minDist * 0.5f;
					for_count(int, i, textCoversLines.get_size() - 1)
					{
						if (tc->y.min < tn->y.max + minDist)
						{
							float y = (tc->y.min + tn->y.max) * 0.5f;
							tc->y.min = y + halfMinDist;
							tc->y.max = y - halfMinDist;
						}
						++tc;
						++tn;
					}
				}
				for_every(textCovers, textCoversLines)
				{
					::System::Video3DPrimitives::fill_rect_xz(backgroundColour.get().with_alpha(active* activeDueToCamera), *textCovers, true);
				}
			}
			int lineIdx = 0;
			for_every(line, lines)
			{
				Vector2 pt = Vector2::half;
				pt.x = horizontalAlign;
				float lineAdvance = 1.0f + lineSpacing;
				{
					float currLineVA1 = 1.0f + (float)lineIdx * lineAdvance;
					float currLineVA0 = 0.0f - (float)(lines.get_size() - 1 - lineIdx) * lineAdvance;

					pt.y = lerp(verticalAlign, currLineVA0, currLineVA1);
				}
				float alpha = 1.0f;
				if (scrollDown.active)
				{
					pt.y -= scrollDown.atLine * lineAdvance;
					if (!scrollDown.showLines.is_empty())
					{
						float minL = scrollDown.showLines.min;
						float maxL = scrollDown.showLines.max + 1.0f;
						minL *= lineAdvance;
						maxL *= lineAdvance;
						float lineAt = -pt.y;
						if (lineAt > maxL)
						{
							alpha = max(0.0f, 1.0f - (lineAt - maxL));
						}
						else if (lineAt < minL)
						{
							alpha = max(0.0f, 1.0f - (minL - lineAt));
						}
					}
				}
				if (alpha > 0.0f)
				{
					Colour lineColour = useColour;
					Optional<Colour> useLinesColour = line->colour;
					if (useLinesColour.is_set())
					{
						_system->apply_fade_out_due_to_tips(this, REF_ useLinesColour.access());
					}
					TextColours::alter_colour(REF_ lineColour, useLinesColour);
					lineColour.a *= alpha;
					fdc.blend = forceBlend || lineColour.a < 0.99f;
					f->draw_text_at(v3d, line->line.to_char(), lineColour, Vector3::zero, Vector2::one * useScale, pt, false, fdc);
				}
				++lineIdx;
			}
			mvs.pop();
			mvs.pop();
		}
	}
}
