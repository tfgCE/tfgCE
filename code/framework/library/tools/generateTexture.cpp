#include "libraryTools.h"

#include "..\..\..\core\mainSettings.h"
#include "..\..\..\core\io\xml.h"
#include "..\..\..\core\random\random.h"
#include "..\..\..\core\types\colour.h"
#include "..\..\..\core\serialisation\serialiser.h"

#include "..\..\video\texture.h"

//

#ifdef AN_ALLOW_OPTIMIZE_OFF
#pragma optimize("", off)
#endif

//

using namespace ::Framework;

//

namespace Noise
{
	enum Type
	{
		Normal,
		Triangular,
	};

	// based on: https://www.shadertoy.com/view/4t2SDh

	float get_normal(REF_ Random::Generator & randomGenerator)
	{
		return mod(sin_deg(randomGenerator.get_float(-90.0f, 90.0f)* 43758.5453f), 1.0f);
	}

	float get_triangular(REF_ Random::Generator & randomGenerator)
	{
		return (get_normal(REF_ randomGenerator) + get_normal(REF_ randomGenerator)) / 2.0f;
	}

};

struct GenerateTexture
{
	String fileName;
	::System::TextureSetup textureSetup;

	bool load_from_xml(IO::XML::Node const * _node)
	{
		fileName = _node->get_string_attribute(TXT("file"));
		if (fileName.is_empty())
		{
			error(TXT("no name provided for library tool \"generate_texture\""));
			return false;
		}
		if (!textureSetup.load_from_xml(_node))
		{
			return false;
		}
		return true;
	}
};

static bool load_generate_texture_function(REF_ Array<GenerateTexture> & _generateTextures, int _level, IO::XML::Node const * dataNode, bool _scaleToLevel)
{
	if (dataNode->get_name() == TXT("texture"))
	{
		// just info about texture
		return true;
	}
	
	int idx = dataNode->get_int_attribute(TXT("textureIdx"), 0);
	auto & textureSetup = _generateTextures[idx].textureSetup;
	if (dataNode->get_name() == TXT("noise"))
	{
		int seedBase = 23595647;
		int seedOffset = 230480752;
		seedBase = dataNode->get_int_attribute(TXT("seedBase"), seedBase);
		seedOffset = dataNode->get_int_attribute(TXT("seedOffset"), seedOffset);
		Random::Generator randomGenerator(seedBase, seedOffset);
		Noise::Type noise = Noise::Normal;
		if (dataNode->get_string_attribute(TXT("type")) == TXT("triangular")) noise = Noise::Triangular;
		bool normalise = dataNode->get_bool_attribute(TXT("normalise"), false);
		bool mono = dataNode->get_bool_attribute(TXT("mono"), false);
		for_count(int, x, textureSetup.levelSize[_level].x)
		{
			for_count(int, y, textureSetup.levelSize[_level].y)
			{
				Colour colour;
				if (noise == Noise::Triangular)
				{
					colour.r = Noise::get_triangular(REF_ randomGenerator);
					if (!mono)
					{
						colour.g = Noise::get_triangular(REF_ randomGenerator);
						colour.b = Noise::get_triangular(REF_ randomGenerator);
					}
					else
					{
						colour.b = colour.g = colour.r;
					}
				}
				else
				{
					colour.r = randomGenerator.get_float(0.0f, 1.0f);
					if (! mono)
					{
						colour.g = randomGenerator.get_float(0.0f, 1.0f);
						colour.b = randomGenerator.get_float(0.0f, 1.0f);
					}
					else
					{
						colour.b = colour.g = colour.r;
					}
				}
				colour.a = 1.0f;
				if (normalise && ! mono)
				{
					colour = Colour(colour.to_vector3().normal(), colour.a);
				}
				textureSetup.draw_pixel(_level, VectorInt2(x, y), colour);
			}
		}
		return true;
	}
	else if (dataNode->get_name() == TXT("verticalLines"))
	{
		int mask = 0xffff;
		if (auto const * attr = dataNode->get_attribute(TXT("mask")))
		{
			mask = 0;
			if (String::does_contain(attr->get_as_string().to_char(), TXT("r"))) mask |= 0x1;
			if (String::does_contain(attr->get_as_string().to_char(), TXT("g"))) mask |= 0x2;
			if (String::does_contain(attr->get_as_string().to_char(), TXT("b"))) mask |= 0x4;
			if (String::does_contain(attr->get_as_string().to_char(), TXT("a"))) mask |= 0x8;
		}
		struct ColourElement
		{
			float at;
			Colour colour;

			bool load_from_xml(IO::XML::Node const * _node, int _workingWith, int _mask)
			{
				at = _node->get_float_attribute(TXT("at"), 0.0f);
				bool result = true;
				if (_workingWith != 2)
				{
					bool allowToModulate = true;
					if (_node->get_attribute(TXT("r")))
					{
						result = colour.load_from_xml(_node);
						allowToModulate = false; // colour handles modulation
					}
					else
					{
						result = colour.load_from_string(_node->get_text());
						// move them in right place
						if (!(_mask & 0x1)) { colour.a = colour.b; colour.b = colour.g; colour.g = colour.r; }
						if (!(_mask & 0x2)) { colour.a = colour.b; colour.b = colour.g; }
						if (!(_mask & 0x4)) { colour.a = colour.b; }
					}
					if (_workingWith == 1)
					{
						colour.r = colour.r * 0.5f + 0.5f;
						colour.g = colour.g * 0.5f + 0.5f;
						colour.b = colour.b * 0.5f + 0.5f;
						colour.a = colour.a * 0.5f + 0.5f;
					}
					if (allowToModulate)
					{
						float modulatergb = _node->get_float_attribute(TXT("modulate"), 1.0f);
						colour = colour.mul_rgb(modulatergb);
					}
				}
				else
				{
					if (_node->get_attribute(TXT("a")))
					{
						colour.a = _node->get_float_attribute(TXT("a"));
					}
					else
					{
						colour.a = _node->get_float();
					}
					colour.r = colour.g = colour.b = 1.0f;
					float modulatea = _node->get_float_attribute(TXT("modulate"), 1.0f);
					colour.a *= modulatea;
				}
				return result;
			}
			static void process(Array<ColourElement> & elements, ::System::TextureSetup & textureSetup, int _level, int xStart, int xEnd, int workingWith, int _mask)
			{
				if (elements.get_size() == 0)
				{
					return;
				}
				ColourElement* nextToPrevElement = nullptr;
				ColourElement* nextElement = nullptr;
				ColourElement* element = elements.get_size() == 1 ? &elements[0] : &elements[1];
				ColourElement* prevElement = &elements[0];
				for_range(int, iElement, 1, elements.get_size() - 1)
				{
					nextElement = iElement < elements.get_size() - 1 ? &elements[iElement + 1] : nullptr;

					int yStart = (int)(prevElement->at * (float)(textureSetup.levelSize[_level].y - 1));
					int yEnd = (int)(element->at * (float)(textureSetup.levelSize[_level].y - 1));
					int yStep = yEnd >= yStart ? 1 : -1;
					float yWeightStep = yEnd != yStart ? 1.0f / ((float)abs(yEnd - yStart)) : 0.0f;
					float yWeight = 0.0f;
					BezierCurve<Colour> prev2element(prevElement->colour, element->colour);
					if (nextToPrevElement)
					{
						BezierCurve<Colour> prev(nextToPrevElement->colour, prevElement->colour);
						// we deal with p2->p3 and p0->p1 which is 1/3 of T, use that when calculating TDists
						float aTDist = (prevElement->at - nextToPrevElement->at) / 3.0f;
						float bTDist = (element->at - prevElement->at) / 3.0f;
						if (aTDist != 0.0f && bTDist != 0.0f && aTDist + bTDist != 0.0f)
						{
							// we have /dist and then *dist but for sake of clarity it is kept as it is
							// dir
							Colour aDir = (prev.p3 - prev.p2) / aTDist;
							Colour bDir = (prev2element.p1 - prev2element.p0) / bTDist;
							// calculate unified dir using weights
							Colour unifiedDir = (aDir * aTDist + bDir * bTDist) / (aTDist + bTDist);
							prev2element.p1 = prev2element.p0 + unifiedDir * aTDist;
						}
					}
					if (nextElement)
					{
						BezierCurve<Colour> next(element->colour, nextElement->colour);
						// we deal with p2->p3 and p0->p1 which is 1/3 of T, use that when calculating TDists
						float aTDist = (element->at - prevElement->at) / 3.0f;
						float bTDist = (nextElement->at - element->at) / 3.0f;
						if (aTDist != 0.0f && bTDist != 0.0f && aTDist + bTDist != 0.0f)
						{
							// we have /dist and then *dist but for sake of clarity it is kept as it is
							// dir
							Colour aDir = (prev2element.p3 - prev2element.p2) / aTDist;
							Colour bDir = (next.p1 - next.p0) / bTDist;
							// calculate unified dir using weights
							Colour unifiedDir = (aDir * aTDist + bDir * bTDist) / (aTDist + bTDist);
							prev2element.p2 = prev2element.p3 - unifiedDir * bTDist;
						}
					}
					for (int y = yStart; /*.*/; y += yStep, yWeight += yWeightStep)
					{
						Colour colour = prev2element.calculate_at(yWeight);
						for_range(int, x, xStart, xEnd)
						{
							Colour existingColour = textureSetup.get_pixel(_level, VectorInt2(x, y));
							if (workingWith == 2)
							{
								colour.r = existingColour.r;
								colour.g = existingColour.g;
								colour.b = existingColour.b;
							}
							if (!(_mask & 0x1)) colour.r = existingColour.r;
							if (!(_mask & 0x2)) colour.g = existingColour.g;
							if (!(_mask & 0x4)) colour.b = existingColour.b;
							if (!(_mask & 0x8)) colour.a = existingColour.a;
							textureSetup.draw_pixel(_level, VectorInt2(x, y), colour);
						}
						if (y == yEnd)
						{
							break;
						}
					}

					nextToPrevElement = prevElement;
					++element;
					++prevElement;
				}

			}
			static void process_gradient(Array<ColourElement>& elementsStart, Array<ColourElement>& elementsEnd, ::System::TextureSetup& textureSetup, int _level, int xStart, int xEnd, int workingWith, int _mask)
			{
				if (elementsStart.get_size() == 0 || elementsEnd.get_size() == 0)
				{
					return;
				}
				int yStart = 0;
				int yEnd = textureSetup.levelSize[_level].y - 1;
				for_range(int, y, yStart, yEnd)
				{
					Optional<Colour> cStart;
					Optional<Colour> cEnd;
					for_count(int, side, 2)
					{
						auto& c = side == 0 ? cStart : cEnd;

						auto& elements = side == 0 ? elementsStart : elementsEnd;

						ColourElement* nextToPrevElement = nullptr;
						ColourElement* nextElement = nullptr;
						ColourElement* element = elements.get_size() == 1 ? &elements[0] : &elements[1];
						ColourElement* prevElement = &elements[0];
						for_range(int, iElement, 1, elements.get_size() - 1)
						{
							nextElement = iElement < elements.get_size() - 1 ? &elements[iElement + 1] : nullptr;

							int yStart = (int)(prevElement->at * (float)(textureSetup.levelSize[_level].y - 1));
							int yEnd = (int)(element->at * (float)(textureSetup.levelSize[_level].y - 1));
							if (min(yStart, yEnd) <= y &&
								max(yStart, yEnd) >= y)
							{
								int yStep = yEnd >= yStart ? 1 : -1;
								float yWeightStep = yEnd != yStart ? 1.0f / ((float)abs(yEnd - yStart)) : 0.0f;
								float yWeight = 0.0f;
								BezierCurve<Colour> prev2element(prevElement->colour, element->colour);
								if (nextToPrevElement)
								{
									BezierCurve<Colour> prev(nextToPrevElement->colour, prevElement->colour);
									// we deal with p2->p3 and p0->p1 which is 1/3 of T, use that when calculating TDists
									float aTDist = (prevElement->at - nextToPrevElement->at) / 3.0f;
									float bTDist = (element->at - prevElement->at) / 3.0f;
									if (aTDist != 0.0f && bTDist != 0.0f && aTDist + bTDist != 0.0f)
									{
										// we have /dist and then *dist but for sake of clarity it is kept as it is
										// dir
										Colour aDir = (prev.p3 - prev.p2) / aTDist;
										Colour bDir = (prev2element.p1 - prev2element.p0) / bTDist;
										// calculate unified dir using weights
										Colour unifiedDir = (aDir * aTDist + bDir * bTDist) / (aTDist + bTDist);
										prev2element.p1 = prev2element.p0 + unifiedDir * aTDist;
									}
								}
								if (nextElement)
								{
									BezierCurve<Colour> next(element->colour, nextElement->colour);
									// we deal with p2->p3 and p0->p1 which is 1/3 of T, use that when calculating TDists
									float aTDist = (element->at - prevElement->at) / 3.0f;
									float bTDist = (nextElement->at - element->at) / 3.0f;
									if (aTDist != 0.0f && bTDist != 0.0f && aTDist + bTDist != 0.0f)
									{
										// we have /dist and then *dist but for sake of clarity it is kept as it is
										// dir
										Colour aDir = (prev2element.p3 - prev2element.p2) / aTDist;
										Colour bDir = (next.p1 - next.p0) / bTDist;
										// calculate unified dir using weights
										Colour unifiedDir = (aDir * aTDist + bDir * bTDist) / (aTDist + bTDist);
										prev2element.p2 = prev2element.p3 - unifiedDir * bTDist;
									}
								}
								int forY = y;
								for (int y = yStart; /*.*/; y += yStep, yWeight += yWeightStep)
								{
									Colour colour = prev2element.calculate_at(yWeight);
									if (y == forY)
									{
										c = colour;
										break;
									}
									if (y == yEnd)
									{
										break;
									}
								}
							}

							nextToPrevElement = prevElement;
							++element;
							++prevElement;

							if (c.is_set())
							{
								break;
							}
						}
					}
					if (cStart.is_set() && cEnd.is_set())
					{
						int xStep = xEnd >= xStart ? 1 : -1;
						float xWeightStep = xEnd != xStart ? 1.0f / ((float)abs(xEnd - xStart)) : 0.0f;
						float xWeight = 0.0f;
						for (int x = xStart; /*.*/; x += xStep, xWeight += xWeightStep)
						{
							Colour colour = Colour::lerp(xWeight, cStart.get(), cEnd.get());
							Colour existingColour = textureSetup.get_pixel(_level, VectorInt2(x, y));
							if (workingWith == 2)
							{
								colour.r = existingColour.r;
								colour.g = existingColour.g;
								colour.b = existingColour.b;
							}
							if (!(_mask & 0x1)) colour.r = existingColour.r;
							if (!(_mask & 0x2)) colour.g = existingColour.g;
							if (!(_mask & 0x4)) colour.b = existingColour.b;
							if (!(_mask & 0x8)) colour.a = existingColour.a;
							textureSetup.draw_pixel(_level, VectorInt2(x, y), colour);
							if (x == xEnd)
							{
								break;
							}
						}
					}
				}
			}
		};

		int lineIdx = 0;
		for_every(verticalLineNode, dataNode->children_named(TXT("line")))
		{
			int width = verticalLineNode->get_int_attribute(TXT("width"), 1);
			lineIdx = verticalLineNode->get_int_attribute(TXT("u"), lineIdx);
	
			int xStart = lineIdx;
			int xEnd = lineIdx + width - 1;
			if (_scaleToLevel)
			{
				xStart = xStart * textureSetup.levelSize[_level].x / textureSetup.size.x;
				xEnd = max(xStart, (xEnd + 1) * textureSetup.levelSize[_level].x / textureSetup.size.x - 1);
			}
			xStart = clamp(xStart, 0, textureSetup.levelSize[_level].x - 1);
			xEnd = clamp(xEnd, 0, textureSetup.levelSize[_level].x - 1);

			Array<ColourElement> elements;
			bool firstElement = true;
			int workingWith = 0; // 0 colour, 1 normal, 2 alpha
			for_every(node, verticalLineNode->all_children())
			{
				if (node->get_name() == TXT("mask"))
				{
					if (!firstElement)
					{
						ColourElement::process(elements, textureSetup, _level, xStart, xEnd, workingWith, mask);
						elements.clear();
					}
					mask = 0;
					String text = node->get_text();
					if (String::does_contain(text.to_char(), TXT("r"))) mask |= 0x1;
					if (String::does_contain(text.to_char(), TXT("g"))) mask |= 0x2;
					if (String::does_contain(text.to_char(), TXT("b"))) mask |= 0x4;
					if (String::does_contain(text.to_char(), TXT("a"))) mask |= 0x8;
					firstElement = false;
				}
				if (node->get_name() == TXT("colour") ||
					node->get_name() == TXT("normal") ||
					node->get_name() == TXT("alpha"))
				{
					int nowWorkingWith = 0;
					if (node->get_name() == TXT("normal")) nowWorkingWith = 1;
					if (node->get_name() == TXT("alpha")) nowWorkingWith = 2;
					if (!firstElement)
					{
						if (nowWorkingWith != workingWith)
						{
							ColourElement::process(elements, textureSetup, _level, xStart, xEnd, workingWith, mask);
							elements.clear();
						}
					}
					workingWith = nowWorkingWith;
					ColourElement element;
					element.load_from_xml(node, workingWith, mask);
					elements.push_back(element);
					firstElement = false;
				}
				if (node->get_name() == TXT("break"))
				{
					ColourElement::process(elements, textureSetup, _level, xStart, xEnd, workingWith, mask);
					elements.clear();
					firstElement = true;
				}
			}
			ColourElement::process(elements, textureSetup, _level, xStart, xEnd, workingWith, mask);
			elements.clear();
			firstElement = false;
			lineIdx += width;
		}
		for_every(verticalLineNode, dataNode->children_named(TXT("gradient")))
		{
			int width = verticalLineNode->get_int_attribute(TXT("width"), 1);
			lineIdx = verticalLineNode->get_int_attribute(TXT("u"), lineIdx);
	
			int xStart = lineIdx;
			int xEnd = lineIdx + width - 1;
			if (_scaleToLevel)
			{
				xStart = xStart * textureSetup.levelSize[_level].x / textureSetup.size.x;
				xEnd = max(xStart, (xEnd + 1) * textureSetup.levelSize[_level].x / textureSetup.size.x - 1);
			}
			xStart = clamp(xStart, 0, textureSetup.levelSize[_level].x - 1);
			xEnd = clamp(xEnd, 0, textureSetup.levelSize[_level].x - 1);

			Array<ColourElement> leftElements;
			Array<ColourElement> rightElements;
			int workingWith = 0; // 0 colour, 1 normal, 2 alpha
			for_count(int, elIdx, 2)
			{
				if (auto* gradientElementNode = verticalLineNode->first_child_named(elIdx == 0 ? TXT("left") : TXT("right")))
				{
					auto& elements = elIdx == 0 ? leftElements : rightElements;
					bool firstElement = true;
					for_every(node, gradientElementNode->all_children())
					{
						if (node->get_name() == TXT("mask"))
						{
							if (!firstElement)
							{
								error_loading_xml(node, TXT("can't mix, use separate pass"));
								return false;
							}
							mask = 0;
							String text = node->get_text();
							if (String::does_contain(text.to_char(), TXT("r"))) mask |= 0x1;
							if (String::does_contain(text.to_char(), TXT("g"))) mask |= 0x2;
							if (String::does_contain(text.to_char(), TXT("b"))) mask |= 0x4;
							if (String::does_contain(text.to_char(), TXT("a"))) mask |= 0x8;
							firstElement = false;
						}
						if (node->get_name() == TXT("colour") ||
							node->get_name() == TXT("normal") ||
							node->get_name() == TXT("alpha"))
						{
							int nowWorkingWith = 0;
							if (node->get_name() == TXT("normal")) nowWorkingWith = 1;
							if (node->get_name() == TXT("alpha")) nowWorkingWith = 2;
							if (!firstElement)
							{
								if (nowWorkingWith != workingWith)
								{
									error_loading_xml(node, TXT("can't mix, use separate pass"));
									return false;
								}
							}
							workingWith = nowWorkingWith;
							ColourElement element;
							element.load_from_xml(node, workingWith, mask);
							elements.push_back(element);
							firstElement = false;
						}
						if (node->get_name() == TXT("break"))
						{
							if (!firstElement)
							{
								error_loading_xml(node, TXT("can't break, use separate pass"));
								return false;
							}
						}
					}
				}
				else
				{
					error_loading_xml(verticalLineNode, TXT("gradient incomplete setup"));
					return false;
				}
			}
			ColourElement::process_gradient(leftElements, rightElements, textureSetup, _level, xStart, xEnd, workingWith, mask);
			lineIdx += width;
		}
		return true;
	}
	else if (dataNode->get_name() == TXT("copyVerticalLines"))
	{
		int mask = 0xffff;
		Colour tint = Colour::white;
		tint.load_from_xml_child_or_attr(dataNode, TXT("colour"));
		tint.load_from_xml_child_or_attr(dataNode, TXT("tintColour"));

		if (auto const * attr = dataNode->get_attribute(TXT("mask")))
		{
			mask = 0;
			if (String::does_contain(attr->get_as_string().to_char(), TXT("r"))) mask |= 0x1;
			if (String::does_contain(attr->get_as_string().to_char(), TXT("g"))) mask |= 0x2;
			if (String::does_contain(attr->get_as_string().to_char(), TXT("b"))) mask |= 0x4;
			if (String::does_contain(attr->get_as_string().to_char(), TXT("a"))) mask |= 0x8;
		}

		int fromU = dataNode->get_int_attribute(TXT("fromU"), 0);
		int toU = dataNode->get_int_attribute(TXT("toU"), 0);
		int width = dataNode->get_int_attribute(TXT("width"), 1);

		int xStart = fromU;
		int xEnd = fromU + width - 1;
		int xStartTo = toU;
		if (_scaleToLevel)
		{
			xStart = xStart * textureSetup.levelSize[_level].x / textureSetup.size.x;
			xEnd = max(xStart, (xEnd + 1) * textureSetup.levelSize[_level].x / textureSetup.size.x - 1);
			xStartTo = xStartTo * textureSetup.levelSize[_level].x / textureSetup.size.x;
		}
		xStart = clamp(xStart, 0, textureSetup.levelSize[_level].x - 1);
		xEnd = clamp(xEnd, 0, textureSetup.levelSize[_level].x - 1);

		int yStart = 0;
		int yEnd = textureSetup.levelSize[_level].y - 1;

		for (int y = yStart; y <= yEnd; ++ y)
		{
			for_range(int, x, xStart, xEnd)
			{
				Colour existingColour = textureSetup.get_pixel(_level, VectorInt2(x, y));
				Colour colour = existingColour * tint;
				if (!(mask & 0x1)) colour.r = existingColour.r;
				if (!(mask & 0x2)) colour.g = existingColour.g;
				if (!(mask & 0x4)) colour.b = existingColour.b;
				if (!(mask & 0x8)) colour.a = existingColour.a;
				textureSetup.draw_pixel(_level, VectorInt2(x - xStart + xStartTo, y), colour);
			}
		}
		
		return true;
	}
	else if (dataNode->get_name() == TXT("clamp"))
	{
		int mask = 0xffff;

		if (auto const * attr = dataNode->get_attribute(TXT("mask")))
		{
			mask = 0;
			if (String::does_contain(attr->get_as_string().to_char(), TXT("r"))) mask |= 0x1;
			if (String::does_contain(attr->get_as_string().to_char(), TXT("g"))) mask |= 0x2;
			if (String::does_contain(attr->get_as_string().to_char(), TXT("b"))) mask |= 0x4;
			if (String::does_contain(attr->get_as_string().to_char(), TXT("a"))) mask |= 0x8;
		}

		float minv = dataNode->get_float_attribute(TXT("min"), 0.0f);
		float maxv = dataNode->get_float_attribute(TXT("max"), 1.0f);

		int xStart = 0;
		int xEnd = textureSetup.levelSize[_level].x - 1;

		int yStart = 0;
		int yEnd = textureSetup.levelSize[_level].y - 1;

		for (int y = yStart; y <= yEnd; ++ y)
		{
			for_range(int, x, xStart, xEnd)
			{
				Colour existingColour = textureSetup.get_pixel(_level, VectorInt2(x, y));
				Colour colour = existingColour;
				if ((mask & 0x1)) colour.r = clamp(existingColour.r, minv, maxv);
				if ((mask & 0x2)) colour.g = clamp(existingColour.g, minv, maxv);
				if ((mask & 0x4)) colour.b = clamp(existingColour.b, minv, maxv);
				if ((mask & 0x8)) colour.a = clamp(existingColour.a, minv, maxv);
				textureSetup.draw_pixel(_level, VectorInt2(x, y), colour);
			}
		}
		
		return true;
	}
	else if (dataNode->get_name() == TXT("rectangles"))
	{
		int seedBase = 23595647;
		int seedOffset = 230480752;
		seedBase = dataNode->get_int_attribute(TXT("seedBase"), seedBase);
		seedOffset = dataNode->get_int_attribute(TXT("seedOffset"), seedOffset);
		Random::Generator rg(seedBase, seedOffset);

		int step = dataNode->get_int_attribute(TXT("step"), min(16, textureSetup.levelSize[_level].x / 32));
		int maxSteps = dataNode->get_int_attribute(TXT("maxSteps"), 8);
		float mipMapColour= dataNode->get_float_attribute(TXT("mipMapColour"), 0.25);
		Colour paper = Colour::white;
		Colour paperDark = Colour::greyLight;
		Colour ink = Colour::black;
		paper.load_from_xml_child_node(dataNode, TXT("paper"));
		paperDark.load_from_xml_child_node(dataNode, TXT("paperDark"));
		ink.load_from_xml_child_node(dataNode, TXT("ink"));

		for_count(int, il, _level)
		{
			step = step / 2;

			ink = Colour::lerp(mipMapColour, ink, paper);
			paperDark = Colour::lerp(mipMapColour, paperDark, paper);
		}

		struct Draw
		{
			::System::TextureSetup& textureSetup;
			int level;
			Draw(::System::TextureSetup& _textureSetup, int _level) : textureSetup(_textureSetup), level(_level) {}

			void pixel(VectorInt2 const& _at, Colour const& _colour)
			{
				textureSetup.draw_pixel(level, VectorInt2(mod(_at.x, textureSetup.levelSize[level].x), mod(_at.y, textureSetup.levelSize[level].y)), _colour);
			}

			void rect(RangeInt2 const& _at, Colour const& _colour, Optional<Colour> const & _fill = NP)
			{
				if (_fill.is_set())
				{
					for_range(int, y, _at.y.min + 1, _at.y.max - 1)
					{
						for_range(int, x, _at.x.min + 1, _at.x.max - 1)
						{
							pixel(VectorInt2(x, y), _fill.get());
						}
					}
				}

				for_range(int, x, _at.x.min, _at.x.max)
				{
					pixel(VectorInt2(x, _at.y.min), _colour);
					pixel(VectorInt2(x, _at.y.max), _colour);
				}
				for_range(int, y, _at.y.min, _at.y.max)
				{
					pixel(VectorInt2(_at.x.min, y), _colour);
					pixel(VectorInt2(_at.x.max, y), _colour);
				}
			}
		};
		Draw draw(textureSetup, _level);

		// clear
		for_count(int, x, textureSetup.levelSize[_level].x)
		{
			for_count(int, y, textureSetup.levelSize[_level].y)
			{
				textureSetup.draw_pixel(_level, VectorInt2(x, y), paper);
			}
		}

		if (step > 1)
		{
			VectorInt2 size = textureSetup.levelSize[_level];

			{
				VectorInt2 at = VectorInt2::zero;

				while (at.y < size.y)
				{
					int rectYSize = rg.get_int_from_range(1, maxSteps) * step;
					while (at.x < size.x)
					{
						int rectXSize = rg.get_int_from_range(1, maxSteps) * step;
						RangeInt2 rect = RangeInt2::empty;
						rect.include(at);
						rect.include(at + VectorInt2(rectXSize, rectYSize));
						draw.rect(rect, ink, Colour::lerp(rg.get_float(0.0f, 1.0f), paperDark, paper));

						at.x += rectXSize;
					}

					at.y += rectYSize;
				}
			}

			for_count(int, iter, 128)
			{
				VectorInt2 atStep;
				atStep.x = rg.get_int_from_range(0, size.x / step);
				atStep.y = rg.get_int_from_range(0, size.y / step);
				VectorInt2 sizeStep;
				sizeStep.x = rg.get_int_from_range(0, maxSteps);
				sizeStep.y = rg.get_int_from_range(0, maxSteps);
				bool horizontal = rg.get_bool();
				if (horizontal)
				{
					sizeStep.x *= rg.get_int_from_range(1, 3);
				}
				else
				{
					sizeStep.y *= rg.get_int_from_range(1, 3);
				}

				int startAt = horizontal ? atStep.x : atStep.y;
				int endAt = startAt + (horizontal? sizeStep.x : sizeStep.y);
				int at = startAt;
				while (at <= endAt)
				{
					int width = rg.get_int_from_range(1, maxSteps);
					width = min(width, endAt + 1 - at);

					int startAt2 = horizontal ? atStep.y : atStep.x;
					int endAt2 = startAt + (horizontal ? sizeStep.y : sizeStep.x);
					int at2 = startAt2;
					while (at2 <= endAt2)
					{
						int height = rg.get_int_from_range(1, maxSteps);
						height = min(height, endAt2 + 1 - at2);

						RangeInt2 rect = RangeInt2::empty;
						rect.include((horizontal ? VectorInt2(at, at2) : VectorInt2(at2, at)) * step);
						rect.include((horizontal ? VectorInt2(at + width, at2 + height) : VectorInt2(at2 + height, at + width)) * step);

						draw.rect(rect, ink, Colour::lerp(rg.get_float(0.0f, 1.0f), paperDark, paper));

						at2 += height;
					}

					at += width;
				}
			}
		}
		return true;
	}

	if (dataNode->get_name() == TXT("mipMap"))
	{
		error_loading_xml(dataNode, TXT("not expecting \"mipMap\" at this point"));
		return false;
	}

	error_loading_xml(dataNode, TXT("method \"%S\" not recognised"), dataNode->get_name().to_char());
	return true;
}

bool LibraryTools::generate_texture(IO::XML::Node const * _node, LibraryLoadingContext & _lc)
{
	if (!MainSettings::global().should_allow_importing())
	{
		// no importing? don't create texture, as this is almost same as importing - otherwise force importing
		return true;
	}

	bool result = true;

	// digest node as source
	IO::Digested digestedSource;
	digestedSource.digest(_node);

	Array<GenerateTexture> generateTextures;
	generateTextures.set_size(1);

	bool texture0Loaded = false;
	for_every(dataNode, _node->all_children())
	{
		if (dataNode->get_name() == TXT("texture"))
		{
			int idx = dataNode->get_int_attribute(TXT("idx"), 0);
			generateTextures.set_size(max(generateTextures.get_size(), idx + 1));
			if (idx != 0)
			{
				// copy setup
				generateTextures[idx].textureSetup = generateTextures[0].textureSetup;
			}
			if (!generateTextures[idx].load_from_xml(dataNode))
			{
				return false;
			}
			texture0Loaded |= idx == 0;
		}
	}

	// load data
	if (!texture0Loaded)
	{
		if (!generateTextures[0].load_from_xml(_node))
		{
			return false;
		}

	}

	// compare to file
	if (! MainSettings::global().should_force_reimporting() &&
		! MainSettings::global().should_force_reimporting_using_tools())
	{
		bool needsToRegenerate = false;
		for_every(generateTexture, generateTextures)
		{
			if (generateTexture->fileName.is_empty())
			{
				error(TXT("no name provided for library tool \"generate_texture\""));
				return false;
			}
			String path = IO::Utils::make_path(_lc.get_dir(), generateTexture->fileName);
			if (IO::File::does_exist(path))
			{
				::System::Texture texture;
				if (texture.serialise(Serialiser::reader(IO::File(path).temp_ptr()).temp_ref()))
				{
					if (!(digestedSource == texture.get_digested_source()))
					{
						needsToRegenerate = true;
					}
				}
				else
				{
					needsToRegenerate = true;
				}
			}
			else
			{
				needsToRegenerate = true;
			}
		}
		if (!needsToRegenerate)
		{
			return true;
		}
	}

	// create place for pixels
	for_every(generateTexture, generateTextures)
	{
		if (generateTexture->fileName.is_empty())
		{
			error(TXT("no name provided for library tool \"generate_texture\""));
			return false;
		}
		generateTexture->textureSetup.create_pixels();
	}

	for_every(dataNode, _node->all_children())
	{
		if (dataNode->get_name() == TXT("mipMap"))
		{
			int level = dataNode->get_int_attribute(TXT("level"));
			for_every(subLevelDataNode, dataNode->all_children())
			{
				result &= load_generate_texture_function(generateTextures, level, subLevelDataNode, false);
			}
		}
		else
		{
			// do for all!
			for_count(int, level, generateTextures[0].textureSetup.mipMapLevels)
			{
				result &= load_generate_texture_function(generateTextures, level, dataNode, true);
			}
		}
	}

	// create texture(s)
	for_every(generateTexture, generateTextures)
	{
		::System::Texture* texture = new ::System::Texture();
		texture->use_setup_to_init(generateTexture->textureSetup);

		// serialise texture to file
		texture->set_digested_source(digestedSource);
		texture->serialise_resource_to_file(IO::Utils::make_path(_lc.get_dir(), generateTexture->fileName));

		// we no longer need it
		delete texture;
	}

	return result;
}
