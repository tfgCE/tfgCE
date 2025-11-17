#include "framework.h"

#include "frameworkRegisteredTypes.h"

#include "ai\aiLogics.h"
#include "ai\aiSocialRules.h"
#include "appearance\animationGraphNodes.h"
#include "appearance\appearanceControllers.h"
#include "appearance\graphNodes\agn_stateMachine.h"
#include "debug\previewGame.h"
#include "debug\testConfig.h"
#include "display\displayButton.h"
#include "game\bullshotSystem.h"
#include "game\gameConfig.h"
#include "game\gameInputDefinition.h"
#include "game\tweakingContext.h"
#include "gameScript\registeredGameScriptConditions.h"
#include "gameScript\registeredGameScriptElements.h"
#include "library\customLibraryDatas.h"
#include "module\modules.h"
#include "module\custom\mc_lcdLetters.h"
#include "meshGen\meshGenConfig.h"
#include "meshGen\customData\meshGenCustomData.h"
#include "meshGen\meshGenElementBone.h"
#include "meshGen\meshGenElementMeshProcessor.h"
#include "meshGen\meshGenElementModifier.h"
#include "meshGen\meshGenElementShape.h"
#include "pipelines\pipelines.h"
#include "render\renderContext.h"
#include "render\renderSystem.h"
#include "text\localisedCharacters.h"
#include "text\localisedString.h"
#include "text\stringFormatterSerialisationHandler.h"
#include "text\stringFormatterSerialisationHandlers.h"
#include "video\basicDrawRenderingBuffer.h"
#include "video\sprite.h"
#include "video\texturePart.h"
#include "wheresMyPoint\wmp_tools.h"
#include "world\doorType.h"
#include "world\room.h"
#include "world\roomGeneratorInfo.h"
#include "world\regionGeneratorInfo.h"

#include "debug\previewGame.h"

#include "display\drawCommands\displayDrawCommand_textAt.h"
#include "display\drawCommands\displayDrawCommand_texturePart.h"

//

#ifdef AN_NO_RARE_ADVANCE_AVAILABLE
bool g_noRareAdvance = false;

bool& Framework::access_no_rare_advance()
{
	return g_noRareAdvance;
}

#endif

//

void Framework::initialise_static()
{
	NAME_POOLED_OBJECT(Framework::Nav::Node);
	NAME_POOLED_OBJECT(Framework::DisplayDrawCommands::TextAt);
	NAME_POOLED_OBJECT(Framework::DisplayDrawCommands::TexturePart);

	Framework::LibraryName::initialise_static();
	Framework::RegisteredTypes::initialise_static();

#ifdef AN_TWEAKS
	Framework::TweakingContext::initialise_static();
#endif

#ifdef AN_ALLOW_BULLSHOTS
	Framework::BullshotSystem::initialise_static();
#endif

	Framework::Render::Context::initialise_static();
	Framework::Render::System::initialise_static();
	Framework::TexturePart::initialise_static();
	Framework::Sprite::initialise_static();
	Framework::BasicDrawRenderingBuffer::initialise_static();

	Framework::WheresMyPointTools::initialise_static();
	
	Framework::Room::initialise_static();
	Framework::DoorType::initialise_static();
	Framework::Nav::DefinedFlags::initialise_static();
	Framework::TestConfig::initialise_static();
	Framework::CustomLibraryStoredData::initialise_static();
	Framework::CustomLibraryDatas::initialise_static();
	Framework::StringFormatterSerialisationHandlers::initialise_static();
	Framework::Pipelines::initialise_static();
	Framework::Modules::initialise_static();
	Framework::AnimationGraphNodes::initialise_static();
	Framework::AnimationGraphNodesLib::StateMachineDecisions::initialise_static();
	Framework::AppearanceControllers::initialise_static();
	Framework::GameStaticInfo::initialise_static();
	Framework::GameConfig::initialise_static();
	Framework::GameInputDefinitions::initialise_static();
	Framework::AI::Logics::initialise_static();
	Framework::AI::SocialRules::initialise_static();
	Framework::LocalisedCharacters::initialise_static();
	Framework::LocalisedStrings::initialise_static();
	Framework::MeshGeneration::RegisteredElementBones::initialise_static();
	Framework::MeshGeneration::RegisteredElementMeshProcessorOperators::initialise_static();
	Framework::MeshGeneration::RegisteredElementModifiers::initialise_static();
	Framework::MeshGeneration::RegisteredElementShapes::initialise_static();
	Framework::MeshGeneration::CustomData::initialise_static();
	Framework::MeshGeneration::Config::initialise_static();
	Framework::DisplayButton::initialise_static();
	Framework::RoomGeneratorInfo::initialise_static();
	Framework::RegionGeneratorInfo::initialise_static();
	Framework::LCDLetters::initialise_static();
	Framework::GameScript::RegisteredScriptConditions::initialise_static();
	Framework::GameScript::RegisteredScriptElements::initialise_static();

	// register StringFormatterSerialisationHandler
	Framework::StringFormatterSerialisationHandlers::add_serialisation_handler(new Framework::StringFormatterSerialisationHandler());
}

void Framework::close_static()
{
	Framework::GameScript::RegisteredScriptElements::close_static();
	Framework::GameScript::RegisteredScriptConditions::close_static();
	Framework::LCDLetters::close_static();
	Framework::RegionGeneratorInfo::close_static();
	Framework::RoomGeneratorInfo::close_static();
	Framework::DisplayButton::close_static();
	Framework::MeshGeneration::Config::close_static();
	Framework::MeshGeneration::CustomData::close_static();
	Framework::MeshGeneration::RegisteredElementShapes::close_static();
	Framework::MeshGeneration::RegisteredElementModifiers::close_static();
	Framework::MeshGeneration::RegisteredElementMeshProcessorOperators::close_static();
	Framework::MeshGeneration::RegisteredElementBones::close_static();
	Framework::LocalisedStrings::close_static();
	Framework::LocalisedCharacters::close_static();
	Framework::AI::SocialRules::close_static();
	Framework::AI::Logics::close_static();
	Framework::GameInputDefinitions::close_static();
	Framework::GameConfig::close_static();
	Framework::AppearanceControllers::close_static();
	Framework::AnimationGraphNodesLib::StateMachineDecisions::close_static();
	Framework::AnimationGraphNodes::close_static();
	Framework::Modules::close_static();
	Framework::Pipelines::close_static();
	Framework::CustomLibraryStoredData::close_static();
	Framework::StringFormatterSerialisationHandlers::close_static();
	Framework::BasicDrawRenderingBuffer::close_static();
	Framework::Sprite::close_static();
	Framework::TexturePart::close_static();
	Framework::Render::System::close_static();
	Framework::Render::Context::close_static();
	Framework::GameStaticInfo::close_static();
	Framework::TestConfig::close_static();
	Framework::Nav::DefinedFlags::close_static();
	Framework::DoorType::close_static();
	Framework::Room::close_static();

#ifdef AN_ALLOW_BULLSHOTS
	Framework::BullshotSystem::close_static();
#endif

#ifdef AN_TWEAKS
	Framework::TweakingContext::close_static();
#endif

	Framework::RegisteredTypes::close_static();
	Framework::LibraryName::close_static();
}

bool Framework::is_preview_game()
{
	return fast_cast<PreviewGame>(Game::get());
}

bool previewGameRequiresInfoAboutMissingLibraryStored = false;

bool Framework::does_preview_game_require_info_about_missing_library_stored()
{
	return previewGameRequiresInfoAboutMissingLibraryStored;
}

void set_preview_game_require_info_about_missing_library_stored(bool _require)
{
	previewGameRequiresInfoAboutMissingLibraryStored = _require;
}
