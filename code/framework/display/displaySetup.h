#pragma once

#include "displayTypes.h"

#include "..\library\usedLibraryStored.h"

#include "..\..\core\types\colour.h"
#include "..\..\core\types\name.h"
#include "..\..\core\tags\tag.h"
#include "..\..\core\math\math.h"
#include "..\..\core\other\simpleVariableStorage.h"
#include "..\..\core\system\video\videoEnums.h"

namespace Framework
{
	class Font;
	class PostProcess;
	class Texture;
	class Display;

	struct DisplayRegion
	{
		Name name;
		VectorInt2 at; // in chars
		VectorInt2 size; // in chars
		VectorInt2 scale = VectorInt2::one;
		DisplayHAlignment::Type textHAlignment;
		DisplayVAlignment::Type textVAlignment;

		DisplayRegion();
		bool load_from_xml(IO::XML::Node const * _node);

		VectorInt2 get_left_bottom() const { return at; }
		VectorInt2 get_top_right() const { return at + size - VectorInt2::one; }
		int get_length() const { return size.x; }
		int get_height() const { return size.y; }
	};

	struct DisplayColourPair
	{
		Name name;
		Optional<Colour> ink;
		Optional<Colour> paper;

		bool load_from_xml(IO::XML::Node const * _node);

		Colour const & get_ink(Display* _display) const;
		Colour const & get_paper(Display* _display) const;
	};

	struct DisplayCursor
	{
		UsedLibraryStored<Texture> texture;
		VectorInt2 pointAt = VectorInt2::zero;
		bool useOwnColours = false;

		bool load_from_xml(IO::XML::Node const * _node, LibraryLoadingContext & _lc);
		bool prepare_for_game(Library* _library, LibraryPrepareContext& _pfgContext);
	};

	struct DisplaySetup
	{
		bool extraPreDistortionSize = false; // to increase pre distortion size to maintain pixels etc
		bool limitPreDistortionSizeToScreenSize = false; // do not exceed screen size - but not smaller than outputSize

		// if you need to have pointers around here, consider having customised copy operator!
		Tags kind; // what kind of display it is - to allow different displays to be used with same system if they are of same kind ("80x24" "32x24" etc)
		DisplayCoordinates::Type useCoordinates = DisplayCoordinates::Char; // otherwise pixel
		bool loopedNavigation = true;
		bool useUpperCaseForSoftSelection = false; // useful for black and white displays where it can't show soft selected items (selected in unselected control)
		VectorInt2 screenResolution; // this is just for main screen part
		VectorInt2 wholeDisplayResolution; // this is screen resolution + borders
		Vector2 pixelScale; // this is used by some post process (bleed etc) to scale properly
		float pixelAspectRatio; // x / y - preferred, if output size is set directly this is ignored, it is used only if size has to be automatically calculated
		VectorInt2 outputSize; // this is output render target size
		float refreshFrequency;
		float retraceVisibleFrequency;
		int drawCyclesPerSecond; // if negative, will always draw commands immediately
		Map<Name, UsedLibraryStored<Font>> fonts;
		UsedLibraryStored<PostProcess> postProcess; // post process - it should have input node with inputs/outputs "current" and "previous"
#ifdef USE_DISPLAY_CASING_POST_PROCESS
		UsedLibraryStored<PostProcess> postProcessForCasing; // post process for the casing/border (takes as input the result of normal post process
#endif
#ifdef USE_DISPLAY_SHADOW_MASK
		UsedLibraryStored<Texture> shadowMask;
		Vector2 shadowMaskPerPixel = Vector2(0.0f, 1.0f); // if x is zero, takes from the texture's size or texture's parameters (shadowMaskAspectRatio), has to be explicitly provided as shadowMaskPerPixelX
		float shadowMaskBaseLevel = 1.0f; // shadow masks are by their nature dimmer, this is used to make the base level lower than 1.0 to apply extra boost, if texture provides this, it is overriden by the value from texture
#endif
		DisplayCursor cursor; // default
		Map<Name, DisplayCursor> namedCursors;
		SimpleVariableStorage postProcessParameters;
		Colour defaultInk;
		Colour defaultPaper;
		Map<Name, DisplayRegion> regions;
		Map<Name, String> crosshairs;
		Map<Name, DisplayColourPair> colourPairs;

		float powerTurnOnBlendTime;
		float powerTurnOffBlendTime;
		Vector2 turnOnScaleDisplayBlendTime;
		Vector2 turnOffScaleDisplayBlendTime;

		::System::BlendOp::Type renderDisplaySrcBlendOp = ::System::BlendOp::One;
		::System::BlendOp::Type renderDisplayDestBlendOp = ::System::BlendOp::OneMinusSrcAlpha;

		DisplaySetup();

		void setup_resolution_with_borders(VectorInt2 const & _screenResolution, VectorInt2 const & _border);
		bool setup_output_size(VectorInt2 const & _outputSize);
			
		bool load_from_xml(IO::XML::Node const * _node, LibraryLoadingContext & _lc);
		bool prepare_for_game(Library* _library, LibraryPrepareContext& _pfgContext);

		UsedLibraryStored<Font> const & get_font(Name const & _alphabet = Name::invalid()) const;
		void use_font(LibraryName const & _fontName, Name const & _alphabet = Name::invalid()) { fonts[_alphabet].set_name(_fontName); }

#ifdef AN_DEVELOPMENT
		void debug_log() const;
#endif
	};

};
