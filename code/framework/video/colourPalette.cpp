#include "colourPalette.h"

//

#ifdef AN_CLANG
#include "..\library\usedLibraryStored.inl"
#endif

//

#ifdef AN_ALLOW_OPTIMIZE_OFF
#pragma optimize("", off)
#endif

//

using namespace Framework;

//

LIBRARY_STORED_DEFINE_TYPE(ColourPalette, colourPalette);

REGISTER_FOR_FAST_CAST(ColourPalette);

ColourPalette::ColourPalette(Library * _library, LibraryName const & _name)
: base(_library, _name)
{
}

bool ColourPalette::load_from_xml(IO::XML::Node const * _node, LibraryLoadingContext & _lc)
{
	bool result = base::load_from_xml(_node, _lc);

	// clear
	//
	colours.clear();
	preferredGrid.clear();

	// load
	//
	preferredGrid.load_from_xml(_node, TXT("preferredGrid"));
	for_every(node, _node->all_children())
	{
		if (node->get_name() == TXT("colour"))
		{
			Colour c = Colour::black;
			c.load_from_xml(node);
			add(c);
		}
		if (node->get_name() == TXT("blend"))
		{
			int shadeCount = 16;
			Colour a = Colour::black;
			Colour b = Colour::black;
			Optional<Colour> mid;
			shadeCount = node->get_int_attribute(TXT("shades"), shadeCount);
			shadeCount = node->get_int_attribute(TXT("shadeCount"), shadeCount);
			a.load_from_xml_child_or_attr(node, TXT("a"));
			b.load_from_xml_child_or_attr(node, TXT("b"));
			mid.load_from_xml(node, TXT("mid"));
			mid.load_from_xml(node, TXT("m"));
			if (mid.is_set())
			{
				int midAt = shadeCount / 2;
				midAt = node->get_int_attribute(TXT("midAt"), midAt);
				for_every(n, node->children_named(TXT("mid")))
				{
					midAt = n->get_int_attribute(TXT("at"), midAt);
				}
				for_every(n, node->children_named(TXT("m")))
				{
					midAt = n->get_int_attribute(TXT("at"), midAt);
				}
				add_blend(midAt, a, mid.get(), false);
				add_blend(shadeCount - midAt, mid.get(), b);
			}
			else
			{
				add_blend(shadeCount, a, b);
			}
		}
		if (node->get_name() == TXT("hue"))
		{
			int shadeCount = 24;
			float low = 0.0f;
			float high = 1.0f;
			shadeCount = node->get_int_attribute(TXT("shades"), shadeCount);
			shadeCount = node->get_int_attribute(TXT("shadeCount"), shadeCount);
			low = node->get_float_attribute(TXT("low"), low);
			high = node->get_float_attribute(TXT("high"), high);
			add_hue(shadeCount, low, high);
		}
	}

	if (_node->get_bool_attribute_or_from_child_presence(TXT("beCommodore64")))
	{
		be_commodore_64();
	}

	if (_node->get_bool_attribute_or_from_child_presence(TXT("beZXSpectrum")))
	{
		be_zx_spectrum();
	}

	if (_node->get_bool_attribute_or_from_child_presence(TXT("beAtari2600")) ||
		_node->get_bool_attribute_or_from_child_presence(TXT("beAtari2600NTSC")))
	{
		be_atari_2600_ntsc();
	}

	if (_node->get_bool_attribute_or_from_child_presence(TXT("beStandardVGA")))
	{
		be_standard_vga();
	}

	if (_node->get_bool_attribute_or_from_child_presence(TXT("beNiceVGA")))
	{
		be_nice_vga();
	}

	if (_node->get_bool_attribute_or_from_child_presence(TXT("beFullVGA")))
	{
		be_full_vga();
	}

	return result;
}

bool ColourPalette::save_to_xml(IO::XML::Node* _node) const
{
	bool result = base::save_to_xml(_node);

	if (preferredGrid.is_set())
	{
		if (auto* n = _node->add_node(TXT("preferredGrid")))
		{
			preferredGrid.get().save_to_xml(n);
		}
	}

	for_every(c, colours)
	{
		c->save_to_xml_child_node(_node, TXT("colour"));
	}

	return result;
}

void ColourPalette::clear()
{
	colours.clear();
}

void ColourPalette::add(Colour const& _colour)
{
	colours.push_back(_colour);
}

void ColourPalette::add_blend(int _amount, Colour const& _a, Colour const& _b, bool _blendToLast)
{
	for_count(int, s, _amount)
	{
		float pt = (float)s / (float)(_blendToLast ? _amount - 1 : _amount);
		colours.push_back(Colour::lerp(pt, _a, _b));
	}
}

void ColourPalette::add_hue(int _amount, float l, float h)
{
	int total = _amount;
	int soFar = 0;
	{	int now = (total - soFar) / 6;		add_blend(now, Colour(l, l, h), Colour(h, l, h), false);	soFar += now;	}
	{	int now = (total - soFar) / 5;		add_blend(now, Colour(h, l, h), Colour(h, l, l), false);	soFar += now;	}
	{	int now = (total - soFar) / 4;		add_blend(now, Colour(h, l, l), Colour(h, h, l), false);	soFar += now;	}
	{	int now = (total - soFar) / 3;		add_blend(now, Colour(h, h, l), Colour(l, h, l), false);	soFar += now;	}
	{	int now = (total - soFar) / 2;		add_blend(now, Colour(l, h, l), Colour(l, h, h), false);	soFar += now;	}
	{	int now = (total - soFar) / 1;		add_blend(now, Colour(l, h, h), Colour(l, l, h), false);	soFar += now;	}
}

void ColourPalette::be_commodore_64()
{
	colours.clear();
	colours.push_back(Colour::c64Black);
	colours.push_back(Colour::c64White);
	colours.push_back(Colour::c64Red);
	colours.push_back(Colour::c64Cyan);
	colours.push_back(Colour::c64Violet);
	colours.push_back(Colour::c64Green);
	colours.push_back(Colour::c64Blue);
	colours.push_back(Colour::c64Yellow);
	colours.push_back(Colour::c64Orange);
	colours.push_back(Colour::c64Brown);
	colours.push_back(Colour::c64LightRed);
	colours.push_back(Colour::c64Grey1);
	colours.push_back(Colour::c64Grey2);
	colours.push_back(Colour::c64LightGreen);
	colours.push_back(Colour::c64LightBlue);
	colours.push_back(Colour::c64Grey3);
}

void ColourPalette::be_zx_spectrum()
{
	colours.clear();
	colours.push_back(Colour::zxBlack);
	colours.push_back(Colour::zxBlue);
	colours.push_back(Colour::zxRed);
	colours.push_back(Colour::zxMagenta);
	colours.push_back(Colour::zxGreen);
	colours.push_back(Colour::zxCyan);
	colours.push_back(Colour::zxYellow);
	colours.push_back(Colour::zxWhite);
	colours.push_back(Colour::zxBlackBright);
	colours.push_back(Colour::zxBlueBright);
	colours.push_back(Colour::zxRedBright);
	colours.push_back(Colour::zxMagentaBright);
	colours.push_back(Colour::zxGreenBright);
	colours.push_back(Colour::zxCyanBright);
	colours.push_back(Colour::zxYellowBright);
	colours.push_back(Colour::zxWhiteBright);
}

void ColourPalette::be_atari_2600_ntsc()
{
	colours.clear();
	add_blend(8, Colour(0.000f, 0.000f, 0.000f), Colour(0.925f, 0.925f, 0.925f), true); // 0
	add_blend(8, Colour(0.267f, 0.267f, 0.000f), Colour(0.988f, 0.988f, 0.408f), true); // 1
	add_blend(8, Colour(0.439f, 0.157f, 0.000f), Colour(0.925f, 0.784f, 0.471f), true); // 2
	add_blend(8, Colour(0.518f, 0.094f, 0.000f), Colour(0.988f, 0.737f, 0.580f), true); // 3
	add_blend(8, Colour(0.533f, 0.000f, 0.000f), Colour(0.988f, 0.706f, 0.706f), true); // 4
	add_blend(8, Colour(0.471f, 0.000f, 0.361f), Colour(0.925f, 0.690f, 0.878f), true); // 5
	add_blend(8, Colour(0.282f, 0.000f, 0.471f), Colour(0.831f, 0.690f, 0.988f), true); // 6
	add_blend(8, Colour(0.078f, 0.000f, 0.518f), Colour(0.737f, 0.706f, 0.988f), true); // 7
	add_blend(8, Colour(0.000f, 0.000f, 0.533f), Colour(0.643f, 0.722f, 0.988f), true); // 8
	add_blend(8, Colour(0.000f, 0.094f, 0.486f), Colour(0.643f, 0.784f, 0.988f), true); // 9
	add_blend(8, Colour(0.000f, 0.173f, 0.361f), Colour(0.643f, 0.878f, 0.988f), true); // 10
	add_blend(8, Colour(0.000f, 0.235f, 0.173f), Colour(0.643f, 0.988f, 0.831f), true); // 11
	add_blend(8, Colour(0.000f, 0.235f, 0.000f), Colour(0.722f, 0.988f, 0.722f), true); // 12
	add_blend(8, Colour(0.078f, 0.220f, 0.000f), Colour(0.784f, 0.988f, 0.643f), true); // 13
	add_blend(8, Colour(0.173f, 0.188f, 0.000f), Colour(0.878f, 0.925f, 0.612f), true); // 14
	add_blend(8, Colour(0.267f, 0.157f, 0.000f), Colour(0.988f, 0.878f, 0.549f), true); // 15

	preferredGrid = VectorInt2(8, 0);
}

void ColourPalette::be_standard_vga()
{
	colours.clear();
	colours.make_space_for(256);

	// 00-0F
	colours.push_back(Colour(0.000f, 0.000f, 0.000f));
	colours.push_back(Colour(0.000f, 0.000f, 0.667f));
	colours.push_back(Colour(0.000f, 0.667f, 0.000f));
	colours.push_back(Colour(0.000f, 0.667f, 0.667f));
	colours.push_back(Colour(0.667f, 0.000f, 0.000f));
	colours.push_back(Colour(0.667f, 0.000f, 0.667f));
	colours.push_back(Colour(0.667f, 0.333f, 0.000f));
	colours.push_back(Colour(0.667f, 0.667f, 0.667f));
	colours.push_back(Colour(0.333f, 0.333f, 0.333f));
	colours.push_back(Colour(0.333f, 0.333f, 1.000f));
	colours.push_back(Colour(0.333f, 1.000f, 0.333f));
	colours.push_back(Colour(0.333f, 1.000f, 1.000f));
	colours.push_back(Colour(1.000f, 0.333f, 0.333f));
	colours.push_back(Colour(1.000f, 0.333f, 1.000f));
	colours.push_back(Colour(1.000f, 1.000f, 0.333f));
	colours.push_back(Colour(1.000f, 1.000f, 1.000f));
	// 10-1F
	add_blend(16, Colour::black, Colour::white, true);
	// 20-37
	add_hue(24, 0.000f, 1.000f);
	// 38-4f
	add_hue(24, 0.510f, 1.000f);
	// 50-67
	add_hue(24, 0.729f, 1.000f);
	// 68-7f
	add_hue(24, 0.000f, 0.443f);
	// 80-97
	add_hue(24, 0.224f, 0.443f);
	// 98-af
	add_hue(24, 0.318f, 0.443f);
	// b0-c7
	add_hue(24, 0.000f, 0.255f);
	// c8-df
	add_hue(24, 0.125f, 0.255f);
	// e0-f7
	add_hue(24, 0.176f, 0.255f);
	// f8-ff
	add_blend(8, Colour(0.0f, 0.0f, 0.0f), Colour(0.0f, 0.0f, 0.0f), true);
}

void ColourPalette::be_nice_vga()
{
	colours.clear();
	colours.make_space_for(256);

	// 00-0F
	colours.push_back(Colour(0.000f, 0.000f, 0.000f));
	colours.push_back(Colour(0.000f, 0.000f, 0.667f));
	colours.push_back(Colour(0.000f, 0.667f, 0.000f));
	colours.push_back(Colour(0.000f, 0.667f, 0.667f));
	colours.push_back(Colour(0.667f, 0.000f, 0.000f));
	colours.push_back(Colour(0.667f, 0.000f, 0.667f));
	colours.push_back(Colour(0.667f, 0.333f, 0.000f));
	colours.push_back(Colour(0.667f, 0.667f, 0.667f));
	colours.push_back(Colour(0.333f, 0.333f, 0.333f));
	colours.push_back(Colour(0.333f, 0.333f, 1.000f));
	colours.push_back(Colour(0.333f, 1.000f, 0.333f));
	colours.push_back(Colour(0.333f, 1.000f, 1.000f));
	colours.push_back(Colour(1.000f, 0.333f, 0.333f));
	colours.push_back(Colour(1.000f, 0.333f, 1.000f));
	colours.push_back(Colour(1.000f, 1.000f, 0.333f));
	colours.push_back(Colour(1.000f, 1.000f, 1.000f));
	// 10-17
	add_blend(8, Colour::black, Colour::black, true);
	// 18-2f
	add_hue(24, 0.000f, 1.000f);
	// 30-47
	add_hue(24, 0.510f, 1.000f);
	// 48-5f
	add_hue(24, 0.729f, 1.000f);
	// 60-77
	add_hue(24, 0.000f, 0.443f);
	// 78-8f
	add_hue(24, 0.224f, 0.443f);
	// 90-a7
	add_hue(24, 0.318f, 0.443f);
	// a8-bf
	add_hue(24, 0.000f, 0.255f);
	// c0-d7
	add_hue(24, 0.125f, 0.255f);
	// d8-ef
	add_hue(24, 0.176f, 0.255f);
	// f0-ff
	add_blend(16, Colour::black, Colour::white, true);

	preferredGrid = VectorInt2(24, 0);
}

void ColourPalette::be_full_vga()
{
	colours.clear();
	colours.make_space_for(256);
	{
		int grayShades = 16;
		int maxRange = grayShades - 1;
		for_range(int, s, 0, maxRange)
		{
			float fs = (float)s / (float)maxRange;
			colours.push_back(Colour(fs, fs, fs));
		}
	}
	{
		int levelCount = 6;
		int maxRange = levelCount - 1;
		for_range(int, r, 0, maxRange)
		{
			float fr = (float)r / (float)maxRange;
			for_range(int, g, 0, maxRange)
			{
				float fg = (float)g / (float)maxRange;
				for_range(int, b, 0, maxRange)
				{
					float fb = (float)b / (float)maxRange;
					colours.push_back(Colour(fr, fg, fb));
				}
			}
		}
	}
	// fill up to 256
	{
		while (colours.get_size() < 256)
		{
			colours.push_back(Colour(0.0f, 0.0f, 0.0f));
		}
	}
}

//--

void RemapColours::remap(int _from, int _to)
{
	for_every(rm, remapColours)
	{
		if (rm->from == _from)
		{
			rm->to = _to;
			return;
		}
	}
	remapColours.push_back();
	auto& rm = remapColours.get_last();
	rm.from = _from;
	rm.to = _to;
}

void RemapColours::prepare_colour_map(ColourPalette const* _for, REF_ Array<int>& _map)
{
	_map.clear();

	int colourCount = _for->get_colours().get_size();
	_map.make_space_for(colourCount);
	for_count(int, idx, colourCount)
	{
		_map.push_back(idx);
	}

	for_every(rm, remapColours)
	{
		_map[rm->from] = rm->to;
	}
}

