#include "displaySetup.h"

#include "display.h"

#include "..\library\library.h"
#include "..\library\usedLibraryStored.inl"
#include "..\postProcess\postProcess.h"
#include "..\video\font.h"
#include "..\video\texture.h"

using namespace Framework;

//

DEFINE_STATIC_NAME(displays);
DEFINE_STATIC_NAME(default);

//

DisplayRegion::DisplayRegion()
: at(VectorInt2::zero)
, size(VectorInt2::one)
, textHAlignment(DisplayHAlignment::Left)
, textVAlignment(DisplayVAlignment::Top)
{
}

bool DisplayRegion::load_from_xml(IO::XML::Node const * _node)
{
	bool result = true;

	name = _node->get_name_attribute(TXT("name"), name);
	if (!name.is_valid())
	{
		error_loading_xml(_node, TXT("no name provided for display region"));
		result = false;
	}

	if (!at.load_from_xml_child_node(_node, TXT("at")))
	{
		error_loading_xml(_node, TXT("no valid \"at\" info provided for display region"));
		result = false;
	}

	if (!size.load_from_xml_child_node(_node, TXT("size")))
	{
		error_loading_xml(_node, TXT("no valid \"size\" info provided for display region"));
		result = false;
	}

	scale.load_from_xml_child_node(_node, TXT("scale"));

	// scale has to be at least 1
	scale.x = max(1, scale.x);
	scale.y = max(1, scale.y);

	// size has to be at least scale
	size.x = max(scale.x, size.x);
	size.y = max(scale.y, size.y);

	textHAlignment = DisplayHAlignment::parse(_node->get_string_attribute(TXT("textHAlign")), textHAlignment);
	textHAlignment = DisplayHAlignment::parse(_node->get_string_attribute(TXT("textHAlignment")), textHAlignment);
	textVAlignment = DisplayVAlignment::parse(_node->get_string_attribute(TXT("textVAlign")), textVAlignment);
	textVAlignment = DisplayVAlignment::parse(_node->get_string_attribute(TXT("textVAlignment")), textVAlignment);

	return result;
}

//

bool DisplayColourPair::load_from_xml(IO::XML::Node const * _node)
{
	bool result = true;

	name = _node->get_name_attribute(TXT("name"), name);
	if (IO::XML::Node const * node = _node->first_child_named(TXT("ink")))
	{
		Colour colour = Colour::white;
		if (colour.load_from_xml(node))
		{
			ink = colour;
		}
		else
		{
			error_loading_xml(node, TXT("error loading ink"));
			result = false;
		}
	}
	if (IO::XML::Node const * node = _node->first_child_named(TXT("paper")))
	{
		Colour colour = Colour::white;
		if (colour.load_from_xml(node))
		{
			paper = colour;
		}
		else
		{
			error_loading_xml(node, TXT("error loading paper"));
			result = false;
		}
	}
	return result;
}

Colour const & DisplayColourPair::get_ink(Display* _display) const
{
	return ink.is_set() ? ink.get() : (_display ? _display->get_current_ink() : Colour::white);
}

Colour const & DisplayColourPair::get_paper(Display* _display) const
{
	return paper.is_set() ? paper.get() : (_display ? _display->get_current_paper() : Colour::black);
}

//

bool DisplayCursor::load_from_xml(IO::XML::Node const * _node, LibraryLoadingContext & _lc)
{
	bool result = true;

	if (!_node)
	{
		return result;
	}

	texture.load_from_xml(_node, TXT("texture"), _lc);
	pointAt.load_from_xml_child_node(_node, TXT("point"));
	useOwnColours = _node->get_bool_attribute_or_from_child_presence(TXT("useOwnColours"), useOwnColours);

	return result;
}

bool DisplayCursor::prepare_for_game(Library* _library, LibraryPrepareContext& _pfgContext)
{
	bool result = true;

	if (texture.is_name_valid())
	{
		result &= texture.prepare_for_game_find(_library, _pfgContext, LibraryPrepareLevel::Resolve);
	}

	return result;
}


//

DisplaySetup::DisplaySetup()
: screenResolution(VectorInt2::one)
, wholeDisplayResolution(VectorInt2::one)
, pixelScale(Vector2::one)
, pixelAspectRatio(1.0f)
, outputSize(VectorInt2::one)
, refreshFrequency(10.0f)
, retraceVisibleFrequency(-1.0f)
, drawCyclesPerSecond(1200)
, defaultInk(Colour::white)
, defaultPaper(Colour::black)
, powerTurnOnBlendTime(2.0f)
, powerTurnOffBlendTime(0.2f)
, turnOnScaleDisplayBlendTime(0.005f, 0.005f)
, turnOffScaleDisplayBlendTime(0.2f, 0.05f)
{
	fonts[Name::invalid()].set_name(LibraryName(NAME(displays), NAME(default)));
	postProcess.set_name(LibraryName(NAME(displays), NAME(default)));
	setup_resolution_with_borders(VectorInt2(240, 160), VectorInt2(30, 25));
}

void DisplaySetup::setup_resolution_with_borders(VectorInt2 const & _screenResolution, VectorInt2 const & _border)
{
	screenResolution = _screenResolution;
	wholeDisplayResolution = screenResolution + _border * 2;
}

bool DisplaySetup::setup_output_size(VectorInt2 const & _outputSize)
{
	bool changed = outputSize != _outputSize;
	outputSize = _outputSize;
	return changed;
}

static bool load_value_or_vector(IO::XML::Node const * _node, REF_ Vector2 & _value, tchar const * _name)
{
	if (_node->has_attribute(_name))
	{
		float v = _node->get_float_attribute(_name, _value.x);
		_value.x = v;
		_value.y = v;
		return true;
	}
	else if (IO::XML::Node const * child = _node->first_child_named(_name))
	{
		return _value.load_from_xml(child);
	}
	return true;
}

bool DisplaySetup::load_from_xml(IO::XML::Node const * _node, LibraryLoadingContext & _lc)
{
	bool result = true;

	if (!_node)
	{
		return true;
	}

	extraPreDistortionSize |= _node->first_child_named(TXT("extraPreDistortionSize")) != nullptr;
	limitPreDistortionSizeToScreenSize |= _node->first_child_named(TXT("limitPreDistortionSizeToScreenSize")) != nullptr;
	
	kind.load_from_xml_attribute_or_child_node(_node, TXT("kind"));
	
	useCoordinates = DisplayCoordinates::parse(_node->get_string_attribute_or_from_child(TXT("useCoordinates")), useCoordinates);
	loopedNavigation = _node->get_bool_attribute_or_from_child(TXT("loopedNavigation"), loopedNavigation);
	useUpperCaseForSoftSelection = _node->get_bool_attribute_or_from_child(TXT("useUpperCaseForSoftSelection"), useUpperCaseForSoftSelection);

	screenResolution.load_from_xml_child_node(_node, TXT("screen"));
	VectorInt2 border = VectorInt2::zero;
	border.load_from_xml_child_node(_node, TXT("border"));
	setup_resolution_with_borders(screenResolution, border);
	if (IO::XML::Node const * child = _node->first_child_named(TXT("output")))
	{
		pixelScale.load_from_xml_child_node(child, TXT("pixelScale"));
		pixelAspectRatio = child->get_float_attribute(TXT("pixelAspectRatio"), pixelAspectRatio);
	}

	refreshFrequency = _node->get_float_attribute(TXT("refreshFrequency"), refreshFrequency);
	retraceVisibleFrequency = _node->get_float_attribute(TXT("retraceVisibleFrequency"), retraceVisibleFrequency);
	drawCyclesPerSecond = _node->get_int_attribute(TXT("drawCyclesPerSecond"), drawCyclesPerSecond);
	
	defaultInk.load_from_xml_child_node(_node, TXT("defaultInk"));
	defaultPaper.load_from_xml_child_node(_node, TXT("defaultPaper"));

	fonts[Name::invalid()].load_from_xml(_node, TXT("font"), _lc);
	for_every(containerNode, _node->children_named(TXT("fonts")))
	{
		for_every(node, containerNode->children_named(TXT("font")))
		{
			Name alphabet = node->get_name_attribute(TXT("alphabet"));
			fonts[alphabet].load_from_xml(node, TXT("font"), _lc);
		}
	}
	postProcess.load_from_xml(_node, TXT("postProcess"), _lc);
#ifdef USE_DISPLAY_CASING_POST_PROCESS
	postProcessForCasing.load_from_xml(_node, TXT("postProcessForCasing"), _lc);
#endif
#ifdef USE_DISPLAY_SHADOW_MASK
	{	// legacy load
		shadowMask.load_from_xml(_node, TXT("shadowMask"), _lc);
		shadowMaskPerPixel.load_from_xml_child_node(_node, TXT("shadowMaskPerPixel"));
		if (auto* attr = _node->get_attribute(TXT("shadowMaskPerPixel")))
		{
			shadowMaskPerPixel = Vector2(0.0f, attr->get_as_float());
		}
		shadowMaskBaseLevel = _node->get_float_attribute_or_from_child(TXT("shadowMaskBaseLevel"), shadowMaskBaseLevel);
	}
	{	// single line load
		for_every(node, _node->children_named(TXT("shadowMask")))
		{
			shadowMask.load_from_xml(node, TXT("use"), _lc);
			shadowMaskPerPixel.load_from_xml(node, TXT("perPixelX"), TXT("perPixelY"));
			if (auto* attr = node->get_attribute(TXT("perPixel")))
			{
				shadowMaskPerPixel = Vector2(0.0f, attr->get_as_float());
			}
			if (auto* attr = node->get_attribute(TXT("baseLevel")))
			{
				shadowMaskBaseLevel = attr->get_as_float();
			}
		}
	}
	if (!shadowMask.is_name_valid())
	{
		error_loading_xml(_node, TXT("provide something, default white one at least (\"default:white\")"));
	}
#endif
	for_every(containerNode, _node->children_named(TXT("cursors")))
	{
		for_every(node, containerNode->children_named(TXT("cursor")))
		{
			Name name = node->get_name_attribute(TXT("name"));
			if (name.is_valid())
			{
				result &= namedCursors[name].load_from_xml(node, _lc);
			}
			else
			{
				result &= cursor.load_from_xml(node, _lc);
			}
		}
	}
	result &= cursor.load_from_xml(_node->first_child_named(TXT("cursor")), _lc);

	for_every(child, _node->children_named(TXT("postProcessParameters")))
	{
		result &= postProcessParameters.load_from_xml(child);
		_lc.load_group_into(postProcessParameters);
	}

	for_every(containerNode, _node->children_named(TXT("regions")))
	{
		for_every(node, containerNode->children_named(TXT("region")))
		{
			DisplayRegion region;
			if (region.load_from_xml(node))
			{
				regions[region.name] = region;
			}
			else
			{
				result = false;
			}
		}
	}

	for_every(containerNode, _node->children_named(TXT("crosshairs")))
	{
		for_every(node, containerNode->children_named(TXT("crosshair")))
		{
			IO::XML::Attribute const * attrName = node->get_attribute(TXT("name"));
			IO::XML::Attribute const * attrAs = node->get_attribute(TXT("as"));
			if (attrName && attrAs)
			{
				crosshairs[attrName->get_as_name()] = attrAs->get_as_string();
			}
			else
			{
				result = false;
			}
		}
	}

	for_every(containerNode, _node->children_named(TXT("colourPairs")))
	{
		for_every(node, containerNode->children_named(TXT("colourPair")))
		{
			DisplayColourPair colourPair;
			if (colourPair.load_from_xml(node))
			{
				colourPairs[colourPair.name] = colourPair;
			}
			else
			{
				result = false;
			}
		}
	}

	powerTurnOnBlendTime = _node->get_float_attribute_or_from_child(TXT("powerTurnOnBlendTime"), powerTurnOnBlendTime);
	powerTurnOffBlendTime = _node->get_float_attribute_or_from_child(TXT("powerTurnOffBlendTime"), powerTurnOffBlendTime);
	load_value_or_vector(_node, turnOnScaleDisplayBlendTime, TXT("turnOnScaleDisplayBlendTime"));
	load_value_or_vector(_node, turnOffScaleDisplayBlendTime, TXT("turnOffScaleDisplayBlendTime"));

	if (!::System::BlendOp::parse(_node->get_string_attribute_or_from_child(TXT("renderDisplaySrcBlendOp")), renderDisplaySrcBlendOp))
	{
		error_loading_xml(_node, TXT("not recognised renderDisplaySrcBlendOp \"%S\""), _node->get_string_attribute_or_from_child(TXT("renderDisplaySrcBlendOp")).to_char());
		result = false;
	}
	if (!::System::BlendOp::parse(_node->get_string_attribute_or_from_child(TXT("renderDisplayDestBlendOp")), renderDisplayDestBlendOp))
	{
		error_loading_xml(_node, TXT("not recognised renderDisplayDestBlendOp \"%S\""), _node->get_string_attribute_or_from_child(TXT("renderDisplayDestBlendOp")).to_char());
		result = false;
	}

	return result;
}

bool DisplaySetup::prepare_for_game(Library* _library, LibraryPrepareContext& _pfgContext)
{
	bool result = true;

	for_every(font, fonts)
	{
		result &= font->prepare_for_game_find(_library, _pfgContext, LibraryPrepareLevel::Resolve);
	}
	result &= postProcess.prepare_for_game_find(_library, _pfgContext, LibraryPrepareLevel::Resolve);
#ifdef USE_DISPLAY_CASING_POST_PROCESS
	result &= postProcessForCasing.prepare_for_game_find(_library, _pfgContext, LibraryPrepareLevel::Resolve);
#endif
#ifdef USE_DISPLAY_SHADOW_MASK
	result &= shadowMask.prepare_for_game_find(_library, _pfgContext, LibraryPrepareLevel::Resolve);
#endif
	result &= cursor.prepare_for_game(_library, _pfgContext);
	for_every(namedCursor, namedCursors)
	{
		result &= namedCursor->prepare_for_game(_library, _pfgContext);
	}

	// add default colour pair
	DisplayColourPair defaultColourPair;
	defaultColourPair.ink = defaultInk;
	defaultColourPair.paper = defaultPaper;
	colourPairs[Name::invalid()] = defaultColourPair;

	return result;
}

UsedLibraryStored<Font> const & DisplaySetup::get_font(Name const & _alphabet) const
{
	if (auto const * font = fonts.get_existing(_alphabet))
	{
		return *font;
	}
	else
	{
		font = fonts.get_existing(Name::invalid());
		an_assert(font);
		return *font;
	}
}

#ifdef AN_DEVELOPMENT
void DisplaySetup::debug_log() const
{
	output(TXT("DISPLAY SETUP"));
	output(TXT("extraPreDistortionSize             : %S"), extraPreDistortionSize? TXT("true") : TXT("false"));
	output(TXT("limitPreDistortionSizeToScreenSize : %S"), limitPreDistortionSizeToScreenSize ? TXT("true") : TXT("false"));
	
	// if you need to have pointers around here, consider having customised copy operator!
	output(TXT("kind                               : %S"), kind.to_string().to_char());
	output(TXT("useCoordinates                     : %S"), DisplayCoordinates::to_char(useCoordinates));
	output(TXT("loopedNavigation                   : %S"), loopedNavigation ? TXT("true") : TXT("false"));
	output(TXT("useUpperCaseForSoftSelection       : %S"), useUpperCaseForSoftSelection ? TXT("true") : TXT("false"));
	output(TXT("screenResolution                   : %S"), screenResolution.to_string().to_char());
	output(TXT("wholeDisplayResolution             : %S"), wholeDisplayResolution.to_string().to_char());
	output(TXT("pixelScale                         : %.3f x %.3f"), pixelScale, pixelScale);
	output(TXT("pixelAspectRatio                   : %.3f"), pixelAspectRatio);
	output(TXT("outputSize                         : %S"), outputSize.to_string().to_char());
	output(TXT("refreshFrequency                   : %.3f"), refreshFrequency);
	output(TXT("retraceVisibleFrequency            : %.3f"), retraceVisibleFrequency);
	output(TXT("drawCyclesPerSecond                : %.3f"), drawCyclesPerSecond);
	output(TXT("powerTurnOnBlendTime               : %.3f"), powerTurnOnBlendTime);
	output(TXT("powerTurnOffBlendTime              : %.3f"), powerTurnOffBlendTime);
	output(TXT("turnOnScaleDisplayBlendTime        : %S"), turnOnScaleDisplayBlendTime.to_string().to_char());
	output(TXT("turnOffScaleDisplayBlendTime       : %S"), turnOffScaleDisplayBlendTime.to_string().to_char());
	output(TXT("postProcessParameters"));
	
	for_every(variable, postProcessParameters.get_all())
	{
		String text;
		text += variable->get_name().to_char();
		text += String::printf(TXT(" [%S] : "), RegisteredType::get_name_of(variable->type_id()));
		if (variable->type_id() == type_id<float>()) text += String::printf(TXT("%.3f"), variable->access<float>()); else
		if (variable->type_id() == type_id<int>()) text += String::printf(TXT("%i"), variable->access<int>()); else
		if (variable->type_id() == type_id<Vector2>()) text += variable->access<Vector2>().to_string(); else
		if (variable->type_id() == type_id<Vector3>()) text += variable->access<Vector3>().to_string(); else
		if (variable->type_id() == type_id<Vector4>()) text += variable->access<Vector4>().to_string(); else
			text += TXT("??");
		output(TXT(" %S"), text.to_char());
	}
}
#endif
