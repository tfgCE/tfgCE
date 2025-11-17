#include "system\core.h"
#include "system\faultHandler.h"

#include "coreRegisteredTypes.h"
#include "mainConfig.h"
#include "mainSettings.h"

#include "collision\collisionFlags.h"
#include "debug\debugRenderer.h"
#include "debug\extendedDebug.h"
#include "gameEnvironment\iGameEnvironment.h"
#include "latent\latentBlock.h"
#include "latent\latentStackVariables.h"
#include "mesh\animationSet.h"
#include "mesh\mesh3d.h"
#include "mesh\poseManager.h"
#include "performance\performanceSystem.h"
#include "physicalSensations\allPhysicalSensations.h"
#include "sound\sample.h"
#include "splash\splashInfo.h"
#include "splash\splashScreen.h"
#include "system\systemInfo.h"
#include "system\video\primitivesPipeline.h"
#include "system\video\video3dPrimitives.h"
#include "types\name.h"
#include "types\names.h"
#include "types\unicode.h"
#include "wheresMyPoint\iWMPTool.h"
#include "vr\vrInput.h"

#include <time.h>

//

void core_initialise_restartable_static();
void core_close_restartable_static(bool _restart = false);

//

void core_initialise_static()
{
	// randomise random
	srand((int)time(nullptr));

	NAME_POOLED_OBJECT(Latent::Block);
	NAME_POOLED_OBJECT(Latent::StackDataBuffer);
	NAME_POOLED_OBJECT(SimpleVariableStorageBuffer);
	NAME_POOLED_OBJECT(System::ViewFrustum);

	Unicode::initialise_static();
	String::initialise_static();
	Name::initialise_static();
	Names::initialise_static();
	IO::initialise_static();

	core_initialise_restartable_static();
}

void core_initialise_restartable_static()
{
	MainSettings::initialise_static();
	System::FaultHandler::initialise_static();
	System::Core::initialise_static();
	Splash::Info::initialise_static();
	RegisteredType::initialise_static();
	CoreRegisteredTypes::initialise_static();
	VR::Input::Devices::initialise_static();
	MainConfig::initialise_static();
	ExtendedDebug::initialise_static();
	Performance::System::initialise_static();
#ifdef AN_DEBUG_RENDERER
	DebugRenderer::initialise_static();
#endif
	Meshes::Mesh3DPart::initialise_static();
	Meshes::Mesh3D::initialise_static();
	Meshes::AnimationSet::initialise_static();
	Meshes::PoseManager::initialise_static();
	System::SystemInfo::initialise_static();
	Sound::Sample::initialise_static();
	Collision::DefinedFlags::initialise_static();
	WheresMyPoint::RegisteredTools::initialise_static();
	System::Pipelines::initialise_static();
	System::Video3DPrimitives::initialise_static();
	RegisteredColours::initialise_static();
	System::ShaderSourceCustomisation::initialise_static();
#ifdef WITH_PHYSICAL_SENSATIONS
	PhysicalSensations::initialise_static();
#endif
}

void core_restart_static()
{
	core_close_restartable_static(true);

	MainSettings::reset_static();
	MainConfig::reset_static();

	core_initialise_restartable_static();
}

void core_close_restartable_static(bool _restart)
{
	Splash::Screen::close(); // just in any case

#ifdef WITH_PHYSICAL_SENSATIONS
	PhysicalSensations::close_static();
#endif
	System::ShaderSourceCustomisation::close_static();
	RegisteredColours::close_static();
	System::Video3DPrimitives::close_static();
	System::Pipelines::close_static();
	WheresMyPoint::RegisteredTools::close_static();
	Collision::DefinedFlags::close_static();
	Sound::Sample::close_static();
	System::SystemInfo::close_static();
	Meshes::PoseManager::close_static();
	Meshes::AnimationSet::close_static();
	Meshes::Mesh3D::close_static();
	Meshes::Mesh3DPart::close_static();
	ExtendedDebug::close_static();
	Performance::System::close_static();
#ifdef AN_DEBUG_RENDERER
	DebugRenderer::close_static();
#endif
	Splash::Info::close_static();
	MainConfig::close_static();
	VR::Input::Devices::close_static();
	CoreRegisteredTypes::close_static();
	RegisteredType::close_static();
	System::Core::close_static(_restart);
	System::FaultHandler::close_static();
	MainSettings::close_static();
}

void core_close_static()
{
	core_close_restartable_static();

	GameEnvironment::IGameEnvironment::close_static();

	IO::close_static();
	Name::close_static();
	String::close_static();
	Unicode::close_static();
}
