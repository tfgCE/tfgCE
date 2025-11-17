#include "aiLogic_pilgrimTank.h"

#include "utils\aiLogicUtil_deviceDisplayStart.h"

#include "..\..\..\framework\ai\aiMessageHandler.h"
#include "..\..\..\framework\ai\aiMindInstance.h"
#include "..\..\..\framework\display\display.h"
#include "..\..\..\framework\display\displayDrawCommands.h"
#include "..\..\..\framework\module\custom\mc_display.h"
#include "..\..\..\framework\modulesOwner\modulesOwnerImpl.inl"

#include "..\..\..\core\system\video\video3dPrimitives.h"

//

using namespace TeaForGodEmperor;
using namespace AI;
using namespace Logics;

//

// ai messages
DEFINE_STATIC_NAME_STR(aimIdle, TXT("pilgrim tank; idle"));
DEFINE_STATIC_NAME_STR(aimRun, TXT("pilgrim tank; run"));
DEFINE_STATIC_NAME_STR(aimRunSync, TXT("pilgrim tank; run sync"));
DEFINE_STATIC_NAME_STR(aimRandomise, TXT("pilgrim tank; randomise"));
DEFINE_STATIC_NAME_STR(aimChangeBlinking, TXT("pilgrim tank; change blinking"));
DEFINE_STATIC_NAME_STR(aimPauseBlinking, TXT("pilgrim tank; pause blinking"));

//

REGISTER_FOR_FAST_CAST(PilgrimTank);

PilgrimTank::PilgrimTank(::Framework::AI::MindInstance* _mind, ::Framework::AI::LogicData const * _logicData)
: base(_mind, _logicData, execute_logic)
{
	pilgrimTankData = fast_cast<PilgrimTankData>(_logicData);
	executionData = new ExecutionData();
	executionData->timeToRedraw = 0.06f;
}

PilgrimTank::~PilgrimTank()
{
}

void PilgrimTank::advance(float _deltaTime)
{
	base::advance(_deltaTime);

	if (display && !displaySetup)
	{
		displaySetup = true;
		display->draw_all_commands_immediately();
		display->set_on_update_display(this,
			[this](Framework::Display* _display)
			{
				if (!executionData->redrawNow)
				{
					return;
				}
				executionData->redrawNow = false;

				_display->drop_all_draw_commands();
				_display->add((new Framework::DisplayDrawCommands::Border(Colour::black)));

				_display->add((new Framework::DisplayDrawCommands::Custom())->use_simple(
					[this](Framework::Display* _display, ::System::Video3D* _v3d)
					{
						executionData->redrawNow = false;
						if (executionData->currentState == ExecutionData::State::StartUp)
						{
							if (executionData->inStateDrawnFrames == 0) _display->add((new Framework::DisplayDrawCommands::CLS(Colour::black)));
							executionData->timeToRedraw = 0.06f;
							int frames = executionData->inStateDrawnFrames;
							_display->add((new Framework::DisplayDrawCommands::Custom())->use_simple(
								[frames](Framework::Display* _display, ::System::Video3D* _v3d)
								{
									int const minf = 10;
									if (frames >= minf)
									{
										Utils::draw_device_display_start(_display, _v3d, frames - minf);
									}
								}));
						}
						if (executionData->currentState == ExecutionData::State::Idle ||
							executionData->currentState == ExecutionData::State::Run ||
							executionData->currentState == ExecutionData::State::RunSync)
						{
							if (executionData->randomiseNow)
							{
								_display->add((new Framework::DisplayDrawCommands::CLS(Colour::black)));
								executionData->randomiseNow = false;
								executionData->blinking.clear();

								executionData->colours.clear();
								{
									executionData->colours.set_size(rg.get_int_from_range(2, 8));
									for_every(c, executionData->colours)
									{
										bool isBlack = true;
										bool repeats = true;
										int tryIdx = 0;
										while (isBlack || (repeats && tryIdx < 10))
										{
											++tryIdx;
											c->r = (float)rg.get_int_from_range(0, 2) / 2.0f;
											c->g = (float)rg.get_int_from_range(0, 2) / 2.0f;
											c->b = (float)rg.get_int_from_range(0, 2) / 2.0f;
											c->a = 1.0f;
											isBlack = c->r <= 0.1f && c->g <= 0.1f && c->b <= 0.1f;
											repeats = false;
											for_every(oc, executionData->colours)
											{
												if (oc == c)
												{
													break;
												}
												if (*oc == *c)
												{
													repeats = true;
													break;
												}
											}
										}
									}
								}

								int res = max(_display->get_setup().screenResolution.x, _display->get_setup().screenResolution.y);
								int count;
								{
									count = TypeConversions::Normal::f_i_closest(log2((float)res));
									count -= 2;
									int minCount = 3;
									count = max(minCount, count);
									count = rg.get_int_from_range(minCount, count);
									count = TypeConversions::Normal::f_i_closest(exp2((float)count));
								}
								int size = max(_display->get_setup().screenResolution.x / count, _display->get_setup().screenResolution.y / count);
								size = max(size, 2);
								executionData->idleSpacing = max(1, size / 5);
								executionData->idleSize = size - executionData->idleSpacing;
								executionData->idleCount.x = _display->get_setup().screenResolution.x / size;
								executionData->idleCount.y = _display->get_setup().screenResolution.y / size;
								VectorInt2 gridSize = executionData->idleCount * size - VectorInt2(executionData->idleSpacing);
								executionData->idleOffset = (_display->get_setup().screenResolution - gridSize).to_vector2() / 2.0f;

								_display->add((new Framework::DisplayDrawCommands::Custom())->use_simple(
									[this](Framework::Display* _display, ::System::Video3D* _v3d)
									{
										if (!executionData->colours.is_empty())
										{
											Vector2 gridOffset = Vector2::one * (float)(executionData->idleSize + executionData->idleSpacing);
											Vector2 elSize = Vector2::one * (float)(executionData->idleSize - 1);
											for_count(int, y, executionData->idleCount.y)
											{
												for_count(int, x, executionData->idleCount.x)
												{
													Vector2 at = _display->get_left_bottom_of_screen() + executionData->idleOffset + gridOffset * Vector2((float)x, (float)y);
													::System::Video3DPrimitives::fill_rect_2d(executionData->colours[rg.get_int(executionData->colours.get_size())], at, at + elSize);
												}
											}
										}
									}));
							}
							if (executionData->currentState == ExecutionData::State::Run ||
								executionData->currentState == ExecutionData::State::RunSync)
							{
								if (executionData->changeBlinkingNow)
								{
									executionData->changeBlinkingNow = false;
									executionData->prevBlinking = executionData->blinking;
									_display->add((new Framework::DisplayDrawCommands::Custom())->use_simple(
										[this](Framework::Display* _display, ::System::Video3D* _v3d)
										{
											if (!executionData->colours.is_empty())
											{
												Vector2 gridOffset = Vector2::one * (float)(executionData->idleSize + executionData->idleSpacing);
												Vector2 elSize = Vector2::one * (float)(executionData->idleSize - 1);
												for_every(b, executionData->prevBlinking)
												{
													Vector2 at = _display->get_left_bottom_of_screen() + executionData->idleOffset + gridOffset * b->to_vector2();
													Colour c = executionData->colours[mod(b->x * 967 + b->y * 235 + executionData->blinkCount * (1 + mod(b->x * 3 + b->y * 7, 3)), executionData->colours.get_size())];
													::System::Video3DPrimitives::fill_rect_2d(c, at, at + elSize);
												}
											}
										}));
									executionData->blinking.set_size(rg.get(pilgrimTankData->blinkingCount));
									for_every(b, executionData->blinking)
									{
										b->x = rg.get_int_from_range(0, executionData->idleCount.x - 1);
										b->y = rg.get_int_from_range(0, executionData->idleCount.y - 1);
									}
									executionData->blinkNow = true;
								}
								if (executionData->blinkNow)
								{
									executionData->blinkNow = false;
									_display->add((new Framework::DisplayDrawCommands::Custom())->use_simple(
										[this](Framework::Display* _display, ::System::Video3D* _v3d)
										{
											if (!executionData->colours.is_empty())
											{
												if (executionData->blinkOn && !executionData->blinkPaused)
												{
													++executionData->blinkCount;
												}
												Vector2 gridOffset = Vector2::one * (float)(executionData->idleSize + executionData->idleSpacing);
												Vector2 elSize = Vector2::one * (float)(executionData->idleSize - 1);
												for_every(b, executionData->blinking)
												{
													Vector2 at = _display->get_left_bottom_of_screen() + executionData->idleOffset + gridOffset * b->to_vector2();
													Colour c = _display->get_current_paper();
													if (executionData->blinkOn || executionData->blinkPaused)
													{
														c = executionData->colours[mod(b->x * 967 + b->y * 235 + executionData->blinkCount * (1 + mod(b->x * 3 + b->y * 7, 3)), executionData->colours.get_size())];
													}
													::System::Video3DPrimitives::fill_rect_2d(c, at, at + elSize);
												}
											}
										}));
								}
							}
						}
					}));
			});
	}

	// advance time
	{
		executionData->timeSinceLastDraw += _deltaTime;
		if (executionData->timeSinceLastDraw > executionData->timeToRedraw)
		{
			executionData->redrawNow = true;
			if (executionData->timeToRedraw > 0.0f)
			{
				executionData->timeSinceLastDraw = mod(executionData->timeSinceLastDraw, executionData->timeToRedraw);
			}
			else
			{
				executionData->timeSinceLastDraw = 0.0f;
			}
		}

		if (executionData->redrawNow)
		{
			++executionData->inStateDrawnFrames;
		}
	}

	// advance state
	{
		if (executionData->currentState == ExecutionData::None)
		{
			executionData->set_state(ExecutionData::StartUp);
		}
		if (executionData->currentState == ExecutionData::StartUp &&
			executionData->inStateDrawnFrames > 30)
		{
			executionData->set_state(ExecutionData::Idle);
			executionData->timeToRandomise = 0.0f;
			executionData->timeToChangeBlinking = 0.0f;
			executionData->blinking.clear();
		}
	}

	if (executionData->currentState == ExecutionData::Idle ||
		executionData->currentState == ExecutionData::Run)
	{
		executionData->timeToRandomise -= _deltaTime;
		if (executionData->timeToRandomise <= 0.0f)
		{
			executionData->timeToRandomise = rg.get(pilgrimTankData->randomiseInterval);
			executionData->randomiseNow = true;
			executionData->timeToChangeBlinking = 0.0f;
		}
		executionData->timeToChangeBlinking -= _deltaTime;
		if (executionData->timeToChangeBlinking <= 0.0f)
		{
			executionData->timeToChangeBlinking = rg.get(pilgrimTankData->blinkingInterval);
			executionData->changeBlinkingNow = true;
		}
	}
	{
		if (mod(executionData->inStateDrawnFrames, pilgrimTankData->blinkingFrames) == 0)
		{
			executionData->blinkNow = true;
			executionData->blinkOn = true;// !executionData->blinkOn;
		}
	}
}

void PilgrimTank::learn_from(SimpleVariableStorage & _parameters)
{
	base::learn_from(_parameters);
}

LATENT_FUNCTION(PilgrimTank::execute_logic)
{
	PERFORMANCE_GUARD_LIMIT(0.005f, TXT("[ai pilgrimTank] execute logic"));

	LATENT_SCOPE();
	LATENT_SCOPE_INFO(TXT("execute logic"));

	LATENT_PARAM(::Framework::AI::MindInstance*, mind); // auto
	LATENT_PARAM(::Framework::AI::Logic*, logic); // auto
	LATENT_END_PARAMS();

	LATENT_VAR(::Framework::AI::MessageHandler, messageHandler);

	auto * imo = mind->get_owner_as_modules_owner();
	auto * self = fast_cast<PilgrimTank>(logic);

	LATENT_BEGIN_CODE();

	messageHandler.use_with(mind);
	{
		messageHandler.set(NAME(aimIdle), [self](Framework::AI::Message const& _message)
			{
				self->executionData->set_state(ExecutionData::State::Idle);
			}
		);
		messageHandler.set(NAME(aimRun), [self](Framework::AI::Message const& _message)
			{
				self->executionData->set_state(ExecutionData::State::Run);
			}
		);
		messageHandler.set(NAME(aimRunSync), [self](Framework::AI::Message const& _message)
			{
				if (self->executionData->set_state(ExecutionData::State::RunSync))
				{
					self->executionData->randomiseNow = true;
				}
			}
		);
		messageHandler.set(NAME(aimRandomise), [self](Framework::AI::Message const& _message)
			{
				if (self->executionData->currentState == ExecutionData::State::RunSync)
				{
					self->executionData->randomiseNow = true;
					self->executionData->blinkPaused = false;
				}
			}
		);
		messageHandler.set(NAME(aimChangeBlinking), [self](Framework::AI::Message const& _message)
			{
				if (self->executionData->currentState == ExecutionData::State::RunSync)
				{
					self->executionData->changeBlinkingNow = true;
					self->executionData->blinkPaused = false;
				}
			}
		);
		messageHandler.set(NAME(aimPauseBlinking), [self](Framework::AI::Message const& _message)
			{
				if (self->executionData->currentState == ExecutionData::State::RunSync)
				{
					self->executionData->blinkNow = true;
					self->executionData->blinkPaused = true;
				}
			}
		);
	}

	LATENT_WAIT(0.1f);

	if (imo)
	{
		if (auto* cmd = imo->get_custom<Framework::CustomModules::Display>())
		{
			self->display = cmd->get_display();
		}
	}

	while (true)
	{
		LATENT_WAIT(Random::get_float(0.05f, 0.1f));
	}

	LATENT_ON_BREAK();
	//

	LATENT_ON_END();
	//

	LATENT_END_CODE();
	LATENT_RETURN();
}

//

bool PilgrimTank::ExecutionData::set_state(State _state)
{
	if (currentState != _state)
	{
		currentState = _state;
		inStateDrawnFrames = 0;
		redrawNow = true;
		timeSinceLastDraw = 0.0f;
		timeToRedraw = 0.0f;
		blinkPaused = false;
		return true;
	}
	else
	{
		return false;
	}
}

//

REGISTER_FOR_FAST_CAST(PilgrimTankData);

bool PilgrimTankData::load_from_xml(IO::XML::Node const * _node, ::Framework::LibraryLoadingContext & _lc)
{
	bool result = base::load_from_xml(_node, _lc);

	result &= randomiseInterval.load_from_xml(_node, TXT("randomiseInterval"));
	result &= blinkingInterval.load_from_xml(_node, TXT("blinkingInterval"));
	result &= blinkingCount.load_from_xml(_node, TXT("blinkingCount"));
	blinkingFrames = _node->get_int_attribute(TXT("blinkingFrames"), blinkingFrames);

	return result;
}

bool PilgrimTankData::prepare_for_game(::Framework::Library* _library, ::Framework::LibraryPrepareContext& _pfgContext)
{
	bool result = base::prepare_for_game(_library, _pfgContext);

	return result;
}
