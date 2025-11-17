#include "aiLogic_exmInstallerKiosk.h"

#include "utils\aiLogicUtil_deviceDisplayStart.h"

#include "..\..\game\game.h"
#include "..\..\library\exmType.h"
#include "..\..\library\lineModel.h"
#include "..\..\modules\gameplay\modulePilgrim.h"

#include "..\..\..\framework\ai\aiMindInstance.h"
#include "..\..\..\framework\display\display.h"
#include "..\..\..\framework\display\displayDrawCommands.h"
#include "..\..\..\framework\library\library.h"
#include "..\..\..\framework\library\usedLibraryStored.inl"
#include "..\..\..\framework\module\modulePresence.h"
#include "..\..\..\framework\module\custom\mc_display.h"
#include "..\..\..\framework\modulesOwner\modulesOwnerImpl.inl"
#include "..\..\..\framework\object\actor.h"

#include "..\..\..\core\system\video\video3dPrimitives.h"

#ifdef AN_CLANG
#include "..\..\..\framework\library\usedLibraryStored.inl"
#endif

//

#ifdef AN_ALLOW_OPTIMIZE_OFF
#pragma optimize("", off)
#endif

//

using namespace TeaForGodEmperor;
using namespace AI;
using namespace Logics;

//

// variables
DEFINE_STATIC_NAME(chooseEXMTypeId);
DEFINE_STATIC_NAME(chooseEXMId);
DEFINE_STATIC_NAME(upgradeCanisterId);

// emissive layers
DEFINE_STATIC_NAME(ready);
DEFINE_STATIC_NAME(held);
DEFINE_STATIC_NAME(bad);

// tags
DEFINE_STATIC_NAME(onShipKiosk);

//

static void draw_line(REF_ float& drawLines, Colour const& _colour, float _size, Vector2 _a, Vector2 _b)
{
	float lineLength = (_b - _a).length() / (_size * 0.3f);

	if (lineLength > 0.0f && lineLength > drawLines)
	{
		_b = _a + (_b - _a) * (drawLines / lineLength);
		drawLines = 0.0f;
	}
	else
	{
		drawLines -= lineLength;
	}
	::System::Video3DPrimitives::line_2d(_colour, _a, _b);
}

//

REGISTER_FOR_FAST_CAST(EXMInstallerKiosk);

EXMInstallerKiosk::EXMInstallerKiosk(::Framework::AI::MindInstance* _mind, ::Framework::AI::LogicData const * _logicData)
: base(_mind, _logicData, execute_logic)
{
	exmInstallerKioskData = fast_cast<EXMInstallerKioskData>(_logicData);
}

EXMInstallerKiosk::~EXMInstallerKiosk()
{
	if (display && displayOwner.get())
	{
		display->use_background(nullptr);
		display->set_on_update_display(this, nullptr);
	}
}

void EXMInstallerKiosk::advance(float _deltaTime)
{
	base::advance(_deltaTime);

	chooseEXMTypeHandler.advance(_deltaTime);
	chooseEXMHandler.advance(_deltaTime);

	if (display && displaySetup)
	{
		{
			timeToRedraw -= _deltaTime;
			inStateTime += _deltaTime;
			if (timeToRedraw <= 0.0f)
			{
				redrawNow = true;
				timeToRedraw = 0.1f;
				if (state == ShowContent)
				{
					timeToRedraw = variousDrawingVariables.contentDrawn ? 0.3f : 0.02f;
				}
			}
		}
		State prevState = state;
		{
			if (state == Off)
			{
				currentPilgrim.clear();
			}
			if (currentPilgrim != presentPilgrim || !currentPilgrim.is_set())
			{
				if (state != Off && state != ShutDown)
				{
					state = ShutDown;
				}
			}
			if (currentPilgrim != presentPilgrim && !currentPilgrim.is_set() && presentPilgrim.is_set() && state == Off)
			{
				currentPilgrim = presentPilgrim;

				availableActiveEXMs.clear();
				availablePassiveEXMs.clear();

				bool allowFromPilgrimagesInventory = true;

				if (auto* gs = TeaForGodEmperor::PlayerSetup::access_current().access_active_game_slot())
				{
					if (gs->get_game_slot_mode() == TeaForGodEmperor::GameSlotMode::Missions)
					{
						if (auto* o = get_imo()->get_as_object())
						{
							if (o->get_tags().get_tag(NAME(onShipKiosk)))
							{
								allowFromPilgrimagesInventory = false;
							}
						}
					}
				}

				Array<Name> availableEXMs;
				// get from persistence and from pilgrim
				{
					if (auto* p = Persistence::access_current_if_exists())
					{
						Concurrency::ScopedSpinLock lock(p->access_lock());
						for_every(exmName, p->get_all_exms())
						{
							availableEXMs.push_back_unique(*exmName);
						}
					}
					if (currentPilgrim.is_set())
					{
						if (ModulePilgrim* mp = currentPilgrim->get_gameplay_as<ModulePilgrim>())
						{
							Concurrency::ScopedMRSWLockRead lock(mp->get_pilgrim_inventory().exmsLock);
							for_every(exmName, mp->get_pilgrim_inventory().availableEXMs)
							{
								availableEXMs.push_back_unique(*exmName);
							}
						}
					}
				}
				for_every(exmName, availableEXMs)
				{
					if (auto* exm = EXMType::find(*exmName))
					{
						if (!exm->is_permanent() &&
							!exm->is_integral())
						{
							if (exm->is_active())
							{
								availableActiveEXMs.push_back_unique(exm);
							}
							else
							{
								availablePassiveEXMs.push_back_unique(exm);
							}
						}
					}
				}
				if (! availablePassiveEXMs.is_empty() ||
					! availableActiveEXMs.is_empty())
				{
					if (state == Off)
					{
						state = Start;
					}
				}
				activeEXMIdx = availableActiveEXMs.get_size() / 2;
				passiveEXMIdx = availablePassiveEXMs.get_size() / 2;
			}
			if (state != prevState)
			{
				inStateDrawnFrames = 0;
				inStateTime = 0.0f;
				redrawNow = true;
				prevState = state;
			}
		}
		// update selected exm to show and in canister
		{
			EXMType const* selectedEXM = nullptr;

			if (state == ShowContent || state == Start)
			{
				bool prevSelectedActivEXMs = selectedActiveEXMs;
				selectedActiveEXMs = TypeConversions::Normal::f_i_closest(chooseEXMTypeHandler.get_output()) == 0;

				auto& from = selectedActiveEXMs ? availableActiveEXMs : availablePassiveEXMs;
				auto& idx = selectedActiveEXMs ? activeEXMIdx : passiveEXMIdx;

				{
					int prevIdx = idx;

					if (selectedActiveEXMs != prevSelectedActivEXMs)
					{
						chooseEXMHandler.reset_dial_zero(idx);

						if (state == ShowContent)
						{
							redrawNow = true;
						}
					}

					idx = from.is_empty() ? 0 : mod(chooseEXMHandler.get_dial_at(), from.get_size());

#ifdef AN_DEVELOPMENT_OR_PROFILER
#ifdef AN_STANDARD_INPUT
					if (::System::Input::get()->has_key_been_pressed(System::Key::F))
					{
						if (::System::Input::get()->is_key_pressed(System::Key::LeftAlt))
						{
							selectedActiveEXMs = !selectedActiveEXMs;
						}
						else if (::System::Input::get()->is_key_pressed(System::Key::LeftShift))
						{
							idx = from.is_empty() ? 0 : mod(idx - 1, from.get_size());
							chooseEXMHandler.reset_dial_zero(idx);
						}
						else 
						{
							idx = from.is_empty() ? 0 : mod(idx + 1, from.get_size());
							chooseEXMHandler.reset_dial_zero(idx);
						}
						redrawNow = true;
					}
#endif
#endif
					if (idx != prevIdx)
					{
						redrawNow = true;
					}
				}

				if (from.is_index_valid(idx))
				{
					selectedEXM = from[idx];
				}
			}

			{
				LineModel* contentLineModel = nullptr;
				LineModel* borderLineModel = nullptr;

				if (selectedEXM)
				{
					contentLineModel = selectedEXM->get_line_model_for_display();
					borderLineModel = selectedEXM->is_active() ? exmInstallerKioskData->activeEXMLineModel.get() : exmInstallerKioskData->passiveEXMLineModel.get();
				}

				if (variousDrawingVariables.contentLineModel != contentLineModel ||
					variousDrawingVariables.borderLineModel != borderLineModel)
				{
					variousDrawingVariables.contentDrawLines = 1000.0f;
					variousDrawingVariables.contentDrawn = false;
					variousDrawingVariables.contentLineModel = contentLineModel;
					variousDrawingVariables.borderLineModel = borderLineModel;
					if (state == ShowContent)
					{
						redrawNow = true;
					}
				}
			}

			canister.content.exm = selectedEXM;
		}
		if (redrawNow)
		{
			++inStateDrawnFrames;
			variousDrawingVariables.contentDrawLines += 10.0f;

			if (state == Start)
			{
				if (inStateTime > 0.5f)
				{
					state = ShowContent;
					timeToRedraw = 0.1f;
				}
			}
			else if (state == ShutDown)
			{
				int frames = 10;
				int frameNo = inStateDrawnFrames;

				timeToRedraw = 0.03f;

				variousDrawingVariables.shutdownAtPt = (float)frameNo / (float)frames;

				if (frameNo >= frames)
				{
					state = Off;
				}
			}
			else if (state == ShowContent)
			{
				// handled earlier
			}
		}
	}

	{
		canister.set_active(state == ShowContent);
		canister.advance(_deltaTime);

		if (state == ShowContent &&
			canister.should_give_content())
		{
			if (canister.give_content())
			{
				if (!markedAsKnownForOpenWorldDirection)
				{
					if (auto* piow = PilgrimageInstanceOpenWorld::get())
					{
						piow->mark_pilgrimage_device_direction_known(get_imo());
						markedAsKnownForOpenWorldDirection = true;
					}
				}
			}
		}
	}

	if (display && !displaySetup)
	{
		displaySetup = true;
		display->draw_all_commands_immediately();
		display->set_on_update_display(this,
			[this](Framework::Display* _display)
			{
				if (!redrawNow)
				{
					return;
				}
				redrawNow = false;

				_display->drop_all_draw_commands();
				if (state != Start)
				{
					_display->add((new Framework::DisplayDrawCommands::CLS(Colour::black)));
				}
				_display->add((new Framework::DisplayDrawCommands::Border(Colour::black)));
				{
					auto* lm = variousDrawingVariables.contentLineModel;
					auto* blm = variousDrawingVariables.borderLineModel;
					float drawLines = variousDrawingVariables.contentDrawLines;
					_display->add((new Framework::DisplayDrawCommands::Custom())->use_simple(
						[this, lm, blm, drawLines](Framework::Display* _display, ::System::Video3D* _v3d)
						{
							if (state == Start)
							{
								if (inStateDrawnFrames == 0) _display->add((new Framework::DisplayDrawCommands::CLS(Colour::black)));
								redrawNow = false;
								timeToRedraw = 0.06f;
								int frames = inStateDrawnFrames;
								_display->add((new Framework::DisplayDrawCommands::Custom())->use_simple(
									[frames](Framework::Display* _display, ::System::Video3D* _v3d)
									{
										Utils::draw_device_display_start(_display, _v3d, frames);
									}));
							}
							else if (state == ShutDown)
							{
								Vector2 bl = _display->get_left_bottom_of_screen() + Vector2::half;
								Vector2 tr = _display->get_right_top_of_screen() - Vector2::half;

								float atY = tr.y + (bl.y - tr.y) * clamp(variousDrawingVariables.shutdownAtPt, 0.0f, 1.0f);

								Vector2 a(bl.x, atY);
								Vector2 b(tr.x, atY);
								_display->add((new Framework::DisplayDrawCommands::Custom())->use_simple(
									[a, b](Framework::Display* _display, ::System::Video3D* _v3d)
									{
										::System::Video3DPrimitives::line_2d(_display->get_current_ink(), a, b);
									}));
							}
							else if (state == State::ShowContent)
							{
								Colour const ink = _display->get_current_ink();
								{
									float linesLeft = drawLines;

									Vector2 bl = _display->get_left_bottom_of_screen() + Vector2::half;
									Vector2 tr = _display->get_right_top_of_screen() - Vector2::half;
									bl.y = tr.y - (tr.x - bl.x); // draw using upper part

									Vector2 c = TypeConversions::Normal::f_i_cells((bl + tr) * 0.5f).to_vector2() + Vector2::half;

									Vector2 size(tr.x - c.x, tr.y - c.y);
									size.x = min(size.x, size.y);
									size.y = size.x;

									size = TypeConversions::Normal::f_i_closest(size * 2.0f).to_vector2();

									if (blm)
									{
										for_every(l, blm->get_lines())
										{
											if (linesLeft <= 0.0f)
											{
												break;
											}
											draw_line(REF_ linesLeft, l->colour.get(ink), size.y,
												TypeConversions::Normal::f_i_cells(c + size * l->a.to_vector2()).to_vector2() + Vector2::half,
												TypeConversions::Normal::f_i_cells(c + size * l->b.to_vector2()).to_vector2() + Vector2::half);
										}

										size *= 0.8f;
									}
									if (lm)
									{
										for_every(l, lm->get_lines())
										{
											if (linesLeft <= 0.0f)
											{
												break;
											}
											draw_line(REF_ linesLeft, l->colour.get(ink), size.y,
												TypeConversions::Normal::f_i_cells(c + size * l->a.to_vector2()).to_vector2() + Vector2::half,
												TypeConversions::Normal::f_i_cells(c + size * l->b.to_vector2()).to_vector2() + Vector2::half);
										}
									}
									else
									{
										// draw anything for time being
										//::System::Video3DPrimitives::line_2d(ink, Vector2(bl.x, bl.y), Vector2(tr.x, tr.y));
										//::System::Video3DPrimitives::line_2d(ink, Vector2(bl.x, tr.y), Vector2(tr.x, bl.y));
									}
									if (linesLeft > 5.0f)
									{
										variousDrawingVariables.contentDrawn = true;
									}
								}
								{
									Vector2 bl = _display->get_left_bottom_of_screen() + Vector2::half;
									Vector2 tr = _display->get_right_top_of_screen() - Vector2::half;
									tr.y = tr.y - (tr.x - bl.x) - 3.0f; // draw using bottom part

									float rowSize = (float)TypeConversions::Normal::f_i_cells((tr.y - bl.y + 1.0f) / 2.0f);

									Range row(bl.y, bl.y + rowSize - 1.0f);
									for_count(int, rowIdx, 2)
									{
										auto& availableEXMs = rowIdx == 0 ? availablePassiveEXMs : availableActiveEXMs;

										Range rowInt = row;
										rowInt.min += 2.0f;
										rowInt.max -= 2.0f;

										float fullWidth = (rowInt.max - rowInt.min + 1.0f) + 4.0f;
										int amount = availableEXMs.get_size();
										if (amount > 0)
										{
											fullWidth = clamp((floor)((tr.x - bl.x + 1.0f) / (float)amount), 1.0f, fullWidth);
										}

										float width = max(1.0f, fullWidth - 4.0f);

										float allWidth = fullWidth * (float)amount;
										float startAt = max(bl.x, (tr.x + bl.x) * 0.5f - allWidth * 0.5f);
										float startRect = (float)TypeConversions::Normal::f_i_cells(startAt + (fullWidth - width) * 0.5f);

										Colour colour = rowIdx == 0 ? exmInstallerKioskData->passiveEXMColour : exmInstallerKioskData->activeEXMColour;

										{
											float rect = startRect;
											for_count(int, i, amount)
											{
												::System::Video3DPrimitives::fill_rect_2d(colour, Vector2(rect, rowInt.min), Vector2(rect + width, rowInt.max + 1.0f));
												rect += fullWidth;
											}
										}

										if ((rowIdx == 1 && selectedActiveEXMs) ||
											(rowIdx == 0 && !selectedActiveEXMs))
										{
											int selectedEXMIdx = rowIdx == 0 ? passiveEXMIdx : activeEXMIdx;
											::System::Video3DPrimitives::rect_2d(exmInstallerKioskData->selectColour, Vector2(bl.x, row.min), Vector2(tr.x - 1.0f /* align with border line model */, row.max));

											float rect = startAt + (float)selectedEXMIdx * fullWidth;
											::System::Video3DPrimitives::rect_2d(exmInstallerKioskData->selectColour, Vector2(rect, row.min), Vector2(rect + fullWidth - 1.0f, row.max));
										}

										row.min += rowSize;
										row.max += rowSize;
									}
								}
							}
						}));
				}
			});
	}

}

void EXMInstallerKiosk::learn_from(SimpleVariableStorage& _parameters)
{
	base::learn_from(_parameters);
}

LATENT_FUNCTION(EXMInstallerKiosk::execute_logic)
{
	PERFORMANCE_GUARD_LIMIT(0.005f, TXT("[ai exm installer kiosk] execute logic"));

	LATENT_SCOPE();
	LATENT_SCOPE_INFO(TXT("execute logic"));

	LATENT_PARAM(::Framework::AI::MindInstance*, mind); // auto
	LATENT_PARAM(::Framework::AI::Logic*, logic); // auto
	LATENT_END_PARAMS();

	auto * imo = mind->get_owner_as_modules_owner();
	auto * self = fast_cast<EXMInstallerKiosk>(logic);

	LATENT_BEGIN_CODE();

	{
		while (!self->display)
		{
			if (auto* cmd = imo->get_custom<Framework::CustomModules::Display>())
			{
				self->display = cmd->get_display();
				self->displayOwner = imo;
				self->redrawNow = true;
			}

			self->chooseEXMTypeHandler.initialise(imo, NAME(chooseEXMTypeId));
			self->chooseEXMHandler.initialise(imo, NAME(chooseEXMId));
			self->canister.initialise(imo, NAME(upgradeCanisterId));

			LATENT_WAIT(Random::get_float(0.2f, 0.5f));
		}
	}

	while (true)
	{
		{
			todo_multiplayer_issue(TXT("we just get player here, we should get all players?"));
			ModulePilgrim* mp = nullptr;
			if (auto* g = Game::get_as<Game>())
			{
				if (auto* pa = g->access_player().get_actor())
				{
					if (pa->get_presence() &&
						pa->get_presence()->get_in_room() == imo->get_presence()->get_in_room())
					{
						mp = pa->get_gameplay_as<ModulePilgrim>();
					}
				}
			}

			self->presentPilgrim = mp? mp->get_owner() : nullptr;

		}
		LATENT_YIELD(); // to allow advance every frame
	}

	LATENT_ON_BREAK();
	//

	LATENT_ON_END();
	//

	LATENT_END_CODE();
	LATENT_RETURN();
}

//

REGISTER_FOR_FAST_CAST(EXMInstallerKioskData);

bool EXMInstallerKioskData::load_from_xml(IO::XML::Node const * _node, ::Framework::LibraryLoadingContext & _lc)
{
	bool result = base::load_from_xml(_node, _lc);

	activeEXMLineModel.load_from_xml(_node, TXT("activeEXMLineModel"), _lc);
	passiveEXMLineModel.load_from_xml(_node, TXT("passiveEXMLineModel"), _lc);

	selectColour.load_from_xml_child_or_attr(_node, TXT("selectColour"));
	activeEXMColour.load_from_xml_child_or_attr(_node, TXT("activeEXMColour"));
	passiveEXMColour.load_from_xml_child_or_attr(_node, TXT("passiveEXMColour"));

	return result;
}

bool EXMInstallerKioskData::prepare_for_game(::Framework::Library* _library, ::Framework::LibraryPrepareContext& _pfgContext)
{
	bool result = base::prepare_for_game(_library, _pfgContext);

	result &= activeEXMLineModel.prepare_for_game_find(_library, _pfgContext, Framework::LibraryPrepareLevel::Resolve);
	result &= passiveEXMLineModel.prepare_for_game_find(_library, _pfgContext, Framework::LibraryPrepareLevel::Resolve);

	return result;
}
