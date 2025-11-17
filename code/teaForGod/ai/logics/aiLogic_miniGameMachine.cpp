#include "aiLogic_miniGameMachine.h"

#ifdef AN_MINIGAME_PLATFORMER
#include "..\perceptionRequests\aipr_FindClosest.h"

#include "..\..\utils.h"

#include "..\..\game\game.h"

#include "..\..\..\core\system\video\renderTarget.h"
#include "..\..\..\core\vr\iVR.h"

#include "..\..\..\framework\ai\aiMindInstance.h"
#include "..\..\..\framework\display\display.h"
#include "..\..\..\framework\display\displayDrawCommands.h"
#include "..\..\..\framework\display\displayText.h"
#include "..\..\..\framework\display\displayUtils.h"
#include "..\..\..\framework\modulesOwner\modulesOwnerImpl.inl"
#include "..\..\..\framework\library\library.h"
#include "..\..\..\framework\library\usedLibraryStored.inl"
#include "..\..\..\framework\module\modulePresence.h"
#include "..\..\..\framework\module\custom\mc_display.h"
#include "..\..\..\framework\game\gameInput.h"
#include "..\..\..\framework\video\font.h"

//#include "..\..\..\framework\ai\aiMessageHandler.h"
//#include "..\..\..\framework\modulesOwner\modulesOwnerImpl.inl"
//#include "..\..\..\framework\module\moduleAppearance.h"
//#include "..\..\..\framework\object\actor.h"
//#include "..\..\..\framework\world\environment.h"
//#include "..\..\..\framework\world\room.h"

using namespace TeaForGodEmperor;
using namespace AI;
using namespace Logics;

//

REGISTER_FOR_FAST_CAST(MiniGameMachine);

MiniGameMachine::MiniGameMachine(::Framework::AI::MindInstance* _mind, ::Framework::AI::LogicData const * _logicData)
: base(_mind, _logicData)
, miniGameMachineData(fast_cast<MiniGameMachineData>(_logicData))
{
}

MiniGameMachine::~MiniGameMachine()
{
	if (display)
	{
		display->use_background(nullptr);
		display->set_on_update_display(this, nullptr);
	}
	delete_and_clear(game);
}

void MiniGameMachine::advance(float _deltaTime)
{
	base::advance(_deltaTime);

	advanceDisplayDeltaTime = _deltaTime;

	bool inputProvided = false;
	if (!inputProvided)
	{
		useInput = Framework::GameInputProxy(); // clear
	}
	if (game)
	{
		game->provide_input(&useInput);
	}
}

void MiniGameMachine::learn_from(SimpleVariableStorage & _parameters)
{
	base::learn_from(_parameters);

	display = nullptr;
	if (auto* mind = get_mind())
	{
		if (auto* imo = mind->get_owner_as_modules_owner())
		{
			if (auto* displayModule = imo->get_custom<::Framework::CustomModules::Display>())
			{
				display = displayModule->get_display();

				if (display && miniGameMachineData->miniGameInfo.get())
				{
					display->set_on_update_display(this, 
						[this](Framework::Display* _display)
						{
							if (!game)
							{
								game = new Platformer::Game(miniGameMachineData->miniGameInfo.get(), _display->get_setup().wholeDisplayResolution);
								game->start_game();
							}
							if (!mainRT.get())
							{
								::System::RenderTargetSetup rtSetup = ::System::RenderTargetSetup(_display->get_setup().wholeDisplayResolution);
								DEFINE_STATIC_NAME(colour);
								// we use ordinary rgb 8bit textures
								rtSetup.add_output_texture(::System::OutputTextureDefinition(NAME(colour),
									::System::VideoFormat::rgba8,
									::System::BaseVideoFormat::rgba));

								mainRT = new ::System::RenderTarget(rtSetup);
								// output from game is presented as background
								_display->use_background(mainRT.get(), 0);
								_display->set_background_fill_mode(Framework::DisplayBackgroundFillMode::Whole);
								_display->drop_all_draw_commands();
								_display->add((new Framework::DisplayDrawCommands::CLS(Colour::none)));
								_display->add((new Framework::DisplayDrawCommands::Border(Colour::none)));
							}
							if (game && advanceDisplayDeltaTime > 0.0f)
							{
								game->advance(advanceDisplayDeltaTime, mainRT.get());
								advanceDisplayDeltaTime = 0.0f;
							}
						});
				}
			}
		}
	}
}

//

REGISTER_FOR_FAST_CAST(MiniGameMachineData);

MiniGameMachineData::MiniGameMachineData()
{
}

MiniGameMachineData::~MiniGameMachineData()
{
}

bool MiniGameMachineData::load_from_xml(IO::XML::Node const * _node, ::Framework::LibraryLoadingContext & _lc)
{
	bool result = base::load_from_xml(_node, _lc);

	result &= miniGameInfo.load_from_xml(_node, TXT("miniGameInfo"), _lc);

	return result;
}

bool MiniGameMachineData::prepare_for_game(::Framework::Library* _library, ::Framework::LibraryPrepareContext& _pfgContext)
{
	bool result = base::prepare_for_game(_library, _pfgContext);

	result &= miniGameInfo.prepare_for_game_find(_library, _pfgContext, Framework::LibraryPrepareLevel::Resolve);

	return result;
}
#endif
