#include "loaderHub.h"

#include "loaderHubScene.h"
#include "loaderHubScreen.h"
#include "widgets\lhw_button.h"
#include "widgets\lhw_image.h"
#include "scenes\lhs_beAtRightPlace.h"

#include "..\..\artDir.h"
#include "..\..\teaForGodMain.h"
#include "..\..\game\environmentPlayer.h"
#include "..\..\game\game.h"
#include "..\..\game\gameConfig.h"
#include "..\..\game\screenShotContext.h"
#include "..\..\library\library.h"
#include "..\..\overlayInfo\overlayInfoSystem.h"
#include "..\..\tutorials\tutorialSystem.h"

#include "..\..\..\framework\display\display.h"
#include "..\..\..\framework\display\displayUtils.h"
#include "..\..\..\framework\render\renderCamera.h"
#include "..\..\..\framework\render\renderContext.h"
#include "..\..\..\framework\render\renderScene.h"
#include "..\..\..\framework\sound\soundSample.h"
#include "..\..\..\framework\sound\soundScene.h"
#include "..\..\..\framework\video\font.h"
#include "..\..\..\framework\video\texturePart.h"
#include "..\..\..\framework\vr\vrMeshes.h"
#include "..\..\..\framework\world\environmentType.h"

#include "..\..\..\framework\library\usedLibraryStored.inl"

#include "..\..\..\core\globalSettings.h"
#include "..\..\..\core\mainConfig.h"
#include "..\..\..\core\debug\debugRenderer.h"
#include "..\..\..\core\mesh\skeleton.h"
#include "..\..\..\core\system\core.h"
#include "..\..\..\core\system\input.h"
#include "..\..\..\core\system\video\renderTarget.h"
#include "..\..\..\core\system\video\renderTargetUtils.h"
#include "..\..\..\core\system\video\video3dPrimitives.h"
#include "..\..\..\core\vr\iVR.h"
#include "..\..\..\core\vr\vrOffsets.h"

//

#ifdef AN_ALLOW_OPTIMIZE_OFF
#pragma optimize("", off)
#endif

//#define DEBUG_READING_POSES

#define EXTENSIVE_LOGS

#define LOG_HUB

#define FADE_TIME 0.1f

#ifdef AN_ALLOW_EXTENSIVE_LOGS
	#ifndef EXTENSIVE_LOGS
		#define EXTENSIVE_LOGS
	#endif
#endif

//

using namespace Loader;

// references
DEFINE_STATIC_NAME_STR(loaderHubClick, TXT("loader hub click"));
DEFINE_STATIC_NAME_STR(loaderHubMove, TXT("loader hub move"));
DEFINE_STATIC_NAME_STR(loaderHubDisplay, TXT("loader hub display"));
DEFINE_STATIC_NAME_STR(loaderHubHandControllerLeft, TXT("loader hub hand controller left"));
DEFINE_STATIC_NAME_STR(loaderHubHandControllerRight, TXT("loader hub hand controller right"));
DEFINE_STATIC_NAME_STR(loaderHubPointHandControllerLeft, TXT("loader hub point hand controller left"));
DEFINE_STATIC_NAME_STR(loaderHubPointHandControllerRight, TXT("loader hub point hand controller right"));
DEFINE_STATIC_NAME_STR(loaderHubHandTrackingLeft, TXT("loader hub hand tracking left"));
DEFINE_STATIC_NAME_STR(loaderHubHandTrackingRight, TXT("loader hub hand tracking right"));
DEFINE_STATIC_NAME_STR(loaderHubCursorLeft, TXT("loader hub cursor left"));
DEFINE_STATIC_NAME_STR(loaderHubCursorRight, TXT("loader hub cursor right"));
DEFINE_STATIC_NAME_STR(loaderHubLogoBigOneLine, TXT("loader hub logo big one line"));
DEFINE_STATIC_NAME_STR(loaderHubStartHeadMarker, TXT("loader hub start head marker"));
DEFINE_STATIC_NAME_STR(loaderHubStartHeadMarkerClose, TXT("loader hub start head marker close"));
DEFINE_STATIC_NAME_STR(loaderHubStartHeadMarkerFar, TXT("loader hub start head marker far"));
DEFINE_STATIC_NAME_STR(loaderHubHelpIcon, TXT("loader hub help icon"));
DEFINE_STATIC_NAME_STR(loaderHubTipRestartAtAnyChapter, TXT("loader hub tip restart at any chapter"));

// localised strings
DEFINE_STATIC_NAME_STR(lsTipSavingIcon, TXT("hub; common; tip; saving icon"));
DEFINE_STATIC_NAME_STR(lsTipHelpIcon, TXT("hub; common; tip; help icon"));
DEFINE_STATIC_NAME_STR(lsTipGameModifiers, TXT("hub; common; tip; game modifiers"));
DEFINE_STATIC_NAME_STR(lsTipGameDefinitions, TXT("hub; common; tip; game definitions"));
DEFINE_STATIC_NAME_STR(lsTipRestartAtAnyChapter, TXT("hub; common; tip; restart at any chapter"));
DEFINE_STATIC_NAME_STR(lsDismiss, TXT("hub; common; dismiss"));

// library name to hold generated mesh
DEFINE_STATIC_NAME_STR(loaderHub, TXT("loader hub"));
DEFINE_STATIC_NAME(scene);

// custom params
DEFINE_STATIC_NAME(voxelSizeCoef);
DEFINE_STATIC_NAME(loaderHubScreenInterior);

// shader params
DEFINE_STATIC_NAME(voxelSize);
DEFINE_STATIC_NAME(emissiveColour);
DEFINE_STATIC_NAME(emissiveBaseColour);
DEFINE_STATIC_NAME(emissivePower);
DEFINE_STATIC_NAME(beaconActive);
DEFINE_STATIC_NAME(beaconAtRel);
DEFINE_STATIC_NAME(beaconRActive);
DEFINE_STATIC_NAME(beaconZ);

// params
DEFINE_STATIC_NAME_STR(radius, TXT("radius"));
DEFINE_STATIC_NAME_STR(floorWidth, TXT("floor width"));
DEFINE_STATIC_NAME_STR(floorLength, TXT("floor length"));
DEFINE_STATIC_NAME_STR(sphereCentre, TXT("sphere centre"));

// sockets
DEFINE_STATIC_NAME(pointer);

// bones (hand tracking)
DEFINE_STATIC_NAME(pointerTip);
DEFINE_STATIC_NAME(pointerMid);
DEFINE_STATIC_NAME(pointerBase);
DEFINE_STATIC_NAME(middleTip);
DEFINE_STATIC_NAME(middleMid);
DEFINE_STATIC_NAME(middleBase);
DEFINE_STATIC_NAME(ringTip);
DEFINE_STATIC_NAME(ringMid);
DEFINE_STATIC_NAME(ringBase);
DEFINE_STATIC_NAME(pinkyTip);
DEFINE_STATIC_NAME(pinkyMid);
DEFINE_STATIC_NAME(pinkyBase);
DEFINE_STATIC_NAME(thumbTip);
DEFINE_STATIC_NAME(thumbMid);
DEFINE_STATIC_NAME(thumbBase);

// input
DEFINE_STATIC_NAME(select);
DEFINE_STATIC_NAME(shift);
DEFINE_STATIC_NAME(movePointer);
DEFINE_STATIC_NAME(debugMovement);
DEFINE_STATIC_NAME(debugMovementVerticalSwitch);
DEFINE_STATIC_NAME(debugMovementSlow);
DEFINE_STATIC_NAME(debugMovementSlowAlt);
DEFINE_STATIC_NAME(debugRotation);
DEFINE_STATIC_NAME(debugRotationSlow);
DEFINE_STATIC_NAME(debugRotationSlowAlt);

// fading reasons
DEFINE_STATIC_NAME(openingHub);
DEFINE_STATIC_NAME(closingHub);
DEFINE_STATIC_NAME(quitingGame);

// colours
DEFINE_STATIC_NAME(loader_hub_fade);

// screens
DEFINE_STATIC_NAME(hubBackground);
DEFINE_STATIC_NAME(autoTip);

// system tags
DEFINE_STATIC_NAME(lowGraphics);

//

/*
// light
Colour HubColours::widget_background() { return Colour::lerp(0.6f, Colour::black, Colour::white); }
Colour HubColours::widget_background_highlight() { return Colour::lerp(0.9f, Colour::black, Colour::white); }
Colour HubColours::screen_border() { return Colour::lerp(0.2f, Colour::black, Colour::white).with_alpha(0.8f); }
Colour HubColours::screen_interior() { return Colour::lerp(0.2f, Colour::black, Colour::white).with_alpha(0.12f); }
Colour HubColours::border() { return Colour::lerp(0.2f, Colour::black, Colour::white); }
Colour HubColours::mid() { return Colour::lerp(0.5f, Colour::black, Colour::white); }
Colour HubColours::selected_highlighted() { return Colour::c64Red; }
Colour HubColours::highlight() { return Colour::white; }
Colour HubColours::text() { return Colour::black; }
Colour HubColours::text_inv() { return Colour::white; }
Colour HubColours::warning() { return Colour::red; }
Colour HubColours::unavailable() { return Colour::black; }
Colour HubColours::unlockable() { return Colour::yellow; }
Colour HubColours::ink() { return Colour::blue; }
Colour HubColours::ink_highlighted() { return Colour::c64LightBlue; }
*/

// dark
Colour HubColours::widget_background() { return Colour::lerp(0.1f, Colour::black, Colour::white).with_alpha(0.6f); }
Colour HubColours::widget_background_highlight() { return Colour::lerp(0.2f, Colour::black, Colour::lerp(0.5f, Colour::c64Red, Colour::white)).with_alpha(0.8f); }
Colour HubColours::screen_border()
{
	if (::System::Core::get_system_tags().get_tag(NAME(lowGraphics)))
	{
		return Colour::lerp(0.25f, Colour::lerp(0.05f, Colour::black, Colour::blue), Colour::white);
	}
	else
	{
		return Colour::lerp(0.7f, Colour::black, Colour::lerp(0.75f, Colour::c64Red, Colour::white)).with_alpha(0.8f);
	}
}
Colour HubColours::screen_interior()
{
	if (::System::Core::get_system_tags().get_tag(NAME(lowGraphics)))
	{
		return Colour::lerp(0.15f, Colour::lerp(0.05f, Colour::black, Colour::blue), Colour::white);
	}
	else
	{
		return Colour::lerp(0.15f, Colour::lerp(0.05f, Colour::black, Colour::blue), Colour::white).with_alpha(0.4f);
		//return text().with_alpha(0.12f);
	}
	//return Colour::lerp(0.2f, Colour::blue, Colour::white).with_alpha(0.08f);
}
Colour HubColours::border() { return Colour::lerp(0.8f, Colour::black, Colour::white).with_alpha(0.8f); }
Colour HubColours::mid() { return Colour::lerp(0.5f, Colour::black, Colour::white); }
Colour HubColours::selected_highlighted() { return Colour::lerp(0.5f, Colour::c64LightBlue, Colour::white); }
Colour HubColours::highlight() { return Colour::lerp(0.2f, Colour::black, Colour::lerp(0.5f, Colour::c64LightBlue, Colour::white)); }
Colour HubColours::text() { return Colour::lerp(0.8f, Colour::black, Colour::white); }
Colour HubColours::text_bright() { return Colour::lerp(0.99f, Colour::black, Colour::white); }
Colour HubColours::text_inv() { return Colour::black; }
Colour HubColours::warning() { return Colour::lerp(0.5f, Colour::red, Colour::white); }
Colour HubColours::match() { return Colour::lerp(0.5f, Colour::green, Colour::white); }
Colour HubColours::unavailable() { return Colour::lerp(0.25f, Colour::black, Colour::white); }
Colour HubColours::unlockable() { return Colour::lerp(0.5f, Colour::yellow, Colour::white); }
Colour HubColours::ink() { return Colour::lerp(0.5f, Colour::blue, Colour::white); }
Colour HubColours::ink_highlighted() { return Colour::lerp(0.5f, Colour::c64LightBlue, Colour::white); }
Colour HubColours::button_cant() { return Colour::lerp(0.5f, Colour::red, Colour::white); }
Colour HubColours::button_not_yet() { return Colour::lerp(0.5f, Colour::blue, Colour::white); }
Colour HubColours::special_highlight() { return Colour::lerp(0.5f, Colour::gold, Colour::white); }
Colour HubColours::special_highlight_background() { return Colour::lerp(0.2f, Colour::black, Colour::lerp(0.5f, Colour::gold, Colour::white)); }
Colour HubColours::special_highlight_background_for(Optional<Colour> const& _colour)
{
	if (_colour.is_set())
	{
		float mix = _colour.get().to_vector4().to_vector3().length();
		return Colour::lerp(mix, Colour::black, Colour::lerp(0.5f, Colour::gold, Colour::white));
	}
	else
	{
		return special_highlight_background();
	}
}

//

Hub::QueuedForcedClicks::QueuedForcedClicks()
{
}

Hub::QueuedForcedClicks::QueuedForcedClicks(HubWidget* _widget, bool _shifted)
: widget(_widget)
, shifted(_shifted)
{
}

//

Optional<Rotator3> Hub::s_lastForward;
System::TimeStamp Hub::s_lastForwardTimeStamp;
bool Hub::s_autoPointWithWholeHands[] = { false, false };

Hub::Hub(TeaForGodEmperor::Game* _game, tchar const* _loaderName, Optional<Name> const& _sceneGlobalReferenceName,
	Optional<Name> const& _backgroundGlobalReferenceName, Optional<Name> const& _environmentGlobalReferenceName, bool _forceRecreateSceneMesh)
: game(_game)
{
	SET_EXTRA_DEBUG_INFO(queuedForcedClicks, TXT("Hub.queuedForcedClicks"));

	loaderName = _loaderName? _loaderName : TXT("main");
	forceRecreateSceneMesh = _forceRecreateSceneMesh;

	timeSinceLastActive.reset(-10.0f); // this way it will be considered not active right away

	// what the hell is going on here?
	// I ask myself again a question about it
	lazyScreenRenderCount = 2;
	if (::System::Core::get_system_tags().get_tag(NAME(lowGraphics)))
	{
		lazyScreenRenderCount = 4;
	}
	lazyScreenRenderCount = 4;

	if (Framework::DisplayType * displayType = ::Framework::Library::get_current()->get_global_references().get<Framework::DisplayType>(NAME(loaderHubDisplay)))
	{
		if (auto * font = displayType->get_setup().get_font().get())
		{
			HubScreen::s_fontSizeInPixels = font->get_font()->calculate_char_size();
			use_font(font);
		}
	}

	fadeColour = RegisteredColours::get_colour(NAME(loader_hub_fade));
	screenInteriorColour = HubColours::screen_interior();

	if (auto* library = Framework::Library::get_current())
	{
		if (_sceneGlobalReferenceName.is_set())
		{
			sceneMesh = library->get_global_references().get<Framework::Mesh>(_sceneGlobalReferenceName.get(), true);
			sceneMeshGenerator = library->get_global_references().get<Framework::MeshGenerator>(_sceneGlobalReferenceName.get(), true);
		}
		if (_backgroundGlobalReferenceName.is_set())
		{
			backgroundMesh = library->get_global_references().get<Framework::Mesh>(_backgroundGlobalReferenceName.get(), true);
			backgroundMeshGenerator = library->get_global_references().get<Framework::MeshGenerator>(_backgroundGlobalReferenceName.get(), true);
		}
		if (_environmentGlobalReferenceName.is_set())
		{
			environment = library->get_global_references().get<Framework::EnvironmentType>(_environmentGlobalReferenceName.get(), true);
		}
	}

	if (environment.get())
	{
		screenInteriorColour = environment->get_custom_parameters().get_value(NAME(loaderHubScreenInterior), screenInteriorColour);
		if (! ::System::Core::get_system_tags().get_tag(NAME(lowGraphics)))
		{
			// keep alpha for non quest
			screenInteriorColour.a = HubColours::screen_interior().a;
		}
	}

	update_background_mesh();
	update_scene_mesh();

	if (auto* library = Framework::Library::get_current())
	{
		clickSample.set(library->get_global_references().get<Framework::Sample>(NAME(loaderHubClick)));
		moveSample.set(library->get_global_references().get<Framework::Sample>(NAME(loaderHubMove)));

		helpIcon = library->get_global_references().get<Framework::TexturePart>(NAME(loaderHubHelpIcon));
		restartAtAnyChapterInfo = library->get_global_references().get<Framework::TexturePart>(NAME(loaderHubTipRestartAtAnyChapter));

		hands[::Hand::Left].meshController = library->get_global_references().get<Framework::Mesh>(NAME(loaderHubHandControllerLeft));
		hands[::Hand::Right].meshController = library->get_global_references().get<Framework::Mesh>(NAME(loaderHubHandControllerRight));
		hands[::Hand::Left].meshPointHandController = library->get_global_references().get<Framework::Mesh>(NAME(loaderHubPointHandControllerLeft));
		hands[::Hand::Right].meshPointHandController = library->get_global_references().get<Framework::Mesh>(NAME(loaderHubPointHandControllerRight));
		hands[::Hand::Left].meshHandTracking = library->get_global_references().get<Framework::Mesh>(NAME(loaderHubHandTrackingLeft));
		hands[::Hand::Right].meshHandTracking = library->get_global_references().get<Framework::Mesh>(NAME(loaderHubHandTrackingRight));
		hands[::Hand::Left].cursor = library->get_global_references().get<Framework::TexturePart>(NAME(loaderHubCursorLeft));
		hands[::Hand::Right].cursor = library->get_global_references().get<Framework::TexturePart>(NAME(loaderHubCursorRight));
#ifdef AN_DEVELOPMENT_OR_PROFILER
		if (!VR::IVR::get())
		{
			hands[::Hand::Left].cursor = hands[::Hand::Right].cursor;
		}
#endif

		for_count(int, handIdx, 2)
		{
			auto & hand = hands[handIdx];
			DEFINE_STATIC_NAME(loaderHub);
			hand.input.use(NAME(loaderHub));
			hand.input.use(handIdx == ::Hand::Left ? Framework::GameInputIncludeVR::Left : Framework::GameInputIncludeVR::Right);
#ifdef AN_DEVELOPMENT_OR_PROFILER
			if (!VR::IVR::get())
			{
				hand.input.use(Framework::GameInputIncludeVR::None);
			}
#endif

			hand.meshInstanceController.set_mesh(hand.meshController->get_mesh());
			hand.meshInstancePointHandController.set_mesh(hand.meshPointHandController->get_mesh());
			hand.meshInstanceHandTracking.set_mesh(hand.meshHandTracking->get_mesh(), hand.meshHandTracking->get_skeleton()? hand.meshHandTracking->get_skeleton()->get_skeleton() : nullptr);
			Framework::MaterialSetup::apply_material_setups(hand.meshController->get_material_setups(), hand.meshInstanceController, ::System::MaterialSetting::IndividualInstance);
			Framework::MaterialSetup::apply_material_setups(hand.meshPointHandController->get_material_setups(), hand.meshInstancePointHandController, ::System::MaterialSetting::IndividualInstance);
			Framework::MaterialSetup::apply_material_setups(hand.meshHandTracking->get_material_setups(), hand.meshInstanceHandTracking, ::System::MaterialSetting::IndividualInstance);
			hand.meshInstanceController.fill_materials_with_missing();
			hand.meshInstancePointHandController.fill_materials_with_missing();
			hand.meshInstanceHandTracking.fill_materials_with_missing();
			hand.pointerSocket.set_name(NAME(pointer));
			hand.pointerSocket.look_up(hand.meshController.get());
			hand.pointHandSocket.set_name(NAME(pointer));
			hand.pointHandSocket.look_up(hand.meshPointHandController.get());
			int tempBoneIdx;
			hand.pointerMS = Transform::identity;
			hand.meshController->get_socket_info(hand.pointerSocket.get_index(), OUT_ tempBoneIdx, OUT_ hand.pointerMS);
			hand.pointHandMS = Transform::identity;
			hand.meshPointHandController->get_socket_info(hand.pointHandSocket.get_index(), OUT_ tempBoneIdx, OUT_ hand.pointHandMS);
		}

		startHeadMarkerMesh = library->get_global_references().get<Framework::Mesh>(NAME(loaderHubStartHeadMarker));
		startHeadMarkerMeshInstance.set_mesh(startHeadMarkerMesh->get_mesh());
		Framework::MaterialSetup::apply_material_setups(startHeadMarkerMesh->get_material_setups(), startHeadMarkerMeshInstance, ::System::MaterialSetting::IndividualInstance);
		startHeadMarkerMeshInstance.fill_materials_with_missing();

		startHeadMarkerCloseMesh = library->get_global_references().get<Framework::Mesh>(NAME(loaderHubStartHeadMarkerClose));
		startHeadMarkerCloseMeshInstance.set_mesh(startHeadMarkerCloseMesh->get_mesh());
		Framework::MaterialSetup::apply_material_setups(startHeadMarkerCloseMesh->get_material_setups(), startHeadMarkerCloseMeshInstance, ::System::MaterialSetting::IndividualInstance);
		startHeadMarkerCloseMeshInstance.fill_materials_with_missing();

		startHeadMarkerFarMesh = library->get_global_references().get<Framework::Mesh>(NAME(loaderHubStartHeadMarkerFar));
		startHeadMarkerFarMeshInstance.set_mesh(startHeadMarkerFarMesh->get_mesh());
		Framework::MaterialSetup::apply_material_setups(startHeadMarkerFarMesh->get_material_setups(), startHeadMarkerFarMeshInstance, ::System::MaterialSetting::IndividualInstance);
		startHeadMarkerFarMeshInstance.fill_materials_with_missing();
	}

	update_graphics();

	//useSceneScale = 0.03f;
	//useSceneScale = 0.06f;
	useSceneScale = 0.05f;
	if (!VR::IVR::get())
	{
		useSceneScale = 1.0f;
	}
}

Hub::~Hub()
{
}

void Hub::update_scene_mesh()
{
	float floorWidth = 2.0f;
	float floorLength = 2.0f;
	if (auto* vr = VR::IVR::get())
	{
		floorWidth = vr->get_play_area_rect_half_right().length() * 2.0f;
		floorLength = vr->get_play_area_rect_half_forward().length() * 2.0f;
	}
	else if (MainConfig::global().should_be_immobile_vr())
	{
		floorWidth = MainConfig::global().get_immobile_vr_size().x;
		floorLength = MainConfig::global().get_immobile_vr_size().y;
	}

	float voxelSizeCoef = 1.0f;
	Framework::MeshGenerator* useMeshGenerator = sceneMeshGenerator.get();
	if (useMeshGenerator)
	{
		voxelSizeCoef = useMeshGenerator->get_custom_parameters().get_value<float>(NAME(voxelSizeCoef), voxelSizeCoef);
	}
	if (forceRecreateSceneMesh ||
		!sceneMesh.is_set() ||
		floorWidth != hubInfo.floorWidth ||
		floorLength != hubInfo.floorLength)
	{
		an_assert(useMeshGenerator);

		hubInfo.floorWidth = floorWidth;
		hubInfo.floorLength = floorLength;
		hubInfo.radius = clamp(sqrt(sqr(hubInfo.floorWidth) + sqr(hubInfo.floorLength)) * 1.3f, 40.0f, 100.0f);

		SimpleVariableStorage parameters;
		parameters.access<float>(NAME(radius)) = hubInfo.radius;
		parameters.access<float>(NAME(floorWidth)) = hubInfo.floorWidth;
		parameters.access<float>(NAME(floorLength)) = hubInfo.floorLength;
		parameters.access<Vector3>(NAME(sphereCentre)) = hubInfo.sphereCentre;

		sceneMesh = useMeshGenerator->generate_temporary_library_mesh(Framework::Library::get_current(), Framework::MeshGeneratorRequest().using_parameters(parameters).no_lods());
	}

	if (sceneMesh.get())
	{
		sceneMeshInstance.set_mesh(sceneMesh->get_mesh());
		Framework::MaterialSetup::apply_material_setups(sceneMesh->get_material_setups(), sceneMeshInstance, ::System::MaterialSetting::IndividualInstance);
		sceneMeshInstance.fill_materials_with_missing();
		for_count(int, idx, sceneMeshInstance.get_material_instance_count())
		{
			sceneMeshInstance.access_material_instance(idx)->set_uniform(NAME(voxelSize), voxelSizeCoef * hubInfo.radius / 12.0f);
			sceneMeshInstance.access_material_instance(idx)->set_uniform(NAME(emissiveColour), emissiveColourOverride.get(RegisteredColours::get_colour(String(TXT("loader_hub_floor_emissive")))).with_alpha(0.0f).to_vector4());
			sceneMeshInstance.access_material_instance(idx)->set_uniform(NAME(emissiveBaseColour), emissiveColourOverride.get(RegisteredColours::get_colour(String(TXT("loader_hub_floor_emissive")))).to_vector4());
			sceneMeshInstance.access_material_instance(idx)->set_uniform(NAME(emissivePower), 1.0f);
		}
	}
	else
	{
		sceneMeshInstance.set_mesh(nullptr);
	}
}

void Hub::update_background_mesh()
{
	Framework::MeshGenerator* useMeshGenerator = backgroundMeshGenerator.get();

	if (!backgroundMesh.is_set() &&
		useMeshGenerator)
	{
		an_assert(useMeshGenerator);

		backgroundMesh = useMeshGenerator->generate_temporary_library_mesh(Framework::Library::get_current(), Framework::MeshGeneratorRequest().no_lods().using_random_random_regenerator());
	}

	if (backgroundMesh.get())
	{
		backgroundMeshInstance.set_mesh(backgroundMesh->get_mesh());
		Framework::MaterialSetup::apply_material_setups(backgroundMesh->get_material_setups(), backgroundMeshInstance, ::System::MaterialSetting::IndividualInstance);
		backgroundMeshInstance.fill_materials_with_missing();
	}
	else
	{
		backgroundMeshInstance.set_mesh(nullptr);
	}
}

Hub & Hub::set_scene(HubScene* _scene)
{
	if (_scene)
	{
		_scene->set_hub(this);
	}
#ifdef LOG_HUB
	output(TXT("[hub] switch scene to %p"), _scene);
#endif
	nextScene = _scene;
	if (is_active())
	{
		activate_next_scene();
	}
	return *this;
}

void Hub::activate_next_scene()
{
	if (currentScene.is_set())
	{
		currentScene->on_deactivate(nextScene.get());
	}
	update_graphics();
	RefCountObjectPtr<HubScene> prevScene = currentScene;
	currentScene = nextScene;
	nextScene.clear();

#ifdef LOG_HUB
	output(TXT("[hub] activate scene to %p"), currentScene.get());
#endif

	if (currentScene.is_set())
	{
		currentScene->on_activate(prevScene.get());
	}
}

void Hub::remove_all_screens()
{
#ifdef LOG_HUB
	output(TXT("[hub] remove all screens"));
#endif
	for_every_ref(screen, screens)
	{
		screen->clear();
	}
	screens.clear();
}

void Hub::remove_old_screens()
{
	for (int i = 0; i < screens.get_size(); ++i)
	{
		auto* screen = screens[i].get();
		if (screen->timeSinceLastActive.get_time_since() > 0.5f)
		{
#ifdef LOG_HUB
			output(TXT("[hub] remove old screen %p \"%S\""), screen, screen->id.to_char());
#endif
			screen->clear();
			screens.remove_at(i);
			--i;
		}
	}
}

void Hub::remove_deactivated_screens()
{
	for (int i = 0; i < screens.get_size(); ++i)
	{
		auto& screen = screens[i];
		if (!screen->is_active())
		{
			screen->clear();
#ifdef LOG_HUB
			output(TXT("[hub] remove deactivated screen %p \"%S\""), screen.get(), screen->id.to_char());
#endif
			screens.remove_at(i);
			--i;
		}
	}
}

bool Hub::activate()
{
	bool activated = base::activate();

	{
		bool changed = false;
		changed |= System::FoveatedRenderingSetup::set_temporary_foveation_level_offset(NP);
		if (changed)
		{
			System::FoveatedRenderingSetup::force_set_foveation();
		}
	}

	TeaForGodEmperor::TutorialSystem::set_active_hub(this);

	// stop all sounds to be hearable
	game->manage_game_sounds_pause(true);

	if (auto * vr = VR::IVR::get())
	{
		vr->set_render_mode(VR::RenderMode::HiRes);
	}
	::System::FoveatedRenderingSetup::set_hi_res_scene(true);

	TeaForGodEmperor::ArtDir::set_global_tint(); // reset/clear
	TeaForGodEmperor::ArtDir::set_global_desaturate(); // reset/clear

	if (activated)
	{
		wantsToExit = false;
		requiresToFadeOut = false;
		if (game->get_fade_out_target() != 0.0f)
		{
			// fade in quickly
			game->fade_in(0.2f);
		}

		remove_old_screens();

		if (timeSinceLastActive.get_time_since() > 0.5f)
		{
#ifdef EXTENSIVE_LOGS
			REMOVE_AS_SOON_AS_POSSIBLE_ output(TXT("[LoaderHub] on activate, clear forward"));
#endif
			forward.clear();
			viewForward.clear();
			initialForward.clear();
			currentScene.clear();
			referencePoint.clear();
		}
		
		//

		// do it before updating forward, as otherwise we will be offset
		if (timeSinceLastActive.get_time_since() > 0.5f)
		{
			if (auto* vr = VR::IVR::get())
			{
				vr->reset_immobile_origin_in_vr_space();
			}
		}

		update_forward();

		store_start_movement_centre();
		show_start_movement_centre(false);
		set_beacon_active(false);

		//

		activate_next_scene();

		if (timeSinceLastActive.get_time_since() > 0.5f)
		{
			beaconActiveValue = 0.0f;
			beaconRActiveValue = 0.0f;
			beaconFarActiveValue = 1.0f;
#ifdef EXTENSIVE_LOGS
			REMOVE_AS_SOON_AS_POSSIBLE_ output(TXT("[LoaderHub] on activate, maybe we should realign background mesh instance?"));
#endif
			if (forward.is_set())
			{
#ifdef EXTENSIVE_LOGS
				REMOVE_AS_SOON_AS_POSSIBLE_ output(TXT("[LoaderHub] we have forward, direction, do so %.1f'"), forward.get().yaw);
#endif
				backgroundMeshInstance.set_placement(look_matrix(Vector3::zero, (forward.get() * Rotator3(0.0f, 1.0f, 0.0f)).get_forward(), Vector3::zAxis).to_transform());
			}
		}
	}

	timeSinceLastActive.reset();

	return activated;
}

void Hub::update_forward(bool _initialToo)
{
	auto* vr = VR::IVR::get();

#ifdef DEBUG_READING_POSES 
	output(TXT("[hub] Hub::update_forward"));
#endif
	if (vr)
	{
		if (vr->is_render_view_valid())
		{
			Transform viewPlacement = vr->get_render_view();

			Vector3 forwardAxis = viewPlacement.get_axis(Axis::Forward);
			if (!forward.is_set() || (abs(forwardAxis.z) < 0.707f && does_allow_updating_forward()))
			{
				forward = Rotator3(0.0f, Rotator3::get_yaw(forwardAxis), 0.0f);

#ifdef DEBUG_READING_POSES 
				output(TXT("[hub] forward % S"), forward.get().to_string().to_char());
#endif
			}
			if (!viewForward.is_set() || (abs(forwardAxis.z) < 0.707f))
			{
				viewForward = Rotator3(0.0f, Rotator3::get_yaw(forwardAxis), 0.0f);
			}
			if (!referencePoint.is_set())
			{
#ifdef DEBUG_READING_POSES 
				output(TXT("[hub] set reference point to %S"), viewPlacement.get_translation().to_string().to_char());
#endif
			}
			referencePoint.set_if_not_set(viewPlacement.get_translation());
		}
	}
	else
	{
		forward = Rotator3(0.0f, lastViewRotation.yaw, 0.0f);
		viewForward = Rotator3(0.0f, lastViewRotation.yaw, 0.0f);
		referencePoint.set_if_not_set(Vector3::zero);
	}

	if (!forward.is_set())
	{
		forward = Rotator3::zero; // better this than nothing
	}

	if (!viewForward.is_set())
	{
		viewForward = Rotator3::zero; // better this than nothing
	}

	if (!initialForward.is_set() || _initialToo)
	{
		initialForward = forward;
	}
}

void Hub::store_start_movement_centre()
{
	startMovementCentre.clear();
	auto* vr = VR::IVR::get();

	if (vr && vr->get_last_valid_render_pose_set().view.is_set()) // although it should always be valid at this point!
	{
		startMovementCentre = vr->get_last_valid_render_pose_set().view.get().to_world(vr->get_movement_centre_in_view_space());
	}
}

void Hub::deactivate()
{
	if (auto* cs = currentScene.get())
	{
		cs->on_loader_deactivate();
	}

	if (allowDeactivatingWithLoaderImmediately)
	{
		deactivate_now_as_loader();
	}
}

void Hub::deactivate_now_as_loader()
{
	TeaForGodEmperor::TutorialSystem::set_active_hub(nullptr);

	if (fadeOutAllowed)
	{
		requiresToFadeOut = game->get_fade_out() < 1.0f;
	}
	base::deactivate();

	// sounds will resume on sound update

	if (auto * vr = VR::IVR::get())
	{
		vr->set_render_mode(VR::RenderMode::Normal);
	}
	::System::FoveatedRenderingSetup::set_hi_res_scene(false);

	// we no longer need scenes
	currentScene.clear();
	nextScene.clear();
}

void Hub::force_reset_and_update_forward()
{
	reset_last_forward();
	forward.clear();
	viewForward.clear();
	referencePoint.clear();
	update_forward(true);
}

void Hub::reset_last_forward()
{
#ifdef EXTENSIVE_LOGS
	REMOVE_AS_SOON_AS_POSSIBLE_ output(TXT("[hub] reset last forward"));
#endif
	s_lastForward.clear();
	s_lastForwardTimeStamp.reset();
}

void Hub::update(float _deltaTime)
{
	scoped_call_stack_info(TXT("hub update"));

	allowedCreateDisplayAndMesh = 1; // one screen per frame

	{
		Optional<Transform> markerPlacement;
		if (TeaForGodEmperor::Game::is_using_sliding_locomotion())
		{
			beaconActive = false;
		}
		else
		{
			markerPlacement = calculate_start_movement_centre_marker();
			bool beaconFarActive = false;
			if (markerPlacement.is_set())
			{
				float dist2D = (lastMovementCentrePlacement.get_translation() - markerPlacement.get().get_translation()).length_2d();
				if (dist2D > 0.4f)
				{
					float const halfHeight = 2.0f * 0.5f;
					Vector3 markerCentre = markerPlacement.get().get_translation() + Vector3::zAxis * halfHeight;
					dist2D = max(0.1f, dist2D);
					float longDist = sqrt(sqr(halfHeight) + sqr(dist2D));
					float dotSize = dist2D / max(0.1f, longDist);
					// alternatively:
					// float tanSize = halfHeight / dist2D;
					// float degSize = atan_deg(tanSize);
					// float dotSize = cos_deg(degSize);
					float offCentre = Vector3::dot(lastMovementCentrePlacement.location_to_local(markerCentre), Vector3::yAxis);
					beaconFarActive = offCentre < dotSize;
				}
			}
			beaconFarActiveValue = blend_to_using_speed_based_on_time(beaconFarActiveValue, beaconFarActive ? 1.0f : 0.0f, beaconActiveValue == 0.0f ? 0.0f : 0.5f, _deltaTime);
			beaconActiveValue = blend_to_using_speed_based_on_time(beaconActiveValue, beaconActive && markerPlacement.is_set() ? 1.0f : 0.0f, 0.5f, _deltaTime);
			beaconRActiveValue = blend_to_using_speed_based_on_time(beaconRActiveValue, beaconRActive && beaconActive && markerPlacement.is_set() ? 1.0f : 0.0f, 0.5f, _deltaTime);
		}

		{
			bool useNormal = beaconActive && markerPlacement.is_set();
			if (auto* vr = VR::IVR::get())
			{
				vr->set_render_mode(useNormal ? VR::RenderMode::Normal : VR::RenderMode::HiRes);
			}
#ifdef AN_ANDROID
			::System::FoveatedRenderingSetup::set_hi_res_scene(!useNormal);
#else
			::System::FoveatedRenderingSetup::set_hi_res_scene(true);
#endif
		}
	}

	auto* vr = VR::IVR::get();

	if (vr)
	{
		if (vr->get_controls().requestRecenter)
		{
#ifdef DEBUG_READING_POSES 
#endif
			output(TXT("[hub] recenter"));
			if (MainConfig::global().should_be_immobile_vr())
			{
				// for immobile vr this should be handled by immobile vr's origin
				// we only need to reset reference point to match origin at zero
				referencePoint.clear();
				update_forward(); // to get the right value for reference point
			}
			else
			{
				Optional<Rotator3> prevForward = forward;
				force_reset_and_update_forward();
				if (prevForward.is_set() && forward.is_set())
				{
					rotate_screens(((forward.get() - prevForward.get()) * Rotator3(0.0f, 1.0f, 0.0f)).normal_axes());
				}
			}
		}
	}

#ifdef AN_DEVELOPMENT_OR_PROFILER
	if (vr && allowDebugMovement)
	{
		if (auto* g = TeaForGodEmperor::Game::get_as<TeaForGodEmperor::Game>())
		{
			auto& input = g->access_all_time_controls_input();
			Vector2 movementStick = input.get_stick(NAME(debugMovement));
			bool verticalMovement = input.is_button_pressed(NAME(debugMovementVerticalSwitch));
			// clamp to sane values
			movementStick.x = clamp(movementStick.x, -1.0f, 1.0f);
			movementStick.y = clamp(movementStick.y, -1.0f, 1.0f);
			Vector3 moveBy = Vector3::zero;
			float movementSpeed = 1.0f * _deltaTime;
			if (::System::Input::get()->is_grabbed())
			{
				if (input.is_button_pressed(NAME(debugMovementSlow)))
				{
					movementSpeed *= 0.1f;
				}
				if (input.is_button_pressed(NAME(debugMovementSlowAlt)))
				{
					movementSpeed *= 0.1f;
				}
			}
			moveBy.x = movementStick.x * movementSpeed;
			if (verticalMovement)
			{
				moveBy.z = movementStick.y * movementSpeed;
			}
			else
			{
				moveBy.y = movementStick.y * movementSpeed;
			}
			//
			Vector2 rotationStick = input.get_stick(NAME(debugRotation));
			// clamp to sane values
			rotationStick.x = clamp(rotationStick.x, -1.0f, 1.0f);
			rotationStick.y = clamp(rotationStick.y, -1.0f, 1.0f);
			Rotator3 rotateBy = Rotator3::zero;
			float rotationSpeed = 180.0f * _deltaTime;
			if (::System::Input::get()->is_grabbed())
			{
				if (input.is_button_pressed(NAME(debugRotationSlow)))
				{
					rotationSpeed *= 0.1f;
				}
				if (input.is_button_pressed(NAME(debugRotationSlowAlt)))
				{
					rotationSpeed *= 0.1f;
				}
			}
			rotateBy.yaw = rotationStick.x * rotationSpeed;
			vr->move_relative_dev_in_vr_space(moveBy, rotateBy);
		}
	}
#endif

	update_forward();

	if (currentScene.is_set())
	{
		scoped_call_stack_info(TXT("update current scene"));
		currentScene->on_update(_deltaTime);

		static ArrayStatic<int, 32> shownAutoTipIds; SET_EXTRA_DEBUG_INFO(shownAutoTipIds, TXT("Hub::update.shownAutoTipIds"));
		static Optional<float> autoHideIn;
		static Optional<float> blockShowingFor;

		if (currentScene->does_allow_auto_tips())
		{
			if (! get_screen(NAME(autoTip)))
			{
				Name show;
				Array<Name> availableAutoTips;
				if (blockShowingFor.is_set())
				{
					blockShowingFor = blockShowingFor.get() - _deltaTime;
					if (blockShowingFor.get() <= 0.0f)
					{
						blockShowingFor.clear();
					}
				}
				else 
				{
#ifdef WITH_SAVING_ICON
					if (TeaForGodEmperor::PlayerPreferences::should_show_saving_icon())
					{
						availableAutoTips.push_back(NAME(lsTipSavingIcon));
					}
#endif
					if (false)
					{
						auto& ps = TeaForGodEmperor::PlayerSetup::access_current();
						if (auto* gs = ps.access_active_game_slot())
						{
							if (gs->unlockedPilgrimages.get_size() > 1
#ifdef BUILD_PREVIEW
								|| true /* easy access to all chapters for preview */
#endif				
								)
							{
								availableAutoTips.push_back(NAME(lsTipRestartAtAnyChapter));
							}
						}
					}
					availableAutoTips.push_back(NAME(lsTipHelpIcon));
					availableAutoTips.push_back(NAME(lsTipGameModifiers));
					availableAutoTips.push_back(NAME(lsTipGameDefinitions));
					for (int i = 0; i < availableAutoTips.get_size(); ++i)
					{
						if (shownAutoTipIds.does_contain(availableAutoTips[i].get_index()))
						{
							availableAutoTips.remove_at(i);
							--i;
						}
					}
					for (int i = 0; i < availableAutoTips.get_size(); ++i)
					{
						if (!currentScene->does_allow_auto_tip(availableAutoTips[i]))
						{
							availableAutoTips.remove_at(i);
							--i;
						}
					}
					if (!availableAutoTips.is_empty())
					{
						show = availableAutoTips.get_first();
					}
				}
				if (show.is_valid())
				{
					shownAutoTipIds.push_back_unique(show.get_index());
					String text = LOC_STR(show);
					String dismissText = LOC_STR(NAME(lsDismiss));
					Vector2 usePPA = Vector2(16.0f, 16.0f);
					auto const& fs = HubScreen::s_fontSizeInPixels;
					float fsxy = max(fs.x, fs.y);
					int width;
					int lineCount;
					float buttonHeight = fs.y * 2.0f;
					if (!text.does_contain('~'))
					{
						int const maxLength = 40;
						if (text.get_length() > maxLength)
						{
							int at = 0;
							int beginAt = at;
							int spaceAt = at;
							while (at < text.get_length())
							{
								if (text[at] == ' ')
								{
									if (at - beginAt > maxLength)
									{
										if (spaceAt > beginAt)
										{
											// at last space
											text[spaceAt] = '~';
											beginAt = spaceAt;
										}
										else
										{
											// at this space
											text[at] = '~';
											beginAt = at;
										}
									}
									spaceAt = at;
								}
								++at;
							}
						}
					}
					HubWidgets::Text::measure(get_font(), NP, NP, text, lineCount, width);
					Vector2 textRequiredSize = Vector2((float)(width) + 4.0f * fsxy, ((float)(lineCount)) * fsxy + fsxy /*separator*/ + buttonHeight);
					Vector2 requiredSize(textRequiredSize.x, textRequiredSize.y + fsxy * 2.0f);

					float imageTextSeparator = fsxy * 2.0f;
					float imageScale = 2.0f;
					Framework::TexturePart* image = nullptr;
					if (auto* g = TeaForGodEmperor::Game::get_as<TeaForGodEmperor::Game>())
					{
						if (show == NAME(lsTipSavingIcon))
						{
							if ((image = g->access_saving_icon().get_icon()))
							{
								requiredSize.x += image->get_size().x * imageScale + imageTextSeparator;
								requiredSize.y = max(requiredSize.y, fsxy * 2.0f * 2.0f + image->get_size().y * imageScale);
							}
						}
						if (show == NAME(lsTipHelpIcon))
						{
							if ((image = helpIcon.get()))
							{
								requiredSize.x += image->get_size().x * imageScale + imageTextSeparator;
								requiredSize.y = max(requiredSize.y, fsxy * 2.0f * 2.0f + image->get_size().y * imageScale);
							}
						}
						if (show == NAME(lsTipRestartAtAnyChapter))
						{
							if ((image = restartAtAnyChapterInfo.get()))
							{
								requiredSize.x += image->get_size().x * imageScale + imageTextSeparator;
								requiredSize.y = max(requiredSize.y, fsxy * 2.0f * 2.0f + image->get_size().y * imageScale);
							}
						}
					}

					requiredSize /= usePPA;
					Rotator3 atDir = get_current_forward();
					atDir.pitch -= 35.0f;
					if (useSceneScale < 0.3f)
					{
						// even lower
						atDir.pitch -= 10.0f;
					}
					auto* screen = new HubScreen(this, NAME(autoTip), requiredSize, atDir, get_radius() * 0.5f, false, NP, usePPA);
					screen->activate();
					screen->followYawDeadZoneByBorder = HubScreen::DEFAULT_YAW_DEAD_ZONE_BY_BORDER;
					add_screen(screen);

					float textX = screen->mainResolutionInPixels.x.min + fsxy * 2.0f;
					if (image)
					{
						Range2 imageRange = screen->mainResolutionInPixels;
						imageRange.x.min = textX;
						imageRange.x.max = imageRange.x.min + image->get_size().x * imageScale;
						textX += image->get_size().x * imageScale + imageTextSeparator;
						auto* w = new HubWidgets::Image(Name::invalid(), imageRange, image, Vector2(imageScale));
						screen->add_widget(w);
					}
					{
						Range2 textRange = screen->mainResolutionInPixels;
						textRange.x.min = textX;
						textRange.y.min = screen->mainResolutionInPixels.y.centre() - textRequiredSize.y * 0.5f;
						textRange.y.max = screen->mainResolutionInPixels.y.centre() + textRequiredSize.y * 0.5f;
						auto* w = new HubWidgets::Text(Name::invalid(), textRange, text);
						w->alignPt = Vector2(0.0f, 1.0f);
						screen->add_widget(w);
					}
					{
						int width;
						int lc;
						HubWidgets::Text::measure(get_font(), NP, NP, dismissText, lc, width);

						Range2 buttonRange = screen->mainResolutionInPixels;
						buttonRange.x.max = screen->mainResolutionInPixels.x.max - fsxy * 2.0f;
						buttonRange.x.min = buttonRange.x.max - ((float)width + fsxy * 2.0f);
						buttonRange.y.min = screen->mainResolutionInPixels.y.centre() - textRequiredSize.y * 0.5f;
						buttonRange.y.max = buttonRange.y.min + buttonHeight;
						auto* w = new HubWidgets::Button(Name::invalid(), buttonRange, dismissText);
						w->on_click = [screen](HubWidget* _widget, Vector2 const& _at, HubInputFlags::Flags _flags) { screen->deactivate(); blockShowingFor = 0.5f; };
						screen->add_widget(w);
					}
					autoHideIn = 8.0f;
				}
			}
			if (autoHideIn.is_set())
			{
				if (auto* scr = get_screen(NAME(autoTip)))
				{
					if (get_hand(Hand::Left).onScreen == scr ||
						get_hand(Hand::Right).onScreen == scr)
					{
						autoHideIn = max(2.0f, autoHideIn.get()); // keep it there until you click or stop looking
					}
					autoHideIn = autoHideIn.get() - _deltaTime;
					if (autoHideIn.get() <= 0.0f)
					{
						autoHideIn.clear();
							scr->deactivate();
						blockShowingFor = 0.5f;
					}
				}
			}
		}
		else
		{
			autoHideIn.clear();
		}
	}

	timeSinceLastActive.reset();

	if (s_lastForwardTimeStamp.get_time_since() > 1.0f || !s_lastForward.is_set())
	{
		s_lastForward = get_background_dir_or_main_forward();
#ifdef EXTENSIVE_LOGS
		REMOVE_AS_SOON_AS_POSSIBLE_ output(TXT("[hub] new last forward %.3f"), s_lastForward.get().yaw);
#endif
	}
	if (s_lastForward.is_set())
	{
		s_lastForwardTimeStamp.reset();
	}

	{
		scoped_call_stack_info(TXT("fading"));

		if (requiresToFadeOut)
		{
			if (game->get_fade_out_target() != 1.0f)
			{
				game->fade_out(FADE_TIME, NAME(closingHub), 0.0f, true, fadeColour);
			}
		}
		else
		{
			if (game->get_fade_out_target() != 0.0f)
			{
				game->fade_in(FADE_TIME, NAME(openingHub));
			}
		}
		game->advance_fading(_deltaTime);
	}

#ifndef AN_DEVELOPMENT_OR_PROFILER
	if (vr)
#endif
	{
		Vector2 mouseMove = hands[0].input.get_mouse(NAME(movePointer));
		mouseRotation.yaw += mouseMove.x;
		mouseRotation.pitch += mouseMove.y;
		Optional<Transform> mousePretendPose = Transform(mousePretendLoc, mouseRotation.to_quat());
		for_count(int, handIdx, (vr? 2 : 1))
		{
			scoped_call_stack_info(handIdx == Hand::Left? TXT("left hand") : TXT("right hand"));

			auto & hand = hands[handIdx];
			auto & handPose = vr? (vr->get_render_pose_set().hands[vr->get_hand((Hand::Type)handIdx)].placement) : mousePretendPose;
			bool handAvailable = vr? vr->is_hand_available((Hand::Type)handIdx) : true;

			hand.prevOverWidget = hand.overWidget;
			hand.overWidget.clear();
			hand.onScreenTarget.clear();
			if (handPose.is_set() && handAvailable)
			{
				hand.placement = handPose.get();

				/*
				{
					Transform h = handPose.get();
					if (vr)
					{
						Transform rP = vr->get_render_pose_set().hands[vr->get_hand((Hand::Type)handIdx)].rawPlacement.get(h);
						Transform rHT = vr->get_render_pose_set().hands[vr->get_hand((Hand::Type)handIdx)].rawHandTrackingRootPlacement.get(h);

						debug_draw_transform_size_coloured(true, rP, 0.02f, Colour::green, Colour::green, Colour::green);
						debug_draw_transform_size_coloured(true, rHT, 0.02f, Colour::blue, Colour::blue, Colour::blue);
						debug_draw_transform_size_coloured(true, PP, 0.015f, Colour::red, Colour::red, Colour::red);
						debug_draw_transform_size_coloured(true, rPP, 0.015f, Colour::yellow, Colour::yellow, Colour::yellow);
						debug_draw_transform_size_coloured(true, h, 0.015f, Colour::magenta, Colour::magenta, Colour::magenta);
						//debug_draw_arrow(true, Colour::red, rHT.get_translation(), rHT.to_world(vr->get_render_pose_set().get_raw_bone_rs((Hand::Type)handIdx, VR::VRHandBoneIndex::Wrist)).get_translation());
						for_count(int, b, VR::VRHandPose::MAX_RAW_BONES)
						{
							//debug_draw_transform_size(true, rHT.to_world(vr->get_reference_pose_set().get_raw_bone_rs((Hand::Type)handIdx, b)), 0.01f);
							debug_draw_transform_size_coloured(true, rHT.to_world(vr->get_render_pose_set().get_raw_bone_rs((Hand::Type)handIdx, b)), 0.005f, Colour::grey, Colour::grey, Colour::grey);
						}
						debug_draw_transform_size(true, h.to_world(vr->get_render_pose_set().get_hand((Hand::Type)handIdx).bonesPS[VR::VRHandBoneIndex::Wrist].get(Transform::identity)), 0.01f);
						debug_draw_transform_size(true, h.to_world(vr->get_render_pose_set().get_hand((Hand::Type)handIdx).bonesPS[VR::VRHandBoneIndex::ThumbBase].get(Transform::identity)), 0.01f);
						debug_draw_transform_size(true, h.to_world(vr->get_render_pose_set().get_hand((Hand::Type)handIdx).bonesPS[VR::VRHandBoneIndex::PointerBase].get(Transform::identity)), 0.01f);
						debug_draw_transform_size(true, h.to_world(vr->get_render_pose_set().get_hand((Hand::Type)handIdx).bonesPS[VR::VRHandBoneIndex::MiddleBase].get(Transform::identity)), 0.01f);
						debug_draw_transform_size(true, h.to_world(vr->get_render_pose_set().get_hand((Hand::Type)handIdx).bonesPS[VR::VRHandBoneIndex::RingBase].get(Transform::identity)), 0.01f);
						debug_draw_transform_size(true, h.to_world(vr->get_render_pose_set().get_hand((Hand::Type)handIdx).bonesPS[VR::VRHandBoneIndex::PinkyBase].get(Transform::identity)), 0.01f);
						debug_draw_transform_size(true, h.to_world(vr->get_render_pose_set().get_hand((Hand::Type)handIdx).bonesPS[VR::VRHandBoneIndex::ThumbTip].get(Transform::identity)), 0.01f);
						debug_draw_transform_size(true, h.to_world(vr->get_render_pose_set().get_hand((Hand::Type)handIdx).bonesPS[VR::VRHandBoneIndex::PointerTip].get(Transform::identity)), 0.01f);
						debug_draw_transform_size(true, h.to_world(vr->get_render_pose_set().get_hand((Hand::Type)handIdx).bonesPS[VR::VRHandBoneIndex::MiddleTip].get(Transform::identity)), 0.01f);
						debug_draw_transform_size(true, h.to_world(vr->get_render_pose_set().get_hand((Hand::Type)handIdx).bonesPS[VR::VRHandBoneIndex::RingTip].get(Transform::identity)), 0.01f);
						debug_draw_transform_size(true, h.to_world(vr->get_render_pose_set().get_hand((Hand::Type)handIdx).bonesPS[VR::VRHandBoneIndex::PinkyTip].get(Transform::identity)), 0.01f);
					}
				}
				*/

				Transform pointer = hand.placement.to_world(hand.pointerMS);
				if (vr)
				{
					bool pointWithWholeHand = false;
					if (vr->is_using_hand_tracking((Hand::Type)handIdx))
					{
						pointWithWholeHand = true;
					}
					else
					{
						if (vr->is_render_view_valid())
						{
							Transform pointHand = hand.placement.to_world(hand.pointerMS);

							Transform view = vr->get_render_view();

							// first value is switching to pointer, second for switching to hand
							// first is harder to trigger the greater the value is
							// second is harder to trigger the lesser the value is
							float threshold = s_autoPointWithWholeHands[handIdx] ? 0.65f : 0.35f;
							if (useSceneScale < 0.2f)
							{
								threshold = s_autoPointWithWholeHands[handIdx] ? 0.6f : 0.25f;
							}
							Vector3 viewToHandDir = (pointHand.get_translation() - view.get_translation()).normal();
							if (abs(Vector3::dot(pointHand.get_axis(Axis::Forward), viewToHandDir)) < threshold && // pointing perpendicular
								Vector3::dot(view.get_axis(Axis::Forward), viewToHandDir) > 0.75f &&
								abs(Vector3::dot(pointHand.get_axis(Axis::Right), viewToHandDir)) > 0.707f) // sideways
							{
								s_autoPointWithWholeHands[handIdx] = true;
							}
							else
							{
								s_autoPointWithWholeHands[handIdx] = false;
							}
						}
						pointWithWholeHand = should_use_point_with_whole_hand((Hand::Type)handIdx);
					}
					if (pointWithWholeHand)
					{
						if (vr->is_render_view_valid())
						{
							Transform pointerHand = hand.placement.to_world(hand.pointHandMS);
							Vector3 pointHand = pointerHand.get_translation();

							Transform view = vr->get_render_view();

							Transform atPointHand = look_at_matrix(view.get_translation(), pointHand, view.get_axis(Axis::Up)).to_transform();

							Rotator3 offset = Rotator3(15.0f, 10.0f * (handIdx == Hand::Left ? 1.0f : -1.0f), 0.0f);
							float distanceBase = 0.6f;
							float distance = Vector3::distance(atPointHand.get_translation(), view.get_translation());
							offset *= clamp(distance != 0.0f? distanceBase / distance : 100.0f, 0.5f, 1.5f);

							Vector3 dir = atPointHand.vector_to_world(offset.get_forward());

							pointer = look_matrix(view.get_translation(), dir, pointerHand.get_axis(Axis::Forward)).to_transform();
						}
						else
						{
							pointer = hand.lastPointerPlacement;
						}
					}
				}
				hand.lastPointerPlacement = pointer;

				Vector3 startAt = pointer.get_translation();
				Vector3 endAt = startAt + pointer.get_axis(Axis::Forward) * max(50.0f, (hubInfo.radius * 2.0f));
				Vector3 handUp = pointer.get_axis(Axis::Up);

				// within main screen
				float bestT = 2.0f;
				HubScreen* bestScreen = nullptr;
				Vector2 bestAt = hand.onScreenTargetAt;
				float bestUpAngle = hand.onScreenTargetUpAngle;

				bool currentIsStillValid = false;

				//debug_draw_transform_size(true, pointer, 0.02f);
				//debug_draw_arrow(true, Colour::magenta, pointer.get_translation(), pointer.get_translation() + handUp * 0.04f);

				bool inBackground = false;
				for_every_reverse_ref(screen, screens)
				{
					if (screen->is_active())
					{
						if (inBackground)
						{
							screen->into_background();
						}
						else
						{
							screen->into_foreground();
							if (screen->can_be_used())
							{
								if (!screen->notAvailableToCursor)
								{
									Vector3 scrStartAt = adjust_pointer_placement(startAt, screen);
									Vector3 scrEndAt = adjust_pointer_placement(endAt, screen);

									Transform screenPlacement = screen->placement;
									float screenRadius = screen->radius;
									debug_push_transform(screenPlacement);
									Segment localSegment = Segment(screenPlacement.location_to_local(scrStartAt), screenPlacement.location_to_local(scrEndAt));
									Sphere sphere(Vector3::zero, screenRadius);

									float t = sphere.calc_intersection_t(localSegment);
									if (t < 2.0f)
									{
										if (t < bestT || !bestScreen || screen == hand.onScreen.get())
										{
											Vector3 hit = localSegment.get_at_t(t);
											//debug_draw_sphere(true, true, Colour::red, 0.4f, Sphere(hit, 0.1f));
											//debug_draw_arrow(true, Colour::red, hit, hit + handUp);
											Rotator3 at = hit.to_rotator();
											Vector2 cursorAt = Vector2((at.yaw - screen->wholeScreen.x.min) * screen->pixelsPerAngle.x, (at.pitch - screen->wholeScreen.y.min) * screen->pixelsPerAngle.y);
											Vector3 handUpLocal = screenPlacement.vector_to_local(handUp);
											float cursorUpAngle = Rotator3::get_roll(handUpLocal);
											if ((t < bestT || !bestScreen) && screen->mainScreen.does_contain(Vector2(at.yaw, at.pitch)))
											{
												bestScreen = screen;
												bestT = t;
												bestAt = cursorAt;
												bestUpAngle = cursorUpAngle;
											}
											if (screen == hand.onScreen.get())
											{
												//debug_draw_sphere(true, true, Colour::yellow, 0.4f, Sphere(hit, 0.15f));
												hand.onScreenAt = cursorAt;
												hand.onScreenUpAngle = cursorUpAngle;
												if (screen->wholeScreen.does_contain(Vector2(at.yaw, at.pitch)))
												{
													//debug_draw_sphere(true, true, Colour::green, 0.4f, Sphere(hit, 0.2f));
													currentIsStillValid = true; // to stay on current if we just move a little bit beyond actual screen
												}
											}
										}
									}
									debug_pop_transform();
								}
							}
						}
						if (screen->is_modal())
						{
							inBackground = true;
						}
					}
				}

				if (bestScreen)
				{
					hand.onScreenTarget = bestScreen;
					hand.onScreenTargetAt = bestAt;
					hand.onScreenTargetUpAngle = bestUpAngle;
				}
				else if (currentIsStillValid)
				{
					hand.onScreenTarget = hand.onScreen;
					hand.onScreenTargetAt = hand.onScreenAt;
					hand.onScreenTargetUpAngle = hand.onScreenUpAngle;
				}
				else
				{
					hand.onScreenTarget.clear();
				}

				hand.onScreenActive = blend_to_using_speed_based_on_time(hand.onScreenActive, (hand.onScreenTarget.get() && hand.onScreenTarget.get() == hand.onScreen.get()) ? 1.0f : 0.0f, 0.1f, _deltaTime);
				if (hand.onScreenActive < 0.001f)
				{
					hand.onScreen = hand.onScreenTarget;
					hand.onScreenAt = hand.onScreenTargetAt;
					hand.onScreenUpAngle = hand.onScreenTargetUpAngle;
				}
			}
		}
	}

	{
		queueForcedClicks = true;
		{
			bool beyondModal = false;
			for (int idx = screens.get_size() - 1; idx >= 0; --idx)
			{
				auto * screen = screens[idx].get();
				scoped_call_stack_info(TXT("advance screen"));
				scoped_call_stack_info(screen->id.to_char());
				{
					screen->advance(_deltaTime, beyondModal);
				}
				if (screen->is_modal())
				{
					beyondModal = true;
				}
				if (screen->active == 0.0f && screen->activeTarget == 0.0f)
				{
					screen->clear();
#ifdef LOG_HUB
					output(TXT("[hub] remove deactivated and invisible screen %p \"%S\""), screen, screen->id.to_char());
#endif
					screens.remove_at(idx);
					continue;
				}
			}
		}
		queueForcedClicks = false;
		while (!queuedForcedClicks.is_empty())
		{
			if (HubWidget* widget = queuedForcedClicks.get_first().widget.get())
			{
				bool shifted = queuedForcedClicks.get_first().shifted;
				force_click(widget, shifted);
			}
			queuedForcedClicks.pop_front();
		}
	}

	int checkCount = vr ? 2 : 0;
#ifdef AN_DEVELOPMENT_OR_PROFILER
	if (!vr)
	{
		checkCount = 1;
	}
#endif
	if (inputAllowed && checkCount)
	{
		if (requiresReleaseSelect)
		{
			bool pressed = false;
			for_count(int, handIdx, checkCount)
			{
				auto& hand = hands[handIdx];
				pressed |= hand.input.is_button_pressed(NAME(select));
			}
			if (!pressed)
			{
				requiresReleaseSelect = false;
			}
		}
		for_count(int, handIdx, checkCount)
		{
			auto & hand = hands[handIdx];
			hand.timeSinceLastClick += _deltaTime;
			bool clicked = false;
			bool doubleClicked = false;
			bool pressed = false;
			bool shifted = false;
			if (!requiresReleaseSelect)
			{
				clicked = hand.input.has_button_been_pressed(NAME(select));
				pressed = hand.input.is_button_pressed(NAME(select));
				shifted = hand.input.is_button_pressed(NAME(shift));
			}
			if (clicked)
			{
				if (hand.timeSinceLastClick < 0.4f)
				{
					doubleClicked = true;
				}
				hand.timeSinceLastClick = 0.0f;
			}
			if (TeaForGodEmperor::TutorialSystem::is_hub_input_allowed())
			{
				if (currentScene.is_set())
				{
					currentScene->process_input(handIdx, hand.input, _deltaTime);
				}
				for_every_ref(screen, screens)
				{
					if (screen->can_be_used() &&
						(screen->alwaysProcessInput || hand.onScreen == screen) &&
						TeaForGodEmperor::TutorialSystem::does_allow_interaction(screen, nullptr))
					{
						screen->process_input(handIdx, hand.input, _deltaTime);
					}
				}
			}
			float prevPressingFor = hand.pressingFor;
			auto* prevOverWidgetOnPress = hand.overWidgetOnPress.get();
			if (hand.heldWidget.is_set() &&
				(!TeaForGodEmperor::TutorialSystem::does_allow_interaction(hand.onScreen.get(), hand.heldWidget.get()) ||
				 !TeaForGodEmperor::TutorialSystem::is_hub_input_allowed()))
			{
				// force release
				pressed = false;
			}
			if (pressed)
			{
				if (hand.pressingFor == 0.0f)
				{
					hand.overWidgetOnPress = hand.overWidget;
				}
				hand.pressingFor += _deltaTime;
			}
			else
			{
				hand.pressingFor = 0.0f;
				hand.overWidgetOnPress.clear();
			}
			if (pressed && hand.heldWidget.is_set())
			{
				// keep holding same widget
				if (hand.heldWidgetOnScreen.is_set() &&
					hand.heldWidgetOnScreen == hand.onScreen)
				{
					for_every_ref(widget, hand.heldWidgetOnScreen->widgets)
					{
						if (widget == hand.heldWidget.get())
						{
							if (hand.heldWidgetGripped)
							{
								hand.heldWidget->internal_on_hold_grip(handIdx, true, hand.onScreenAt);
								if (hand.heldWidget->on_hold_grip)
								{
									hand.heldWidget->on_hold_grip(hand.heldWidget.get(), hand.onScreenAt, hand.pressingFor, prevPressingFor);
								}
							}
							else
							{
								hand.heldWidget->internal_on_hold(handIdx, true, hand.onScreenAt);
								if (hand.heldWidget->on_hold)
								{
									hand.heldWidget->on_hold(hand.heldWidget.get(), hand.onScreenAt, hand.pressingFor, prevPressingFor);
								}
							}
						}
					}
				}
			}
			else
			{
				if (hand.overWidget.is_set() && 
					TeaForGodEmperor::TutorialSystem::does_allow_interaction(hand.onScreen.get(), hand.overWidget.get()) && 
					TeaForGodEmperor::TutorialSystem::is_hub_input_allowed())
				{
					hand.overWidget->process_input(handIdx, hand.input, _deltaTime);
				}
				if (!pressed)
				{
					if (hand.heldWidget.get())
					{
						auto on_release_copy = hand.heldWidget->on_release; // in case we clear this widget mid-through
						hand.heldWidget->internal_on_release(handIdx, hand.onScreenAt);
						if (on_release_copy)
						{
							on_release_copy(hand.heldWidget.get(), hand.onScreenAt);
						}
					}
					hand.heldWidget = nullptr;
					hand.heldWidgetOnScreen = nullptr;
				}
				if (clicked &&
					TeaForGodEmperor::TutorialSystem::does_allow_interaction(hand.onScreen.get(), hand.overWidget.get()) &&
					TeaForGodEmperor::TutorialSystem::is_hub_input_allowed())
				{
					TeaForGodEmperor::TutorialSystem::mark_clicked_hub(hand.onScreen.get(), hand.overWidget.get());
					play(clickSample);
					if (hand.overWidget.is_set())
					{
						if (doubleClicked && hand.overWidget->on_double_click)
						{
							hand.overWidget->on_double_click(hand.overWidget.get(), hand.onScreenAt, shifted ? HubInputFlags::Shift : 0);
						}
						else if (hand.overWidget->on_click &&
							hand.overWidget->does_allow_to_process_on_click(hand.onScreen.get(), handIdx, hand.onScreenAt, shifted ? HubInputFlags::Shift : 0))
						{
							REMOVE_AS_SOON_AS_POSSIBLE_ scoped_call_stack_info(TXT("overWidget -> on_click"));
							REMOVE_AS_SOON_AS_POSSIBLE_ scoped_call_stack_info(hand.overWidget.get()? TXT("overWidget exists") : TXT("no overWidget?"));
							REMOVE_AS_SOON_AS_POSSIBLE_ scoped_call_stack_info(hand.overWidget->id.to_char());
							auto on_click_copy = hand.overWidget->on_click; // in case we clear this widget mid-through
							on_click_copy(hand.overWidget.get(), hand.onScreenAt, shifted? HubInputFlags::Shift : 0);
						}
					}
					else if (! screens.is_empty() && screens.get_last()->does_clicking_outside_deactivate() &&
							 (hand.onScreen.get() != screens.get_last().get() ||
							  ! screens.get_last()->mainResolutionInPixels.does_contain(hand.onScreenAt)))
					{
						screens.get_last()->deactivate();
					}
					if (hand.overWidget.is_set())
					{
						hand.overWidget->internal_on_click(hand.onScreen.get(), handIdx, hand.onScreenAt, shifted ? HubInputFlags::Shift : 0);
					}
					if (hand.overWidget.is_set())
					{
						if (hand.overWidget->internal_on_hold(handIdx, false, hand.onScreenAt))
						{
							hand.heldWidget = hand.overWidget;
							hand.heldWidgetGripped = false;// shifted;
							hand.heldWidgetOnScreen = hand.onScreen;

							if (hand.heldWidgetGripped)
							{
								if (hand.overWidget->on_hold_grip)
								{
									hand.overWidget->on_hold_grip(hand.overWidget.get(), hand.onScreenAt, hand.pressingFor, prevPressingFor);
								}
							}
							else
							{
								if (hand.overWidget->on_hold)
								{
									hand.overWidget->on_hold(hand.overWidget.get(), hand.onScreenAt, hand.pressingFor, prevPressingFor);
								}
							}
						}
					}
				}
			}
			float dragThreshold = 0.1f;
			if (hand.pressingFor > dragThreshold && prevPressingFor <= dragThreshold &&
				hand.overWidget == hand.overWidgetOnPress)
			{
				scoped_call_stack_info(TXT("find to drag"));

				if (hand.overWidget.is_set() &&
					TeaForGodEmperor::TutorialSystem::does_allow_interaction(hand.onScreen.get(), hand.overWidget.get()) &&
					TeaForGodEmperor::TutorialSystem::is_hub_input_allowed())
				{
					if (hand.overWidget->get_to_drag(handIdx, hand.onScreenAt, hand.dragged, OUT_ hand.placeHolder))
					{
						scoped_call_stack_info(TXT("found draggable"));

						if (auto * d = hand.dragged.get())
						{
							d->initialUpAngle = hand.onScreenUpAngle;
							if (d->heldByHand.is_set())
							{
								// trying to drag the same
								hand.overWidget->remove_draggable(hand.placeHolder.get());
								hand.dragged.clear();
								hand.placeHolder.clear();
							}
							else
							{
								{
									d->heldByHand = (Hand::Type)handIdx;
									hand.draggedFromWidget = hand.overWidget;
									select(hand.draggedFromWidget.get(), hand.dragged.get(), true);
								}

								if (hand.draggedFromWidget.get() && hand.draggedFromWidget->on_drag_begin)
								{
									hand.draggedFromWidget->on_drag_begin(hand.dragged.get());
								}
							}
						}
					}
				}
			}
			else if (hand.pressingFor == 0.0f && prevPressingFor > 0.0f && prevPressingFor <= dragThreshold &&
				hand.overWidget == prevOverWidgetOnPress)
			{
				scoped_call_stack_info(TXT("find to select"));

				if (hand.overWidget.is_set() &&
					TeaForGodEmperor::TutorialSystem::does_allow_interaction(hand.onScreen.get(), hand.overWidget.get()) &&
					TeaForGodEmperor::TutorialSystem::is_hub_input_allowed())
				{
					hand.overWidget->select_instead_of_drag(handIdx, hand.onScreenAt);
				}
			}
			if (pressed && hand.dragged.is_set())
			{
				scoped_call_stack_info(TXT("holding draggable"));

				if (hand.hoveringDraggedOver.is_set() &&
					hand.hoveringDraggedOver.get() != hand.overWidget.get())
				{
					hand.hoveringDraggedOver->remove_draggable(hand.dragged.get());
					play(moveSample);
					hand.hoveringDraggedOver.clear();
				}
				if (hand.overWidget.is_set())
				{
					if (hand.overWidget->can_hover &&
						hand.overWidget->can_hover(hand.dragged.get(), hand.onScreenAt))
					{
						hand.hoveringDraggedOver = hand.overWidget;
						hand.hoveringDraggedOver->hover(handIdx, hand.onScreenAt, hand.dragged.get());
					}
					else if (hand.hoveringDraggedOver.is_set())
					{
						// remove from the one we're supposed to cover because apparently, we're no longer allowed to
						hand.hoveringDraggedOver->remove_draggable(hand.dragged.get());
						hand.hoveringDraggedOver.clear();
						play(moveSample);
					}
				}
			}
			if (auto * ph = hand.placeHolder.get())
			{
				scoped_call_stack_info(TXT("manage placeholder"));

				if (hand.draggedFromWidget.is_set() && hand.dragged.is_set())
				{
					if (hand.draggedFromWidget->can_hover &&
						hand.draggedFromWidget->can_hover(hand.dragged.get(), hand.onScreenAt) && // if we can't hover over this, ignore having a place holder
						hand.draggedFromWidget->should_stay_on_drag &&
						hand.draggedFromWidget->should_stay_on_drag(hand.dragged.get()))
					{
						// place holder should be visible if we're not dragging inside
						ph->forceVisible = hand.draggedFromWidget != hand.hoveringDraggedOver;
					}
					else
					{
						ph->forceVisible = false;
					}
				}
			}
			if (!pressed && hand.dragged.is_set())
			{
				scoped_call_stack_info(TXT("drop dragged"));
				drop_dragged_for(handIdx);
			}
		}
	}

	if (!currentScene.is_set() || currentScene->does_want_to_end())
	{
		wantsToExit = true;
	}

	if (wantsToExit)
	{
		if (!fadeOutAllowed)
		{
			deactivate_now_as_loader();
		}
		else
		{
			requiresToFadeOut = true;
			if (game->get_fade_out() == 1.0f)
			{
				deactivate_now_as_loader();
			}
		}
	}

	if (game->get_fade_out() == 1.0f)
	{
		// no longer requires as we're already faded out
		requiresToFadeOut = false;
	}

	if (!highlights.is_empty())
	{
		auto& hl = highlights.get_first();
		if (!hl.active)
		{
			if (hl.screen.is_valid() || hl.widget.is_valid())
			{
				special_highlight(hl.screen, hl.widget, true);
			}
			hl.active = true;
		}
		else
		{
			hl.timeLeft -= _deltaTime;
			if (hl.timeLeft <= 0.0f)
			{
				hl.active = false;
				if (hl.screen.is_valid() || hl.widget.is_valid())
				{
					special_highlight(hl.screen, hl.widget, false);
				}
				highlights.pop_front();
			}
		}
	}

}

void Hub::force_click(HubWidget* _widget, bool _shifted)
{
	if (queueForcedClicks)
	{
		queuedForcedClicks.push_back(QueuedForcedClicks(_widget, _shifted));
		return;
	}
	play(clickSample);
	if (_widget && _widget->on_click)
	{
		HubInputFlags::Flags flags = 0;
		if (_shifted)
		{
			set_flag(flags, HubInputFlags::Shift);
		}
		scoped_call_stack_info(_widget->screen ? _widget->screen->id.to_char() : TXT("<no screen>"));
		scoped_call_stack_info(_widget->id.to_char());
		_widget->on_click(_widget, _widget->at.centre(), flags);
	}
}

void Hub::force_drop_dragged()
{
	for_count(int, handIdx, Hand::MAX)
	{
		drop_dragged_for(handIdx);
	}
	deselect();
}

void Hub::drop_dragged_for(int handIdx)
{
	HubHand& hand = hands[handIdx];
	if (!hand.dragged.is_set())
	{
		return;
	}
	hand.dragged->heldByHand.clear();
	an_assert(hand.draggedFromWidget.is_set());
	if (hand.overWidget.get() &&
		hand.overWidget->can_drop_hovering(hand.dragged.get()) &&
		hand.overWidget->can_drop &&
		hand.overWidget->can_drop(hand.dragged.get(), hand.onScreenAt))
	{
		RefCountObjectPtr<HubDraggable> dropAs = hand.dragged;
		if (hand.overWidget->on_drag_drop)
		{
			hand.overWidget->on_drag_drop(dropAs.get());
		}
		if (hand.overWidget->on_drop_change)
		{
			dropAs = hand.overWidget->on_drop_change(dropAs.get());
		}
		HubDraggable* dropped = hand.overWidget->drop_dragged(handIdx, hand.onScreenAt, hand.dragged.get(), dropAs.get());
		if (hand.overWidget->on_drag_dropped)
		{
			hand.overWidget->on_drag_dropped(dropped);
		}
		if (is_selected(hand.draggedFromWidget.get(), hand.dragged.get()))
		{
			if (dropped)
			{
				select(hand.overWidget.get(), dropped);
			}
			else
			{
				deselect();
			}
		}
		if (auto* ph = hand.placeHolder.get())
		{
			if (hand.draggedFromWidget.get())
			{
				if (hand.draggedFromWidget != hand.overWidget &&
					hand.draggedFromWidget->should_stay_on_drag &&
					hand.draggedFromWidget->should_stay_on_drag(hand.dragged.get()))
				{
					ph->placeHolder = false;
					ph->forceVisible = false;
				}
				else
				{
					hand.draggedFromWidget->remove_draggable(ph);
				}
			}
		}
		if (hand.draggedFromWidget.get() != hand.overWidget.get())
		{
			hand.draggedFromWidget->remove_draggable(hand.dragged.get());
		}
	}
	else
	{
		// restore place holder (if present)
		if (auto* ph = hand.placeHolder.get())
		{
			if (is_selected(hand.draggedFromWidget.get(), hand.dragged.get()))
			{
				select(hand.draggedFromWidget.get(), ph);
			}
			ph->placeHolder = false;
			ph->forceVisible = false;
		}
		else
		{
			if (is_selected(hand.draggedFromWidget.get(), hand.dragged.get()))
			{
				deselect();
			}
		}
		if (auto* ow = hand.overWidget.get())
		{
			// remove from where we wanted to put
			ow->remove_draggable(hand.dragged.get());
			play(moveSample);
		}
		if (auto * dfw = hand.draggedFromWidget.get())
		{
			if (hand.placeHolder.is_set())
			{
				// we didn't want to put anywhere, remove from original, placeholder is there
				dfw->remove_draggable(hand.dragged.get());
			}
			else if (hand.dragged.is_set() &&
				dfw->on_drop_discard &&
				dfw->can_drop_discard &&
				dfw->can_drop_discard(hand.dragged.get())) // discard
			{
				dfw->on_drop_discard(hand.dragged.get());
			}
		}
	}
	if (auto* dfw = hand.draggedFromWidget.get())
	{
		dfw->mark_requires_rendering();
	}
	if (hand.overWidget.get() && hand.overWidget->on_drag_done)
	{
		hand.overWidget->on_drag_done();
	}
	if (hand.draggedFromWidget.get() != hand.overWidget.get())
	{
		if (hand.draggedFromWidget->on_drag_done)
		{
			hand.draggedFromWidget->on_drag_done();
		}
	}
	hand.draggedFromWidget.clear();
	hand.dragged.clear();
	hand.placeHolder.clear();
}

static void store_hand_bone(VR::IVR* vr, Meshes::Mesh3DInstance * meshInstance, Hand::Type hand, Name const& boneName, VR::VRHandBoneIndex::Type const& vrBone)
{
	if (auto* s = meshInstance->get_skeleton())
	{
		int idx = s->find_bone_index(boneName);
		if (idx != NONE)
		{
			meshInstance->set_render_bone_matrix(idx, vr->get_render_pose_set().get_bone_ps(hand, vrBone).to_matrix());
		}
	}
}

bool Hub::should_lazy_render_screen_display(HubScreen* screen, int screenIdx) const
{
	if (screen->alwaysRender)
	{
		return false;
	}

	for_count(int, handIdx, 2)
	{
		if (hands[handIdx].onScreen == screen ||
			hands[handIdx].onScreenTarget == screen)
		{
			return false;
		}
	}

	if (lazyScreenRenderCount > 1)
	{
		return mod(screenIdx, lazyScreenRenderCount) != lazyScreenRenderIdx;
	}
	else
	{
		return false;
	}
}

float Hub::adjust_screen_radius(float _radius, HubScreen* _forScreen) const
{
	if (!referencePoint.is_set())
	{
		return _radius;
	}

	return useSceneScale * _radius;
}

Transform Hub::adjust_screen_placement(Transform _placement, HubScreen* _forScreen) const
{
	if (!referencePoint.is_set())
	{
		return _placement;
	}

	_placement.set_scale(useSceneScale * _placement.get_scale());
	Vector3 loc = _placement.get_translation();
	Vector3 beAt = _forScreen->followHead.get(false)? lastMovementCentrePlacement.get_translation() : referencePoint.get();
	loc = beAt + useSceneScale * (loc - referencePoint.get());
	_placement.set_translation(loc);
	return _placement;
}

Vector3 Hub::adjust_pointer_placement(Vector3 _pointer, HubScreen* _forScreen) const
{
	if (!referencePoint.is_set())
	{
		return _pointer;
	}
	Vector3 beAt = _forScreen->followHead.get(false)? lastMovementCentrePlacement.get_translation() : referencePoint.get();
	_pointer = referencePoint.get() + (_pointer - beAt) / useSceneScale;
	return _pointer;
}

void Hub::display(System::Video3D* _v3d, bool _vr)
{
	TeaForGodEmperor::ScreenShotContext screenShotContext(game);

	screenShotContext.pre_render();

	//System::RenderTarget::bind_none();
	//_v3d->set_default_viewport();
	//_v3d->set_near_far_plane(0.02f, 100.0f);
	//_v3d->setup_for_2d_display();
	//_v3d->clear_colour(Colour::black);

	// get view placement ignoring custom render context
	Transform viewPlacement;
	Transform movementCentreInViewSpace = Transform::identity;
	if (_vr)
	{
		auto* vr = VR::IVR::get();

		Vector3 eyeLoc = Vector3(0.0f, 0.0f, 1.65f);
		viewPlacement = look_at_matrix(eyeLoc, eyeLoc + lastViewRotation.get_forward(), Vector3::zAxis).to_transform();
		if (vr->is_render_view_valid())
		{
			viewPlacement = vr->get_render_view();
		}
		lastViewRotation = viewPlacement.get_orientation().to_rotator();
		movementCentreInViewSpace = vr->get_movement_centre_in_view_space();
	}
	else
	{
		Rotator3 diff = followMouseRotation.to_quat().to_local(mouseRotation.to_quat()).to_rotator();
		diff.roll = 0.0f;
		float dist = diff.length();
		float maxAngle = 40.0f;
		float followAngle = 30.0f;
		float t = 0.0;
		if (dist > maxAngle)
		{
			t = (dist - maxAngle) / dist;
		}
		else if (dist > followAngle)
		{
			t = (10.0f * clamp(::System::Core::get_raw_delta_time(), 0.0f, 1.0f)) / dist;
		}
		if (t != 0.0f)
		{
			followMouseRotation = Quat::slerp(t, followMouseRotation.to_quat(), mouseRotation.to_quat()).to_rotator();
			followMouseRotation.roll = 0.0f;
		}
		Vector3 eyeLoc = mousePretendLoc;
		Vector3 eyeDir = followMouseRotation.get_forward();
		viewPlacement = look_at_matrix(eyeLoc, eyeLoc + eyeDir, Vector3::zAxis).to_transform();
		lastViewRotation = viewPlacement.get_orientation().to_rotator();
	}
	// store it
	lastViewPlacement = viewPlacement;

	Transform movementCentrePlacement = viewPlacement.to_world(movementCentreInViewSpace);
	lastMovementCentrePlacement = lastViewPlacement.to_world(movementCentreInViewSpace);

	if (auto* ois = TeaForGodEmperor::OverlayInfo::System::get())
	{
		ois->advance(TeaForGodEmperor::OverlayInfo::Usage::LoaderHub, viewPlacement, nullptr);
	}

	if (lazyScreenRenderCount > 1)
	{
		lazyScreenRenderIdx = mod(lazyScreenRenderIdx + 1, lazyScreenRenderCount);
	}
	else
	{
		lazyScreenRenderIdx = 0;
	}

	Optional<Transform> markerPlacement;
	if (! TeaForGodEmperor::Game::is_using_sliding_locomotion())
	{
		for_count(int, idx, startHeadMarkerMeshInstance.get_material_instance_count())
		{
			float useFull = remap_value(beaconFarActiveValue, 0.5f, 0.0f, 0.0f, 1.0f);
			startHeadMarkerCloseMeshInstance.access_material_instance(idx)->set_uniform(NAME(beaconActive), BlendCurve::cubic(useFull * beaconActiveValue * (1.0f - beaconRActiveValue)));
			startHeadMarkerCloseMeshInstance.access_material_instance(idx)->set_uniform(NAME(beaconRActive), BlendCurve::cubic(useFull * beaconActiveValue * beaconRActiveValue));
			startHeadMarkerCloseMeshInstance.access_material_instance(idx)->set_uniform(NAME(beaconZ), beaconZValue);
		}

		if (showStartMovementCentre && startMovementCentre.is_set())
		{
			markerPlacement = calculate_start_movement_centre_marker();
		}
	}

	for_count(int, idx, startHeadMarkerFarMeshInstance.get_material_instance_count())
	{
		float useFull = remap_value(beaconFarActiveValue, 0.5f, 1.0f, 0.0f, 1.0f);
		startHeadMarkerFarMeshInstance.access_material_instance(idx)->set_uniform(NAME(beaconActive), BlendCurve::cubic(useFull * beaconActiveValue * (1.0f - beaconRActiveValue)));
	}

	if (markerPlacement.is_set())
	{
		Transform farPlacement(movementCentrePlacement.get_translation() * Vector3(1.0f, 1.0f, 0.0f), Quat::identity);
		startHeadMarkerFarMeshInstance.set_placement(farPlacement);
		Vector3 beaconAtRel = farPlacement.to_local(markerPlacement.get(farPlacement)).get_translation();
		for_count(int, idx, startHeadMarkerFarMeshInstance.get_material_instance_count())
		{
			startHeadMarkerFarMeshInstance.access_material_instance(idx)->set_uniform(NAME(beaconAtRel), beaconAtRel);
		}
	}

	// render display as vr scene (if not doing a screenshot)
	if (_vr && !screenShotContext.useCustomRenderContext)
	{
		auto* vr = VR::IVR::get();

		Framework::Render::Camera camera;
		camera.set_placement(nullptr, viewPlacement);
		camera.set_near_far_plane(TeaForGodEmperor::Game::s_renderingNearZ, TeaForGodEmperor::Game::s_renderingFarZ);
		camera.set_background_near_far_plane();

		Framework::Render::Scene* scenes[2];
		scenes[0] = Framework::Render::Scene::build(camera, NP, nullptr, NP, Framework::Render::SceneMode::VR_Left);
		scenes[1] = Framework::Render::Scene::build(camera, NP, nullptr, NP, Framework::Render::SceneMode::VR_Right);

		for_count(int, handIdx, 2)
		{
			Hand::Type hand = (Hand::Type)handIdx;
			bool handAvailable = VR::IVR::get()->is_hand_available(hand);

			if (handAvailable)
			{
				for_count(int, sceneIdx, 2)
				{
					auto* meshInstance = vr->is_using_hand_tracking(hand) ? &hands[handIdx].meshInstanceHandTracking : (should_use_point_with_whole_hand(hand) ? &hands[handIdx].meshInstancePointHandController : &hands[handIdx].meshInstanceController);
					if (vr->is_using_hand_tracking(hand))
					{
						if (auto* s = meshInstance->get_skeleton())
						{
							// fill default bones
							for_count(int, i, s->get_num_bones())
							{
								meshInstance->set_render_bone_matrix(i, s->get_bones_default_matrix_MS()[i]);
							}
							store_hand_bone(vr, meshInstance, hand, NAME(pointerTip), VR::VRHandBoneIndex::PointerTip);
							store_hand_bone(vr, meshInstance, hand, NAME(pointerMid), VR::VRHandBoneIndex::PointerMid);
							store_hand_bone(vr, meshInstance, hand, NAME(pointerBase), VR::VRHandBoneIndex::PointerBase);
							store_hand_bone(vr, meshInstance, hand, NAME(middleTip), VR::VRHandBoneIndex::MiddleTip);
							store_hand_bone(vr, meshInstance, hand, NAME(middleMid), VR::VRHandBoneIndex::MiddleMid);
							store_hand_bone(vr, meshInstance, hand, NAME(middleBase), VR::VRHandBoneIndex::MiddleBase);
							store_hand_bone(vr, meshInstance, hand, NAME(ringTip), VR::VRHandBoneIndex::RingTip);
							store_hand_bone(vr, meshInstance, hand, NAME(ringMid), VR::VRHandBoneIndex::RingMid);
							store_hand_bone(vr, meshInstance, hand, NAME(ringBase), VR::VRHandBoneIndex::RingBase);
							store_hand_bone(vr, meshInstance, hand, NAME(pinkyTip), VR::VRHandBoneIndex::PinkyTip);
							store_hand_bone(vr, meshInstance, hand, NAME(pinkyMid), VR::VRHandBoneIndex::PinkyMid);
							store_hand_bone(vr, meshInstance, hand, NAME(pinkyBase), VR::VRHandBoneIndex::PinkyBase);
							store_hand_bone(vr, meshInstance, hand, NAME(thumbTip), VR::VRHandBoneIndex::ThumbTip);
							store_hand_bone(vr, meshInstance, hand, NAME(thumbMid), VR::VRHandBoneIndex::ThumbMid);
							store_hand_bone(vr, meshInstance, hand, NAME(thumbBase), VR::VRHandBoneIndex::ThumbBase);
						}
					}
					scenes[sceneIdx]->add_extra(meshInstance, hands[handIdx].placement);
				}
			}
		}

		if (sceneMeshInstance.get_mesh())
		{
			scenes[0]->add_extra(&sceneMeshInstance, Transform::identity);
			scenes[1]->add_extra(&sceneMeshInstance, Transform::identity);
		}

		if (backgroundMeshInstance.get_mesh())
		{
			scenes[0]->add_extra(&backgroundMeshInstance, Transform::identity, true);
			scenes[1]->add_extra(&backgroundMeshInstance, Transform::identity, true);
		}

		if (showStartMovementCentre)
		{
			if (markerPlacement.is_set())
			{
				scenes[0]->add_extra(&startHeadMarkerMeshInstance, markerPlacement.get());
				scenes[1]->add_extra(&startHeadMarkerMeshInstance, markerPlacement.get());
				if (!TeaForGodEmperor::Game::is_using_sliding_locomotion())
				{
					if (beaconActiveValue > 0.0f)
					{
						if (beaconFarActiveValue <= 0.5f)
						{
							scenes[0]->add_extra(&startHeadMarkerCloseMeshInstance, markerPlacement.get());
							scenes[1]->add_extra(&startHeadMarkerCloseMeshInstance, markerPlacement.get());
						}
						else
						{
							scenes[0]->add_extra(&startHeadMarkerFarMeshInstance, Transform::identity /* uses view placement */);
							scenes[1]->add_extra(&startHeadMarkerFarMeshInstance, Transform::identity /* uses view placement */);
						}
					}
				}
			}
		}

		if (!screens.is_empty())
		{
			ARRAY_STACK(HubScreen*, screensSorted, screens.get_size());
			for_every_ref(screen, screens)
			{
				screensSorted.push_back(screen);
			}
			sort(screensSorted, HubScreen::compare);
			for_every_ptr(screen, screensSorted)
			{
				if (!screen->can_be_used()) continue;

				scoped_call_stack_info(TXT("screen to display"));
				scoped_call_stack_info(screen->id.to_char());
				screen->meshInstance.set_rendering_order_priority(1000 - for_everys_index(screen));
				screen->render_display_and_ready_to_render(should_lazy_render_screen_display(screen, for_everys_index(screen)));
				Transform finalPlacement = adjust_screen_placement(screen->finalPlacement, screen);
				scenes[0]->add_extra(&screen->meshInstance, finalPlacement);
				scenes[1]->add_extra(&screen->meshInstance, finalPlacement);
			}
		}


		scenes[0]->prepare_extras();
		scenes[1]->prepare_extras();

		Framework::Render::Context rc(_v3d);

		rc.set_default_texture_id(Framework::Library::get_current()->get_default_texture()->get_texture()->get_texture_id());

		for_count(int, i, 2)
		{
			if (environment.is_set())
			{
				scenes[i]->set_on_pre_render([this](Framework::Render::Scene* _scene, Framework::Render::Context & _context)
				{
					::System::ShaderProgramBindingContext shaderProgramBindingContext;
					auto & matrixStack = ::System::Video3D::get()->get_model_view_matrix_stack();
					Matrix44 modelViewMatrix = matrixStack.get_current();
					matrixStack.ready_for_rendering(REF_ modelViewMatrix);
					environment->get_info().get_params().setup_shader_binding_context(&shaderProgramBindingContext, modelViewMatrix);
					_context.set_shader_program_binding_context(shaderProgramBindingContext);
				});
			}
			scenes[i]->set_on_post_render([this](Framework::Render::Scene* _scene, Framework::Render::Context & _context)
			{
				if (currentScene.is_set())
				{
					currentScene->on_post_render(_scene, _context);
				}
			});
			scenes[i]->render(rc, nullptr); // it's ok, it's vr
			if (auto* ois = TeaForGodEmperor::OverlayInfo::System::get())
			{
				VR::Eye::Type eye = (VR::Eye::Type)scenes[i]->get_render_camera().get_eye_idx();
				ois->render(scenes[i]->get_vr_anchor(), TeaForGodEmperor::OverlayInfo::Usage::LoaderHub, eye);
			}
#ifdef AN_DEBUG_RENDERER
			VR::Eye::Type eye = (VR::Eye::Type)scenes[i]->get_render_camera().get_eye_idx();
			if (System::RenderTarget* rt = VR::IVR::get()->get_render_render_target(eye))
			{
				// to render debug
				rt->bind();
				debug_renderer_render(_v3d);
				rt->unbind();
			}
			debug_renderer_undefine_contexts();
#endif
			if (currentScene.is_set())
			{
				currentScene->on_post_render_render_scene(scenes[i]->get_vr_eye_idx());
			}

			scenes[i]->release();
		}
		debug_renderer_already_rendered();

		VR::IVR::get()->copy_render_to_output(_v3d);

		for (int rtIdx = 0; rtIdx < 2; ++rtIdx)
		{
			if (System::RenderTarget* rt = VR::IVR::get()->get_output_render_target((VR::Eye::Type)rtIdx))
			{
				game->apply_fading_to(rt);
			}
		}
	}
	else
	{
		System::RenderTarget* renderTarget = screenShotContext.useCustomRenderContext? screenShotContext.useCustomRenderContext->finalRenderTarget.get() : game->get_main_render_buffer();

		Framework::Render::Camera camera;
		camera.set_placement(nullptr, viewPlacement);
		camera.set_near_far_plane(TeaForGodEmperor::Game::s_renderingNearZ, TeaForGodEmperor::Game::s_renderingFarZ);
		camera.set_background_near_far_plane();
		camera.set_fov(70.0f);
		camera.set_view_origin(VectorInt2::zero);
		camera.set_view_size(renderTarget->get_size());
		camera.set_view_aspect_ratio(aspect_ratio(renderTarget->get_full_size_for_aspect_ratio_coef()));

		Framework::Render::Scene* scene;
		scene = Framework::Render::Scene::build(camera);

		scene->add_extra(&sceneMeshInstance, Transform::identity);
		scene->add_extra(&backgroundMeshInstance, Transform::identity, true);

		if (showStartMovementCentre && startMovementCentre.is_set())
		{
			if (markerPlacement.is_set())
			{
				scene->add_extra(&startHeadMarkerMeshInstance, markerPlacement.get());
				if (!TeaForGodEmperor::Game::is_using_sliding_locomotion())
				{
					if (beaconActiveValue > 0.0f)
					{
						if (beaconFarActiveValue <= 0.5f)
						{
							scene->add_extra(&startHeadMarkerCloseMeshInstance, markerPlacement.get());
						}
						else
						{
							scene->add_extra(&startHeadMarkerFarMeshInstance, Transform::identity /* uses view placement */);
						}
					}
				}
			}
		}

		if (!screens.is_empty())
		{
			ARRAY_STACK(HubScreen*, screensSorted, screens.get_size());
			for_every_ref(screen, screens)
			{
				screensSorted.push_back(screen);
			}
			sort(screensSorted, HubScreen::compare);
			for_every_ptr(screen, screensSorted)
			{
				if (!screen->can_be_used()) continue;

				screen->meshInstance.set_rendering_order_priority(1000 - for_everys_index(screen));
				screen->render_display_and_ready_to_render(should_lazy_render_screen_display(screen, for_everys_index(screen)));
				scene->add_extra(&screen->meshInstance, screen->finalPlacement); // don't adjust for non vr
			}
		}

		scene->prepare_extras();

		Framework::Render::Context rc(_v3d);

		rc.set_default_texture_id(Framework::Library::get_current()->get_default_texture()->get_texture()->get_texture_id());

		{
			if (environment.is_set())
			{
				scene->set_on_pre_render([this](Framework::Render::Scene* _scene, Framework::Render::Context & _context)
				{
					::System::ShaderProgramBindingContext shaderProgramBindingContext;
					auto & matrixStack = ::System::Video3D::get()->get_model_view_matrix_stack();
					Matrix44 modelViewMatrix = matrixStack.get_current();
					matrixStack.ready_for_rendering(REF_ modelViewMatrix);
					environment->get_info().get_params().setup_shader_binding_context(&shaderProgramBindingContext, modelViewMatrix);
					_context.set_shader_program_binding_context(shaderProgramBindingContext);
				});
			}
			scene->set_on_post_render([this](Framework::Render::Scene* _scene, Framework::Render::Context & _context)
			{
				if (currentScene.is_set())
				{
					currentScene->on_post_render(_scene, _context);
				}
			});
			scene->render(rc, renderTarget);
			if (auto* ois = TeaForGodEmperor::OverlayInfo::System::get())
			{
				ois->render(scene->get_vr_anchor(), TeaForGodEmperor::OverlayInfo::Usage::LoaderHub, NP, renderTarget);
			}
#ifdef AN_DEBUG_RENDERER
			renderTarget->bind();
			debug_renderer_render(_v3d);
			renderTarget->unbind();
			debug_renderer_undefine_contexts();
#endif
			if (currentScene.is_set())
			{
				currentScene->on_post_render_render_scene(0);
			}
			scene->release();
		}
		debug_renderer_already_rendered();

		renderTarget->resolve();

		game->setup_to_render_to_main_render_target(::System::Video3D::get());
		::System::Video3D::get()->clear_colour(Colour::black);
		Vector2 leftBottom;
		Vector2 size;
		VectorInt2 targetSize = MainConfig::global().get_video().resolution;
		::System::RenderTargetUtils::fit_into(renderTarget->get_full_size_for_aspect_ratio_coef(), targetSize, OUT_ leftBottom, OUT_ size);
		renderTarget->render(0, ::System::Video3D::get(), leftBottom, size);
	}

	screenShotContext.post_render();

	game->apply_fading_to(nullptr);
}

HubScreen* Hub::get_screen(Name const & _id, bool _onlyActive) const
{
	for_every_ref(screen, screens)
	{
		if (screen->id == _id)
		{
			if (!_onlyActive || screen->activeTarget > 0.5f)
			{
				return screen;
			}
		}
	}
	return nullptr;
}

void Hub::add_screen(HubScreen* _screen)
{
#ifdef LOG_HUB
	output(TXT("[hub] add screen %p \"%S\""), _screen, _screen->id.to_char());
#endif
	screens.push_back(RefCountObjectPtr<HubScreen>(_screen));
}

void Hub::deactivate_all_screens()
{
	for_every_ref(screen, screens)
	{
		screen->deactivate();
	}
}

void Hub::keep_only_screen(Name const & _id, bool _onlyActive)
{
	for_every_ref(screen, screens)
	{
		if (screen->id == _id && (! _onlyActive || screen->activeTarget > 0.5f))
		{
			screen->activate();
		}
		else
		{
			screen->deactivate();
		}
	}
}

void Hub::keep_only_screens(Array<Name> const & _ids, bool _onlyActive)
{
	for_every_ref(screen, screens)
	{
		if (_ids.does_contain(screen->id) && (!_onlyActive || screen->activeTarget > 0.5f))
		{
			screen->activate();
		}
		else
		{
			screen->deactivate();
		}
	}
}

void Hub::rotate_screens(Rotator3 const & _by)
{
	for_every_ref(screen, screens)
	{
		screen->set_placement(screen->at + _by);
	}
}

void Hub::rotate_everything_by(float _yaw)
{
	for_every_ref(screen, screens)
	{
		screen->set_placement(screen->at + Rotator3(0.0f, _yaw, 0.0f));
		screen->advance(0.0f, true); // force to update, pretend beyond modal to avoid following etc
	}
	if (forward.is_set())
	{
		forward = forward.get() + Rotator3(0.0f, _yaw, 0.0f);
	}
	if (viewForward.is_set())
	{
		viewForward = viewForward.get() + Rotator3(0.0f, _yaw, 0.0f);
	}
	if (initialForward.is_set())
	{
		initialForward = initialForward.get() + Rotator3(0.0f, _yaw, 0.0f);
	}
}

void Hub::update_graphics()
{
	if (auto* library = Framework::Library::get_current())
	{
		if (!graphics[HubGraphics::LogoBigOneLine].is_set())
		{
			graphics[HubGraphics::LogoBigOneLine] = library->get_global_references().get<Framework::TexturePart>(NAME(loaderHubLogoBigOneLine), true);
		}
	}
}

Optional<Transform> Hub::calculate_start_movement_centre_marker() const
{
	struct Placement
	{
		static Transform make_sane(Transform t)
		{
			t.set_translation(t.get_translation() * Vector3::xy);
			t.set_orientation((t.get_orientation().to_rotator() * Rotator3(0.0f, 1.0f, 0.0f)).to_quat());
			return t;
		}
	};

	if (HubScenes::BeAtRightPlace::is_currently_active())
	{
		bool waitForBeAt;
		bool ignoreRotation;
		auto beAt = HubScenes::BeAtRightPlace::get_be_at(&waitForBeAt, nullptr, &ignoreRotation);
		if (beAt.is_set())
		{
			Transform sane = Placement::make_sane(beAt.get());
			if (ignoreRotation)
			{
				// rotate so it matches our orientation
				Vector3 toMarker = sane.get_translation() - lastViewPlacement.get_translation();
				toMarker *= Vector3::xy;
				float farYaw = Rotator3::get_yaw(toMarker);
				float dist = toMarker.length();
				float closeYaw = farYaw;
				{
					Vector3 fwd = lastViewPlacement.vector_to_world(Vector3(0.0f, 1.0f, 0.0f)).normal();
					Vector3 up = lastViewPlacement.vector_to_world(Vector3(0.0f, 0.5f, 1.0f)).normal();
					Vector3 down = -lastViewPlacement.vector_to_world(Vector3(0.0f, 0.5f, -1.0f)).normal();
					float fwdS = clamp(1.0f - abs(fwd.z / 0.3f), 0.0f, 1.0f);
					float upS = clamp(-fwd.z / 0.5f, 0.0f, 1.0f);
					float downS = clamp(fwd.z / 0.5f, 0.0f, 1.0f);

					float sum = fwdS + upS + downS;
					if (sum > 0.0f)
					{
						float invSum = 1.0f / sum;
						fwdS *= invSum;
						upS *= invSum;
						downS *= invSum;
					}

					Vector3 weighted = fwd * fwdS + up * upS + down * downS;
					closeYaw = Rotator3::get_yaw(weighted);
				}
				float useFar = clamp((dist - 0.3f) / 0.4f, 0.0f, 1.0f);
				closeYaw = farYaw + Rotator3::normalise_axis(closeYaw - farYaw);
				float reqYaw = farYaw * useFar + (1.0f - useFar) * closeYaw;

				sane.set_orientation(Rotator3(0.0f, reqYaw, 0.0f).to_quat());
			}
			return sane;
		}
		if (waitForBeAt)
		{
			return beAt;
		}
	}
	if (startMovementCentre.is_set())
	{
		return Placement::make_sane(startMovementCentre.get());
	}
	else
	{
		return NP;
	}
}

void Hub::quit_game(bool _unload)
{
	set_scene(nullptr);
	remove_all_screens();
	if (auto* vr = VR::IVR::get())
	{
		vr->get_ready_to_exit();
	}
	if (::System::Core::restartRequired)
	{
		_unload = true; // we will be restarting the game
	}
#ifdef AN_DEVELOPMENT_OR_PROFILER
	_unload = true; // always unload during development
#endif
	if (_unload)
	{
		if (game)
		{
			game->set_wants_to_exit();
			deactivate();
			return;
		}
	}
	::System::Core::quick_exit();
}

void Hub::play_move_sample()
{
	play(moveSample);
}

Sound::Playback Hub::play(Framework::UsedLibraryStored<Framework::Sample> const & _sample) const
{
	if (_sample.is_set())
	{
		if (auto* sample = _sample->get_sample())
		{
			return sample->play();
		}
	}
	return Sound::Playback();
}

void Hub::select(HubWidget * _widget, HubDraggable const * _draggable, bool _clicked)
{
	// rerender to dehighlight
	if (auto* w = selectedWidget.get())
	{
		w->mark_requires_rendering();
	}

	selectedWidget = _widget;
	selectedDraggable = _draggable;
	if (_widget && _widget->on_select)
	{
		_widget->on_select(_draggable, _clicked);
	}

	// rerender to highlight
	if (auto* w = selectedWidget.get())
	{
		w->mark_requires_rendering();
	}
}

void Hub::deselect()
{
	select(nullptr, nullptr);
}

bool Hub::is_selected(HubWidget * _widget, HubDraggable const * _draggable) const
{
	return selectedWidget == _widget && (!_draggable || selectedDraggable == _draggable);
}

bool Hub::is_hand_engaged(Hand::Type _hand) const
{
	auto& hand = hands[_hand];
	if (hand.heldWidget.is_set())
	{
		return true;
	}
	else if (auto* w = hand.overWidget.get())
	{
		if (w->on_click || w->on_double_click || w->on_hold || w->on_hold_grip)
		{
			return true;
		}
	}
	else if (auto* w = hand.prevOverWidget.get())
	{
		if (w->on_click || w->on_double_click || w->on_hold || w->on_hold_grip)
		{
			return true;
		}
	}
	return false;
}

void Hub::use_font(Framework::Font* _font)
{
	font = _font;
}

Framework::Font const* Hub::get_font() const
{
	return font.get();
}

Rotator3 Hub::get_background_dir_or_main_forward() const
{
	if (auto* s = get_screen(NAME(hubBackground), true))
	{
		Rotator3 result = s->at;
		if (abs(result.pitch) < 5.0f)
		{
			result.pitch = 0.0f;
			return result;
		}
	}
	return get_main_forward();
}

Rotator3 Hub::get_background_dir_or_current_forward() const
{
	if (auto* s = get_screen(NAME(hubBackground), true))
	{
		Rotator3 result = s->at;
		if (abs(result.pitch) < 5.0f)
		{
			result.pitch = 0.0f;
			return result;
		}
	}
	return get_current_forward();
}

Rotator3 Hub::get_main_forward() const
{
	if (s_lastForwardTimeStamp.get_time_since() < 1.0f && s_lastForward.is_set())
	{
		return s_lastForward.get();
	}
	return get_current_forward();
}

Rotator3 Hub::get_snapped_current_forward(Optional<float> const & _threshold) const
{
	float threshold = _threshold.get(30.0f);
	Rotator3 mainForward = get_main_forward();
	Rotator3 currForward = get_current_forward();
	if (abs(Rotator3::normalise_axis(mainForward.yaw - currForward.yaw)) < threshold)
	{
		return mainForward;
	}
	else
	{
		return currForward;
	}
}

void Hub::force_deactivate()
{
	// force deactivation
	allow_to_deactivate_with_loader_immediately(true);
	deactivate();
}

bool Hub::should_use_point_with_whole_hand(Hand::Type _hand)
{
	if (auto* gc = TeaForGodEmperor::GameConfig::get_as<TeaForGodEmperor::GameConfig>())
	{
		return (gc->get_hub_point_with_whole_hand() == Option::True) ||
			   (gc->get_hub_point_with_whole_hand() == Option::Auto && s_autoPointWithWholeHands[_hand]);
	}
	return false;
}

void Hub::clear_special_highlights()
{
	for_every_ref(screen, screens)
	{
		screen->clear_special_highlights();
	}
}

void Hub::special_highlight(Name const& _screenId, Name const& _widgetId, bool _highlight)
{
	for_every_ref(screen, screens)
	{
		if (!_screenId.is_valid() ||
			screen->id == _screenId)
		{
			screen->special_highlight(_widgetId, _highlight);
		}
	}
}

bool Hub::is_trigger_held_for_button_hold() const
{
	if (auto* vr = VR::IVR::get())
	{
		for_count(int, hIdx, Hand::MAX)
		{
			Hand::Type hand = (Hand::Type)hIdx;
			if (!is_hand_engaged(hand) && VR::IVR::get() && !VR::IVR::get()->is_using_hand_tracking(hand))
			{
				bool overBlockTriggerHoldScreen = false;
				{
					auto& h = get_hand(hand);
					if (auto* s = h.onScreen.get())
					{
						if (s->blockTriggerHold)
						{
							overBlockTriggerHoldScreen = true;
						}
					}
				}
				if (!overBlockTriggerHoldScreen)
				{
					if (vr->get_controls().is_button_pressed(VR::Input::Button::WithSource(VR::Input::Button::Trigger, VR::Input::Devices::all, hand)))
					{
						return true;
					}
				}
			}
		}
	}
	return false;
}

void Hub::reset_highlights()
{
	while (!highlights.is_empty())
	{
		auto& hl = highlights.get_first();
		if (hl.active)
		{
			hl.active = false;
			if (hl.screen.is_valid() || hl.widget.is_valid())
			{
				special_highlight(hl.screen, hl.widget, false);
			}
		}
		highlights.pop_front();
	}
	highlights.clear();
}

void Hub::add_highlight_long(Name const& _screen, Name const& _widget)
{
	highlights.push_back(Highlight(_screen, _widget, 10.0f));
}

void Hub::add_highlight_infinite(Name const& _screen, Name const& _widget)
{
	highlights.push_back(Highlight(_screen, _widget, 10000000.0f)); // no one is going to wait this long, it's infinite for this
}

void Hub::add_highlight_blink(Name const& _screen, Name const& _widget)
{
	float blinkOn = 0.4f;
	float blinkOff = 0.1f;
	highlights.push_back(Highlight(_screen, _widget, blinkOn));
	highlights.push_back(Highlight(blinkOff));
	highlights.push_back(Highlight(_screen, _widget, blinkOn));
	highlights.push_back(Highlight(blinkOff));
}

void Hub::add_highlight_pause()
{
	float blinkPause = 0.2f;
	highlights.push_back(Highlight(blinkPause));
}

