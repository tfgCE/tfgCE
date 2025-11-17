#include "textureUtils.h"

#include "texture.h"

//

#ifdef AN_ALLOW_OPTIMIZE_OFF
#pragma optimize("", off)
#endif

//

using namespace System;

//

bool TextureUtils::AddBorder::load_from_xml(IO::XML::Node const* _node)
{
	bool result = true;

	addBorder = _node->get_bool_attribute_or_from_child_presence(TXT("addBorder"), false) ? 1.0f : 0.0f;
	addBorder = _node->get_float_attribute_or_from_child(TXT("addBorder"), addBorder);
	smoothBorder = _node->get_bool_attribute_or_from_child_presence(TXT("smoothBorder"), false) ? 1.0f : 0.0f;
	smoothBorder = _node->get_float_attribute_or_from_child(TXT("smoothBorder"), smoothBorder);
	fullBorderSourceThreshold = _node->get_float_attribute_or_from_child(TXT("fullBorderSourceThreshold"), fullBorderSourceThreshold);
	borderColour.load_from_xml_child_or_attr(_node, TXT("borderColour"));

	return result;
}

void TextureUtils::AddBorder::process(REF_ TextureSetup& _setup)
{
	if (addBorder <= 0.0f)
	{
		return;
	}

	for_count(int, levelIdx, _setup.pixels.get_size())
	{
		VectorInt2 size = _setup.levelSize[levelIdx];

		Array<Colour> pixels;
		// read original, so we can setup border from it
		{
			pixels.set_size(size.x * size.y);

			for_count(int, y, size.y)
			{
				for_count(int, x, size.x)
				{
					pixels[y * size.x + x] = _setup.get_pixel(levelIdx, VectorInt2(x, y));
				}
			}
		}

		// clear to border colour
		{
			Colour bcZeroAlpha = borderColour.with_alpha(0.0f);
			for_count(int, y, size.y)
			{
				for_count(int, x, size.x)
				{
					_setup.draw_pixel(levelIdx, VectorInt2(x, y), bcZeroAlpha);
				}
			}
		}

		// draw border only
		{
			int addBorderMax = TypeConversions::Normal::f_i_closest(ceil(addBorder));
			for (int yo = -addBorderMax; yo <= addBorderMax; ++yo)
			{
				for (int xo = -addBorderMax; xo <= addBorderMax; ++xo)
				{
					float dist = VectorInt2(xo, yo).to_vector2().length();
					if (dist <= addBorder)
					{
						float applyBorder = 1.0f;
						if (smoothBorder > 0.0f)
						{
							applyBorder = lerp(smoothBorder, 1.0f, clamp(1.0f / dist, 0.0f, 1.0f));
						}
						for_count(int, y, size.y)
						{
							for_count(int, x, size.x)
							{
								VectorInt2 from(x + xo, y + yo);
								if (_setup.is_pos_valid(levelIdx, from))
								{
									from = _setup.validate_pos(levelIdx, from);
									float p = pixels[from.y * size.x + from.x].a;
									if (fullBorderSourceThreshold > 0.0f)
									{
										p = min(p / fullBorderSourceThreshold, 1.0f);
									}
									p *= applyBorder;
									VectorInt2 at(x, y);
									_setup.draw_pixel(levelIdx, at, borderColour.with_alpha(max(p, _setup.get_pixel(levelIdx, at).a)));
								}
							}
						}
					}
				}
			}
		}

		// add actual image on top
		{
			for_count(int, y, size.y)
			{
				for_count(int, x, size.x)
				{
					Colour p = pixels[y * size.x + x];
					VectorInt2 at(x, y);
					_setup.draw_pixel(levelIdx, at, Colour::lerp(p.a, _setup.get_pixel(levelIdx, at), p.with_alpha(1.0f)));
				}
			}
		}
	}
}
