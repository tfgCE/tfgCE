#include "display.h"

#include "displayButton.h"
#include "displayDrawCommands.h"
#include "displayDrawCommand.h"
#include "displayHardwareMeshGenerator.h"
#include "displayTable.h"
#include "displayType.h"

#include "..\debugSettings.h"

#include "..\game\game.h"
#include "..\game\gameInput.h"
#include "..\game\gameInputProxy.h"
#include "..\library\library.h"
#include "..\library\usedLibraryStored.inl"
#include "..\pipelines\displayPipeline.h"
#include "..\postProcess\postProcessInstance.h"
#include "..\postProcess\postProcessRenderTargetManager.h"
#include "..\postProcess\graph\postProcessGraphProcessingContext.h"
#include "..\postProcess\graph\postProcessOutputNode.h"
#include "..\render\renderContext.h"
#include "..\render\renderCamera.h"
#include "..\render\renderScene.h"
#include "..\video\font.h"
#include "..\vr\vrMeshes.h"

#include "..\..\core\mainConfig.h"
#include "..\..\core\concurrency\asynchronousJob.h"
#include "..\..\core\concurrency\scopedSpinLock.h"
#include "..\..\core\graph\graphImplementation.h"
#include "..\..\core\system\core.h"
#include "..\..\core\system\input.h"
#include "..\..\core\system\video\renderTarget.h"
#include "..\..\core\types\names.h"
#include "..\..\core\vr\iVR.h"

#include "..\..\core\debug\debugRenderer.h"

//

#ifdef AN_ALLOW_OPTIMIZE_OFF
#pragma optimize("", off)
#endif

//

using namespace Framework;

//

DEFINE_STATIC_NAME(colour);
DEFINE_STATIC_NAME(deviceMesh2DSize); // store deviceMesh2DSize in library object to allow to reuse it without createing a mesh
DEFINE_STATIC_NAME(deviceMesh3DSize); // store deviceMesh3DSize in library object to allow to reuse it without createing a mesh
DEFINE_STATIC_NAME(vr);
DEFINE_STATIC_NAME_STR(displayVRScene, TXT("display vr scene"));
DEFINE_STATIC_NAME(screen);
DEFINE_STATIC_NAME(final);
DEFINE_STATIC_NAME(finalX);
DEFINE_STATIC_NAME(finalY);
DEFINE_STATIC_NAME(current);
DEFINE_STATIC_NAME(previous);
DEFINE_STATIC_NAME(inCursor);
DEFINE_STATIC_NAME(cursorPlacement);
DEFINE_STATIC_NAME(cursorTransform);
DEFINE_STATIC_NAME(cursorVisible);
DEFINE_STATIC_NAME(cursorInk);
DEFINE_STATIC_NAME(cursorPaper);
DEFINE_STATIC_NAME(cursorUseColours);
DEFINE_STATIC_NAME(retraceAtY);
DEFINE_STATIC_NAME(retraceVisibleAtY);
DEFINE_STATIC_NAME(noiseTextureCurrentOffset);
DEFINE_STATIC_NAME(noiseTexturePreviousOffset);
DEFINE_STATIC_NAME(displayIntensity);
DEFINE_STATIC_NAME(displayIntensityColourUse);
DEFINE_STATIC_NAME(additionalDisplayIntensity);
DEFINE_STATIC_NAME(scaleDisplayUV);
DEFINE_STATIC_NAME(displayPixelSize); // scaled
DEFINE_STATIC_NAME(displayPixelResolution); // scaled
DEFINE_STATIC_NAME(inBackground);
DEFINE_STATIC_NAME(backgroundColourBased);
DEFINE_STATIC_NAME(useBackground);
DEFINE_STATIC_NAME(backgroundTransform);
DEFINE_STATIC_NAME(backgroundPlacement);
DEFINE_STATIC_NAME(inDisplay);
DEFINE_STATIC_NAME(inDisplaySize);
DEFINE_STATIC_NAME(inDisplayTexelSize);
DEFINE_STATIC_NAME(inCasing);
DEFINE_STATIC_NAME(inShadowMask);
DEFINE_STATIC_NAME(shadowMaskSize);
DEFINE_STATIC_NAME(shadowMaskBaseLevel);
DEFINE_STATIC_NAME(shadowMaskBaseLevelInv);
DEFINE_STATIC_NAME(shadowMaskAspectRatio);
DEFINE_STATIC_NAME(cursor);
DEFINE_STATIC_NAME(cursorAbsolute01); // absolute value from 0 to 1 (quite specific)
DEFINE_STATIC_NAME(cursorPress);
DEFINE_STATIC_NAME(cursorTouch);
DEFINE_STATIC_NAME(cursorFakeTouch); // to simulate some of the stuff touch does (show cursor etc)
DEFINE_STATIC_NAME(selector);
DEFINE_STATIC_NAME(selectorPress);
DEFINE_STATIC_NAME(scrollUp);
DEFINE_STATIC_NAME(scrollDown);
DEFINE_STATIC_NAME(selectorPageUp);
DEFINE_STATIC_NAME(selectorPageDown);
DEFINE_STATIC_NAME(selectorHome);
DEFINE_STATIC_NAME(selectorEnd);
DEFINE_STATIC_NAME(cursorHoverScrollUp);
DEFINE_STATIC_NAME(cursorHoverScrollDown);

//

bool Display::s_cursorIsActive = false;

//

float calc_vr_scene_radius()
{
	return max(2.0f, (VR::IVR::get()->get_play_area_rect_half_forward() + VR::IVR::get()->get_play_area_rect_half_right()).length() * 1.4f);
}

float calc_display_vr_dist()
{
	return min(2.0f, calc_vr_scene_radius() * 0.9f);
}

//

bool Display::s_touchIsPress = true;

void Display::set_cursor_active(bool _active)
{
	cursorIsActive = _active;
	s_cursorIsActive = _active;
}

Display::Display()
: currentFrame(nullptr)
, previousFrame(nullptr)
, currentRotatedFrame(nullptr)
, previousRotatedFrame(nullptr)
, outputRenderTarget(nullptr)
, ownOutputRenderTarget(nullptr)
, postProcessInstance(nullptr)
#ifdef USE_DISPLAY_CASING_POST_PROCESS
, postProcessForCasingInstance(nullptr)
#endif
, postProcessRenderTargetManager(nullptr)
, ownPostProcessRenderTargetManager(nullptr)
, noiseTextureCurrentOffset(Vector2::zero)
, noiseTexturePreviousOffset(Vector2::zero)
, cursorAt(Vector2::zero)
, cursorMovedBy(Vector2::zero)
, cursorDraggedBy(Vector2::zero)
, cursorIsDraggedBy(false)
, cursorIsActive(s_cursorIsActive)
, cursorIsPressed(false)
, cursorWasPressed(false)
, cursorIsTouched(false)
, cursorWasTouched(false)
#ifdef USE_DISPLAY_MESH
, deviceMesh2D(nullptr)
, deviceMesh2DFromLibrary(false)
, deviceMesh3D(nullptr)
, deviceMesh3DFromLibrary(false)
, deviceMesh3DSize(Vector2::zero)
#endif
#ifdef USE_DISPLAY_VR_SCENE
, vrSceneMesh3D(nullptr)
, vrSceneMesh3DFromLibrary(false)
#endif
{
	setup_resolution_with_borders(VectorInt2(240, 160), VectorInt2(30, 20));
	//setup_output_size(VectorInt2(1000, 700));
	//setup_output_size(VectorInt2(500, 350));
	setup_output_size(VectorInt2(714, 500));
	setup_output_size(VectorInt2(857, 600));

	charSize = VectorInt2(8, 8);

	currentInk = Colour::white;
	currentPaper = Colour::black;

	ignoreGameFrames = false;
	lastAdvanceGameFrameNo = 0;
	lastUpdateDisplayGameFrameNo = 0;
	doAllDrawCycles = false;
	doAllDrawCyclesGathered = false;
	retraceAtPct = 0.0f;
	retraceVisibleAtPct = 0.0f;
	accumulatedDrawCycles = 0;
	drawCyclesUsedByCurrentDrawCommand = 0;
	doNextFrame = false;

	randomGenerator = Random::Generator::get_universal().spawn();

	use_no_background();

	turn_on();
}

struct Framework::Display_DeleteRendering
: public Concurrency::AsynchronousJobData
{
	PostProcessRenderTargetManager* ownPostProcessRenderTargetManager; // created if render target manager was not provided (or couldn't get it from game)
	::System::RenderTargetPtr currentFrame;
	::System::RenderTargetPtr previousFrame;
	::System::RenderTargetPtr currentRotatedFrame;
	::System::RenderTargetPtr previousRotatedFrame;
	::System::RenderTargetPtr ownOutputRenderTarget;
	bool releaseShaders;

	Display_DeleteRendering(Display* _display)
	: ownPostProcessRenderTargetManager(_display->ownPostProcessRenderTargetManager)
	, currentFrame(_display->currentFrame)
	, previousFrame(_display->previousFrame)
	, currentRotatedFrame(_display->currentRotatedFrame)
	, previousRotatedFrame(_display->previousRotatedFrame)
	, ownOutputRenderTarget(_display->ownOutputRenderTarget)
	, releaseShaders(_display->initialised)
	{}

	static void perform(Concurrency::JobExecutionContext const * _executionContext, void* _data)
	{
		Display_DeleteRendering* data = (Display_DeleteRendering*)_data;
		assert_rendering_thread();
		delete_and_clear(data->ownPostProcessRenderTargetManager);
		data->currentFrame = nullptr;
		data->previousFrame = nullptr;
		data->currentRotatedFrame = nullptr;
		data->previousRotatedFrame = nullptr;
		data->ownOutputRenderTarget = nullptr;
		if (data->releaseShaders)
		{
			DisplayPipeline::release_shaders();
		}
	}
};

void Display::defer_rendering_delete()
{
	Game::async_system_job(Game::get(), Display_DeleteRendering::perform, new Display_DeleteRendering(this));
	ownPostProcessRenderTargetManager = nullptr;
	currentFrame = nullptr;
	previousFrame = nullptr;
	currentRotatedFrame = nullptr;
	previousRotatedFrame = nullptr;
	ownOutputRenderTarget = nullptr;
}

Display::~Display()
{
#ifdef USE_DISPLAY_MESH
	deviceMesh2DInstance.set_mesh(nullptr);
	if (!deviceMesh2DFromLibrary)
	{
		delete_and_clear(deviceMesh2D);
	}
	deviceMesh3DInstance.set_mesh(nullptr);
	if (!deviceMesh3DFromLibrary)
	{
		delete_and_clear(deviceMesh3D);
	}
#endif
#ifdef USE_DISPLAY_VR_SCENE
	vrSceneMesh3DInstance.set_mesh(nullptr);
	if (!vrSceneMesh3DFromLibrary)
	{
		delete_and_clear(vrSceneMesh3D);
	}
#endif

	controls.clear();

	drawCommands.clear();

	ppOutputRenderTarget.clear();
	delete_and_clear(postProcessInstance);
#ifdef USE_DISPLAY_CASING_POST_PROCESS
	ppOutputForCasingRenderTarget.clear();
	delete_and_clear(postProcessForCasingInstance);
#endif

	defer_rendering_delete();

	s_cursorIsActive = cursorIsActive;
}

void Display::setup_with(DisplayType * _displayType)
{
	displayType.set(_displayType);

	if (_displayType)
	{
		use_setup(_displayType->get_setup());
	}
}

void Display::use_setup(DisplaySetup const & _displaySetup)
{
	// overwrite
	setup = _displaySetup;

#ifdef USE_DISPLAY_SHADOW_MASK
	if (auto* t = setup.shadowMask.get())
	{
		if (setup.shadowMaskPerPixel.x == 0.0f)
		{
			float aspectRatio = 1.0f;
			if (auto* st = t->get_texture())
			{
				aspectRatio = aspect_ratio(st->get_size());
			}
			if (auto* shadowMaskAspectRatio = t->get_custom_parameters().get_existing<float>(NAME(shadowMaskAspectRatio)))
			{
				aspectRatio = *shadowMaskAspectRatio;
			}
			// aspect ratio is x / y
			setup.shadowMaskPerPixel.x = setup.shadowMaskPerPixel.y * aspectRatio;
			if (auto* shadowMaskBaseLevel = t->get_custom_parameters().get_existing<float>(NAME(shadowMaskBaseLevel)))
			{
				setup.shadowMaskBaseLevel = *shadowMaskBaseLevel;
			}
		}
	}
#endif

	useCoordinates = setup.useCoordinates;
 	currentInk = setup.defaultInk;
	currentPaper = setup.defaultPaper;

	use_default_cursor();
}

void Display::use_colour_pair(Name const & _colourPairName)
{
	DisplayColourPair const * cp = get_colour_pair(_colourPairName);
	DisplayColourPair const * dcp = get_colour_pair();
	an_assert(dcp);
	an_assert(dcp->ink.is_set());
	an_assert(dcp->paper.is_set());
	currentInk = cp && cp->ink.is_set() ? cp->ink.get() : dcp->ink.get();
	currentPaper = cp && cp->paper.is_set() ? cp->paper.get() : dcp->paper.get();
}

void Display::setup_resolution_with_borders(VectorInt2 const & _screenResolution, VectorInt2 const & _border)
{
	an_assert(currentFrame == nullptr, TXT("should be done before frames are created"));
	setup.setup_resolution_with_borders(_screenResolution, _border);
}

void Display::setup_output_size(VectorInt2 const & _outputSize)
{
	an_assert(currentFrame == nullptr, TXT("should be done before frames are created"));
	setup.setup_output_size(_outputSize);
}

void Display::setup_output_size_as_is()
{
	Vector2 wdr = setup.wholeDisplayResolution.to_vector2();
	wdr.x *= setup.pixelAspectRatio;
	setup_output_size(wdr.to_vector_int2_cells());
}

void Display::setup_output_size_min(VectorInt2 const & _spaceAvailable, float _sizeMultiplier)
{
	Vector2 wdr = setup.wholeDisplayResolution.to_vector2();
	wdr.x *= setup.pixelAspectRatio;
	Vector2 spaceAvailable = _spaceAvailable.to_vector2();
	if (spaceAvailable.x > wdr.x * _sizeMultiplier)
	{
		spaceAvailable *= (wdr.x * _sizeMultiplier) / spaceAvailable.x;
	}
	if (spaceAvailable.y > wdr.y * _sizeMultiplier)
	{
		spaceAvailable *= (wdr.y * _sizeMultiplier) / spaceAvailable.y;
	}
	setup_output_size((wdr * (spaceAvailable.y) / wdr.y).to_vector_int2_cells());
	if (setup.outputSize.x > spaceAvailable.x)
	{
		float maxSizeMultiplier = spaceAvailable.x / setup.outputSize.x;
		setup_output_size((wdr * ((maxSizeMultiplier * spaceAvailable.y) / wdr.y)).to_vector_int2_cells());
	}
}

void Display::setup_output_size_max(VectorInt2 const & _spaceAvailable, float _maxSizeMultiplier)
{
	Vector2 wdr = setup.wholeDisplayResolution.to_vector2();
	wdr.x *= setup.pixelAspectRatio;
	Vector2 spaceAvailable = _spaceAvailable.to_vector2();
	setup_output_size((wdr * (spaceAvailable.y * _maxSizeMultiplier) / wdr.y).to_vector_int2_cells());
	if (setup.outputSize.x > spaceAvailable.x * _maxSizeMultiplier)
	{
		_maxSizeMultiplier = _maxSizeMultiplier * spaceAvailable.x / setup.outputSize.x;
		setup_output_size((wdr * (spaceAvailable.y * _maxSizeMultiplier) / wdr.y).to_vector_int2_cells());
	}
}

void Display::setup_output_size_limit(VectorInt2 const & _spaceAvailable)
{
	VectorInt2 useSpaceAvailable = _spaceAvailable;
	VectorInt2 wdr = setup.wholeDisplayResolution;
	useSpaceAvailable.x = max(useSpaceAvailable.x, wdr.x);
	useSpaceAvailable.y = max(useSpaceAvailable.y, wdr.y);
	Vector2 spaceAvailable = useSpaceAvailable.to_vector2();
	if (setup.outputSize.x > spaceAvailable.x)
	{
		setup_output_size((setup.outputSize.to_vector2() * (spaceAvailable.x / (float)setup.outputSize.x)).to_vector_int2_cells());
	}
	if (setup.outputSize.y > spaceAvailable.y)
	{
		setup_output_size((setup.outputSize.to_vector2() * (spaceAvailable.y / (float)setup.outputSize.y)).to_vector_int2_cells());
	}
}

void Display::use_font(LibraryName const & _fontName)
{
	an_assert(currentFrame == nullptr, TXT("should be done before frames are created"));
	setup.use_font(_fontName);
}

void Display::use_post_process(LibraryName const & _postProcess)
{
	an_assert(currentFrame == nullptr, TXT("should be done before frames are created"));
	setup.postProcess.set_name(_postProcess);
}

void Display::use_render_target_manager(PostProcessRenderTargetManager* _postProcessRenderTargetManager)
{
	an_assert(currentFrame == nullptr, TXT("should be done before frames are created"));
	postProcessRenderTargetManager = _postProcessRenderTargetManager;
}

void Display::init(Library * _library, Optional<DisplayInitContext> const & _initContext, LibraryName const & _meshNameForReusing, DisplayHardwareMeshSetup* _overrideHardwareMeshSetup)
{
	DisplayInitContext initContext = _initContext.get(DisplayInitContext());

	assert_rendering_thread();
	an_assert(!initialised);
	an_assert(currentFrame == nullptr, TXT("do not try to reinit"));

	DisplayPipeline::will_use_shaders();

	{
		::System::RenderTargetSetup rtSetup = ::System::RenderTargetSetup(setup.wholeDisplayResolution, 0/*MainConfig::global().get_video().get_ui_msaa_samples()*/); // we don't need that and it gives a blurry display with MSAAx16
		// we use rgb 10bit textures
		rtSetup.add_output_texture(::System::OutputTextureDefinition(NAME(colour),
			initContext.outputTexture.get(::System::VideoFormat::rgb10_a2),
			::System::BaseVideoFormat::rgba)); // mag=linear doesn't work as we have jaggies, hence using larger output size

		RENDER_TARGET_CREATE_INFO(TXT("display \"%S\", current frame"), displayType.get()? displayType->get_name().to_string().to_char() : TXT("no display type"));
		currentFrame = new ::System::RenderTarget(rtSetup);
		RENDER_TARGET_CREATE_INFO(TXT("display \"%S\", previous frame"), displayType.get() ? displayType->get_name().to_string().to_char() : TXT("no display type"));
		previousFrame = new ::System::RenderTarget(rtSetup);

		if (rtSetup.get_size().x == rtSetup.get_size().y)
		{
			RENDER_TARGET_CREATE_INFO(TXT("display \"%S\", current rotated frame"), displayType.get() ? displayType->get_name().to_string().to_char() : TXT("no display type"));
			currentRotatedFrame = new ::System::RenderTarget(rtSetup);
			RENDER_TARGET_CREATE_INFO(TXT("display \"%S\", previous rotated frame"), displayType.get() ? displayType->get_name().to_string().to_char() : TXT("no display type"));
			previousRotatedFrame = new ::System::RenderTarget(rtSetup);
		}
	}

	if (useMeshes)
	{
#ifdef USE_DISPLAY_MESH
		deviceMesh2DInstance.set_mesh(nullptr);
		if (!deviceMesh2DFromLibrary)
		{
			delete_and_clear(deviceMesh2D);
		}
		else
		{
			deviceMesh2D = nullptr;
		}
		deviceMesh3DInstance.set_mesh(nullptr);
		if (!deviceMesh3DFromLibrary)
		{
			delete_and_clear(deviceMesh3D);
		}
		else
		{
			deviceMesh3D = nullptr;
		}

		// load setup and set size and border
		DisplayHardwareMeshSetup setup = _overrideHardwareMeshSetup ? *_overrideHardwareMeshSetup : (displayType.get() ? displayType->get_hardware_mesh_setup() : DisplayHardwareMeshSetup());
		if (setup.get_size_2d().is_zero())
		{
			setup.with_size_2d(get_setup().outputSize.to_vector2());
		}

		deviceMesh2DFromLibrary = false;
		deviceMesh3DFromLibrary = false;

		if (_library && _meshNameForReusing.is_valid())
		{
			LibraryName meshNameForReusing2D = _meshNameForReusing;
			meshNameForReusing2D.setup_with_string(meshNameForReusing2D.get_name().to_string() + TXT("2d"));
			deviceMesh2D = _library->get_meshes().find(meshNameForReusing2D);
			deviceMesh2DFromLibrary = true;
			if (!deviceMesh2D)
			{
				deviceMesh2D = DisplayHardwareMeshGenerator::create_2d_mesh(setup, _library, meshNameForReusing2D);
				deviceMesh2D->access_custom_parameters().access<Vector2>(NAME(deviceMesh2DSize)) = setup.get_size_2d();
			}
			deviceMesh2DSize = deviceMesh2D->get_custom_parameters().get_value<Vector2>(NAME(deviceMesh2DSize), Vector2::zero);

			//
			LibraryName meshNameForReusing3D = _meshNameForReusing;
			meshNameForReusing3D.setup_with_string(meshNameForReusing3D.get_name().to_string() + TXT("3d"));
			deviceMesh3D = _library->get_meshes().find(meshNameForReusing3D);
			deviceMesh3DFromLibrary = true;
			if (!deviceMesh3D)
			{
				deviceMesh3D = DisplayHardwareMeshGenerator::create_3d_mesh(setup, _library, meshNameForReusing3D, &deviceMesh3DSize);
				deviceMesh3D->access_custom_parameters().access<Vector2>(NAME(deviceMesh3DSize)) = deviceMesh3DSize;
			}
			else
			{
				deviceMesh3DSize = deviceMesh3D->get_custom_parameters().get_value<Vector2>(NAME(deviceMesh3DSize), Vector2::zero);
			}
		}
		else
		{
			// create mesh
			deviceMesh2D = DisplayHardwareMeshGenerator::create_2d_mesh(setup);
			deviceMesh2DSize = setup.get_size_2d();
			setup.with_size_2d(Vector2(4.0f, 3.0f) / 3.0f);
			deviceMesh3D = DisplayHardwareMeshGenerator::create_3d_mesh(setup, nullptr, LibraryName::invalid(), &deviceMesh3DSize);
		}

		deviceMesh2DInstance.set_mesh(deviceMesh2D->get_mesh());
		MaterialSetup::apply_material_setups(deviceMesh2D->get_material_setups(), deviceMesh2DInstance, ::System::MaterialSetting::IndividualInstance);
		deviceMesh2DInstance.fill_materials_with_missing();

		deviceMesh3DInstance.set_mesh(deviceMesh3D->get_mesh());
		MaterialSetup::apply_material_setups(deviceMesh3D->get_material_setups(), deviceMesh3DInstance, ::System::MaterialSetting::IndividualInstance);
		deviceMesh3DInstance.fill_materials_with_missing();
#else
		error(TXT("enable USE_DISPLAY_MESH to allow using meshes"));
#endif

		// for vr

#ifdef USE_DISPLAY_VR_SCENE
		if (VR::IVR::get() && Game::get() && Game::get()->does_use_vr())
		{
			// we may end skipping a frame
			vrSceneMesh3DFromLibrary = false;
			if (_library)
			{
				LibraryName vrSceneMesh3DName(NAME(vr), NAME(displayVRScene));
				vrSceneMesh3D = _library->get_meshes().find(vrSceneMesh3DName);
				if (!vrSceneMesh3D)
				{
					vrSceneMesh3D = DisplayHardwareMeshGenerator::create_vr_scene_mesh(calc_vr_scene_radius(), _library, vrSceneMesh3DName);
				}
				vrSceneMesh3DFromLibrary = true;
			}
			if (!vrSceneMesh3D)
			{
				vrSceneMesh3D = DisplayHardwareMeshGenerator::create_vr_scene_mesh(calc_vr_scene_radius(), _library, LibraryName::invalid());
			}

			if (vrSceneMesh3D)
			{
				vrSceneMesh3DInstance.set_mesh(vrSceneMesh3D->get_mesh());
				MaterialSetup::apply_material_setups(vrSceneMesh3D->get_material_setups(), vrSceneMesh3DInstance, ::System::MaterialSetting::IndividualInstance);
				vrSceneMesh3DInstance.fill_materials_with_missing();
			}
		}
#endif
	}

	// make sure all values are valid
	if (setup.outputSize.is_zero())
	{
		setup.outputSize = setup.wholeDisplayResolution;
	}
	outputPreDistortionSize = setup.outputSize;
	originalOutputPreDistortionSize = outputPreDistortionSize;
	if (setup.extraPreDistortionSize)
	{
		outputPreDistortionSize *= 2;
		// add minimum to prevent artefacts
		outputPreDistortionSize.x = max(outputPreDistortionSize.x, setup.wholeDisplayResolution.x * 2);
		outputPreDistortionSize.y = max(outputPreDistortionSize.y, setup.wholeDisplayResolution.y * 2);
	}
	{
		if (setup.limitPreDistortionSizeToScreenSize)
		{
			VectorInt2 screenSize;
			if (auto* vr = VR::IVR::get())
			{
				screenSize = vr->get_render_size(VR::Eye::Left);
			}
			else
			{
				screenSize = ::System::Video3D::get()->get_screen_size();
			}
			// if we really want something that is greater than screen resolution, keep it that way
			if (screenSize.x >= setup.outputSize.x &&
				screenSize.y >= setup.outputSize.y)
			{
				if (outputPreDistortionSize.x > screenSize.x)
				{
					outputPreDistortionSize.y = outputPreDistortionSize.y * screenSize.x / outputPreDistortionSize.x;
					outputPreDistortionSize.x = screenSize.x;
				}
				if (outputPreDistortionSize.y > screenSize.y)
				{
					outputPreDistortionSize.x = outputPreDistortionSize.x * screenSize.y / outputPreDistortionSize.y;
					outputPreDistortionSize.y = screenSize.y;
				}
			}
		}
	}

	// resolve assets
	for_every(font, setup.fonts)
	{
		font->find(_library);
	}
	{
		UsedLibraryStored<Font> const & font = setup.get_font();
		if (font.get())
		{
			charSize = font.get()->calculate_char_size().to_vector_int2_cells();
		}
	}

	// update cached values
	screenSize = setup.screenResolution.to_vector2();
	leftBottomOfScreen = (setup.wholeDisplayResolution - setup.screenResolution).to_vector2() * 0.5f;
	rightTopOfScreen = leftBottomOfScreen + screenSize - Vector2::one;
	leftBottomOfDisplay = Vector2::zero;
	rightTopOfDisplay = setup.wholeDisplayResolution.to_vector2() - Vector2::one;

	charSizeAsVector2 = charSize.to_vector2();
	charResolution = (charSizeAsVector2.x != 0.0f && charSizeAsVector2.y != 0.0f) ? (screenSize / charSizeAsVector2).to_vector_int2_cells() : CharCoords::zero;

	// store sizes in provider
	postProcessResolutionProvider.set(NAME(screen), setup.screenResolution);
	postProcessResolutionProvider.set(NAME(final), outputPreDistortionSize);
	postProcessResolutionProvider.set(NAME(finalX), VectorInt2(outputPreDistortionSize.x, 0));
	postProcessResolutionProvider.set(NAME(finalY), VectorInt2(0, outputPreDistortionSize.y));

	// clear current frame and do next frame to have both looking same
	{
		::System::Video3D* v3d = ::System::Video3D::get();

		Colour clearWithColour = initContext.clearColour.get(Colour::black);

		currentFrame->bind();
		v3d->clear_colour_depth_stencil(clearWithColour);
		currentFrame->unbind();
		currentFrame->resolve();

		// thee have to be resolved too
		if (currentRotatedFrame.is_set())
		{
			currentRotatedFrame->bind();
			v3d->clear_colour_depth_stencil(clearWithColour);
			currentRotatedFrame->unbind();
			currentRotatedFrame->resolve();
		}
		if (previousRotatedFrame.is_set())
		{
			previousRotatedFrame->bind();
			v3d->clear_colour_depth_stencil(clearWithColour);
			previousRotatedFrame->unbind();
			previousRotatedFrame->resolve();
		}
	}

	next_frame();

	if (postProcessRenderTargetManager == nullptr)
	{
		if (Game* game = Game::get())
		{
			postProcessRenderTargetManager = Game::get()->get_post_process_render_target_manager();
		}
		if (postProcessRenderTargetManager == nullptr)
		{
			ownPostProcessRenderTargetManager = new PostProcessRenderTargetManager();
			postProcessRenderTargetManager = ownPostProcessRenderTargetManager;
		}
	}

	// update post process
	setup.postProcess.find(_library);
	delete_and_clear(postProcessInstance);
	if (PostProcess* pp = setup.postProcess.get())
	{
		postProcessInstance = new PostProcessInstance(pp);
		postProcessInstance->initialise();

		if (auto * ppGraphInstance = postProcessInstance->get_graph_instance())
		{
			// use parameters from setup
			ppGraphInstance->manage_activation_using(setup.postProcessParameters); // this is done here, on start, if that would change, we require user to call manage_activation_using
			ppGraphInstance->for_every_shader_program_instance(
				[this](PostProcessNode const * _node, ::System::ShaderProgramInstance & _shaderProgramInstance)
			{
				_shaderProgramInstance.set_uniforms_from(_shaderProgramInstance.get_shader_program()->get_default_values());
				_shaderProgramInstance.set_uniforms_from(setup.postProcessParameters);
				return true;
			}
			);
		}
	}

#ifdef USE_DISPLAY_CASING_POST_PROCESS
	// update post process for casing
	setup.postProcessForCasing.find(_library);
	delete_and_clear(postProcessForCasingInstance);
	if (PostProcess* pp = setup.postProcessForCasing.get())
	{
		postProcessForCasingInstance = new PostProcessInstance(pp);
		postProcessForCasingInstance->initialise();

		if (auto * ppGraphInstance = postProcessForCasingInstance->get_graph_instance())
		{
			// use parameters from setup
			ppGraphInstance->manage_activation_using(setup.postProcessParameters); // this is done here, on start, if that would change, we require user to call manage_activation_using
			ppGraphInstance->for_every_shader_program_instance(
				[this](PostProcessNode const * _node, ::System::ShaderProgramInstance & _shaderProgramInstance)
			{
				_shaderProgramInstance.set_uniforms_from(_shaderProgramInstance.get_shader_program()->get_default_values());
				_shaderProgramInstance.set_uniforms_from(setup.postProcessParameters);
				return true;
			}
			);
		}
	}
#endif

	if (!postProcessInstance)
	{
		::System::RenderTargetSetup rtSetup = ::System::RenderTargetSetup(outputPreDistortionSize, 0/*MainConfig::global().get_video().get_ui_msaa_samples()*/);
		// we use 8bit rgba
		rtSetup.add_output_texture(::System::OutputTextureDefinition(NAME(colour),
			::System::VideoFormat::rgba8,
			::System::BaseVideoFormat::rgba)); // mag=linear doesn't work as we have jaggies, hence using larger output size

		RENDER_TARGET_CREATE_INFO(TXT("display \"%S\", own output render target"), displayType.get() ? displayType->get_name().to_string().to_char() : TXT("no display type"));
		ownOutputRenderTarget = new ::System::RenderTarget(rtSetup);
		outputRenderTarget = ownOutputRenderTarget;
	}

	cursorAt = setup.screenResolution.to_vector2() * 0.5f;

	initialised = true;

	todo_note(TXT("add initial screen"));
	doNextFrame = true;
	doAllDrawCycles = true;
	doAllDrawCyclesGathered = true;
	// auto clear
	add((new DisplayDrawCommands::Border()));
	add((new DisplayDrawCommands::CLS()));
	add(new DisplayDrawCommands::HoldDrawing());
}

void Display::next_frame()
{
#ifdef AN_LOG_POST_PROCESS_ALL_FRAMES
	output(TXT("NEXT FRAME FOR DISPLAY %i================================="), this);
#endif
	// swap them
	swap(currentFrame, previousFrame);

	noiseTexturePreviousOffset = noiseTextureCurrentOffset;
	noiseTextureCurrentOffset = Vector2(randomGenerator.get_float(0.0f, 1.0f), randomGenerator.get_float(0.0f, 1.0f));

	::System::Video3D::get()->unbind_frame_buffer();

	previousFrame->copy_to(currentFrame.get());

	update_post_process_frames();
}

void Display::update_post_process_frames()
{
	if (!rotateDisplay)
	{
		postProcessRenderTargetsProvider.set(NAME(current), currentFrame.get(), 0);
		postProcessRenderTargetsProvider.set(NAME(previous), previousFrame.get(), 0);
	}
	else
	{
		postProcessRenderTargetsProvider.set(NAME(current), currentRotatedFrame.get(), 0);
		postProcessRenderTargetsProvider.set(NAME(previous), previousRotatedFrame.get(), 0);
	}
}

void Display::move_cursor(Vector2 const & _by, bool _jump)
{
	cursorAt += _by;

	cursorAt.x = clamp(cursorAt.x, 0.0f, (float)(setup.screenResolution.x - 1));
	cursorAt.y = clamp(cursorAt.y, 0.0f, (float)(setup.screenResolution.y - 1));

	if (_jump)
	{
		cursorMovedBy = Vector2::zero;
	}
	else
	{
		cursorMovedBy = _by;
	}
	if (cursorIsPressed || cursorIsTouched)
	{
		cursorIsDraggedBy = true;
		cursorDraggedBy = cursorMovedBy;
	}
	else
	{
		cursorIsDraggedBy = false;
		cursorDraggedBy = Vector2::zero;
	}	
}

void Display::advance_sounds(float _deltaTime)
{
	soundSources.advance(_deltaTime);
}

bool Display::is_updated_continuously() const
{
	int diff = updatedDisplayOnFrame - advancedOnFrame;
	return diff >= 0 && diff <= 2;
}

void Display::resume()
{
	cursorIsActive = s_cursorIsActive;
}

void Display::advance(float _deltaTime)
{
#ifdef AN_DEVELOPMENT
	if (::System::Core::get_delta_time() == 0.0f)
	{
		return;
	}
#endif

	advancedOnFrame = ::System::Core::get_frame();

	__doing_advancement_now();

	perform_func_info<OnAdvanceDisplay>(on_advance_display, [this, _deltaTime](OnAdvanceDisplay _on_advance_display){_on_advance_display(this, _deltaTime); });

	if (!wasInputProcessed)
	{
		process_input(Framework::GameInputProxy(), _deltaTime); // empty input
	}

	wasInputProcessed = false;

	s_cursorIsActive = cursorIsActive; // store to be used when we have two displays and we switch

	if (state == DisplayState::On)
	{
		powerOn = blend_to_using_time(powerOn, 1.0f, setup.powerTurnOnBlendTime, _deltaTime);
		scaleDisplay = blend_to_using_time(scaleDisplay, Vector2::one, setup.turnOnScaleDisplayBlendTime.x, _deltaTime);
		scaleDisplay = blend_to_using_time(scaleDisplay, Vector2::one, setup.turnOnScaleDisplayBlendTime.y, _deltaTime);
	}
	else
	{
		powerOn = blend_to_using_time(powerOn, 0.0f, setup.powerTurnOffBlendTime, _deltaTime);
		if (setup.turnOffScaleDisplayBlendTime.x > 0.0f)
		{
			scaleDisplay.x = blend_to_using_time(scaleDisplay.x, 0.0f, setup.turnOffScaleDisplayBlendTime.x, _deltaTime);
		}
		if (setup.turnOffScaleDisplayBlendTime.y > 0.0f)
		{
			scaleDisplay.y = blend_to_using_time(scaleDisplay.y, 0.0f, setup.turnOffScaleDisplayBlendTime.y, _deltaTime);
		}
	}
	powerOnLightenUpCoef = 1.0f / (max(0.05f, scaleDisplay.x) * max(0.01f, scaleDisplay.y));
	visiblePowerOn = powerOn * powerOnLightenUpCoef;

	if (!ignoreGameFrames)
	{
		int const gameFrameNo = Game::get()->get_frame_no();
		if (gameFrameNo - lastAdvanceGameFrameNo > 1)
		{
			doAllDrawCycles = true;
			doAllDrawCyclesGathered = true;
		}
		lastAdvanceGameFrameNo = gameFrameNo;
	}

	//

	// keep cursor in sane values

	cursorAt.x = clamp(cursorAt.x, 0.0f, (float)(setup.screenResolution.x - 1));
	cursorAt.y = clamp(cursorAt.y, 0.0f, (float)(setup.screenResolution.y - 1));

	//

	if (setup.refreshFrequency > 0.0f)
	{
		retraceAtPct += setup.refreshFrequency * _deltaTime;

		float retraceAtPctNotModed = retraceAtPct;
		retraceAtPct = mod(retraceAtPct, 1.0f);
		int runCount = int(retraceAtPctNotModed - retraceAtPct + 0.1f); // 0.1f to eat any rounding error

		if (runCount > 0)
		{
			doNextFrame = true;
			accumulatedDrawCycles += max(1, (int)((float)(runCount * setup.drawCyclesPerSecond) / setup.refreshFrequency));
		}
	}
	else
	{
		retraceAtPct = 1.0f;
		doNextFrame = true;
		accumulatedDrawCycles += max(1, (int)((float)setup.drawCyclesPerSecond * _deltaTime));
	}

	if (setup.retraceVisibleFrequency > 0.0f)
	{
		retraceVisibleAtPct = mod(retraceVisibleAtPct + setup.retraceVisibleFrequency * _deltaTime, 1.0f);
	}
	else if (setup.retraceVisibleFrequency == 0.0f)
	{
		retraceVisibleAtPct = 1.0f;
	}
	else
	{
		retraceVisibleAtPct = retraceAtPct;
	}

	if (activeControls.get_size())
	{
		// update active buttons, work on a copy and check if they are still there
		ARRAY_STACK(ActiveDisplayControl, activeControlsCopy, activeControls.get_size());
		activeControlsCopy = activeControls;
		for_every(activeControl, activeControlsCopy)
		{
			bool exists = false;
			for_every(activeControlOrg, activeControls)
			{
				if (activeControlOrg->control == activeControl->control)
				{
					exists = true;
					break;
				}
			}
			if (!exists)
			{
				continue;
			}
			float prevTime = activeControl->activeTime - repeatActionFirstTime;
			activeControl->activeTime += _deltaTime;
			float currTime = activeControl->activeTime - repeatActionFirstTime;
			if (activeControl->control->is_continuous() &&
				currTime >= 0.0f)
			{
				if (prevTime < 0.0f)
				{
					an_assert(has_control(activeControl->control));
					if (!activeControl->control->is_locked())
					{
						activeControl->control->process_activate(this);
					}
				}
				else
				{
					float const prevTimeMod = mod(prevTime, repeatActionTime);
					float const currTimeMod = mod(currTime, repeatActionTime);
					if (currTimeMod < prevTimeMod)
					{
						an_assert(has_control(activeControl->control));
						if (!activeControl->control->is_locked())
						{
							activeControl->control->process_activate(this);
						}
					}
				}
			}
			for_every(activeControlOrg, activeControls)
			{
				if (activeControlOrg->control == activeControl->control)
				{
					*activeControlOrg = *activeControl;
					break;
				}
			}
		}
	}

	__done_advancement_now();
}

void Display::add(DisplayControl* _control)
{
	RefCountObjectPtr<DisplayControl> keepControl(_control);
	if (_control->get_inside_display())
	{
		if (_control->get_inside_display() == this)
		{
			// we're already added here!
			return;
		}
		_control->get_inside_display()->remove(_control);
	}
	Concurrency::ScopedSpinLock cLock(controlsLock);
	controls.push_back(RefCountObjectPtr<DisplayControl>(_control));
	_control->set_controls_stack_level(controlsStackLevel);
	_control->on_added_to(this);
}

void Display::remove(DisplayControl* _control, bool _noRedrawNeeded, bool _dontSelectAntyhing)
{
	if (_control == nullptr)
	{
		return;
	}
	if (! has_control(_control))
	{
		return;
	}
	if (cursorActiveControl.get() == _control)
	{
		cursorActiveControl = nullptr;
	}
	if (cursorHoverControl.get() == _control)
	{
		cursorHoverControl = nullptr;
	}
	if (preShortcutControl.get() == _control)
	{
		preShortcutControl = nullptr;
	}
	_control->on_removed_from(this, _noRedrawNeeded);
	{
		Concurrency::ScopedSpinLock cLock(controlsLock);
		remove_active_control(_control);
	}
	if (!_dontSelectAntyhing)
	{
		if (selectedControl.is_set() && selectedControl.get() == _control)
		{
			if (!is_cursor_active())
			{
				select_anything(_control);
			}
			else
			{
				select(nullptr, NP, NP, true);
			}
		}
	}
	todo_future(TXT("find and remove by index"));
	{
		Concurrency::ScopedSpinLock cLock(controlsLock);
		controls.remove(RefCountObjectPtr<DisplayControl>(_control));
	}
}

DisplayDrawCommand* Display::add(DisplayDrawCommand* _drawCommand)
{
	drawCommandsGatherLock.acquire();
	drawCommandsGather.push_back(DisplayDrawCommandPtr(_drawCommand));
#ifdef AN_DEVELOPMENT
	if (debugOutput)
	{
		output(TXT("[display] added draw command to %p, has %i"), this, drawCommandsGather.get_size());
	}
#endif
	drawCommandsGatherLock.release();
	_drawCommand->prepare(this);
	return _drawCommand;
}

void Display::move_gathered_draw_commands_to_drawing()
{
	drawCommandsGatherLock.acquire();
	drawCommandsLock.acquire();
	if (gatheredDropAllDrawCommands)
	{
		drawCommands.clear();
		gatheredDropAllDrawCommands = false;
	}
	for_every(gathered, drawCommandsGather)
	{
		drawCommands.push_back(*gathered);
	}
	doAllDrawCycles = doAllDrawCyclesGathered;
	doAllDrawCyclesGathered = false;
#ifdef AN_DEVELOPMENT
	if (debugOutput)
	{
		output(TXT("[display] moved gathered draw commands to to-be-drawn for %p, %i -> %i"), this, drawCommandsGather.get_size(), drawCommands.get_size());
	}
#endif
	drawCommandsLock.release();
	drawCommandsGather.clear();
	drawCommandsGatherLock.release();
}

void Display::drop_all_draw_commands(bool _addHoldDrawing)
{
	drawCommandsGatherLock.acquire();
	drawCommandsGather.clear();
	drawCommandsGatherLock.release();
	gatheredDropAllDrawCommands = true; // will drop actual draw commands on next gather->drawing
	if (_addHoldDrawing)
	{
		add(new DisplayDrawCommands::HoldDrawing());
	}
}

bool Display::has_control(DisplayControl* _control) const
{
	Concurrency::ScopedSpinLock cLock(controlsLock);
	for_every(control, controls)
	{
		if (control->get() == _control)
		{
			return true;
		}
	}

	return false;
}

void Display::remove_all_controls(bool _noRedrawNeeded)
{
	select(nullptr, NP, NP, true);

	clear_active_controls();

	Concurrency::ScopedSpinLock cLock(controlsLock);
	ARRAY_STACK(RefCountObjectPtr<DisplayControl>, controlsToRemove, controls.get_size());
	controlsToRemove = controls;
	for_every(control, controlsToRemove)
	{
		if (cursorActiveControl.get() == control->get())
		{
			cursorActiveControl = nullptr;
		}
		if (cursorHoverControl.get() == control->get())
		{
			cursorHoverControl = nullptr;
		}
		if (preShortcutControl.get() == control->get())
		{
			preShortcutControl = nullptr;
		}
		controls.remove(RefCountObjectPtr<DisplayControl>(control->get()));
		control->get()->on_removed_from(this, _noRedrawNeeded);
	}
	controls.clear();

	controlsLockLevel = NONE;
	controlsStackLevel = NONE;
}

DisplayControl* Display::find_control(Name const & _id) const
{
	Concurrency::ScopedSpinLock cLock(controlsLock);
	for_every(control, controls)
	{
		if (control->get()->get_id() == _id)
		{
			return control->get();
		}
	}
	return nullptr;
}

void Display::redraw_controls(bool _clear)
{
	for_every_ref(control, controls)
	{
		control->redraw(this, _clear);
	}
}

void Display::select_by_id(Name const & _id)
{
	if (DisplayControl* control = find_control(_id))
	{
		select(control);
	}
}

void Display::select_anything(DisplayControl* _exceptControl)
{
	if (touchingIsActive)
	{
		return;
	}
	cursorActiveControl = nullptr;
	cursorHoverControl = nullptr;
	for_every(control, controls)
	{
		if (control->get()->is_selectable() &&
			control->get() != _exceptControl)
		{
			if (fast_cast<DisplayButton>(control->get()))
			{
				select(control->get(), NP, NP, true);
				return;
			}
		}
	}
	for_every(control, controls)
	{
		if (control->get()->is_selectable() &&
			control->get() != _exceptControl)
		{
			select(control->get(), NP, NP, true);
			return;
		}
	}
	select(nullptr, NP, NP, true);
}

void Display::use_cursor(Name const & _cursorName)
{
	cursorName = _cursorName;

	currentCursor = &setup.cursor;
	if (DisplayCursor * found = setup.namedCursors.get_existing(cursorName))
	{
		currentCursor = found;
	}
}

void Display::use_cursor_not_touched(Name const & _cursorName)
{
	cursorNameNotTouching = _cursorName;

	currentCursorNotTouching = nullptr; // none by default
	if (DisplayCursor * found = setup.namedCursors.get_existing(cursorNameNotTouching))
	{
		currentCursorNotTouching = found;
	}
}

void Display::lock_cursor_for_drawing(Name const & _cursorNameUntilDrawn, Name const & _cursorNameAfterDrawn)
{
	cursorNameUntilDrawn = _cursorNameUntilDrawn;
	cursorNameAfterDrawn = _cursorNameAfterDrawn;

	use_cursor(cursorNameUntilDrawn);

	if (!cursorIsActive)
	{
		// show in centre
		cursorAt = setup.screenResolution.to_vector2() * 0.5f;
	}
}

void Display::update_display()
{
	updatedDisplayOnFrame = ::System::Core::get_frame();

	bool drawCommandsImmediately = alwaysDrawCommandsImmediately || setup.drawCyclesPerSecond < 0;

	if (!ignoreGameFrames)
	{
		if (lastUpdateDisplayGameFrameNo == lastAdvanceGameFrameNo && !doAllDrawCycles && !drawCommandsImmediately)
		{
			return;
		}
		lastUpdateDisplayGameFrameNo = lastAdvanceGameFrameNo;
	}

	MEASURE_PERFORMANCE(display_updateDisplay);

	assert_rendering_thread();

	::System::Video3D* v3d = ::System::Video3D::get();

#ifdef AN_DEVELOPMENT
	if (debugOutput)
	{
		output(TXT("[display] update %p"), this);
	}
#endif
	perform_func_info<OnUpdateDisplay>(on_update_display, [this](OnUpdateDisplay _on_update_display) {_on_update_display(this); });

	// move here as we could add something during update
	move_gathered_draw_commands_to_drawing();

	if (doNextFrame || doAllDrawCycles || drawCommandsImmediately)
	{
#ifdef AN_DEVELOPMENT
		if (debugOutput)
		{
			output(TXT("[display] frame (%i)"), drawCommands.get_size());
		}
#endif
		MEASURE_PERFORMANCE(display_updateDisplay_render);

		__doing_rendering_now();
		doNextFrame = false;
		currentFrame->resolve_forced_unbind(); // just to make sure it is resolved as we will be swapping it into previous
		next_frame();

		if (!drawCommands.is_empty() || ! on_render_display.is_empty())
		{
			v3d->set_viewport(VectorInt2::zero, currentFrame->get_size());
			v3d->set_near_far_plane(0.02f, 100.0f);

			v3d->set_2d_projection_matrix_ortho();
			v3d->access_model_view_matrix_stack().clear();

			v3d->setup_for_2d_display();

			currentFrame->bind();
			drawCommandsLock.acquire();
			DisplayDrawContext drawContext;
			while ((accumulatedDrawCycles > 0 || doAllDrawCycles || drawCommandsImmediately) && !drawCommands.is_empty())
			{
#ifdef AN_DEVELOPMENT
				if (debugOutput)
				{
					output(TXT("[display] draw command (%i)"), drawCommands.get_size());
				}
#endif
				if (doAllDrawCycles || drawCommandsImmediately)
				{
					accumulatedDrawCycles = 9999999; // should be enough
				}
				DisplayDrawCommand* drawCommand = drawCommands.get_first().get();
				auto * colourReplacer = drawCommand->get_colour_replacer();
				if (colourReplacer)
				{
					drawContext.push_colour_replacer(colourReplacer);
				}
				if (drawCommand->should_draw_immediately() || drawCommandsImmediately)
				{
					int tempDrawCyclesUsedByCurrentDrawCommand = 0; // whole
					int tempAvailableDrawCycles = 9999999; // should be enough;
					drawCommand->draw_onto(this, v3d, REF_ tempDrawCyclesUsedByCurrentDrawCommand, REF_ tempAvailableDrawCycles, drawContext);
				}
				else if (!drawCommand->draw_onto(this, v3d, REF_ drawCyclesUsedByCurrentDrawCommand, REF_ accumulatedDrawCycles, drawContext))
				{
					// wait for next frame to finish it
					if (!drawCommandsImmediately)
					{
#ifdef AN_DEVELOPMENT
						if (debugOutput)
						{
							output(TXT("[display] interrupted"));
						}
#endif
						break;
					}
				}
				if (colourReplacer)
				{
					drawContext.pop_colour_replacer();
				}
				if (accumulatedDrawCycles == 0)
				{
					// broken on purpose
					doAllDrawCycles = false;
				}
				// jump to next command
				drawCommands.remove(DisplayDrawCommandPtr(drawCommand));
				drawCyclesUsedByCurrentDrawCommand = 0;
			}

			// we don't want to accumulate draw cycles that we can't use, because we have nothing to do
			if (drawCommands.get_size() == 0)
			{
				accumulatedDrawCycles = 0;
			}
			drawCommandsLock.release();

			perform_func_info<OnRenderDisplay>(on_render_display, [this](OnRenderDisplay _on_render_display) {_on_render_display(this); });

			currentFrame->unbind();
		}

		if (cursorNameUntilDrawn.is_valid() && drawCommands.is_empty())
		{
			use_cursor(cursorNameAfterDrawn);
			cursorNameUntilDrawn = Name::invalid();
			cursorNameAfterDrawn = Name::invalid();
		}
		currentFrame->resolve(); // resolve here as we could do only next frame or do whole drawing

		doAllDrawCycles = false;
		__done_rendering_now();
	}

	// do post process
	if (is_visible())
	{
#ifdef AN_LOG_POST_PROCESS
		if (PostProcessDebugSettings::log)
		{
			output(TXT("post process for display %i================================="), this);
		}
#endif

		if (rotateDisplay)
		{
			::System::Video3D* v3d = ::System::Video3D::get();

			v3d->set_viewport(VectorInt2::zero, currentFrame->get_size());
			v3d->set_near_far_plane(0.02f, 100.0f);

			v3d->set_2d_projection_matrix_ortho();
			v3d->access_model_view_matrix_stack().clear();

			v3d->setup_for_2d_display();

			{
				Matrix44 trans = Matrix44::identity;
				trans.m30 = currentFrame->get_size().to_vector2().x * 0.5f;
				trans.m31 = currentFrame->get_size().to_vector2().y * 0.5f;
				v3d->access_model_view_matrix_stack().push_to_world(trans);
			}
			{
				Matrix44 rot = Matrix44::identity;
				if (rotateDisplay == 1)
				{
					rot.m00 = 0.0f;
					rot.m01 = -1.0f;
					rot.m10 = 1.0f;
					rot.m11 = 0.0f;
					rot.m30 = 2.0f;
				}
				else if (rotateDisplay == -1)
				{
					rot.m00 = 0.0f;
					rot.m01 = 1.0f;
					rot.m10 = -1.0f;
					rot.m11 = 0.0f;
					rot.m30 = 2.0f;
				}
				else
				{
					rot.m00 = -1.0f;
					rot.m01 = 0.0f;
					rot.m10 = 0.0f;
					rot.m11 = -1.0f;
				}
				v3d->access_model_view_matrix_stack().push_to_world(rot);
			}
			{
				Matrix44 trans = Matrix44::identity;
				trans.m30 = -currentFrame->get_size().to_vector2().x * 0.5f;
				trans.m31 = -currentFrame->get_size().to_vector2().y * 0.5f;
				v3d->access_model_view_matrix_stack().push_to_world(trans);
			}
			currentRotatedFrame->bind();
			currentFrame->render(0, ::System::Video3D::get(), Vector2::zero, currentFrame->get_size().to_vector2());
			currentRotatedFrame->unbind();
			currentRotatedFrame->resolve();
			previousRotatedFrame->bind();
			previousFrame->render(0, ::System::Video3D::get(), Vector2::zero, previousFrame->get_size().to_vector2());
			previousRotatedFrame->unbind();
			previousRotatedFrame->resolve();
			v3d->access_model_view_matrix_stack().pop();
			v3d->access_model_view_matrix_stack().pop();
			v3d->access_model_view_matrix_stack().pop();
		}

		v3d->setup_for_2d_display();

		if (postProcessInstance)
		{
			MEASURE_PERFORMANCE(display_updateDisplay_postProcess);
#ifdef AN_LOG_POST_PROCESS
			if (PostProcessDebugSettings::log)
			{
				output(TXT(" render targets from last frame"));
			}
#endif
			for_every(ppLastFrameOutputRenderTarget, ppLastFrameOutputRenderTargets)
			{
				if (ppLastFrameOutputRenderTarget->get_render_target())
				{
					ppLastFrameOutputRenderTarget->get_render_target()->resolve(); // make sure we provide resolved render targets
					postProcessRenderTargetsProvider.set(for_everys_map_key(ppLastFrameOutputRenderTarget), ppLastFrameOutputRenderTarget->get_render_target(), ppLastFrameOutputRenderTarget->get_data_texture_index());
#ifdef AN_LOG_POST_PROCESS
					if (PostProcessDebugSettings::log)
					{
						output(TXT(" +- %S : %i [%i]"), for_everys_map_key(ppLastFrameOutputRenderTarget).to_char(), ppLastFrameOutputRenderTarget->get_render_target(), ppLastFrameOutputRenderTarget->get_data_texture_index());
					}
#endif
				}
			}

			PostProcessGraphProcessingContext processingContext;
			processingContext.inputRenderTargetsProvider = &postProcessRenderTargetsProvider; // it should be already set up with next_frame() call
			processingContext.resolutionProvider = &postProcessResolutionProvider;
			processingContext.renderTargetManager = postProcessRenderTargetManager;
			processingContext.withMipMaps = true;
			// no camera
			processingContext.video3D = v3d;

			if (auto * ppGraphInstance = postProcessInstance->get_graph_instance())
			{
				if (backgroundRenderTarget.is_set() &&
					!backgroundRenderTarget->is_resolved())
				{
					backgroundRenderTarget->resolve();
				}

				ppOutputRenderTarget.clear(); // no longer needed
				todo_note(TXT("store all shader program instances that have this uniform in initialise and go through that list here"));
				ppGraphInstance->for_every_shader_program_instance(
					[this](PostProcessNode const * _node, ::System::ShaderProgramInstance & _shaderProgramInstance)
					{
						DisplayCursor const * cursor = currentCursor;
						bool useColours = cursorUseColours.is_set() ? cursorUseColours.get() :(cursor ? !cursor->useOwnColours : colourise);
						if (!cursorNameUntilDrawn.is_valid() && cursorNotTouching && !showCursorAlwaysTouching)
						{
							cursor = currentCursorNotTouching;
						}
						Texture const * cursorTexture = cursor ? cursor->texture.get() : nullptr;
						if (cursorTexture)
						{
							Vector2 wdr = setup.wholeDisplayResolution.to_vector2();
							_shaderProgramInstance.set_uniform(NAME(inCursor), cursorTexture->get_texture()->get_texture_id());
							VectorInt2 cursorAtCorrected = cursorAt.to_vector_int2_cells();
							cursorAtCorrected -= cursor->pointAt;
							bool cursorIsVisibleNow = is_cursor_visible();
							_shaderProgramInstance.set_uniform(NAME(cursorPlacement), (cursorAtCorrected + (setup.wholeDisplayResolution - setup.screenResolution) * 0.5f).to_vector2() / wdr);
							_shaderProgramInstance.set_uniform(NAME(cursorTransform), wdr / cursorTexture->get_texture()->get_size().to_vector2());
							_shaderProgramInstance.set_uniform(NAME(cursorVisible), cursorIsVisibleNow ? 1.0f : 0.0f);
							_shaderProgramInstance.set_uniform(NAME(cursorInk), (cursorInk == Colour::none? currentInk : cursorInk).to_vector4());
							_shaderProgramInstance.set_uniform(NAME(cursorPaper), (cursorPaper == Colour::none ? currentPaper : cursorPaper).to_vector4());
							_shaderProgramInstance.set_uniform(NAME(cursorUseColours), cursorIsVisibleNow && useColours ? 1.0f : 0.0f);
						}
						else
						{
							_shaderProgramInstance.set_uniform(NAME(inCursor), ::System::texture_id_none());
							_shaderProgramInstance.set_uniform(NAME(cursorPlacement), Vector2::zero);
							_shaderProgramInstance.set_uniform(NAME(cursorTransform), Vector2::zero);
							_shaderProgramInstance.set_uniform(NAME(cursorVisible), 0.0f);
							_shaderProgramInstance.set_uniform(NAME(cursorUseColours), 0.0f);
						}
						_shaderProgramInstance.set_uniform(NAME(retraceAtY), 1.0f - retraceAtPct);
						_shaderProgramInstance.set_uniform(NAME(retraceVisibleAtY), 1.0f - retraceVisibleAtPct);
						_shaderProgramInstance.set_uniform(NAME(noiseTextureCurrentOffset), noiseTextureCurrentOffset);
						_shaderProgramInstance.set_uniform(NAME(noiseTexturePreviousOffset), noiseTexturePreviousOffset);
						_shaderProgramInstance.set_uniform(NAME(displayIntensity), visiblePowerOn);
						_shaderProgramInstance.set_uniform(NAME(displayIntensityColourUse), get_setup().renderDisplaySrcBlendOp == ::System::BlendOp::One? 1.0f : 0.0f); // if we fully blend in, use intenstity for colour, otherwise use it only for alpha
						_shaderProgramInstance.set_uniform(NAME(additionalDisplayIntensity), 0.1f * max(0.0f, (powerOnLightenUpCoef * 0.4f - 1.0f)));
						_shaderProgramInstance.set_uniform(NAME(displayPixelSize), setup.wholeDisplayResolution.to_vector2().inverted() * setup.pixelScale);
						_shaderProgramInstance.set_uniform(NAME(displayPixelResolution), setup.wholeDisplayResolution.to_vector2() / setup.pixelScale);
						_shaderProgramInstance.set_uniform(NAME(scaleDisplayUV), Vector2(1.0f / max(6.0f / (float)setup.outputSize.x, scaleDisplay.x), 1.0f / max(6.0f / (float)setup.outputSize.y, scaleDisplay.y)));
						{
							if (backgroundRenderTarget.is_set())
							{
								an_assert(backgroundRenderTarget->is_resolved());
							}
							_shaderProgramInstance.set_uniform(NAME(inBackground), backgroundTexture);
							_shaderProgramInstance.set_uniform(NAME(backgroundColourBased), backgroundTexture? backgroundColourBased : 0.0f);
							_shaderProgramInstance.set_uniform(NAME(useBackground), backgroundTexture ? 1.0f : 0.0f);
							Vector2 backgroundLeftBottom = leftBottomOfDisplay;
							Vector2 backgroundSize = rightTopOfDisplay + Vector2::one - backgroundLeftBottom;
							if (backgroundFillMode == DisplayBackgroundFillMode::Screen)
							{
								backgroundLeftBottom = leftBottomOfScreen;
								backgroundSize = screenSize;
							}
							else if (backgroundFillMode == DisplayBackgroundFillMode::ScreenRect)
							{
								backgroundLeftBottom = backgroundAtScreenLocation.to_vector2();
								backgroundSize = backgroundAtScreenSize.to_vector2();
							}
							Vector2 wdr = setup.wholeDisplayResolution.to_vector2();
							// y = ax + b
							// ->
							// b = y0 - a x0
							// b = y1 - a x1
							// y0 - a x0 = y1 - a x1
							// a (x0 - x1) = y1 - y0
							// a = (y1 - y0) / (x0 - x1)
							// b = y0 - x0 (y1 - y0) / (x0 - x1)
							// as x0 is 0 (we will modify left bottom and size instead we end up with b = y0 but it should be normalised
							_shaderProgramInstance.set_uniform(NAME(backgroundTransform), wdr / backgroundSize); // a
							_shaderProgramInstance.set_uniform(NAME(backgroundPlacement), backgroundLeftBottom / wdr); // b
						}
						return true;
					}
					);

				ppGraphInstance->do_post_process(processingContext);

#ifdef AN_LOG_POST_PROCESS
				if (PostProcessDebugSettings::log)
				{
					output(TXT(" this frame output"));
				}
#endif
				// store outputs
				if (auto * outputNode = ppGraphInstance->get_output_node())
				{
					for_count(int, idx, ppGraphInstance->get_output_node()->all_input_connectors().get_size())
					{
						Name outputRenderTargetName;
						PostProcessRenderTargetPointer temp = ppGraphInstance->get_output_render_target(idx, &outputRenderTargetName);
						ppLastFrameOutputRenderTargets[outputRenderTargetName] = temp;
#ifdef AN_LOG_POST_PROCESS
						if (PostProcessDebugSettings::log)
						{
							output(TXT(" +- %S : %i [%i]"), outputRenderTargetName.to_char(), temp.get_render_target(), temp.get_data_texture_index());
						}
#endif
					}
				}

				// store output render target and release other render targets afterwards
				ppOutputRenderTarget = ppGraphInstance->get_output_render_target();
				outputRenderTarget = ppOutputRenderTarget.get_render_target();
				ppGraphInstance->release_render_targets(); // release all previous render targets
			}
		}
		else
		{
			todo_note(TXT("screen tearing not implemented here!"));
			v3d->set_viewport(VectorInt2::zero, outputRenderTarget->get_size());
			v3d->set_near_far_plane(0.02f, 100.0f);

			v3d->set_2d_projection_matrix_ortho();
			v3d->access_model_view_matrix_stack().clear();

			v3d->setup_for_2d_display();

			outputRenderTarget->bind();
			currentFrame->render(0, v3d, Vector2::zero, outputRenderTarget->get_size().to_vector2());
			outputRenderTarget->unbind();
		}

		{
			MEASURE_PERFORMANCE(display_updateDisplay_resolve);
			outputRenderTarget->resolve();
		}

#ifdef USE_DISPLAY_CASING_POST_PROCESS
		if (postProcessForCasingInstance)
		{
			MEASURE_PERFORMANCE(display_updateDisplay_postProcess_forCasing);

			// use output render target as screen to blur etc
			postProcessRenderTargetsProviderForCasing.set(NAME(screen), outputRenderTarget.get(), 0);

			PostProcessGraphProcessingContext processingContext;
			processingContext.inputRenderTargetsProvider = &postProcessRenderTargetsProviderForCasing;
			processingContext.resolutionProvider = &postProcessResolutionProvider; // might be not used at all
			processingContext.renderTargetManager = postProcessRenderTargetManager;
			processingContext.withMipMaps = true;
			// no camera
			processingContext.video3D = v3d;

			if (auto * ppGraphInstance = postProcessForCasingInstance->get_graph_instance())
			{
				ppOutputForCasingRenderTarget.clear(); // no longer needed
				// no need to setup shader program instances here

				ppGraphInstance->do_post_process(processingContext);

				// store output render target and release other render targets afterwards
				ppOutputForCasingRenderTarget = ppGraphInstance->get_output_render_target();
				ppGraphInstance->release_render_targets(); // release all previous render targets

				ppOutputForCasingRenderTarget.get_render_target()->resolve();
			}
		}
#endif
	}
}

void Display::render_2d(::System::Video3D* _v3d, Vector2 const & _centreAt, Optional<Vector2> const& _size, float _angle, ::System::BlendOp::Type _srcBlend, ::System::BlendOp::Type _destBlend)
{
#ifdef USE_DISPLAY_MESH
	if (is_visible())
	{
		if (auto* displayRT = get_output_render_target())
		{
			an_assert(displayRT->is_resolved());
#ifdef USE_DISPLAY_CASING_POST_PROCESS
			auto* casingRT = ppOutputForCasingRenderTarget.get_render_target();
			an_assert(! casingRT || casingRT->is_resolved());
#endif
			if (deviceMesh2D)
			{
				_v3d->access_model_view_matrix_stack().push_to_world(translation_matrix(Vector3(_centreAt.x, _centreAt.y, 0.0f)));
				_v3d->access_model_view_matrix_stack().push_to_world(rotation_xy_matrix(_angle));
				if (_size.is_set())
				{
					Vector2 useSize = _size.get();
					useSize.x = deviceMesh2DSize.x * useSize.y / deviceMesh2DSize.y;
					if (useSize.x > _size.get().x)
					{
						useSize.y *= _size.get().x / useSize.x;
						useSize.x = _size.get().x;
					}

					float scale = useSize.y / deviceMesh2DSize.y;
					_v3d->access_model_view_matrix_stack().push_to_world(scale_matrix(scale));
				}

				if (::System::MaterialInstance * mi = deviceMesh2DInstance.get_material_instance_for_rendering(0))
				{
					mi->set_uniform(NAME(inDisplay), displayRT->get_data_texture_id(0));
					mi->set_uniform(NAME(inDisplaySize), get_setup().wholeDisplayResolution.to_vector2());
					mi->set_uniform(NAME(inDisplayTexelSize), get_setup().wholeDisplayResolution.to_vector2().inverted());
#ifdef USE_DISPLAY_SHADOW_MASK
					if (setup.shadowMask.is_set())
					{
						mi->set_uniform(NAME(inShadowMask), setup.shadowMask->get_texture()->get_texture_id(), ::System::MaterialSetting::Forced);
						mi->set_uniform(NAME(shadowMaskSize), get_setup().wholeDisplayResolution.to_vector2() * setup.shadowMaskPerPixel, ::System::MaterialSetting::Forced);
						mi->set_uniform(NAME(shadowMaskBaseLevelInv), 1.0f / get_setup().shadowMaskBaseLevel, ::System::MaterialSetting::Forced);
					}
#endif
#ifdef USE_DISPLAY_CASING_POST_PROCESS
					mi->set_uniform(NAME(inCasing), casingRT? casingRT->get_data_texture_id(0) : displayRT->get_data_texture_id(0));
#endif
				}
				deviceMesh2DInstance.set_blend(_srcBlend, _destBlend);
				deviceMesh2DInstance.render(_v3d, Meshes::Mesh3DRenderingContext());

				_v3d->access_model_view_matrix_stack().pop();
				_v3d->access_model_view_matrix_stack().pop();
				_v3d->access_model_view_matrix_stack().pop();
			}
			else
			{
				displayRT->render(0, _v3d, _centreAt - displayRT->get_size().to_vector2() * 0.5f, displayRT->get_size().to_vector2());
			}
		}
	}
#else
	error(TXT("enable USE_DISPLAY_MESH to allow using meshes"));
#endif
}

void Display::render_3d(::System::Video3D* _v3d, Transform const & _placement, ::System::BlendOp::Type _srcBlend, ::System::BlendOp::Type _destBlend)
{
#ifdef USE_DISPLAY_MESH
	if (is_visible())
	{
		if (auto* displayRT = get_output_render_target())
		{
			an_assert(useMeshes);
			an_assert(displayRT->is_resolved());
#ifdef USE_DISPLAY_CASING_POST_PROCESS
			auto* casingRT = ppOutputForCasingRenderTarget.get_render_target();
			an_assert(!casingRT || casingRT->is_resolved());
#endif
			if (deviceMesh3D)
			{
				_v3d->access_model_view_matrix_stack().push_to_world(_placement.to_matrix());

				if (::System::MaterialInstance * mi = deviceMesh3DInstance.get_material_instance_for_rendering(0))
				{
					mi->set_uniform(NAME(inDisplay), displayRT->get_data_texture_id(0));
					mi->set_uniform(NAME(inDisplaySize), get_setup().wholeDisplayResolution.to_vector2());
					mi->set_uniform(NAME(inDisplayTexelSize), Vector2::one / get_setup().wholeDisplayResolution.to_vector2());
#ifdef USE_DISPLAY_SHADOW_MASK
					if (setup.shadowMask.is_set())
					{
						mi->set_uniform(NAME(inShadowMask), setup.shadowMask->get_texture()->get_texture_id(), ::System::MaterialSetting::Forced);
						mi->set_uniform(NAME(shadowMaskSize), get_setup().wholeDisplayResolution.to_vector2() * setup.shadowMaskPerPixel, ::System::MaterialSetting::Forced);
						mi->set_uniform(NAME(shadowMaskBaseLevelInv), 1.0f / get_setup().shadowMaskBaseLevel, ::System::MaterialSetting::Forced);
					}
#endif
#ifdef USE_DISPLAY_CASING_POST_PROCESS
					mi->set_uniform(NAME(inCasing), casingRT ? casingRT->get_data_texture_id(0) : displayRT->get_data_texture_id(0));
#endif
				}
				deviceMesh3DInstance.set_blend(_srcBlend, _destBlend);
				deviceMesh3DInstance.render(_v3d, Meshes::Mesh3DRenderingContext());

				_v3d->access_model_view_matrix_stack().pop();
			}
		}
	}
#else
	error(TXT("enable USE_DISPLAY_MESH to allow using meshes"));
#endif
}

void Display::ready_device_mesh_3d_instance_to_render()
{
#ifdef USE_DISPLAY_MESH
	an_assert(useMeshes);
	ready_mesh_3d_instance_to_render(deviceMesh3DInstance);
#else
	error(TXT("enable USE_DISPLAY_MESH to allow using meshes"));
#endif
}

void Display::ready_mesh_3d_instance_to_render(Meshes::Mesh3DInstance & _meshInstance)
{
	if (auto* displayRT = get_output_render_target())
	{
		an_assert(displayRT->is_resolved());
#ifdef USE_DISPLAY_CASING_POST_PROCESS
		auto* casingRT = ppOutputForCasingRenderTarget.get_render_target();
		an_assert(!casingRT || casingRT->is_resolved());
#endif
		if (::System::MaterialInstance * mi = _meshInstance.get_material_instance_for_rendering(0))
		{
			mi->set_uniform(NAME(inDisplay), displayRT->get_data_texture_id(0));
			mi->set_uniform(NAME(inDisplaySize), get_setup().wholeDisplayResolution.to_vector2());
			mi->set_uniform(NAME(inDisplayTexelSize), Vector2::one / get_setup().wholeDisplayResolution.to_vector2());
#ifdef USE_DISPLAY_SHADOW_MASK
			if (setup.shadowMask.is_set())
			{
				mi->set_uniform(NAME(inShadowMask), setup.shadowMask->get_texture()->get_texture_id(), ::System::MaterialSetting::Forced);
				mi->set_uniform(NAME(shadowMaskSize), get_setup().wholeDisplayResolution.to_vector2() * setup.shadowMaskPerPixel, ::System::MaterialSetting::Forced);
				mi->set_uniform(NAME(shadowMaskBaseLevelInv), 1.0f / get_setup().shadowMaskBaseLevel, ::System::MaterialSetting::Forced);
			}
#endif
#ifdef USE_DISPLAY_CASING_POST_PROCESS
			mi->set_uniform(NAME(inCasing), casingRT ? casingRT->get_data_texture_id(0) : displayRT->get_data_texture_id(0));
#endif
		}
	}
}

float Display::calc_device_mesh_3d_scale(Vector2 const & _desiredMinSize, Vector2 const & _desiredMaxSize)
{
#ifdef USE_DISPLAY_MESH
	if (deviceMesh3DSize.is_zero())
	{
		return 1.0f;
	}

	float scale = max(_desiredMinSize.x / deviceMesh3DSize.x, _desiredMinSize.y / deviceMesh3DSize.y);
	if (!_desiredMaxSize.is_zero())
	{
		scale = max(scale, min(_desiredMaxSize.x / deviceMesh3DSize.x, _desiredMaxSize.y / deviceMesh3DSize.y));
	}

	return scale;
#else
	error(TXT("enable USE_DISPLAY_MESH to allow using meshes"));
	return 1.0f;
#endif
}

void Display::render_vr_scene(::System::Video3D* _v3d, DisplayVRSceneContext* _context, ::System::BlendOp::Type _srcBlend, ::System::BlendOp::Type _destBlend)
{
#ifdef USE_DISPLAY_MESH
	an_assert(useMeshes);

	auto* vr = VR::IVR::get();

	Vector3 eyeLoc = Vector3(0.0f, 0.0f, 1.65f);
	Transform eyePlacement = look_at_matrix(eyeLoc, eyeLoc + Vector3(0.0f, 2.0f, 0.0f), Vector3::zAxis).to_transform();
	if (vr->is_render_view_valid())
	{
		eyePlacement = vr->get_render_view();
	}
#ifdef AN_DEVELOPMENT
	else
	{
		static Transform devPlacement = eyePlacement;
		static Rotator3 lookDir = Rotator3::zero;
		Rotator3 rotateBy = Rotator3::zero;
		if (auto* input = ::System::Input::get())
		{
			Vector2 relLoc = input->get_mouse_relative_location();
			rotateBy.yaw += relLoc.x;
			rotateBy.pitch -= relLoc.y * 0.5f;
		}
		lookDir += rotateBy;
		lookDir.pitch = clamp(lookDir.pitch, -80.0f, 80.0f);
		eyePlacement.set_orientation(lookDir.to_quat());
	}
#endif
	eyeLoc = eyePlacement.get_translation();

	Transform displayPlacement;

	Vector3 inDir = VR::IVR::get()->get_play_area_rect_half_forward();
	if (_context)
	{
		if (_context->displayInDir.is_zero())
		{
			// by default, in front
			_context->displayInDir = eyePlacement.vector_to_world(Vector3::yAxis);
			_context->displayInDir.z = 0.0f;
		}
		inDir = _context->displayInDir;
	}

	Vector3 displayAtLoc = Vector3::zero;
	displayAtLoc.z = 1.3f;
	Vector3 displayLoc = displayAtLoc + calc_display_vr_dist() * inDir.normal();
	displayPlacement = look_at_matrix(displayLoc, displayLoc - (displayAtLoc - displayLoc), Vector3::zAxis).to_transform();
	displayPlacement.set_scale(calc_device_mesh_3d_scale(Vector2(2.0f, 1.5f), Vector2(2.0f, 1.5f)));
	deviceMesh3DPlacement = displayPlacement;
	Render::Camera camera;
	camera.set_placement(nullptr, eyePlacement);
	camera.set_near_far_plane(0.02f, 30000.0f);
	camera.set_background_near_far_plane();

	Render::Scene* scenes[2];
	scenes[0] = Framework::Render::Scene::build(camera, NP, nullptr, NP, Framework::Render::SceneMode::VR_Left);
	scenes[1] = Framework::Render::Scene::build(camera, NP, nullptr, NP, Framework::Render::SceneMode::VR_Right);

	ready_device_mesh_3d_instance_to_render();
	
	deviceMesh3DInstanceTempForRendering.hard_copy_from(deviceMesh3DInstance);
	deviceMesh3DInstanceTempForRendering.set_blend(_srcBlend, _destBlend);
	deviceMesh3DInstanceTempForRendering.set_rendering_order_priority(1000); // top most
	
	Library::get_current()->access_vr_meshes().add_hands_to_render_scenes(scenes, 2);

	scenes[0]->add_extra(&deviceMesh3DInstanceTempForRendering, displayPlacement);
	scenes[1]->add_extra(&deviceMesh3DInstanceTempForRendering, displayPlacement);
#ifdef USE_DISPLAY_VR_SCENE
	scenes[0]->add_extra(&vrSceneMesh3DInstance, Transform::identity, true);
	scenes[1]->add_extra(&vrSceneMesh3DInstance, Transform::identity, true);
#else
	error(TXT("no display vr scene, change USE_DISPLAY_VR_SCENE and add references"))
#endif
	scenes[0]->prepare_extras();
	scenes[1]->prepare_extras();

	Framework::Render::Context rc(_v3d);

	rc.set_default_texture_id(Framework::Library::get_current()->get_default_texture()->get_texture()->get_texture_id());

	for_count(int, i, 2)
	{
		scenes[i]->render(rc, nullptr); // it's ok, it's vr
		scenes[i]->release();
#ifdef AN_DEBUG_RENDERER
		// to render debug
		debug_renderer_render(_v3d);
		debug_renderer_undefine_contexts();
#endif
	}
	debug_renderer_already_rendered();
#else
	error(TXT("enable USE_DISPLAY_MESH to allow using meshes"));
#endif
}

void Display::clear_active_controls()
{
	activeControlDueToCursorPress = nullptr;
	activeControlDueToSelectorPress = nullptr;
	activeControls.clear();
}

void Display::add_active_control(DisplayControl* _control, int _flags)
{
	if (!_control)
	{
		return;
	}

	for_every(activeControl, activeControls)
	{
		if (activeControl->control == _control)
		{
			set_flag(activeControl->flags, _flags);
			return;
		}
	}

	activeControls.push_back(ActiveDisplayControl());
	ActiveDisplayControl & newOne = activeControls.get_last();
	newOne.control = _control;
	newOne.activeTime = 0.0f;
	newOne.flags = _flags;
}

bool Display::remove_active_control(DisplayControl* _control, int _flags)
{
	if (!_control)
	{
		return false;
	}

	for_every(activeControl, activeControls)
	{
		if (activeControl->control == _control)
		{
			int flags = activeControl->flags;
			clear_flag(activeControl->flags, _flags);
			if (activeControl->flags == 0)
			{
				activeControls.remove_fast_at(for_everys_index(activeControl));
			}
			return is_flag_set(flags, _flags);
		}
	}
	return false;
}

bool Display::has_active_control(int _flags) const 
{
	for_every(activeControl, activeControls)
	{
		if (is_flag_set(activeControl->flags, _flags))
		{
			return true;
		}
	}
	return false;
}

DisplayControl* Display::get_active_control(int _flags) const
{
	for_every(activeControl, activeControls)
	{
		if (is_flag_set(activeControl->flags, _flags))
		{
			return activeControl->control;
		}
	}
	return nullptr;
}

static void handle_selector_dir(REF_ DisplaySelectorDir::Type & selectorDir, REF_ DisplaySelectorDir::Type & lastSelectorDir, REF_ float & lastSelectorTime, REF_ int & repeatSelectorCount, float repeatActionFirstTime, float repeatActionTime)
{
	if (selectorDir == lastSelectorDir)
	{
		if (selectorDir != DisplaySelectorDir::None)
		{
			lastSelectorTime += ::System::Core::get_raw_delta_time();
			if (lastSelectorTime >= (repeatSelectorCount == 0 ? repeatActionFirstTime : repeatActionTime))
			{
				// allow button cursor dir to be processed
				lastSelectorTime = 0.0f;
				++repeatSelectorCount;
			}
			else
			{
				// block button cursor dir
				selectorDir = DisplaySelectorDir::None;
			}
		}
	}
	else
	{
		// allow button cursor dir as we changed dir
		lastSelectorDir = selectorDir;
		lastSelectorTime = 0.0f;
		repeatSelectorCount = 0;
	}
}

void Display::process_input(IGameInput const & _input, float _deltaTime)
{
#ifdef AN_DEVELOPMENT
	if (::System::Core::get_delta_time() == 0.0f)
	{
		return;
	}
#endif
#ifndef AN_DEVELOPMENT
#ifdef AN_WINDOWS
	__try
	{
#else
	todo_important(TXT("implement!"));
#endif
#endif
		process_input_internal(_input, _deltaTime);
#ifndef AN_DEVELOPMENT
#ifdef AN_WINDOWS
	}
	__except (EXCEPTION_EXECUTE_HANDLER)
	{
		hasCrashed = true;
		crashedCode = 1;//GetExceptionCode();
	}
#else
	todo_important(TXT("implement!"));
#endif
#endif
	wasInputProcessed = true;
}
	
void Display::process_input_internal(IGameInput const & _input, float _deltaTime)
{
	__doing_advancement_now();

	Vector2 cursorMovementThisFrame = Vector2::zero;

	cursorWasPressed = cursorIsPressed;
	cursorWasTouched = cursorIsTouched;

	bool cursorShouldBeActive = cursorIsActive;
	bool jumpMovement = false;
	{
		// normalise movement - relative on screen cursor movement should be independent to resolution
		float const greaterScreenResolution = (float)max(setup.screenResolution.x, setup.screenResolution.y);
		float const normaliseMovement = greaterScreenResolution / 200.0f;
		cursorMovementThisFrame = ((_input.get_stick(NAME(cursor)) + _input.get_mouse(NAME(cursor))) * normaliseMovement);
		if (_input.has_stick(NAME(cursorAbsolute01)))
		{
			Vector2 at = _input.get_stick(NAME(cursorAbsolute01));
			at.x = clamp(at.x, 0.0f, 1.0f);
			at.y = clamp(at.y, 0.0f, 1.0f);
			at *= setup.screenResolution.to_vector2() - Vector2::one;
			cursorMovementThisFrame = at - cursorAt;
			jumpMovement = !cursorAbsolute01ProvidedInLastFrame;
			cursorAbsolute01ProvidedInLastFrame = true;
		}
		else
		{
			cursorAbsolute01ProvidedInLastFrame = false;
		}

		// check how much did we move to avoid short movement of mouse during specific time
		if (!cursorIsActive)
		{
			inputCursorInactiveTime += _deltaTime;
			float const cursorActiveThreshold = greaterScreenResolution * 0.015f;
			cursorMovementSinceLongerInactive += cursorMovementThisFrame;
			if (cursorMovementThisFrame.length() > cursorActiveThreshold ||
				cursorMovementSinceLongerInactive.length() > cursorActiveThreshold)
			{
				cursorShouldBeActive = true;
			}
			if (inputCursorInactiveTime > 0.5f)
			{
				// with this timer we require movement to be fast, not short one
				cursorMovementSinceLongerInactive = Vector2::zero;
			}
		}
		else
		{
			inputCursorInactiveTime = 0.0f;
			cursorMovementSinceLongerInactive = Vector2::zero;
		}

		// deactivate 
		if (!cursorIsActive)
		{
			cursorMovementThisFrame = Vector2::zero;
		}
	}

#ifdef AN_STANDARD_INPUT
	if (cursorDependantOnInput)
	{
		if (::System::Input const * input = _input.get_input())
		{
			todo_note(TXT("looks quite specific, maybe it should go somewhere else? to input?"));
			if (input->is_gamepad_active() || (input->is_keyboard_active() && !input->is_key_pressed(::System::Key::Esc))) // think that keyboard is active but only if not escape has been pressed
			{
				cursorShouldBeActive = false;
			}
		}
	}
#endif

	float thresholdTimeCoef = 1.0f;

	touchingIsActive = false;
	cursorNotTouching = false;
	if (_input.has_button(NAME(cursorTouch)) || _input.has_button(NAME(cursorFakeTouch)))
	{
		touchingIsActive = true;
		bool touching = _input.is_button_pressed(NAME(cursorTouch)) || _input.is_button_pressed(NAME(cursorFakeTouch));
		cursorNotTouching = !touching;
	}

	if (touchingIsActive)
	{
		cursorShouldBeActive = true; // we handle that through touching or not
		thresholdTimeCoef = touchThresholdTimeCoef;
	}

	if (cursorShouldBeActive != cursorIsActive)
	{
		set_cursor_active(cursorShouldBeActive);
	}

	if (cursorNameUntilDrawn.is_valid())
	{
		clear_active_controls();
		__done_advancement_now();
		return;
	}

	// if there is just one button, have it selected
	if (!touchingIsActive && !selectedControl.is_set())
	{
		DisplayButton* singleButton = nullptr;
		for_every(control, controls)
		{
			if (control->get()->is_selectable())
			{
				if (DisplayButton* button = fast_cast<DisplayButton>(control->get()))
				{
					if (!singleButton)
					{
						singleButton = button;
					}
					else
					{
						// no longer a single button
						singleButton = nullptr;
						break;
					}
				}
			}
		}
		if (singleButton)
		{
			select(singleButton, NP, NP, true);
		}
	}

	// read selector dirs
	Vector2 selectorDirVector2 = _input.get_stick(NAME(selector));
	DisplaySelectorDir::Type selectorDir = choose_dir(selectorDirVector2);
	DisplaySelectorDir::Type cursorHoverDir = DisplaySelectorDir::None;

	if (_input.is_button_pressed(NAME(scrollUp)))
	{
		selectorDir = DisplaySelectorDir::ScrollUp;
	}
	else if (_input.is_button_pressed(NAME(scrollDown)))
	{
		selectorDir = DisplaySelectorDir::ScrollDown;
	}
	if (_input.is_button_pressed(NAME(selectorPageUp)))
	{
		selectorDir = DisplaySelectorDir::PageUp;
	}
	else if (_input.is_button_pressed(NAME(selectorPageDown)))
	{
		selectorDir = DisplaySelectorDir::PageDown;
	}
	if (_input.is_button_pressed(NAME(selectorHome)))
	{
		selectorDir = DisplaySelectorDir::Home;
	}
	else if (_input.is_button_pressed(NAME(selectorEnd)))
	{
		selectorDir = DisplaySelectorDir::End;
	}

	if (_input.is_button_pressed(NAME(cursorHoverScrollUp)))
	{
		cursorHoverDir = DisplaySelectorDir::ScrollUp;
	}
	else if (_input.is_button_pressed(NAME(cursorHoverScrollDown)))
	{
		cursorHoverDir = DisplaySelectorDir::ScrollDown;
	}

	if (selectorDir != DisplaySelectorDir::None)
	{
		// auto switch it off
		set_cursor_active(false);
	}

	handle_selector_dir(REF_ selectorDir, REF_ lastSelectorDir, REF_ lastSelectorTime, REF_ repeatSelectorCount, repeatActionFirstTime, repeatActionTime);
	handle_selector_dir(REF_ cursorHoverDir, REF_ lastCursorHoverDir, REF_ lastCursorHoverTime, REF_ repeatCursorHoverCount, repeatActionFirstTime, repeatActionTime);

	// advance time now
	if (cursorState != DisplayCursorState::None)
	{
		cursorActivateTime += _deltaTime;
	}
	else
	{
		cursorActivateTime = 0.0f;
	}

	// if it was broken (control removed, etc) clean up
	/*
	if (!cursorActiveControl.is_set() && cursorState != DisplayCursorState::None)
	{
		cursorState = DisplayCursorState::WaitUntilRelease;
	}
	*/

	DisplayCursorState::Type newCursorState = DisplayCursorState::None;
	DisplayControl* newCursorActiveControl = nullptr;
	bool const cursorRequestedToBeVisible = is_cursor_requested_to_be_visible();
	VectorInt2 cursorAtAsVectorInt2 = cursorAt.to_vector_int2_cells();
	{
		bool const newCursorPressed = cursorRequestedToBeVisible && _input.is_button_pressed(NAME(cursorPress));
		bool newCursorTouch = false;
		cursorIsPressed = newCursorPressed;
		cursorIsTouched = false;
		if (_input.has_button(NAME(cursorTouch)))
		{
			newCursorTouch = /*cursorRequestedToBeVisible && */_input.is_button_pressed(NAME(cursorTouch));
			cursorIsTouched = newCursorTouch; // physically!
			if (!newCursorTouch && !newCursorPressed)
			{
				usingCursorTouch = true; // this means we are now responsive to touch
			}
			if (newCursorPressed)
			{
				usingCursorTouch = false; // since pressed we will ignore touch
			}
		}
		if (cursorRequestedToBeVisible || cursorState != DisplayCursorState::None)
		{
			DisplayControl* cursorOn = nullptr;

			bool findCursorOn = false;
			if (newCursorPressed || (newCursorTouch && usingCursorTouch && should_allow_touch_as_press_internal()))
			{
				if (cursorState == DisplayCursorState::WaitUntilRelease)
				{
					newCursorState = DisplayCursorState::WaitUntilRelease;
				}
				else
				{
					if (cursorState == DisplayCursorState::None ||
						(newCursorPressed && cursorState == DisplayCursorState::WaitForClick)) // we press when waiting for click
					{
						clickCursorAtAsVectorInt2 = cursorAtAsVectorInt2;
						newCursorActiveControl = cursorHoverControl.get(); // store as active control
						if (newCursorActiveControl)
						{
							DisplayControlDrag::Type cursorDragging = newCursorActiveControl->check_process_drag(this, clickCursorAtAsVectorInt2);
							if (cursorDragging == DisplayControlDrag::NotAvailable)
							{
								if (newCursorActiveControl->check_process_double_click(this, clickCursorAtAsVectorInt2))
								{
									newCursorState = DisplayCursorState::ClickOrDoubleClickFirstClick;
								}
								else
								{
									newCursorState = usingCursorTouch && !newCursorPressed ? DisplayCursorState::WaitForClick : DisplayCursorState::Click;
								}
							}
							else if (cursorDragging == DisplayControlDrag::DragImmediately)
							{
								newCursorState = DisplayCursorState::Drag;
							}
							else
							{
								newCursorState = DisplayCursorState::ClickOrDrag;
							}
						}
						else
						{
							newCursorState = usingCursorTouch && !newCursorPressed ? DisplayCursorState::WaitForClick : DisplayCursorState::Click;
						}
					}
					else
					{
						newCursorState = cursorState;
						newCursorActiveControl = cursorActiveControl.get();
						if (cursorState == DisplayCursorState::Drag)
						{
							newCursorState = DisplayCursorState::Drag;
						}
						else if (cursorState == DisplayCursorState::ClickOrDrag &&
							cursorActivateTime > cursorClickThreshold * thresholdTimeCoef)
						{
							newCursorState = DisplayCursorState::Drag;
						}
						else if (cursorState == DisplayCursorState::ClickOrDoubleClickBetweenClicks &&
							cursorActivateTime <= cursorDoubleClickThreshold * thresholdTimeCoef)
						{
							newCursorState = DisplayCursorState::DoubleClick;
						}
					}
					cursorOn = newCursorActiveControl;
				}
			}
			else
			{
				newCursorState = DisplayCursorState::None;
				if (cursorState != DisplayCursorState::WaitUntilRelease)
				{
					if ((cursorState == DisplayCursorState::ClickOrDrag || cursorState == DisplayCursorState::WaitForClick) &&
						cursorActivateTime <= cursorClickThreshold * thresholdTimeCoef)
					{
						newCursorActiveControl = cursorActiveControl.get();
						if (newCursorActiveControl && newCursorActiveControl->check_process_double_click(this, clickCursorAtAsVectorInt2))
						{
							newCursorState = DisplayCursorState::ClickOrDoubleClickBetweenClicks;
						}
						else
						{
							newCursorState = DisplayCursorState::Click;
						}
						cursorOn = newCursorActiveControl;
					}
					else if (cursorState == DisplayCursorState::ClickOrDoubleClickFirstClick ||
							 cursorState == DisplayCursorState::ClickOrDoubleClickBetweenClicks)
					{
						newCursorActiveControl = cursorHoverControl.get();
						if (cursorActivateTime <= cursorDoubleClickThreshold * thresholdTimeCoef)
						{
							newCursorState = DisplayCursorState::ClickOrDoubleClickBetweenClicks; // stay in that state
							cursorOn = cursorActiveControl.get();
						}
						else
						{
							newCursorState = DisplayCursorState::Click; // switch to click
							cursorOn = cursorActiveControl.get();
						}
					}
				}

				if (!cursorOn)
				{
					findCursorOn = true;
				}
			}
			if (newCursorState == DisplayCursorState::WaitForClick)
			{
				findCursorOn = true;
				cursorOn = nullptr;
			}
			if (touchingIsActive)
			{
				findCursorOn = true; // always
			}
			if (findCursorOn)
			{
				// find control to hover over
				int cursorOnDist = 0;
				int const border = 5;
				for_every(control, controls)
				{
					if (control->get()->is_hoverable())
					{
						// allow non-selectable controls, we find hover control here!
						VectorInt2 bl;
						VectorInt2 tr;
						if (useCoordinates == DisplayCoordinates::Char)
						{
							bl = charSize * control->get()->get_bottom_left_corner();
							tr = charSize * (control->get()->get_top_right_corner() + CharCoords::one) - VectorInt2::one;
						}
						else
						{
							bl = control->get()->get_bottom_left_corner();
							tr = control->get()->get_top_right_corner();
						}
						int dist = 0;
						dist += max(0, bl.x - cursorAtAsVectorInt2.x);
						dist += max(0, bl.y - cursorAtAsVectorInt2.y);
						dist += max(0, cursorAtAsVectorInt2.x - tr.x);
						dist += max(0, cursorAtAsVectorInt2.y - tr.y);
						if (cursorAtAsVectorInt2.x >= bl.x - border && cursorAtAsVectorInt2.x <= tr.x + border &&
							cursorAtAsVectorInt2.y >= bl.y - border && cursorAtAsVectorInt2.y <= tr.y + border)
						{
							if (!cursorOn || dist < cursorOnDist)
							{
								cursorOn = control->get();
								cursorOnDist = dist;
							}
						}
					}
				}
			}
			if (newCursorState == DisplayCursorState::None ||
				newCursorState == DisplayCursorState::WaitForClick)
			{
				cursorHoverControl = cursorOn;
				if (cursorHoverControl.is_set())
				{
					cursorHoverControl->process_hover(this, cursorAtAsVectorInt2);

					if (cursorHoverControl->is_selectable())
					{
						select(cursorHoverControl.get(), cursorAtAsVectorInt2);
					}
				}
				else if (touchingIsActive)
				{
					select(nullptr, cursorAtAsVectorInt2);
				}
			}
		}
	}
	// handle dragging
	cursorHiddenDueToDragging = false;
	if (newCursorState == DisplayCursorState::Drag)
	{
		an_assert(newCursorActiveControl);
		an_assert(cursorState != newCursorState || cursorActiveControl == newCursorActiveControl);
		if (cursorState != DisplayCursorState::Drag)
		{
			cursorDragTime = 0.0f;
			// start dragging
			an_assert(has_control(newCursorActiveControl));
			if (newCursorActiveControl && newCursorActiveControl->process_drag_start(this, cursorAtAsVectorInt2))
			{
				cursorHiddenDueToDragging = true;
				cursorMovementThisFrame = Vector2::zero;
			}
		}
		else
		{
			cursorDragTime += _deltaTime;
		}
		// continue dragging
		an_assert(has_control(newCursorActiveControl));
		if (newCursorActiveControl && newCursorActiveControl->process_drag_continue(this, cursorAtAsVectorInt2, cursorMovementThisFrame.to_vector_int2_cells(), cursorDragTime))
		{
			cursorHiddenDueToDragging = true;
			cursorMovementThisFrame = Vector2::zero;
		}
	}
	else
	{
		if (cursorState == DisplayCursorState::Drag &&
			cursorActiveControl.is_set())
		{
			// continue dragging
			if (cursorActiveControl->process_drag_continue(this, cursorAtAsVectorInt2, cursorMovementThisFrame.to_vector_int2_cells(), cursorDragTime))
			{
				cursorMovementThisFrame = Vector2::zero;
			}
			// and end dragging
			VectorInt2 newCursorAtAsVectorInt2;
			if (cursorActiveControl->process_drag_end(this, cursorAtAsVectorInt2, cursorDragTime, OUT_ newCursorAtAsVectorInt2))
			{
				cursorAtAsVectorInt2 = newCursorAtAsVectorInt2;
				cursorAt = cursorAtAsVectorInt2.to_vector2();
			}
		}
	}

	if (newCursorActiveControl &&
		!has_control(newCursorActiveControl))
	{
		// was removed, clear it, if state is set, we will have wait until release
		newCursorActiveControl = nullptr;
	}

	// click
	if (newCursorState == DisplayCursorState::ClickOrDoubleClickFirstClick &&
		cursorState != DisplayCursorState::ClickOrDoubleClickFirstClick &&
		newCursorActiveControl)
	{
		newCursorActiveControl->process_early_click(this, clickCursorAtAsVectorInt2);
	}
	else if (newCursorState == DisplayCursorState::Click &&
			 cursorState != DisplayCursorState::Click)
	{
		perform_func_info<OnClick>(on_click, [this, newCursorActiveControl](OnClick _on_click){_on_click(newCursorActiveControl, cursorAt, true); });

		if (newCursorActiveControl &&
			(clickCursorAtAsVectorInt2 - cursorAtAsVectorInt2).to_vector2().length_squared() <= sqr(10.0f))
		{
			an_assert(!activeControlDueToCursorPress);
			activeControlDueToCursorPress = newCursorActiveControl;
			add_active_control(newCursorActiveControl, ADPF_CursorPress);
			newCursorActiveControl->process_click(this, clickCursorAtAsVectorInt2);
		}
	}
	else if (newCursorState == DisplayCursorState::DoubleClick &&
			 cursorState != DisplayCursorState::DoubleClick)
	{
		if (newCursorActiveControl &&
			(clickCursorAtAsVectorInt2 - cursorAtAsVectorInt2).to_vector2().length_squared() <= sqr(10.0f))
		{
			newCursorActiveControl->process_double_click(this, clickCursorAtAsVectorInt2);
		}
	}
	else if (newCursorState != DisplayCursorState::Click &&
			 cursorState == DisplayCursorState::Click)
	{
		perform_func_info<OnClick>(on_click, [this, newCursorActiveControl](OnClick _on_click){_on_click(newCursorActiveControl, cursorAt, false); });
	}

	if (newCursorActiveControl &&
		!has_control(newCursorActiveControl))
	{
		// was removed
		newCursorActiveControl = nullptr;
	}

	cursorState = newCursorState;
	cursorActiveControl = newCursorActiveControl;

	if (activeControlDueToCursorPress &&
		newCursorState != DisplayCursorState::Click)
	{
		remove_active_control(activeControlDueToCursorPress, ADPF_CursorPress);
		activeControlDueToCursorPress = nullptr;
	}

	// allow current control to handle navigation and eat it/override_ it
	// do cursor hover control too here
	if (selectorDir != DisplaySelectorDir::None ||
		cursorHoverDir != DisplaySelectorDir::None)
	{
		bool handleSelector = selectorDir != DisplaySelectorDir::None;
		bool handleCursorHover = cursorHoverDir != DisplaySelectorDir::None;
		for_every(control, controls)
		{
			if (!control->get()->is_locked())
			{
				if (selectorDir != DisplaySelectorDir::None && control->get()->process_navigation_global(this, selectorDir))
				{
					handleSelector = false;
				}
				if (cursorHoverDir != DisplaySelectorDir::None && control->get()->process_navigation_global(this, cursorHoverDir))
				{
					handleCursorHover = false;
				}
			}
		}
		if (handleSelector && selectedControl.is_set() && !selectedControl->is_locked())
		{
			selectorDir = selectedControl->process_navigation(this, selectorDir);
		}
		if (handleCursorHover && cursorHoverControl.is_set() && !cursorHoverControl->is_locked())
		{
			cursorHoverControl->process_navigation(this, cursorHoverDir);
		}
	}
		
	if (selectorDir == DisplaySelectorDir::Home ||
		selectorDir == DisplaySelectorDir::End)
	{	// check if all selectable controls are buttons and handle home (highest) and end (lowest) properly
		bool allButtons = true;
		RangeCharCoord2 space = RangeCharCoord2::empty;
		for_every_ref(control, controls)
		{
			if (control->is_active_for_user())
			{
				if (fast_cast<DisplayButton>(control) != nullptr)
				{
					space.include(RangeCharCoord2::from_at_and_size(control->get_centre(), CharCoords::one));
				}
				else
				{
					allButtons = false;
					break;
				}
			}
		}
		if (allButtons)
		{
			VectorInt2 inDir = VectorInt2::zero;
			if (space.x.length() < space.y.length() / 5)
			{
				if (selectorDir == DisplaySelectorDir::Home) inDir = VectorInt2(0, 1);
				if (selectorDir == DisplaySelectorDir::End) inDir = VectorInt2(0, -1);
			}
			else if (space.y.length() < space.x.length() / 5)
			{
				if (selectorDir == DisplaySelectorDir::Home) inDir = VectorInt2(-1, 0);
				if (selectorDir == DisplaySelectorDir::End) inDir = VectorInt2(1, 0);
			}
			else
			{
				if (selectorDir == DisplaySelectorDir::Home) inDir = VectorInt2(-1, 1);
				if (selectorDir == DisplaySelectorDir::End) inDir = VectorInt2(1, -1);
			}
			if (!inDir.is_zero())
			{
				DisplayControl* nextControl = nullptr;
				CharCoord nextControlSuit = 0;
				for_every_ref(control, controls)
				{
					if (control->is_selectable())
					{
						CharCoord suit = VectorInt2::dot(control->get_centre(), inDir);
						if (suit > nextControlSuit || !nextControl)
						{
							nextControl = control;
							nextControlSuit = suit;
						}
					}
				}
				if (nextControl && nextControl != selectedControl.get())
				{
					select(nextControl, NP, inDir);
				}
			}
		}
	}

	if (selectorDir != DisplaySelectorDir::None && cursorState == DisplayCursorState::None)
	{
		Vector2 const inDir = DisplaySelectorDir::to_vector2(selectorDir);
		
		if (selectedControl.is_set())
		{
			bool keepTrying = true;
			while (keepTrying)
			{
				keepTrying = false;
				bool considerLooping = is_navigation_looped();
				if (selectorDir == DisplaySelectorDir::Prev ||
					selectorDir == DisplaySelectorDir::Next)
				{
					considerLooping = false;
				}
				// 0.99 is to allow going to controls that share border
				DisplayControl* nextControl = nullptr;
				float nextControlDist = 0.0f;
				float sameControlPenalty = 0.0f;
				Vector2 fromPoint = selectedControl->get_from_point_for_navigation(this, inDir);
				// for other lines, begin just beyond screen ends horizontally-wise and slightly above or below
				// use penalty for same control to try to choose other controls that would fit better
				if (selectorDir == DisplaySelectorDir::PrevInLineAbove)
				{
					fromPoint = selectedControl->get_sub_centre() + selectedControl->get_sub_half_size() * Vector2(1.0f, 1.01f);
					fromPoint.x = (float)setup.screenResolution.x + 0.01f;
					sameControlPenalty = selectedControl->get_sub_half_size().length();
				}
				if (selectorDir == DisplaySelectorDir::NextInLineBelow)
				{
					fromPoint = selectedControl->get_sub_centre() + selectedControl->get_sub_half_size() * Vector2(-1.0f, -1.01f);
					fromPoint.x = -0.01f;
					sameControlPenalty = selectedControl->get_sub_half_size().length();
				}
				for_every(control, controls)
				{
					if (!control->get()->is_selectable())
					{
						continue;
					}
					if (control->get() != selectedControl.get())
					{
						Vector2 toPoint = control->get()->get_sub_centre();
						Vector2 useFromPoint = fromPoint;
						// if furthest point on control (in "inDir" dir) is on opposite side than "fromPoint", assume we will be looping...
						if (considerLooping && Vector2::dot((control->get()->get_sub_centre() + inDir * control->get()->get_sub_half_size()) - fromPoint, inDir) < 0.0f)
						{
							// ...and pretend fromPoint is moved back... by greater distance to still prefer those that are on screen
							useFromPoint -= setup.screenResolution.to_vector2() * inDir * 3.0f;
						}
						Vector2 const lb = control->get()->get_sub_centre() - control->get()->get_sub_half_size();
						Vector2 const rt = control->get()->get_sub_centre() + control->get()->get_sub_half_size();
						toPoint.x = clamp(useFromPoint.x, lb.x, rt.x);
						toPoint.y = clamp(useFromPoint.y, lb.y, rt.y);
						if (Vector2::dot(inDir, toPoint - useFromPoint) > 0.0f)
						{
							Vector2 relDir = toPoint - useFromPoint;
							float dist = abs(relDir.x) + abs(relDir.y);
							float alongDir = Vector2::dot(relDir.normal(), inDir);
							// prefer those right on the line - will help with movement on the grid
							if (alongDir < 0.7f)
							{
								dist *= 4.5f;
							}
							else if (alongDir < 0.99f)
							{
								dist *= 2.5f;
							}
							if (control->get() == selectedControl.get())
							{
								dist += sameControlPenalty;
							}
							if (!nextControl || dist < nextControlDist)
							{
								nextControlDist = dist;
								nextControl = control->get();
							}
						}
					}
				}

				if (nextControl && nextControl != selectedControl.get())
				{
					select(nextControl, NP, inDir.to_vector_int2_cells());
				}

				// check if we need to do one more pass
				if (!nextControl)
				{
					if (selectorDir == DisplaySelectorDir::Prev)
					{
						keepTrying = true;
						selectorDir = DisplaySelectorDir::PrevInLineAbove;
					}
					if (selectorDir == DisplaySelectorDir::Next)
					{
						keepTrying = true;
						selectorDir = DisplaySelectorDir::NextInLineBelow;
					}
				}
			}
		}
		else if (!is_cursor_active() && ! touchingIsActive)
		{
			select_anything();
		}
	}

	move_cursor(cursorMovementThisFrame, jumpMovement);

	bool selectorHasBeenPressed = _input.has_button_been_pressed(NAME(selectorPress));
	bool selectorIsPressed = _input.is_button_pressed(NAME(selectorPress));

	bool checkShortcuts = true;
	if (selectorHasBeenPressed && cursorState == DisplayCursorState::None)
	{
		// auto switch it off
		set_cursor_active(false);
		if (selectedControl.is_set() && !selectedControl->is_locked())
		{
			an_assert(!activeControlDueToSelectorPress);
			activeControlDueToSelectorPress = selectedControl.get();
			add_active_control(activeControlDueToSelectorPress, ADPF_SelectorPress);
			selectedControl->process_activate(this);
			checkShortcuts = false;
		}
		else
		{
			if (!selectedControl.is_set())
			{
				for_every(control, controls)
				{
					if (control->get()->is_selectable())
					{
						if (DisplayButton* button = fast_cast<DisplayButton>(control->get()))
						{
							select(control->get());
							break;
						}
					}
				}
			}
			if (!selectedControl.is_set())
			{
				for_every(control, controls)
				{
					if (control->get()->is_selectable())
					{
						select(control->get());
						break;
					}
				}
			}
		}
	}
	
	if (activeControlDueToSelectorPress &&
		!selectorIsPressed)
	{
		remove_active_control(activeControlDueToSelectorPress, ADPF_SelectorPress);
		activeControlDueToSelectorPress = nullptr;
	}

	if (checkShortcuts)
	{
		DisplayControl* shortcutFound = nullptr; // to activate
		int shortcutFoundPriority = 0;
		for_every_reverse(control, controls) // prefer first found from the back, as most recent
		{
			if (!control->get()->is_visible() || control->get()->is_locked())
			{
				continue;
			}
			Name shortcut = control->get()->get_shortcut_input_button();
			int shourtcutPriority = control->get()->get_shortcut_input_button_priority();
			if (shortcut.is_valid())
			{
				if (_input.has_button_been_pressed(shortcut))
				{
					if (!shortcutFound ||
						shortcutFoundPriority < shourtcutPriority)
					{
						shortcutFound = control->get();
						shortcutFoundPriority = shourtcutPriority;
					}
				}
				else if (!_input.is_button_pressed(shortcut))
				{
					// remove only if shortcut was released
					if (remove_active_control(control->get(), ADPF_Shortcut))
					{
						if (!has_active_control(ADPF_Shortcut))
						{
							if (preShortcutControl.is_set())
							{
								if (preShortcutControl->is_selectable())
								{
									select(preShortcutControl.get(), NP, NP, true);
								}
								preShortcutControl = nullptr;
							}
						}
						else if (auto* control = get_active_control(ADPF_Shortcut))
						{
							if (control->is_selectable())
							{
								select(control);
							}
						}
					}
				}
			}
		}
		if (shortcutFound)
		{
			if (cursorDependantOnInput)
			{
				if (::System::Input const * input = _input.get_input())
				{
					todo_note(TXT("looks quite specific, maybe it should go somewhere else? to input?"));
					// auto switch it off
#ifdef AND_STANDARD_INPUT
					if (!input->is_keyboard_active() || !input->is_key_pressed(::System::Key::Esc))
					{
						set_cursor_active(false); // if pressed escape, ignore deactivating cursor
					}
#endif
					// but just in case it was due to mouse (mouse wheel?) keep it if mouse is active
					if (input->is_mouse_active())
					{
						set_cursor_active(true);
					}
				}
			}
			if (!cursorIsActive && shortcutFound->is_visible() && shortcutFound->is_selectable() && shortcutFound->should_shortcut_select())
			{
				if (!preShortcutControl.is_set())
				{
					preShortcutControl = selectedControl;
					select(shortcutFound);
				}
			}
			add_active_control(shortcutFound, ADPF_Shortcut);
			shortcutFound->process_activate(this);
		}
	}
	__done_advancement_now();
}

void Display::select(DisplayControl* _control, Optional<VectorInt2> _cursor, Optional<VectorInt2> _goingInDir, bool _silent)
{
	an_assert(!_control || has_control(_control));
	an_assert(!_control || _control->is_selectable());
	if ((selectedControl.is_set() && selectedControl.get() == _control) ||
		(!selectedControl.is_set() && !_control))
	{
		return;
	}
	if (selectedControl.is_set())
	{
		selectedControl->process_deselected(this);
	}
	DisplayControl* prevSelected = selectedControl.get();
	selectedControl = _control;
	if (selectedControl.is_set())
	{
		selectedControl->process_selected(this, prevSelected, _cursor, _goingInDir, _silent);
	}
}

DisplaySelectorDir::Type Display::choose_dir(Vector2 const & _dir) const
{
	if (_dir.y >  0.5f) return DisplaySelectorDir::Up;
	if (_dir.y < -0.5f) return DisplaySelectorDir::Down;
	if (_dir.x >  0.5f) return DisplaySelectorDir::Right;
	if (_dir.x < -0.5f) return DisplaySelectorDir::Left;
	return DisplaySelectorDir::None;
}

void Display::use_no_background()
{
	backgroundRenderTarget = nullptr;
	backgroundTexture = ::System::texture_id_none();
}

void Display::use_background(::System::Texture* _texture, float _backgroundColourBased)
{
	backgroundColourBased = _backgroundColourBased;
	if (_texture)
	{
		backgroundRenderTarget = nullptr;
		backgroundTexture = _texture->get_texture_id();
		backgroundTextureSize = _texture->get_size().to_vector2();
	}
	else
	{
		use_no_background();
	}
}

void Display::use_background(::System::RenderTarget* _rt, int _index, float _backgroundColourBased)
{
	backgroundColourBased = _backgroundColourBased;
	if (_rt)
	{
		an_assert(_rt->is_resolved()); // just to get data texture id
		backgroundRenderTarget = _rt;
		backgroundTexture = _rt->get_data_texture_id(_index);
		backgroundTextureSize = _rt->get_size().to_vector2();
	}
	else
	{
		use_no_background();
	}
}

Sample* Display::find_sound_sample(Name const & _name) const
{
	if (DisplayType const * dt = displayType.get())
	{
		if (auto pSoundSample = dt->get_sound_samples().get_existing(_name))
		{
			return pSoundSample->get();
		}
	}
	return nullptr;
}

SoundSource* Display::play_sound(Name const & _name)
{
	if (Sample* sample = find_sound_sample(_name))
	{
		return access_sounds().play(sample);
	}
	return nullptr;
}

static void rearrange_buttons_h(Array<DisplayButton*>& _buttons, int _startIdx, int _endIdx, DisplayHAlignment::Type _hAlignment, CharCoord _startX, CharCoord _totalWidth, CharCoord _lineTotalWidth)
{
	if (_endIdx < _startIdx)
	{
		return;
	}
	CharCoord xOffset = 0;
	if (_hAlignment == DisplayHAlignment::Right)
	{
		xOffset = _totalWidth - _lineTotalWidth;
	}
	else if (_hAlignment == DisplayHAlignment::Centre)
	{
		xOffset = (_totalWidth - _lineTotalWidth) / 2;
	}
	else if (_hAlignment == DisplayHAlignment::CentreRight)
	{
		xOffset = (_totalWidth - _lineTotalWidth) / 2;
		if ((_totalWidth - _lineTotalWidth) % 2 == 1)
		{
			xOffset += 1;
		}
	}
	for_range(CharCoord, i, _startIdx, _endIdx)
	{
		CharCoords at = _buttons[i]->get_at_param();
		at.x += xOffset + _startX;
		_buttons[i]->at(at);
	}
}

int Display::place_and_add(RangeCharCoord2 const & _rect, DisplayHAlignment::Type _hAlignment, DisplayVAlignment::Type _vAlignment, Array<DisplayButton*>& _buttons, CharCoord _innerPadding, CharCoord _spacing)
{
	// place them and add them
	CharCoord totalWidth = _rect.x.length();
	CharCoord x = 0;
	CharCoord y = 0;
	CharCoord lineStartedAtIdx = 0;
	CharCoord lineTotalWidth = 0;
	for_every_ptr(button, _buttons)
	{
		CharCoord width = button->get_caption().get_length() + _innerPadding * 2;
		if (x + width > totalWidth)
		{
			// rearrange previous line
			rearrange_buttons_h(_buttons, lineStartedAtIdx, for_everys_index(button) - 1, _hAlignment, _rect.x.min, totalWidth, lineTotalWidth);
			x = 0;
			y--;
			lineStartedAtIdx = for_everys_index(button);
			lineTotalWidth = 0;
		}
		button->at(x, y)
			->size(width)
			->horizontal_padding(_innerPadding);
		lineTotalWidth = x + width;
		x += width + _spacing;
	}
	// rearrange last added line
	rearrange_buttons_h(_buttons, lineStartedAtIdx, _buttons.get_size() - 1, _hAlignment, _rect.x.min, totalWidth, lineTotalWidth);
	{	// rearrange veritcally
		CharCoord yOffset = 0;
		if (_vAlignment == DisplayVAlignment::Top)
		{
			yOffset = _rect.y.max;
		}
		else if (_vAlignment == DisplayVAlignment::Bottom)
		{
			yOffset = _rect.y.min - y;
		}
		else if (_vAlignment == DisplayVAlignment::Centre)
		{
			yOffset = _rect.y.min + (_rect.y.length() - y) / 2;
		}
		for_every_ptr(button, _buttons)
		{
			CharCoords at = button->get_at_param();
			at.y += yOffset;
			button->at(at);
		}
	}
	for_every_ptr(button, _buttons)
	{
		add(button);
	}

	return -y + 1;
}

RangeCharCoord2 Display::place_on_grid_and_add(RangeCharCoord2 const & _rect, DisplayHAlignment::Type _hAlignment, DisplayVAlignment::Type _vAlignment, Array<DisplayButton*>& _buttons, CharCoords _buttonSize, CharCoords _spacing, CharCoord _innerPadding, VectorInt2 _limit)
{
	if (_buttonSize.x == 0)
	{
		an_assert(_limit.x != 0, TXT("either size or limit is required!"));
		_buttonSize.x = (int)(floor((float)_rect.x.length() / (float)_limit.x));
	}
	if (_buttonSize.y == 0)
	{
		an_assert(_limit.y != 0, TXT("either size or limit is required!"));
		_buttonSize.y = (int)(floor((float)_rect.y.length() / (float)_limit.y));
	}

	// calculate how many buttons are we placing on a grid
	int hButtons = _limit.x;
	if (hButtons == 0)
	{
		int spaceLeft = _rect.x.length() + _spacing.x;
		while (spaceLeft >= _spacing.x + _buttonSize.x)
		{
			++hButtons;
			spaceLeft -= _spacing.x + _buttonSize.x;
		}
	}
	hButtons = min(hButtons, _buttons.get_size());
	hButtons = max(hButtons, 1);

	int vButtons = 0;
	{
		int buttonsLeft = _buttons.get_size();
		while (buttonsLeft > 0)
		{
			buttonsLeft -= hButtons;
			++vButtons;
		}
	}
	vButtons = max(vButtons, 1);

	// try to have last row with as big coverage as possible (so we won't have some orphans, we should avoid 4,1 and 4,2 and prefer 3,2 and 3,3)
	if (_buttons.get_size() > hButtons)
	{
		float lastRowCoverage = (float)(_buttons.get_size() % hButtons) / ((float)hButtons);
		if (lastRowCoverage != 0.0f && lastRowCoverage <= 0.7f) // 0 means that it fully fits
		{
			int altHButtons = hButtons;
			int altVButtons = vButtons;
			while (altHButtons > 3 &&
				altHButtons > altVButtons && // prefer vertical rectangle or square
				_buttons.get_size() > altHButtons) // we will end up with single line
			{
				--altHButtons;
				float altLastRowCoverage = (float)(_buttons.get_size() % altHButtons) / ((float)altHButtons);
				if (altLastRowCoverage == 0.0f || altLastRowCoverage > lastRowCoverage) // 0 means that it fully fits
				{
					altVButtons = 0;
					{
						int buttonsLeft = _buttons.get_size();
						while (buttonsLeft > 0)
						{
							buttonsLeft -= altHButtons;
							++altVButtons;
						}
					}
					if (altVButtons * (_spacing.y + _buttonSize.y) - _spacing.y <= _rect.y.length())
					{
						// use alt
						hButtons = altHButtons;
						vButtons = altVButtons;
					}
					else
					{
						break; // won't fit anymore
					}
				}
			}
		}

	}

	// just make sure spacing is valid
	if (_spacing.x == 0)
	{
		_spacing.x = (int)(floor((float)(_rect.x.length() - _buttonSize.x * hButtons) / (float)hButtons));
	}
	if (_spacing.y == 0)
	{
		_spacing.y = (int)(floor((float)(_rect.y.length() - _buttonSize.y * vButtons) / (float)vButtons));
	}

	CharCoords requiresSpace((_buttonSize.x + _spacing.x) * hButtons - _spacing.x, (_buttonSize.y + _spacing.y) * vButtons - _spacing.y);

	// get rect where to place buttons
	RangeCharCoord2 rect = _rect;
	if (_hAlignment == DisplayHAlignment::Centre)
	{
		rect.x.min += (rect.x.length() - requiresSpace.x) / 2;
	}
	if (_hAlignment == DisplayHAlignment::Right)
	{
		rect.x.min += (rect.x.length() - requiresSpace.x);
	}
	rect.x.max = rect.x.min + requiresSpace.x - 1;

	if (_vAlignment == DisplayVAlignment::Centre)
	{
		rect.y.min += (rect.y.length() - requiresSpace.y) / 2;
	}
	if (_vAlignment == DisplayVAlignment::Top)
	{
		rect.y.min += (rect.y.length() - requiresSpace.y);
	}
	rect.y.max = rect.y.min + requiresSpace.y - 1;

	// place buttons!
	CharCoords at(rect.x.min, rect.y.max - _buttonSize.y + 1);
	CharCoords moveBy = _buttonSize + _spacing;
	int placedInARow = 0;
	int buttonsLeft = _buttons.get_size();
	for_every_ptr(button, _buttons)
	{
		button->at(at)
			->size(_buttonSize)
			->horizontal_padding(_innerPadding);
		++placedInARow;
		--buttonsLeft;
		if (placedInARow >= hButtons && buttonsLeft)
		{
			placedInARow = 0;
			at.y -= moveBy.y;

			at.x = rect.x.min;
			int buttonsInARow = min(hButtons, buttonsLeft);
			CharCoord requiresSpaceForRow = (_buttonSize.x + _spacing.x) * buttonsInARow - _spacing.x;
			if (_hAlignment == DisplayHAlignment::Centre)
			{
				at.x += (rect.x.length() - requiresSpaceForRow) / 2;
			}
			if (_hAlignment == DisplayHAlignment::Right)
			{
				at.x += (rect.x.length() - requiresSpaceForRow);
			}
		}
		else
		{
			at.x += moveBy.x;
		}
	}

	for_every_ptr(button, _buttons)
	{
		add(button);
	}

	return rect;
}

int Display::place_vertically_and_add(RangeCharCoord2 const & _rect, DisplayVAlignment::Type _vAlignment, Array<DisplayButton*>& _buttons, CharCoord _innerPadding, CharCoord _spacing)
{
	CharCoords at = CharCoords(_rect.x.min, _rect.y.min);

	ARRAY_STACK(CharCoord, buttonHeights, _buttons.get_size());

	int buttonSpacing = _spacing;
	int buttonPadding = _innerPadding * 2;
	int totalButtonHeight = 0;
	for_every_ptr(button, _buttons)
	{
		CharCoord buttonHeight = button->calc_caption_height(this, _rect.x.length());
		buttonHeights.push_back(buttonHeight);
		totalButtonHeight += buttonHeight;
	}
	if (totalButtonHeight + buttonSpacing * (_buttons.get_size() - 1) + buttonPadding * _buttons.get_size() > _rect.y.length())
	{
		buttonSpacing = 0;
		if (totalButtonHeight + buttonSpacing * (_buttons.get_size() - 1) + buttonPadding * _buttons.get_size() > _rect.y.length())
		{
			buttonPadding = 0;
		}
	}
	totalButtonHeight += buttonSpacing * (_buttons.get_size() - 1) + buttonPadding * _buttons.get_size();

	if (_vAlignment == DisplayVAlignment::Top)
	{
		at.y += _rect.y.length() - totalButtonHeight;
	}
	else if (_vAlignment == DisplayVAlignment::Centre)
	{
		at.y += (_rect.y.length() - totalButtonHeight) / 2;
	}

	for_every_reverse_ptr(button, _buttons)
	{
		CharCoord buttonHeight = buttonHeights[for_everys_index(button)];
		buttonHeight += buttonPadding;
		button->size(CharCoords(_rect.x.length(), buttonHeight))
			->at(at);

		add(button);

		at.y += buttonHeight + buttonSpacing;
	}

	return totalButtonHeight;
}

int Display::place_horizontally_and_add(RangeCharCoord2 const & _rect, DisplayHAlignment::Type _hAlignment, Array<DisplayButton*>& _buttons, DisplayVAlignment::Type _vAlignment, CharCoord _vPadding, CharCoord _maxButtonWidth, CharCoord _innerPadding, CharCoord _spacing, bool _keepPaddingAndSpacing)
{
	int result = place_horizontally(_rect, _hAlignment, _buttons, _vAlignment, _vPadding, _maxButtonWidth, _innerPadding, _spacing, _keepPaddingAndSpacing);

	for_every_ptr(button, _buttons)
	{
		add(button);
	}

	return result;
}

int Display::place_horizontally(RangeCharCoord2 const & _rect, DisplayHAlignment::Type _hAlignment, Array<DisplayButton*>& _buttons, DisplayVAlignment::Type _vAlignment, CharCoord _vPadding, CharCoord _maxButtonWidth, CharCoord _innerPadding, CharCoord _spacing, bool _keepPaddingAndSpacing)
{
	CharCoords at = CharCoords(_rect.x.min, _rect.y.min);

	ARRAY_STACK(CharCoord, buttonWidths, _buttons.get_size());

	int maxButtonHeight = 0;
	int buttonSpacing = _spacing;
	int buttonPadding = _innerPadding * 2;
	int totalButtonWidth = 0;
	for_every_ptr(button, _buttons)
	{
		int linesCount;
		CharCoord buttonWidth = button->calc_caption_width(this, _maxButtonWidth, &linesCount);
		maxButtonHeight = max(maxButtonHeight, linesCount);
		buttonWidths.push_back(buttonWidth);
		totalButtonWidth += buttonWidth;
	}
	if (!_keepPaddingAndSpacing)
	{
		if (totalButtonWidth + buttonSpacing * (_buttons.get_size() - 1) + buttonPadding * _buttons.get_size() > _rect.x.length())
		{
			buttonSpacing = 0;
			if (totalButtonWidth + buttonSpacing * (_buttons.get_size() - 1) + buttonPadding * _buttons.get_size() > _rect.x.length())
			{
				buttonPadding = 0;
			}
		}
	}
	totalButtonWidth += buttonSpacing * (_buttons.get_size() - 1) + buttonPadding * _buttons.get_size();

	if (totalButtonWidth > _rect.x.length() &&
		_buttons.get_size())
	{
		// reduce width of buttons
		bool keepTrying = true;
		int desiredWidth = _rect.x.length() / _buttons.get_size();
		while (keepTrying && totalButtonWidth > _rect.x.length())
		{
			keepTrying = false;
			for_every(buttonWidth, buttonWidths)
			{
				if (*buttonWidth > desiredWidth)
				{
					*buttonWidth = *buttonWidth - 1;
					--totalButtonWidth;
					keepTrying = true;
				}
			}
			if (!keepTrying)
			{
				--desiredWidth;
				if (desiredWidth >= 3)
				{
					keepTrying = true;
				}
			}
		}

		// recalculate max button height
		maxButtonHeight = 0;
		for_every_ptr(button, _buttons)
		{
			int linesCount;
			button->calc_caption_width(this, buttonWidths[for_everys_index(button)], &linesCount);
			maxButtonHeight = max(maxButtonHeight, linesCount);
		}
	}

	if (_hAlignment == DisplayHAlignment::Right)
	{
		at.x += _rect.x.length() - totalButtonWidth;
	}
	else if (_hAlignment == DisplayHAlignment::Centre)
	{
		at.x += (_rect.x.length() - totalButtonWidth) / 2;
	}
	else if (_hAlignment == DisplayHAlignment::CentreRight)
	{
		at.x += (_rect.x.length() - totalButtonWidth) / 2;
		if ((_rect.x.length() - totalButtonWidth) % 2 == 1)
		{
			at.x += 1;
		}
	}

	CharCoord buttonHeight = _vPadding == NONE ? _rect.y.length() : maxButtonHeight + _vPadding * 2;
	buttonHeight = min(buttonHeight, _rect.y.length());

	if (_vAlignment == DisplayVAlignment::Top)
	{
		at.y += _rect.y.length() - buttonHeight;
	}
	else if (_vAlignment == DisplayVAlignment::Centre)
	{
		at.y += (_rect.y.length() - buttonHeight) / 2;
	}

	for_every_ptr(button, _buttons)
	{
		CharCoord buttonWidth = buttonWidths[for_everys_index(button)];
		buttonWidth += buttonPadding;
		button->size(CharCoords(buttonWidth, buttonHeight))
			->horizontal_padding(buttonPadding / 2)
			->at(at);

		at.x += buttonWidth + buttonSpacing;
	}

	return buttonHeight;
}

int Display::place_on_band_and_add(RangeCharCoord2 const & _rect, DisplayHAlignment::Type _hAlignment, DisplayVAlignment::Type _vAlignment, Array<DisplayButton*>& _buttons, CharCoord _innerPadding, CharCoord _spacing, CharCoord _vPadding)
{
	return place_horizontally_and_add(_rect, _hAlignment, _buttons, _vAlignment, _vPadding, 0, _innerPadding, _spacing, true);
}

void Display::push_controls_lock()
{
	++controlsLockLevel;
	for_every(control, controls)
	{
		if (control->get()->get_controls_lock_level() == NONE)
		{
			if (control->get() == preShortcutControl.get())
			{
				preShortcutControl = nullptr;
			}
			control->get()->set_controls_lock_level(controlsLockLevel);
		}
	}
}

void Display::pop_controls_lock()
{
	--controlsLockLevel;
	for_every(control, controls)
	{
		if (control->get()->get_controls_lock_level() > controlsLockLevel)
		{
			control->get()->set_controls_lock_level(NONE);
		}
	}
}

void Display::pop_all_controls_lock()
{
	controlsLockLevel = NONE;
	for_every(control, controls)
	{
		if (control->get()->get_controls_lock_level() > controlsLockLevel)
		{
			control->get()->set_controls_lock_level(NONE);
		}
	}
}

void Display::push_controls(int _keepThemAtLevel, bool _noRedrawNeeded)
{
	if (controlsStackLevel >= _keepThemAtLevel)
	{
		pop_controls(_keepThemAtLevel + 1);
	}
	controlsStackLevel = _keepThemAtLevel;
}

void Display::pop_controls(int _fromLevel, bool _noRedrawNeeded)
{
	if (controlsStackLevel < _fromLevel)
	{
		return;
	}
	controlsStackLevel = max(NONE, _fromLevel - 1);
	ARRAY_STACK(DisplayControl*, controlsToRemove, controls.get_size());
	for_every(control, controls)
	{
		if (control->get()->get_controls_stack_level() > controlsStackLevel)
		{
			controlsToRemove.push_back(control->get());
		}
	}

	for_every_ptr(control, controlsToRemove)
	{
		remove(control, _noRedrawNeeded, true);
	}
}

void Display::push_controls()
{
	push_controls(controlsStackLevel + 1);
}

void Display::pop_controls(bool _noRedrawNeeded)
{
	pop_controls(controlsStackLevel);
}

void Display::__doing_advancement_now()
{
#ifdef AN_DEVELOPMENT
#ifndef DISPLAY_LOCKLESS
	an_assert(!__rendering, TXT("advancing while rendering!"));
	an_assert(!__advancing, TXT("nested advancement?"));
#endif
	__advancing = true;
#endif
}

void Display::__done_advancement_now()
{
#ifdef AN_DEVELOPMENT
	__advancing = false;
#endif
}

void Display::__doing_rendering_now()
{
#ifdef AN_DEVELOPMENT
#ifndef DISPLAY_LOCKLESS
	an_assert(!__advancing, TXT("rendering while advancing!"));
	an_assert(!__rendering, TXT("nested rendering?"));
#endif
	__rendering = true;
#endif
}

void Display::__done_rendering_now()
{
#ifdef AN_DEVELOPMENT
	__rendering = false;
#endif
}

void Display::turn_on(bool _instant)
{
	if (state == DisplayState::Off)
	{
		// pretend it was no longer visible
		no_longer_visible();
	}
	state = DisplayState::On;
	if (_instant) powerOn = 1.0f;
}

void Display::no_longer_visible()
{
	ppOutputRenderTarget.clear();
	ppLastFrameOutputRenderTargets.clear();
}

void Display::set_retrace_at_pct(float _retraceAtPct)
{
	_retraceAtPct = mod(_retraceAtPct, 1.0f);
	retraceAtPct = _retraceAtPct;
	retraceVisibleAtPct = retraceAtPct;
}

template <typename Func>
void Display::perform_func_info(Array<FuncInfo<Func>> & _array, typename std::function<void(AN_NOT_CLANG_TYPENAME Func _perform)> _performFunc)
{
	ARRAY_STACK(void const *, owners, _array.get_size());
	for_every(fi, _array)
	{
		owners.push_back(fi->owner);
	}
	// do it this way to handle if array has been modified
	for_every_ptr(owner, owners)
	{
		for_every(fi, _array)
		{
			if (fi->owner == owner)
			{
				_performFunc(fi->perform);
				break;
			}
		}
	}
}

template <typename Func>
void Display::set_func_info(Array<FuncInfo<Func>> & _array, Func _perform, void const * _owner)
{
	for_every(info, _array)
	{
		if (info->owner == _owner)
		{
			if (_perform)
			{
				info->perform = _perform;
				return;
			}
			else
			{
				_array.remove_at(for_everys_index(info));
				return;
			}
		}
	}
	if (_perform)
	{
		FuncInfo<Func> funcInfo;
		funcInfo.perform = _perform;
		funcInfo.owner = _owner;
		_array.push_back(funcInfo);
	}
}

void Display::set_on_advance_display(void const * _owner, OnAdvanceDisplay _do)
{
	set_func_info(on_advance_display, _do, _owner);
}

void Display::set_on_update_display(void const * _owner, OnUpdateDisplay _do)
{
	set_func_info(on_update_display, _do, _owner);
}

void Display::set_on_render_display(void const * _owner, OnRenderDisplay _do)
{
	set_func_info(on_render_display, _do, _owner);
}

void Display::set_on_click(void const * _owner, OnClick _do)
{
	set_func_info(on_click, _do, _owner);
}

Vector2 Display::get_cursor_normalised_at() const
{
	Vector2 result = Vector2::half;
	if (screenSize.x > 1.0f)
	{
		result.x = clamp(cursorAt.x / (screenSize.x - 1.0f), 0.0f, 1.0f);
	}
	if (screenSize.y > 1.0f)
	{
		result.y = clamp(cursorAt.y / (screenSize.y - 1.0f), 0.0f, 1.0f);
	}
	return result;
}

Vector2 Display::get_cursor_normalised_moved_by() const
{
	Vector2 result = Vector2::zero;
	if (screenSize.x > 1.0f)
	{
		result.x = cursorMovedBy.x / (screenSize.x - 1.0f);
	}
	if (screenSize.y > 1.0f)
	{
		result.y = cursorMovedBy.y / (screenSize.y - 1.0f);
	}
	return result;
}

Vector2 Display::get_cursor_normalised_dragged_by() const
{
	Vector2 result = Vector2::zero;
	if (screenSize.x > 1.0f)
	{
		result.x = cursorDraggedBy.x / (screenSize.x - 1.0f);
	}
	if (screenSize.y > 1.0f)
	{
		result.y = cursorDraggedBy.y / (screenSize.y - 1.0f);
	}
	return result;
}

void Display::set_rotate_display(int _rotateDisplay)
{
	if (_rotateDisplay)
	{
		if (setup.wholeDisplayResolution.x != setup.wholeDisplayResolution.y)
		{
			warn(TXT("rotation display works only for square displays"));
			_rotateDisplay = 0;
		}
	}
	if (rotateDisplay != _rotateDisplay)
	{
		rotateDisplay = _rotateDisplay;
		update_post_process_frames();
	}
}
