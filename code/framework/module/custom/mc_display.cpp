#include "mc_display.h"

#include "..\modules.h"
#include "..\moduleDataImpl.inl"

#include "..\..\display\display.h"
#include "..\..\display\displayDrawCommands.h"
#include "..\..\library\library.h"
#include "..\..\library\usedLibraryStored.inl"

#include "..\..\..\core\concurrency\asynchronousJob.h"
#include "..\..\..\core\system\video\renderTarget.h"

// !@#
#include "..\..\display\displayButton.h"

using namespace Framework;
using namespace CustomModules;

//

DEFINE_STATIC_NAME(textureParamName);
DEFINE_STATIC_NAME(useWithMaterial);
DEFINE_STATIC_NAME(displayType);
DEFINE_STATIC_NAME(displayOutputSizeMultiplier);
DEFINE_STATIC_NAME(displayMaxOutputSize);
DEFINE_STATIC_NAME(displayMaxOutputSizeX);
DEFINE_STATIC_NAME(displayMaxOutputSizeY);
DEFINE_STATIC_NAME(displayScreenSize);
DEFINE_STATIC_NAME(displayBorderSize);
DEFINE_STATIC_NAME(displayStartingRetraceAtPct);
DEFINE_STATIC_NAME(displayInk);
DEFINE_STATIC_NAME(displayPaper);
DEFINE_STATIC_NAME(shadowMaskTexture);
DEFINE_STATIC_NAME(shadowMaskTextureParamName);
DEFINE_STATIC_NAME(shadowMaskPerPixel);
DEFINE_STATIC_NAME(shadowMaskSizeParamName);

//
DEFINE_STATIC_NAME_STR(displayOff, TXT("display off"));

//

REGISTER_FOR_FAST_CAST(CustomModules::Display);

static Framework::ModuleCustom* create_module(Framework::IModulesOwner* _owner)
{
	return new CustomModules::Display(_owner);
}

static Framework::ModuleData* create_module_data(::Framework::LibraryStored* _inLibraryStored)
{
	return new DisplayData(_inLibraryStored);
}

Framework::RegisteredModule<Framework::ModuleCustom> & CustomModules::Display::register_itself()
{
	return Framework::Modules::custom.register_module(String(TXT("display")), create_module, create_module_data);
}

CustomModules::Display::Display(Framework::IModulesOwner* _owner)
: base(_owner)
{
}

CustomModules::Display::~Display()
{
}

void CustomModules::Display::reset()
{
	base::reset();

	mark_requires(all_customs__advance_post);

	display.clear();
}

struct CustomModules::Display::InitDisplay
: public Concurrency::AsynchronousJobData
{
	Framework::DisplayPtr display;
	InitDisplay(Framework::Display* _display)
	: display(_display)
	{}

	static void perform(Concurrency::JobExecutionContext const * _executionContext, void* _data)
	{
		InitDisplay* data = (InitDisplay*)_data;
		data->display->init(::Framework::Library::get_current());
	}
};

void CustomModules::Display::initialise()
{
	base::initialise();

	mark_requires(all_customs__advance_post);

	// we need to do it as a sync job, as it creates render targets etc - those should be at render/system thread
	// first just create display, will be init later
	display = new Framework::Display();
	display->setup_with(displayType.get());
	if (displayInk.is_set())
	{
		display->set_default_ink(displayInk.get());
	}
	if (displayPaper.is_set())
	{
		display->set_default_paper(displayPaper.get());
	}
	display->setup_resolution_with_borders(displayScreenSize.x >  0 ? displayScreenSize : display->get_setup().screenResolution,
										   displayBorderSize.x >= 0 ? displayBorderSize : display->get_setup().wholeDisplayResolution - display->get_setup().screenResolution);
	display->setup_output_size(display->get_setup().wholeDisplayResolution * displayOutputSizeMultiplier);
	display->setup_output_size_limit(displayMaxOutputSize);
	display->use_meshes(false); // we are not interested in display's own meshes, just output
	display->set_retrace_at_pct(displayStartingRetraceAtPct);

	Game::get()->async_system_job(Game::get(), InitDisplay::perform, new InitDisplay(display.get()));
}

void CustomModules::Display::setup_with(Framework::ModuleData const * _moduleData)
{
	base::setup_with(_moduleData);
	displayData = fast_cast<DisplayData>(_moduleData);
	if (displayData)
	{
		textureParamName = displayData->textureParamName;
		useWithMaterial = displayData->useWithMaterial;
		displayType = displayData->displayType;
		displayScreenSize = displayData->displayScreenSize;
		displayBorderSize = displayData->displayBorderSize;
		displayMaxOutputSize = displayData->displayMaxOutputSize;
		displayOutputSizeMultiplier = displayData->displayOutputSizeMultiplier;
		displayStartingRetraceAtPct = displayData->displayStartingRetraceAtPct;
		displayInk = displayData->displayInk;
		displayPaper = displayData->displayPaper;
#ifdef USE_DISPLAY_SHADOW_MASK
		shadowMask = displayData->shadowMask;
		shadowMaskTextureParamName = displayData->shadowMaskTextureParamName;
		shadowMaskSizeParamName = displayData->shadowMaskSizeParamName;
		if (!displayData->shadowMaskPerPixel.is_zero())
		{
			shadowMaskPerPixel = displayData->shadowMaskPerPixel;
		}
#endif
	}
	if (_moduleData)
	{
		textureParamName = _moduleData->get_parameter<Name>(this, NAME(textureParamName), textureParamName);
		{
			LibraryName libName = _moduleData->get_parameter<LibraryName>(this, NAME(useWithMaterial), useWithMaterial.get_name());
			if (libName != useWithMaterial.get_name())
			{
				useWithMaterial.set_name(libName);
				useWithMaterial.find(Library::get_current());
			}
		}
		{
			LibraryName libName = _moduleData->get_parameter<LibraryName>(this, NAME(displayType), displayType.get_name());
			if (libName != displayType.get_name())
			{
				displayType.set_name(libName);
				displayType.find(Library::get_current());
			}
		}
		{
			displayMaxOutputSize.x = _moduleData->get_parameter<int>(this, NAME(displayMaxOutputSizeX), displayMaxOutputSize.x);
			displayMaxOutputSize.y = _moduleData->get_parameter<int>(this, NAME(displayMaxOutputSizeY), displayMaxOutputSize.y);
			displayMaxOutputSize = _moduleData->get_parameter<VectorInt2>(this, NAME(displayMaxOutputSize), displayMaxOutputSize);
		}
		displayOutputSizeMultiplier = _moduleData->get_parameter<float>(this, NAME(displayOutputSizeMultiplier), displayOutputSizeMultiplier);
		displayScreenSize = _moduleData->get_parameter<VectorInt2>(this, NAME(displayScreenSize), displayScreenSize);
		displayBorderSize = _moduleData->get_parameter<VectorInt2>(this, NAME(displayBorderSize), displayBorderSize);
		displayStartingRetraceAtPct = _moduleData->get_parameter<float>(this, NAME(displayStartingRetraceAtPct), displayStartingRetraceAtPct);
		{
			Colour ink = _moduleData->get_parameter<Colour>(this, NAME(displayInk), Colour::none);
			if (ink != Colour::none)
			{
				displayInk = ink;
			}
			Colour paper = _moduleData->get_parameter<Colour>(this, NAME(displayPaper), Colour::none);
			if (paper != Colour::none)
			{
				displayPaper = paper;
			}
		}
		
#ifdef USE_DISPLAY_SHADOW_MASK
		{
			LibraryName libName = _moduleData->get_parameter<LibraryName>(this, NAME(shadowMaskTexture), shadowMask.get_name());
			if (libName != shadowMask.get_name())
			{
				shadowMask.set_name(libName);
				shadowMask.find(Library::get_current());
			}
		}		
		shadowMaskSizeParamName = _moduleData->get_parameter<Name>(this, NAME(shadowMaskSizeParamName), shadowMaskSizeParamName);
		shadowMaskTextureParamName = _moduleData->get_parameter<Name>(this, NAME(shadowMaskTextureParamName), shadowMaskTextureParamName);
		Vector2 shadowMaskPerPixelProposal = _moduleData->get_parameter<Vector2>(this, NAME(shadowMaskPerPixel), Vector2::zero);
		if (!shadowMaskPerPixelProposal.is_zero())
		{
			shadowMaskPerPixel = shadowMaskPerPixelProposal;
		}
#endif

		if (!useWithMaterial.is_set())
		{
			warn(TXT("no \"useWithMaterial\" for display custom module"));
		}
		if (!displayType.is_set())
		{
			error(TXT("no \"displayType\" for display custom module"));
		}
		if (!textureParamName.is_valid())
		{
			error(TXT("no \"textureParamName\" provided for display custom module"));
		}
	}
}

void CustomModules::Display::advance_post(float _deltaTime)
{
	base::advance_post(_deltaTime);
	if (!display.is_set() || !display->is_initialised())
	{
		return;
	}
	display->advance_sounds(_deltaTime); // should be done before sounds are used for sound scene
	display->advance(_deltaTime);
}

void CustomModules::Display::update_display_in(::System::MaterialInstance & _materialInstance) const
{
	if (!display.is_set() || ! display->is_initialised())
	{
		return;
	}

	if (!textureParamName.is_valid())
	{
		return;
	}
	if (useWithMaterial.is_set())
	{
		if (_materialInstance.get_material() != useWithMaterial.get()->get_material() &&
			displayVisible == display->is_visible())
		{
			return;
		}
	}

	displayVisible = display->is_visible();

	if (auto* displayRT = display->get_output_render_target())
	{
		// force data texture id here as we're talking about individual displays
		_materialInstance.set_uniform(textureParamName, displayRT->get_data_texture_id(0), ::System::MaterialSetting::Forced);
	}
	else
	{
		if (!displayOffTexture.is_set())
		{
			displayOffTexture.set(Framework::Library::get_current()->get_global_references().get<Framework::Texture>(NAME(displayOff)));
		}
		if (displayOffTexture.is_set())
		{
			_materialInstance.set_uniform(textureParamName, displayOffTexture.get()->get_texture()->get_texture_id(), ::System::MaterialSetting::Forced);
		}
	}
#ifdef USE_DISPLAY_SHADOW_MASK
	{
		if (shadowMaskTextureParamName.is_valid())
		{
			Texture* useShadowMask = nullptr;
			if (shadowMask.is_set())
			{
				useShadowMask = shadowMask.get();
			}
			else if (display->get_setup().shadowMask.is_set())
			{
				useShadowMask = display->get_setup().shadowMask.get();
			}
			if (useShadowMask)
			{
				_materialInstance.set_uniform(shadowMaskTextureParamName, useShadowMask->get_texture()->get_texture_id(), ::System::MaterialSetting::Forced);
			}
		}
		if (shadowMaskSizeParamName.is_valid())
		{
			Vector2 useShadowMaskPerPixel = Vector2::one;
			if (shadowMaskPerPixel.is_set())
			{
				useShadowMaskPerPixel = shadowMaskPerPixel.get();
			}
			else
			{
				useShadowMaskPerPixel = display->get_setup().shadowMaskPerPixel;
			}
			_materialInstance.set_uniform(shadowMaskSizeParamName, display->get_setup().wholeDisplayResolution.to_vector2() * useShadowMaskPerPixel, ::System::MaterialSetting::Forced);
		}
	}
#endif
}

//

REGISTER_FOR_FAST_CAST(CustomModules::DisplayData);

DisplayData::DisplayData(LibraryStored* _inLibraryStored)
: base(_inLibraryStored)
{
}

bool DisplayData::read_parameter_from(IO::XML::Attribute const * _attr, LibraryLoadingContext & _lc)
{
	return base::read_parameter_from(_attr, _lc);
}

bool DisplayData::read_parameter_from(IO::XML::Node const * _node, LibraryLoadingContext & _lc)
{
	if (_node->get_name() == TXT("display") ||
		_node->get_name() == TXT("rendering") ||
		_node->get_name() == TXT("shadowMask"))
	{
		// handled in load_from_xml
		return true;
	}
	return base::read_parameter_from(_node, _lc);
}

bool DisplayData::load_from_xml(IO::XML::Node const * _node, LibraryLoadingContext & _lc)
{
	bool result = base::load_from_xml(_node, _lc);

	for_every(node, _node->children_named(TXT("display")))
	{
		result &= displayType.load_from_xml(node, TXT("type"), _lc);
		displayScreenSize.load_from_xml_child_node(node, TXT("screenSize"));
		displayBorderSize.load_from_xml_child_node(node, TXT("borderSize"));
		displayMaxOutputSize.load_from_xml_child_node(node, TXT("maxOutputSize"));
		displayOutputSizeMultiplier = node->get_float_attribute_or_from_child(TXT("outputSizeMultiplier"), displayOutputSizeMultiplier);
		displayStartingRetraceAtPct = node->get_float_attribute_or_from_child(TXT("startingRetraceAtPct"), displayStartingRetraceAtPct);
		displayInk.load_from_xml(node, TXT("ink"));
		displayPaper.load_from_xml(node, TXT("paper"));
	}
	for_every(node, _node->children_named(TXT("rendering")))
	{
		textureParamName = node->get_name_attribute(TXT("textureParamName"), textureParamName);
		result &= useWithMaterial.load_from_xml(node, TXT("useWithMaterial"), _lc);
	}
#ifdef USE_DISPLAY_SHADOW_MASK
	for_every(node, _node->children_named(TXT("shadowMask")))
	{
		result &= shadowMask.load_from_xml(node, TXT("texture"), _lc);
		shadowMaskSizeParamName = node->get_name_attribute_or_from_child(TXT("sizeParamName"), shadowMaskSizeParamName);
		shadowMaskTextureParamName = node->get_name_attribute_or_from_child(TXT("textureParamName"), shadowMaskTextureParamName);
		shadowMaskPerPixel.load_from_xml_child_node(node, TXT("perPixel"));
	}
#endif

	return result;
}

bool DisplayData::prepare_for_game(Library* _library, LibraryPrepareContext& _pfgContext)
{
	bool result = base::prepare_for_game(_library, _pfgContext);

	if (useWithMaterial.is_name_valid())
	{
		result &= useWithMaterial.prepare_for_game_find(_library, _pfgContext, LibraryPrepareLevel::Resolve);
	}
	if (displayType.is_name_valid())
	{
		result &= displayType.prepare_for_game_find(_library, _pfgContext, LibraryPrepareLevel::Resolve);
	}
#ifdef USE_DISPLAY_SHADOW_MASK
	if (shadowMask.is_name_valid())
	{
		result &= shadowMask.prepare_for_game_find(_library, _pfgContext, LibraryPrepareLevel::Resolve);
	}
#endif

	return result;
}
