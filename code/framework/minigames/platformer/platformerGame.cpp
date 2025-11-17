#include "platformerGame.h"

#include "platformerCharacter.h"
#include "platformerInfo.h"
#include "platformerRoom.h"
#include "platformerTile.h"

#include "..\..\pipelines\colourClashPipeline.h"

#include "..\..\video\font.h"
#include "..\..\video\colourClashSprite.h"

#include "..\..\text\stringFormatter.h"

#include "..\..\..\core\containers\arrayStack.h"
#include "..\..\..\core\system\input.h"
#include "..\..\..\core\system\video\renderTarget.h"
#include "..\..\..\core\system\video\video3dPrimitives.h"

#ifdef AN_MINIGAME_PLATFORMER

using namespace Platformer;

//

DEFINE_STATIC_NAME(platformerMiniGame);
DEFINE_STATIC_NAME(itemsTaken);
DEFINE_STATIC_NAME(hours);
DEFINE_STATIC_NAME(minutes);
DEFINE_STATIC_NAME(seconds);
DEFINE_STATIC_NAME(jump);
DEFINE_STATIC_NAME(movement);

//

Game::Game(Info* _info, VectorInt2 const & _wholeDisplayResolution)
: info(_info)
{
	::Framework::ColourClashPipeline::will_use_shaders();

	playerInput.use(NAME(platformerMiniGame));
	playerInput.use(::Framework::GameInputIncludeVR::All, true);

	screenOffset = VectorInt2::zero;
	if (info)
	{
		useBorderSize = _wholeDisplayResolution;
		screenOffset = (_wholeDisplayResolution - info->screenResolution) / 2;
		VectorInt2 createBorderSize = _wholeDisplayResolution;
		createBorderSize.x = (createBorderSize.x % 8) == 0 ? createBorderSize.x : createBorderSize.x + 8 - (createBorderSize.x % 8);
		createBorderSize.y = (createBorderSize.y % 8) == 0 ? createBorderSize.y : createBorderSize.y + 8 - (createBorderSize.y % 8);
		::Framework::ColourClashPipeline::create_render_targets(createBorderSize, borderGraphicsRT, borderColourGridRT);
		::Framework::ColourClashPipeline::create_render_targets(info->screenResolution, backgroundGraphicsRT, backgroundColourGridRT);
		::Framework::ColourClashPipeline::create_render_targets(info->screenResolution, mainGraphicsRT, mainColourGridRT);

		title_screen();
	}
}

Game::~Game()
{
	::Framework::ColourClashPipeline::release_shaders();
}

void Game::title_screen()
{
	atTitleScreen = true;
	atGameOverScreen = false;
	asksForAutoExitFor = 0.0f;

	inRoom = info->titleScreenRoom.get();

	spawn_guardians();
}

void Game::start_game()
{
	++timesPlayed;
	atTitleScreen = false;
	atGameOverScreen = false;
	asksForAutoExitFor = 0.0f;

	inRoom = nullptr;
	visitedRooms.clear();
	on_change_room(info->startsInRoom.get());

	timeHours = 8;
	timeMinutes = 0;
	timeSeconds = 0;

	itemsTaken = 0;
	livesLeftCount = info->livesLimit - 1;
	inRoom = info->startsInRoom.get();
	player.character = info->playerCharacter.get();
	player.at = info->startsAtTile * (info ? info->tileSize : VectorInt2::zero);
	player.dir = VectorInt2(1, 0);
	player.currentMovement = VectorInt2::zero;
	playerAutoMovement = 0;
	playerJumpFrame = NONE;
	playerFalling = false;
	playerFallsFor = 0;
	
	do_checkpoint();

	spawn_guardians();

	renderItemsAndTime = true;
}

void Game::game_over()
{
	atTitleScreen = false;
	atGameOverScreen = true;

	inRoom = info->gameOverScreenRoom.get();
	gameOverFrame = 0;

	spawn_guardians();
}

void do_for_rendered_room(Room* renderedRoom, std::function< void(int x, int y, Tile const * tt) > _for_tile)
{
	if (renderedRoom)
	{
		int *tile = renderedRoom->tiles.get_data();
		for (int y = 0; y < renderedRoom->size.y; ++y)
		{
			for (int x = 0; x < renderedRoom->size.x; ++x)
			{
				if (renderedRoom->tileRefs.is_index_valid(*tile))
				{
					Tile const * tt = renderedRoom->tileRefs[*tile].tile.get();
					_for_tile(x, y, tt);
				}
				++tile;
			}
		}
	}
}

void Game::render_room_name(String const & _name)
{
	VectorInt2 a = info->renderRoomOffset + VectorInt2(0, -info->tileSize.y);
	VectorInt2 b = VectorInt2(info->screenResolution.x, info->renderRoomOffset.y - 1);
	VectorInt2 end = b;
	::System::Video3D* v3d = ::System::Video3D::get();

	if (atTitleScreen || atGameOverScreen)
	{
		a.y -= info->tileSize.y;
		b.y -= info->tileSize.y;
	}

	backgroundGraphicsRT->bind();
	if (atTitleScreen || atGameOverScreen)
	{
		::Framework::ColourClashPipeline::draw_graphics_fill_rect(VectorInt2::zero, b, Colour::black);
	}
	else
	{
		::Framework::ColourClashPipeline::draw_graphics_fill_rect(a, b, Colour::black);
	}

	if (::Framework::Font* font = info->font.get())
	{
		{
			String roomName = _name;
			int charsRequired = info->screenResolution.x / font->calculate_char_size().to_vector_int2_cells().x;
			charsRequired -= roomName.get_length();
			charsRequired /= 2;
			while (charsRequired > 0)
			{
				roomName = String::space() + roomName;
				--charsRequired;
			}
			font->draw_text_at(v3d, roomName, Colour::white, a.to_vector2());
		}
		if (atTitleScreen)
		{
			a.y -= info->tileSize.y * 3;
			String roomName = info->startGameInfo.get();
			int charsRequired = info->screenResolution.x / font->calculate_char_size().to_vector_int2_cells().x;
			charsRequired -= roomName.get_length();
			charsRequired /= 2;
			while (charsRequired > 0)
			{
				roomName = String::space() + roomName;
				--charsRequired;
			}
			font->draw_text_at(v3d, roomName, Colour::white, a.to_vector2());
		}
	}
	backgroundGraphicsRT->unbind();

	backgroundColourGridRT->bind();
	if (atGameOverScreen)
	{
		::Framework::ColourClashPipeline::draw_rect_on_colour_grid(VectorInt2::zero, end, Colour::black, Colour::black, Colour::black);
		::Framework::ColourClashPipeline::draw_rect_on_colour_grid(VectorInt2::zero, end - VectorInt2(0, 1), Colour::white, Colour::black, Colour::black);
	}
	else
	{
		::Framework::ColourClashPipeline::draw_rect_on_colour_grid(VectorInt2::zero, end, !atTitleScreen && !atGameOverScreen ? Colour::yellow : Colour::white, Colour::black, Colour::black);
	}
	backgroundColourGridRT->unbind();
}

void Game::render_items_and_time()
{
	VectorInt2 a = info->renderRoomOffset + VectorInt2(0, -info->tileSize.y * 4);
	VectorInt2 b = VectorInt2(info->screenResolution.x, info->renderRoomOffset.y - 1 - info->tileSize.y*3);

	::System::Video3D* v3d = ::System::Video3D::get();

	backgroundGraphicsRT->bind();
	::Framework::ColourClashPipeline::draw_graphics_fill_rect(a, b, Colour::black);
	if (::Framework::Font* font = info->font.get())
	{
		{
			String text = Framework::StringFormatter::format_loc_str(info->itemsCollectedInfo, nullptr,
				Framework::StringFormatterParams()
				.add(NAME(itemsTaken), itemsTaken));
			font->draw_text_at(v3d, text, Colour::white, a.to_vector2());
			int hoursDisplay = timeHours == 0 ? 12 : ((timeHours - 1) % 12) + 1;
			text = Framework::StringFormatter::format_loc_str(info->timeInfo, nullptr,
				Framework::StringFormatterParams()
				.add(NAME(hours), timeHours)
				.add(NAME(minutes), timeMinutes)
				.add(NAME(seconds), timeSeconds));
			font->draw_text_at(v3d, text, Colour::white, VectorInt2(b.x, a.y).to_vector2(), Vector2::one, Vector2(1.0f, 0.0f));
		}
	}

	backgroundGraphicsRT->unbind();

	backgroundColourGridRT->bind();
	::Framework::ColourClashPipeline::draw_rect_on_colour_grid(a, b, Colour::white, Colour::black, Colour::black);
	Colour gradColours[] = { Colour::blue, Colour::red, Colour::magenta, Colour::green, Colour::cyan, Colour::yellow };
	for (int i = 0; i < 6; ++i)
	{
		VectorInt2 ao = a;
		VectorInt2 bo = b;
		ao.x = info->tileSize.x * i;
		bo.x = ao.x + info->tileSize.x - 1;
		::Framework::ColourClashPipeline::draw_rect_on_colour_grid(ao, bo, gradColours[i], Colour::black, Colour::black);
		ao.x = b.x - info->tileSize.x * (i + 1);
		bo.x = ao.x + info->tileSize.x - 1;
		::Framework::ColourClashPipeline::draw_rect_on_colour_grid(ao, bo, gradColours[i], Colour::black, Colour::black);
	}
	backgroundColourGridRT->unbind();
}

void Game::advance(float _deltaTime, ::System::RenderTarget* _outputRT)
{
	if (!info)
	{
		return;
	}

	if (asksForAutoExit && atTitleScreen)
	{
		asksForAutoExitFor += _deltaTime;
	}

	if (!inRoom)
	{
		return;
	}

	{
		Vector2 currentMovementStick = Vector2::zero;
		Framework::IGameInput* input = &playerInput;
		if (useProvidedInput)
		{
			input = providedInput;
		}
		if (input)
		{
			if (input->is_button_pressed(NAME(jump)))
			{
				jumpButtonPressed = true;
			}
			currentMovementStick = input->get_stick(NAME(movement));
		}
		movementStick.x = min(currentMovementStick.x, movementStick.x);
		movementStick.x = max(currentMovementStick.x, movementStick.x);
	}

	if (!useProvidedInput)
	{
		if (::System::Input* sysInput = ::System::Input::get())
		{
			if (sysInput->has_key_been_pressed(::System::Key::G))
			{
				godMode = !godMode;
			}
			if (sysInput->has_key_been_pressed(::System::Key::L))
			{
				livesLeftCount = min(16, livesLeftCount + 1);
			}
			if (sysInput->has_key_been_pressed(::System::Key::A))
			{
				timeMultiplier *= 0.5f;
			}
			if (sysInput->has_key_been_pressed(::System::Key::S))
			{
				timeMultiplier = 1.0f;
			}
			if (sysInput->has_key_been_pressed(::System::Key::D))
			{
				timeMultiplier *= 2.0f;
			}
		}
	}

	float const maxFramesAdvance = timeMultiplier != 1.0f? 16.0f : 2.0f;
	timeLeftThisFrame -= _deltaTime * timeMultiplier;
	timeLeftThisFrame = max(timeLeftThisFrame, -maxFramesAdvance / (float)info->fps);
	while (timeLeftThisFrame < 0.0f)
	{
		timeLeftThisFrame += 1.0f / (float)info->fps;
		/*
		if (timeLeftThisFrame <= 0.0f)
		{
			timeLeftThisFrame = 1.0f / (float)info->fps;
		}
		*/

		advance_frame();
	}

	// render!

	::System::Video3D* v3d = ::System::Video3D::get();

	if (renderedRoom != inRoom || forceRerender)
	{
		renderItemsAndTime = true;
		forceRerender = false;

		renderedRoom = inRoom;

		borderGraphicsRT->bind();
		v3d->clear_colour_depth(Colour::black);
		borderGraphicsRT->unbind();

		borderColourGridRT->bind();
		::Framework::ColourClashPipeline::draw_rect_on_colour_grid(VectorInt2::zero, useBorderSize, renderedRoom->borderColour, renderedRoom->borderColour, Colour::black);
		borderColourGridRT->unbind();

		backgroundGraphicsRT->bind();
		v3d->clear_colour_depth(Colour::black);

		do_for_rendered_room(renderedRoom,
			[this, v3d](int x, int y, Tile const * tt)
			{
				if (!tt->isItem && !tt->foreground && tt->frames.get_size() == 1)
				{
					if (::Framework::ColourClashSprite const * tp = tt->frames[0].get())
					{
						tp->draw_graphics_at(v3d, (VectorInt2(x, y) * info->tileSize + info->renderRoomOffset));
					}
				}
			}
			);
		backgroundGraphicsRT->unbind();

		backgroundColourGridRT->bind();
		v3d->clear_colour_depth(Colour::black);
		::Framework::ColourClashPipeline::draw_rect_on_colour_grid(info->renderRoomOffset, info->screenResolution, renderedRoom ? renderedRoom->foregroundColour : Colour::white, renderedRoom ? renderedRoom->backgroundColour : Colour::blue, renderedRoom? renderedRoom->brightnessIndicator : Colour::black);

		do_for_rendered_room(renderedRoom,
			[this, v3d](int x, int y, Tile const * tt)
			{
				if (!tt->isItem && !tt->foreground && tt->frames.get_size() == 1)
				{
					if (::Framework::ColourClashSprite const * tp = tt->frames[0].get())
					{
						tp->draw_colour_grid_at(v3d, (VectorInt2(x, y) * info->tileSize + info->renderRoomOffset), tt->foregroundColourOverride, tt->backgroundColourOverride, tt->brightColourOverride);
					}
				}
			}
		);
		backgroundColourGridRT->unbind();

		render_room_name(renderedRoom->roomName.get());
		renderItemsAndTime = true;
	}

	// copy from background to main
	backgroundGraphicsRT->resolve();
	backgroundColourGridRT->resolve();
	::Framework::ColourClashPipeline::copy(backgroundGraphicsRT.get(), backgroundColourGridRT.get(), mainGraphicsRT.get(), mainColourGridRT.get());

	// render animated background and door portals
	{
		mainGraphicsRT->bind();
		do_for_rendered_room(renderedRoom,
			[this, v3d](int x, int y, Tile const * tt)
			{
				if (!tt->isItem && tt->frames.get_size() > 1)
				{
					if (::Framework::ColourClashSprite const * tp = tt->frames[inRoomFrame % tt->frames.get_size()].get())
					{
						tp->draw_graphics_at(v3d, (VectorInt2(x, y) * info->tileSize + info->renderRoomOffset));
					}
				}
			}
			);
		for_every(door, renderedRoom->doors)
		{
			if (Tile const * tt = door->tile.get())
			{
				if (::Framework::ColourClashSprite const * tp = tt->frames[inRoomFrame % tt->frames.get_size()].get())
				{
					if (door->dir == DoorDir::Right)
					{
						int x = door->rect.x.max;
						for_range(int, y, door->rect.y.min, door->rect.y.max)
						{
							tp->draw_graphics_at(v3d, (VectorInt2(x, y) * info->tileSize + info->renderRoomOffset));
						}
					}
					else if (door->dir == DoorDir::Left)
					{
						int x = door->rect.x.min;
						for_range(int, y, door->rect.y.min, door->rect.y.max)
						{
							tp->draw_graphics_at(v3d, (VectorInt2(x, y) * info->tileSize + info->renderRoomOffset));
						}
					}
					if (door->dir == DoorDir::Up)
					{
						int y = door->rect.y.max;
						for_range(int, x, door->rect.x.min, door->rect.x.max)
						{
							tp->draw_graphics_at(v3d, (VectorInt2(x, y) * info->tileSize + info->renderRoomOffset));
						}
					}
					else if (door->dir == DoorDir::Down)
					{
						int y = door->rect.y.min;
						for_range(int, x, door->rect.x.min, door->rect.x.max)
						{
							tp->draw_graphics_at(v3d, (VectorInt2(x, y) * info->tileSize + info->renderRoomOffset));
						}
					}
				}
			}
		}
		mainGraphicsRT->unbind();

		mainColourGridRT->bind();
		do_for_rendered_room(renderedRoom,
			[this, v3d](int x, int y, Tile const * tt)
			{
				if (!tt->isItem && tt->frames.get_size() > 1)
				{
					if (::Framework::ColourClashSprite const * tp = tt->frames[inRoomFrame % tt->frames.get_size()].get())
					{
						tp->draw_colour_grid_at(v3d, (VectorInt2(x, y) * info->tileSize + info->renderRoomOffset), tt->foregroundColourOverride, tt->backgroundColourOverride, tt->brightColourOverride);
					}
				}
			}
		);
		for_every(door, renderedRoom->doors)
		{
			if (Tile const * tt = door->tile.get())
			{
				Colour doorTileColours[] = { Colour::white, Colour::white, Colour::white };
				Colour doorTileBrightColours[] = { Colour::white, Colour::black, Colour::white };
				if (::Framework::ColourClashSprite const * tp = tt->frames[inRoomFrame % tt->frames.get_size()].get())
				{
					if (door->dir == DoorDir::Right)
					{
						int x = door->rect.x.max;
						for_range(int, y, door->rect.y.min, door->rect.y.max)
						{
							tp->draw_colour_grid_at(v3d, (VectorInt2(x, y) * info->tileSize + info->renderRoomOffset), Optional<Colour>(doorTileColours[inRoomFrame % 3]), NP, Optional<Colour>(doorTileBrightColours[inRoomFrame % 3]));
						}
					}
					else if (door->dir == DoorDir::Left)
					{
						int x = door->rect.x.min;
						for_range(int, y, door->rect.y.min, door->rect.y.max)
						{
							tp->draw_colour_grid_at(v3d, (VectorInt2(x, y) * info->tileSize + info->renderRoomOffset), Optional<Colour>(doorTileColours[inRoomFrame % 3]), NP, Optional<Colour>(doorTileBrightColours[inRoomFrame % 3]));
						}
					}
					if (door->dir == DoorDir::Up)
					{
						int y = door->rect.y.max;
						for_range(int, x, door->rect.x.min, door->rect.x.max)
						{
							tp->draw_colour_grid_at(v3d, (VectorInt2(x, y) * info->tileSize + info->renderRoomOffset), Optional<Colour>(doorTileColours[inRoomFrame % 3]), NP, Optional<Colour>(doorTileBrightColours[inRoomFrame % 3]));
						}
					}
					else if (door->dir == DoorDir::Down)
					{
						int y = door->rect.y.min;
						for_range(int, x, door->rect.x.min, door->rect.x.max)
						{
							tp->draw_colour_grid_at(v3d, (VectorInt2(x, y) * info->tileSize + info->renderRoomOffset), Optional<Colour>(doorTileColours[inRoomFrame % 3]), NP, Optional<Colour>(doorTileBrightColours[inRoomFrame % 3]));
						}
					}
				}
			}
		}
		mainColourGridRT->unbind();
	}

	// render items
	{
		mainGraphicsRT->bind();
		for_every(itemAt, renderedRoom->itemsAt)
		{
			int itemIdx = for_everys_index(itemAt);
			if (itemsTakenInThisRoom.is_index_valid(itemIdx) && !itemsTakenInThisRoom[itemIdx])
			{
				if (Tile const * tt = renderedRoom->get_tile_at(*itemAt))
				{
					if (::Framework::ColourClashSprite const * tp = tt->frames[inRoomFrame % tt->frames.get_size()].get())
					{
						tp->draw_graphics_at(v3d, (*itemAt * info->tileSize + info->renderRoomOffset));
					}
				}
			}
		}
		mainGraphicsRT->unbind();

		mainColourGridRT->bind();
		for_every(itemAt, renderedRoom->itemsAt)
		{
			int itemIdx = for_everys_index(itemAt);
			if (itemsTakenInThisRoom.is_index_valid(itemIdx) && !itemsTakenInThisRoom[itemIdx])
			{
				if (Tile const * tt = renderedRoom->get_tile_at(*itemAt))
				{
					if (::Framework::ColourClashSprite const * tp = tt->frames[inRoomFrame % tt->frames.get_size()].get())
					{
						Colour itemColours[] = { Colour::magenta, Colour::green, Colour::yellow, Colour::cyan };
						tp->draw_colour_grid_at(v3d, (*itemAt * info->tileSize + info->renderRoomOffset), Optional<Colour>(itemColours[inRoomFrame % 4]));
					}
				}
			}
		}
		mainColourGridRT->unbind();
	}

	// render guardians
	for_every(guardian, guardians)
	{
		if (guardian->character)
		{
			if (::Framework::ColourClashSprite const * sprite = guardian->character->get_sprite_at(guardian->at, guardian->dir, guardian->frame))
			{
				mainGraphicsRT->bind();
				sprite->draw_graphics_at(v3d, (guardian->at + info->renderRoomOffset));
				mainGraphicsRT->unbind();
				mainColourGridRT->bind();
				sprite->draw_colour_grid_at(v3d, (guardian->at + info->renderRoomOffset), guardian->character->foregroundColourOverride, guardian->character->backgroundColourOverride, guardian->character->brightColourOverride);
				mainColourGridRT->unbind();
			}
		}
	}

	// render player
	if (! atTitleScreen &&
		! atGameOverScreen &&
		player.character)
	{
		if (::Framework::ColourClashSprite const * sprite = player.character->get_sprite_at(player.at, player.dir, player.frame))
		{
			mainGraphicsRT->bind();
			sprite->draw_graphics_at(v3d, (player.at + info->renderRoomOffset));
			mainGraphicsRT->unbind();
			mainColourGridRT->bind();
			sprite->draw_colour_grid_at(v3d, (player.at + info->renderRoomOffset));
			mainColourGridRT->unbind();
		}
	}

	// foreground
	{
		mainGraphicsRT->bind();
		do_for_rendered_room(renderedRoom,
			[this, v3d](int x, int y, Tile const * tt)
			{
				if (!tt->isItem && tt->foreground)
				{
					if (::Framework::ColourClashSprite const * tp = tt->frames[inRoomFrame % tt->frames.get_size()].get())
					{
						tp->draw_graphics_at(v3d, (VectorInt2(x, y) * info->tileSize + info->renderRoomOffset));
					}
				}
			}
		);
		mainGraphicsRT->unbind();

		mainColourGridRT->bind();
		do_for_rendered_room(renderedRoom,
			[this, v3d](int x, int y, Tile const * tt)
			{
				if (!tt->isItem && tt->foreground)
				{
					if (::Framework::ColourClashSprite const * tp = tt->frames[inRoomFrame % tt->frames.get_size()].get())
					{
						tp->draw_colour_grid_at(v3d, (VectorInt2(x, y) * info->tileSize + info->renderRoomOffset), tt->foregroundColourOverride, tt->backgroundColourOverride, tt->brightColourOverride);
					}
				}
			}
		);
		mainColourGridRT->unbind();
	}

	if (atTitleScreen || atGameOverScreen)
	{
		borderColourGridRT->bind();
		::Framework::ColourClashPipeline::draw_rect_on_colour_grid(VectorInt2::zero, useBorderSize, Colour::black, Colour::black, Colour::black);
		borderColourGridRT->unbind();
	}

	if (atGameOverScreen)
	{
		int fallingAlt = info->screenResolution.y - info->renderRoomOffset.y;
		int fallingSpeed = info->tileSize.y / 2;
		int fallsFrames = fallingAlt / fallingSpeed + 3;

		mainGraphicsRT->bind();
		if (gameOverFrame < fallsFrames)
		{
			info->gameOverGeorgeFallingSprite.get()->draw_graphics_at(v3d, VectorInt2(info->screenResolution.x / 2 - 2, info->screenResolution.y - gameOverFrame * fallingSpeed) + info->renderRoomOffset * VectorInt2(1, 0));
		}
		else
		{
			(((gameOverFrame / 6) % 2) == 0 ? info->gameOverGeorgeInACup0Sprite : info->gameOverGeorgeInACup1Sprite).get()->draw_graphics_at(v3d, VectorInt2(info->screenResolution.x / 2, 10) + info->renderRoomOffset);
		}
		info->gameOverCupOfTeaSprite.get()->draw_graphics_at(v3d, VectorInt2(info->screenResolution.x / 2, 0) + info->renderRoomOffset);
		mainGraphicsRT->unbind();

		mainColourGridRT->bind();
		if (gameOverFrame < fallsFrames)
		{
			info->gameOverGeorgeFallingSprite.get()->draw_colour_grid_at(v3d, VectorInt2(info->screenResolution.x / 2 - 2, info->screenResolution.y - gameOverFrame * fallingSpeed) + info->renderRoomOffset * VectorInt2(1, 0));
			::Framework::ColourClashPipeline::draw_rect_on_colour_grid(
				VectorInt2(info->screenResolution.x / 2 - info->tileSize.x, info->renderRoomOffset.y - info->tileSize.y),
				VectorInt2(info->screenResolution.x / 2 + info->tileSize.x - 1, info->renderRoomOffset.y - 1), Colour::black , Colour::black, Colour::black);
		}
		else
		{
			(((gameOverFrame / 6) % 2) == 0 ? info->gameOverGeorgeInACup0Sprite : info->gameOverGeorgeInACup1Sprite).get()->draw_colour_grid_at(v3d, VectorInt2(info->screenResolution.x / 2, 10) + info->renderRoomOffset);
		}
		info->gameOverCupOfTeaSprite.get()->draw_colour_grid_at(v3d, VectorInt2(info->screenResolution.x / 2, 0) + info->renderRoomOffset, Optional<Colour>(Colour::white));
		mainColourGridRT->unbind();
	}

	if (!atTitleScreen &&
		!atGameOverScreen &&
		deathFrame != NONE)
	{
		Colour deathColours[] = { Colour::white, Colour::cyan, Colour::green, Colour::magenta, Colour::blue, Colour::black };
		borderColourGridRT->bind();
		::Framework::ColourClashPipeline::draw_rect_on_colour_grid(VectorInt2::zero, useBorderSize, deathColours[min(7, deathFrame)], deathColours[min(7, deathFrame)], Colour::black);
		borderColourGridRT->unbind();
		mainColourGridRT->bind();
		::Framework::ColourClashPipeline::draw_rect_on_colour_grid(info->renderRoomOffset, info->screenResolution, deathColours[min(5, deathFrame)], Colour::black, Colour::black);
		mainColourGridRT->unbind();
	}

	// render other info
	if (!atTitleScreen &&
		!atGameOverScreen &&
		player.character)
	{
		if (::Framework::ColourClashSprite const * sprite = player.character->get_sprite_at(VectorInt2(info->tileSize.x + livesFrame * player.character->step, 0), VectorInt2(1, 0), livesFrame))
		{
			Colour livesColour[] = { Colour::cyan, Colour::yellow, Colour::green, Colour::blue, Colour::cyan,
									 Colour::magenta, Colour::green, Colour::yellow, Colour::cyan, Colour::red,
									 Colour::blue, Colour::magenta, Colour::cyan, Colour::green, Colour::yellow,
									 Colour::red, Colour::blue };
			Colour livesBright[] = { Colour::white, Colour::black, Colour::black, Colour::black, Colour::black,
									 Colour::white, Colour::white, Colour::white, Colour::black, Colour::black,
									 Colour::white, Colour::white, Colour::black };
			mainGraphicsRT->bind();
			for (int i = 0; i < livesLeftCount; ++i)
			{
				sprite->draw_graphics_at(v3d, VectorInt2(info->tileSize.x / 2 + i * info->tileSize.x * 2 + livesFrame * player.character->step, info->tileSize.y));
			}
			mainGraphicsRT->unbind();
			mainColourGridRT->bind();
			for (int i = 0; i < livesLeftCount; ++i)
			{
				sprite->draw_colour_grid_at(v3d, VectorInt2(info->tileSize.x / 2+ i * info->tileSize.x * 2 + livesFrame * player.character->step, info->tileSize.y),
					Optional<Colour>(livesColour[i % 17]), NP, Optional<Colour>(livesBright[i % 13]));
			}
			mainColourGridRT->unbind();
		}
	}

	if (!atTitleScreen &&
		!atGameOverScreen &&
		renderItemsAndTime)
	{
		render_items_and_time();
		renderItemsAndTime = false;
	}

	if (atTitleScreen || atGameOverScreen)
	{
		Colour startGameColours[] = { Colour::white, Colour::yellow, Colour::cyan, Colour::magenta, Colour::green, Colour::cyan, Colour::yellow };
		backgroundColourGridRT->bind();
		::Framework::ColourClashPipeline::draw_rect_on_colour_grid(VectorInt2::zero, VectorInt2(info->screenResolution.x, info->renderRoomOffset.y - info->tileSize.y * 3), startGameColours[inRoomFrame % 7], Colour::black, Colour::black);
		backgroundColourGridRT->unbind();
	}

	if (timeMultiplier != 1.0f)
	{
		if (::Framework::Font* font = info->font.get())
		{
			tchar text[32];
			if (timeMultiplier > 1.0f)
			{
				tsprintf(text, 32, TXT("x %.0f"), timeMultiplier);
			}
			else
			{
				tsprintf(text, 32, TXT("/ %.0f"), 1.0f / timeMultiplier);
			}
			mainGraphicsRT->bind();
			font->draw_text_at(v3d, text, Colour::white, Vector2::zero);
			mainGraphicsRT->unbind();
		}
	}

	borderGraphicsRT->resolve();
	borderColourGridRT->resolve();
	mainGraphicsRT->resolve();
	mainColourGridRT->resolve();

	// final render
	_outputRT->bind();
	// render border
	::Framework::ColourClashPipeline::combine(VectorInt2::zero, useBorderSize, borderGraphicsRT.get(), borderColourGridRT.get());
	// combine colour clash
	::Framework::ColourClashPipeline::combine(screenOffset, info->screenResolution, mainGraphicsRT.get(), mainColourGridRT.get());
	//mainGraphicsRT->render(0, v3d, Vector2::zero, mainGraphicsRT->get_size().to_vector2());
	//mainColourGridRT->render(0, v3d, Vector2::zero, mainColourGridRT->get_size().to_vector2());
	//mainColourGridRT->render(1, v3d, Vector2(40.0f, 0.0f), mainColourGridRT->get_size().to_vector2());
	_outputRT->unbind();
}

void Game::advance_frame()
{
	++subSecondFrame;
	while (subSecondFrame >= info->fps)
	{
		subSecondFrame -= info->fps;
		++timeSeconds;
		if (deathFrame == NONE)
		{
			renderItemsAndTime = true;
		}
		while (timeSeconds >= 60)
		{
			timeSeconds -= 60;
			++timeMinutes;
			while (timeMinutes >= 60)
			{
				timeMinutes -= 60;
				++timeHours;
				while (timeHours >= 24)
				{
					timeHours -= 24;
				}
			}
		}
	}

	if (atTitleScreen)
	{
		if (jumpButtonPressed)
		{
			start_game();
			supressJumpButton = true;
		}
	}

	if (deathFrame != NONE)
	{
		++deathFrame;
		if (deathFrame >= 8)
		{
			deathFrame = NONE;
			forceRerender = true;
			gameFrameCheck = 0;
			--livesLeftCount;
			if (livesLeftCount == NONE)
			{
				game_over();
			}
			else
			{
				// restore checkpoint
				player = checkpoint;
				playerJumpFrame = checkpointJumpFrame;
				playerFalling = checkpointFalling;
				playerFallsFor = checkpointFallsFor;
				
				spawn_guardians();
			}
		}
		else
		{
			supressJumpButton = false;
			jumpButtonPressed = false;
			movementStick = Vector2::zero;
			return;
		}
	}

	if (atGameOverScreen)
	{
		++gameOverFrame;
		if (gameOverFrame > 120)
		{
			title_screen();
			supressJumpButton = true;
		}
	}

	--gameFrameCheck;
	if (gameFrameCheck > 0)
	{
		return;
	}
	gameFrameCheck = info->gameFrameEvery;

	if (!godMode)
	{
		livesAnimateCounter++;
		if (livesAnimateCounter >= 4)
		{
			livesAnimateCounter = 0;
			livesFrame = (livesFrame + 1) % 4;
		}
	}

	// player
	// player movement is a hack-fest - it was written quickly to work, not to make sense :(
	if (!atTitleScreen &&
		!atGameOverScreen &&
		player.character && inRoom)
	{
		VectorInt2 input = VectorInt2::zero;
		VectorInt2 requestedInput = VectorInt2::zero;
		bool wasJumping = playerJumpFrame != NONE;
		bool wantsToJump = false;
		if (!useProvidedInput)
		{
			if (::System::Input* sysInput = ::System::Input::get())
			{
				if (sysInput->is_key_pressed(::System::Key::K))
				{
					start_death();
				}
			}
		}

		{
			if (movementStick.x <= -0.5f)
			{
				input.x -= 1;
			}
			if (movementStick.x >= 0.5f)
			{
				input.x += 1;
			}
			if (jumpButtonPressed && !supressJumpButton)
			{
				wantsToJump = true;
			}
			requestedInput = input;
		}

		VectorInt2 nextLoc = player.at;
		VectorInt2 nextPlayerDir = player.dir;
		if (playerJumpFrame == NONE && !playerFalling)
		{
			// this order will first turn character and then jump in that direction
			if (wantsToJump && !playerJumpFailedImmediately)
			{
				playerJumpFrame = 0;
				int jumpDir = input.x;
				if (playerAutoMovement)
				{
					jumpDir = playerAutoMovement;
				}
				{
					RangeInt2 belowFeetRect = RangeInt2::empty;
					belowFeetRect.x.min = player.at.x + player.character->movementCollisionOffset.x + player.character->movementCollisionBox.x.min;
					belowFeetRect.x.max = player.at.x + player.character->movementCollisionOffset.x + player.character->movementCollisionBox.x.max;
					belowFeetRect.y.min = player.at.y - 1;
					belowFeetRect.y.max = player.at.y - 1;
					int startsJumpOnAutoMovementX = inRoom->get_auto_movement_dir(info, belowFeetRect);
					if (playerAgainstAutoMovement)
					{
						startsJumpOnAutoMovementX = 0;
					}
					if (startsJumpOnAutoMovementX > 0)
					{
						if (player.dir.x > 0)
						{
							jumpDir = 1;
						}
						else if (jumpDir < 0)
						{
							jumpDir = 0;
						}
					}
					if (startsJumpOnAutoMovementX < 0)
					{
						if (player.dir.x < 0)
						{
							jumpDir = -1;
						}
						else if (jumpDir > 0)
						{
							jumpDir = 0;
						}
					}
				}
				if (jumpDir && player.dir.x == jumpDir)
				{
					player.currentMovement.x = jumpDir;
					// commented out to allow jumping sliding on wall
					/*
					RangeInt2 currRect = RangeInt2::empty;
					currRect.x.min = player.at.x + player.character->movementCollisionOffset.x + player.character->movementCollisionBox.x.min;
					currRect.x.max = player.at.x + player.character->movementCollisionOffset.x + player.character->movementCollisionBox.x.max;
					currRect.y.min = player.at.y + player.character->movementCollisionOffset.y + player.character->movementCollisionBox.y.min;
					currRect.y.max = player.at.y + player.character->movementCollisionOffset.y + player.character->movementCollisionBox.y.max;
					RangeInt2 nextRect = RangeInt2::empty;
					nextRect.x.min = player.at.x + player.character->movementCollisionOffset.x + player.character->movementCollisionBox.x.min;
					nextRect.x.max = player.at.x + player.character->movementCollisionOffset.x + player.character->movementCollisionBox.x.max;
					nextRect.y.min = player.at.y + player.character->movementCollisionOffset.y + player.character->movementCollisionBox.y.min;
					nextRect.y.max = player.at.y + player.character->movementCollisionOffset.y + player.character->movementCollisionBox.y.max;
					nextRect.x.min += player.currentMovement.x * player.character->step;
					nextRect.x.max += player.currentMovement.x * player.character->step;
					if (!inRoom->is_location_valid(info, nextRect, / *ignore* /currRect))
					{
						// there is wall where we jump, jump in place
						player.currentMovement.x = 0;
					}
					*/
				}
				else
				{
					player.currentMovement.x = 0;
				}
			}
			playerJumpFailedImmediately = false;
			if (input.x)
			{
				nextPlayerDir.x = input.x;
			}
		}
		int nextPlayerFrame = player.frame + 1;
		if ((input.is_zero() || playerAutoMovement || playerAgainstAutoMovement || playerAgainstAutoMovementStill) && !playerFalling && playerJumpFrame == NONE)
		{
			// check for auto movement
			RangeInt2 belowFeetRect = RangeInt2::empty;
			belowFeetRect.x.min = player.at.x + player.character->movementCollisionOffset.x + player.character->movementCollisionBox.x.min;
			belowFeetRect.x.max = player.at.x + player.character->movementCollisionOffset.x + player.character->movementCollisionBox.x.max;
			belowFeetRect.y.min = player.at.y - 1;
			belowFeetRect.y.max = player.at.y - 1;
			playerAutoMovement = inRoom->get_auto_movement_dir(info, belowFeetRect);
			if (!playerAutoMovement || input.is_zero())
			{
				playerAgainstAutoMovement = false;
				playerAgainstAutoMovementStill = false;
			}
			if ((!playerAgainstAutoMovement || playerAgainstAutoMovementStill) &&
				playerAutoMovement &&
				player.currentMovement.x == 0 &&
				requestedInput.x == -playerAutoMovement)
			{
				playerAutoMovement = 0;
				input.x = 0;
			}
			if (playerAgainstAutoMovement || playerAgainstAutoMovementStill)
			{
				playerAutoMovement = 0;
			}
		}
		// override_ input with auto movement (if not zero)
		if (playerAutoMovement)
		{
			input = VectorInt2(playerAutoMovement, 0);
			player.dir = VectorInt2(playerAutoMovement, 0);
			nextPlayerDir = VectorInt2(playerAutoMovement, 0);
		}
		if (playerJumpFrame != NONE)
		{
			input = player.currentMovement;
		}
		if (playerFalling || playerAgainstAutoMovementStill)
		{
			input = VectorInt2::zero;
		}
		if (player.dir == nextPlayerDir)
		{
			nextLoc += input * player.character->step;
		}
		
		if (playerJumpFrame != NONE)
		{
			nextLoc.y += player.character->jumpPattern[playerJumpFrame];
			playerAgainstAutoMovement = false;
			playerAgainstAutoMovementStill = false;
		}

		bool standsNow = false;
		bool playerWasFalling = playerFalling;
		if ((playerJumpFrame == NONE || !wasJumping) && !playerFalling)
		{
			RangeInt2 belowFeetRect = RangeInt2::empty;
			belowFeetRect.x.min = player.at.x + player.character->movementCollisionOffset.x + player.character->movementCollisionBox.x.min;
			belowFeetRect.x.max = player.at.x + player.character->movementCollisionOffset.x + player.character->movementCollisionBox.x.max;
			belowFeetRect.y.min = player.at.y - 1;
			belowFeetRect.y.max = player.at.y - 1;
			if (!inRoom->has_something_under_feet(info, belowFeetRect, player.at, player.character->movementCollisionOffset, player.character->movementCollisionBox.x, player.character->step))
			{
				playerJumpFrame = NONE;
				playerFalling = true;
				nextPlayerDir = player.dir;
			}
			else
			{
				standsNow = true;
			}
		}

		if (playerFalling)
		{
			nextLoc = player.at;
			nextLoc.y -= 4;
			playerAgainstAutoMovement = false;
			player.currentMovement.x = 0;
		}

		if (nextLoc.y < player.at.y)// && (player.at.y % info->tileSize.y) == 0)
		{
			RangeInt2 fallingFeetRect = RangeInt2::empty;
			fallingFeetRect.x.min = player.at.x + player.character->movementCollisionOffset.x + player.character->movementCollisionBox.x.min;
			fallingFeetRect.x.max = player.at.x + player.character->movementCollisionOffset.x + player.character->movementCollisionBox.x.max;
			fallingFeetRect.y.min = player.at.y - (player.at.y - nextLoc.y) - 1;
			fallingFeetRect.y.max = player.at.y;
			int fallAtY = nextLoc.y;
			bool ontoStairs = false;
			if (inRoom->can_fall_onto(info, fallingFeetRect, player.currentMovement.x, OUT_ fallAtY, VectorInt2(nextLoc.x, player.at.y), player.character->movementCollisionOffset, player.character->movementCollisionBox.x, player.character->step, OUT_ ontoStairs))
			{
				if (info->deadlyHeightFall > 0 &&
					playerFallsFor >= info->tileSize.y * info->deadlyHeightFall)
				{
					start_death();
				}
				playerJumpFrame = NONE;
				playerFalling = false;
				/*
				i don't know what this was for but it doesn't help anything
				if (! ontoStairs && requestedInput.x != player.currentMovement.x)
				{
					// stop him immediately
					nextLoc = player.at;
				}
				*/
				nextLoc.y = fallAtY;
				{	// check if we landed onto automatic lane
					RangeInt2 belowFeetRect = RangeInt2::empty;
					belowFeetRect.x.min = nextLoc.x + player.character->movementCollisionOffset.x + player.character->movementCollisionBox.x.min;
					belowFeetRect.x.max = nextLoc.x + player.character->movementCollisionOffset.x + player.character->movementCollisionBox.x.max;
					belowFeetRect.y.min = nextLoc.y - 1;
					belowFeetRect.y.max = nextLoc.y - 1;
					playerAutoMovement = inRoom->get_auto_movement_dir(info, belowFeetRect);
					if (playerAutoMovement)
					{
						if (requestedInput.x == -playerAutoMovement && (playerAutoMovement != player.currentMovement.x || wantsToJump))
						{
							// let continue in given movement
							playerAgainstAutoMovement = true;
							if (player.currentMovement.x == 0)
							{
								// or stand still
								playerAgainstAutoMovementStill = true;
								nextLoc.x = player.at.x;
								player.currentMovement.x = 0;
							}
						}
						else if (playerAutoMovement == player.currentMovement.x)// || !wantsToJump)
						{
							// continue in that dir
							nextLoc.x = player.at.x + playerAutoMovement * player.character->step;
							nextPlayerDir = VectorInt2(playerAutoMovement, 0);
						}
						else
						{
							// otherwise stop and we will be moving in next frame
							nextLoc.x = player.at.x;
						}
					}
				}
			}
			else
			{
				playerFallsFor += player.at.y - nextLoc.y;
			}
		}
		else
		{
			playerFallsFor = 0;
		}

		if (standsNow && nextLoc.y == player.at.y && nextLoc.x != player.at.x)
		{
			int stairsOffset = inRoom->get_offset_on_stairs(info, player.at, player.character->movementCollisionOffset, player.character->movementCollisionBox.x, nextPlayerDir.x, player.character->step);
			if (stairsOffset < 0)
			{
				// check if we should land on something
				RangeInt2 steppingDownFeetRect = RangeInt2::empty;
				steppingDownFeetRect.x.min = player.at.x + player.character->movementCollisionOffset.x + player.character->movementCollisionBox.x.min;
				steppingDownFeetRect.x.max = player.at.x + player.character->movementCollisionOffset.x + player.character->movementCollisionBox.x.max;
				steppingDownFeetRect.y.min = player.at.y + stairsOffset;
				steppingDownFeetRect.y.max = player.at.y;
				if (inRoom->can_step_onto_except_stairs(info, steppingDownFeetRect))
				{
					stairsOffset = 0;
				}
			}
			nextLoc.y += stairsOffset;
		}

		bool nextLocIsValid = true;
		if (inRoom && player.at != nextLoc)
		{
			RangeInt2 currRect = RangeInt2::empty;
			currRect.x.min = player.at.x + player.character->movementCollisionOffset.x + player.character->movementCollisionBox.x.min;
			currRect.x.max = player.at.x + player.character->movementCollisionOffset.x + player.character->movementCollisionBox.x.max;
			currRect.y.min = player.at.y + player.character->movementCollisionOffset.y + player.character->movementCollisionBox.y.min;
			currRect.y.max = player.at.y + player.character->movementCollisionOffset.y + player.character->movementCollisionBox.y.max;
			RangeInt2 nextRect = RangeInt2::empty;
			nextRect.x.min = nextLoc.x + player.character->movementCollisionOffset.x + player.character->movementCollisionBox.x.min;
			nextRect.x.max = nextLoc.x + player.character->movementCollisionOffset.x + player.character->movementCollisionBox.x.max;
			nextRect.y.min = nextLoc.y + player.character->movementCollisionOffset.y + player.character->movementCollisionBox.y.min;
			nextRect.y.max = nextLoc.y + player.character->movementCollisionOffset.y + player.character->movementCollisionBox.y.max;
			nextLocIsValid = inRoom->is_location_valid(info, nextRect, /*ignore*/currRect);
		}

		if (nextLocIsValid)
		{
			player.at = nextLoc;
		}
		else
		{
			if (playerJumpFrame == 0)
			{
				playerJumpFailedImmediately = true;
			}
			if (playerJumpFrame >= 0)
			{
				// allow to continue jump if we can slide up/down
				nextLoc.x = player.at.x;
				RangeInt2 currRect = RangeInt2::empty;
				currRect.x.min = player.at.x + player.character->movementCollisionOffset.x + player.character->movementCollisionBox.x.min;
				currRect.x.max = player.at.x + player.character->movementCollisionOffset.x + player.character->movementCollisionBox.x.max;
				currRect.y.min = player.at.y + player.character->movementCollisionOffset.y + player.character->movementCollisionBox.y.min;
				currRect.y.max = player.at.y + player.character->movementCollisionOffset.y + player.character->movementCollisionBox.y.max;
				RangeInt2 nextRect = RangeInt2::empty;
				nextRect.x.min = nextLoc.x + player.character->movementCollisionOffset.x + player.character->movementCollisionBox.x.min;
				nextRect.x.max = nextLoc.x + player.character->movementCollisionOffset.x + player.character->movementCollisionBox.x.max;
				nextRect.y.min = nextLoc.y + player.character->movementCollisionOffset.y + player.character->movementCollisionBox.y.min;
				nextRect.y.max = nextLoc.y + player.character->movementCollisionOffset.y + player.character->movementCollisionBox.y.max;
				if (inRoom->is_location_valid(info, nextRect, /*ignore*/currRect))
				{
					player.at = nextLoc;
				}
				else
				{
					playerJumpFrame = NONE;
				}
			}
		}

		if (playerJumpFrame != NONE)
		{
			++playerJumpFrame;
			if (playerJumpFrame >= player.character->jumpPattern.get_size())
			{
				playerJumpFrame = NONE;
			}
		}

		player.dir = nextPlayerDir;
		player.frame = nextPlayerFrame;
		player.currentMovement = input;
	}

	// guardians
	for_every(guardian, guardians)
	{
		if (!guardian->character)
		{
			continue;
		}
		int nextGuardianFrame = guardian->frame + 1;
		VectorInt2 nextLoc = guardian->at + guardian->currentMovement * guardian->character->step;
		// check if we should change dir
		if (::Framework::ColourClashSprite const * sprite = guardian->character->get_sprite_at(nextLoc, guardian->dir, nextGuardianFrame))
		{
			RangeInt2 rect = sprite->calc_rect();
			rect.x.min += nextLoc.x;
			rect.x.max += nextLoc.x;
			rect.y.min += nextLoc.y;
			rect.y.max += nextLoc.y;
			if (guardian->currentMovement.x > 0 && rect.x.max >= (guardian->guardian->rect.x.max + 1) * info->tileSize.x)
			{
				guardian->currentMovement.x = -1;
			}
			else if (guardian->currentMovement.x < 0 && rect.x.min < guardian->guardian->rect.x.min * info->tileSize.x)
			{
				guardian->currentMovement.x = 1;
			}
			if (guardian->currentMovement.y > 0 && rect.y.max >= (guardian->guardian->rect.y.max + 1) * info->tileSize.y)
			{
				guardian->currentMovement.y = -1;
			}
			else if (guardian->currentMovement.y < 0 && rect.y.min < guardian->guardian->rect.y.min * info->tileSize.y)
			{
				guardian->currentMovement.y = 1;
			}
		}
		nextLoc = guardian->at + guardian->currentMovement * guardian->character->step;
		guardian->at = nextLoc;
		guardian->frame = nextGuardianFrame;
		guardian->dir = guardian->currentMovement;
	}

	// in room frame
	++inRoomFrame;

	// check if we hit something and die, or maybe we have collected something
	if (!atTitleScreen &&
		!atGameOverScreen &&
		player.character)
	{
		if (::Framework::ColourClashSprite const * playerSprite = player.character->get_sprite_at(player.at, player.dir, player.frame))
		{
			RangeInt2 playerRect = playerSprite->calc_rect();
			playerRect.x.min += player.at.x;
			playerRect.x.max += player.at.x;
			playerRect.y.min += player.at.y;
			playerRect.y.max += player.at.y;

			for_every(guardian, guardians)
			{
				if (!guardian->character || guardian->character->harmless)
				{
					continue;
				}
				if (::Framework::ColourClashSprite const * sprite = guardian->character->get_sprite_at(guardian->at, guardian->dir, guardian->frame))
				{
					RangeInt2 rect = sprite->calc_rect();
					rect.x.min += guardian->at.x;
					rect.x.max += guardian->at.x;
					rect.y.min += guardian->at.y;
					rect.y.max += guardian->at.y;

					if (rect.intersects(playerRect))
					{
						if (start_death())
						{
							break;
						}
					}
				}
			}

			if (inRoom->does_kill(info, playerRect))
			{
				start_death();
			}

			ARRAY_STACK(int, collectedItemsIdx, inRoom->itemsAt.get_size());
			inRoom->get_items_at(info, playerRect, OUT_ collectedItemsIdx);
			for_every(collectedItemIdx, collectedItemsIdx)
			{
				if (itemsTakenInThisRoom.is_index_valid(*collectedItemIdx))
				{
					if (!itemsTakenInThisRoom[*collectedItemIdx])
					{
						itemsTakenInThisRoom[*collectedItemIdx] = true;
						++itemsTaken;
						renderItemsAndTime = true;
					}
				}
			}
		}
	}

	// maybe player leaves room?
	if (!atTitleScreen &&
		!atGameOverScreen &&
		player.character && inRoom)
	{
		for_every(door, inRoom->doors)
		{
			if (player.at.y >= door->rect.y.min * info->tileSize.y &&
				player.at.y <= door->rect.y.max * info->tileSize.y)
			{
				todo_note(TXT("tweak exiting room, should be far into last piece"));
				if (door->dir == DoorDir::Right &&
					player.at.x > (door->rect.x.max) * info->tileSize.x + player.character->step * 0 &&
					player.at.x <= (door->rect.x.max + 1) * info->tileSize.x - 1)
				{
					switch_to_room(door->connectsTo.get(), door->connectsToDoor, for_everys_index(door));
					break;
				}
				if (door->dir == DoorDir::Left &&
					player.at.x >= (door->rect.x.min) * info->tileSize.x &&
					player.at.x <= (door->rect.x.min + 1) * info->tileSize.x - 1 - player.character->step * 0)
				{
					switch_to_room(door->connectsTo.get(), door->connectsToDoor, for_everys_index(door));
					break;
				}
				if (door->dir == DoorDir::Up &&
					player.at.y >= (door->rect.y.max) * info->tileSize.y &&
					player.at.y <= (door->rect.y.max + 1) * info->tileSize.y - 1)
				{
					switch_to_room(door->connectsTo.get(), door->connectsToDoor, for_everys_index(door));
					break;
				}
				if (door->dir == DoorDir::Down &&
					player.at.y >= (door->rect.y.min) * info->tileSize.y &&
					player.at.y <= (door->rect.y.min + 1) * info->tileSize.y - 1)
				{
					switch_to_room(door->connectsTo.get(), door->connectsToDoor, for_everys_index(door));
					break;
				}
			}
		}
	}

	if (!jumpButtonPressed)
	{
		supressJumpButton = false;
	}
	jumpButtonPressed = false;
	movementStick = Vector2::zero;
}

void Game::switch_to_room(Room* _room, int _doorIndex, int _leftThroughDoorIndex)
{
	if (!_room || ! _room->doors.is_index_valid(_doorIndex))
	{
		return;
	}

	on_change_room(_room);

	VectorInt2 relLoc = VectorInt2::zero;
	DoorDir::Type leftThroughDir = DoorDir::None;
	if (inRoom && inRoom->doors.is_index_valid(_leftThroughDoorIndex))
	{
		Room::Door const & ld = inRoom->doors[_leftThroughDoorIndex];
		relLoc = player.at - VectorInt2(ld.rect.x.min, ld.rect.y.min) * info->tileSize;
		leftThroughDir = ld.dir;
	}

	bool resetRoom = inRoom != _room;
	inRoom = _room;
	player.frame = 0;
	Room::Door const & d = _room->doors[_doorIndex];
	if (d.dir != DoorDir::get_opposite(leftThroughDir))
	{
		if (d.dir == DoorDir::Right)
		{
			player.dir.x = -1;
		}
		if (d.dir == DoorDir::Left)
		{
			player.dir.x = 1;
		}
		player.currentMovement = VectorInt2::zero;
		playerJumpFrame = NONE;
	}
	if (d.dir == DoorDir::Right || d.dir == DoorDir::Left)
	{
		player.at.x = d.dir == DoorDir::Right ? min(d.rect.x.max, d.rect.x.min + 1) * info->tileSize.x - player.character->step : max(d.rect.x.min + 1, d.rect.x.max - 0) * info->tileSize.x + player.character->step;
		player.at.y = d.rect.y.min * info->tileSize.y + relLoc.y;
	}
	else
	{
		player.at.y = d.dir == DoorDir::Up ? min(d.rect.y.max, d.rect.y.min + 1) * info->tileSize.y - player.character->step : max(d.rect.y.min + 1, d.rect.y.max - 0) * info->tileSize.y;
		player.at.x = d.rect.x.min * info->tileSize.x + relLoc.x;
	}
	playerAutoMovement = 0;

	do_checkpoint();

	if (resetRoom)
	{
		spawn_guardians();
	}
}

void Game::do_checkpoint()
{
	checkpoint = player;
	checkpointJumpFrame = playerJumpFrame;
	checkpointFalling = playerFalling;
	checkpointFallsFor = playerFallsFor;
}

void Game::spawn_guardians()
{
	// reset creatures movement
	guardians.set_size(inRoom->guardians.get_size());
	for_every(guardian, guardians)
	{
		Room::Guardian const & srcGuardian = inRoom->guardians[for_everys_index(guardian)];
		guardian->guardian = &srcGuardian;
		guardian->character = srcGuardian.character.get();
		guardian->at = srcGuardian.startingLoc * info->tileSize;
		guardian->currentMovement = srcGuardian.startingDir;
		guardian->dir = guardian->currentMovement;

		// auto centre
		if (guardian->dir.x == 0)
		{
			guardian->at.x = (guardian->guardian->rect.x.min * info->tileSize.x + (guardian->guardian->rect.x.max + 1) * info->tileSize.x) / 2;
		}
		if (guardian->dir.y == 0)
		{
			guardian->at.y = guardian->guardian->rect.y.min * info->tileSize.y;
		}

		guardian->at.x = clamp(guardian->at.x, guardian->guardian->rect.x.min * info->tileSize.x, (guardian->guardian->rect.x.max + 1) * info->tileSize.x - 1);
		guardian->at.y = clamp(guardian->at.y, guardian->guardian->rect.y.min * info->tileSize.y, (guardian->guardian->rect.y.max + 1) * info->tileSize.y - 1);

		if (guardian->character)
		{
			int triesLeft = 100;
			while (triesLeft > 0)
			{
				bool allOk = true;
				if (::Framework::ColourClashSprite const * sprite = guardian->character->get_sprite_at(guardian->at, guardian->dir, guardian->frame))
				{
					RangeInt2 rect = sprite->calc_rect();
					if (guardian->guardian->rect.x.length() >= rect.x.length())
					{
						rect.x.min += guardian->at.x;
						rect.x.max += guardian->at.x;
						if (rect.x.min < guardian->guardian->rect.x.min * info->tileSize.x)
						{
							allOk = false;
							guardian->at.x += guardian->currentMovement.x != 0? guardian->character->step : 1;
						}
						if (rect.x.max >= (guardian->guardian->rect.x.max + 1) * info->tileSize.x)
						{
							allOk = false;
							guardian->at.x -= guardian->currentMovement.x != 0 ? guardian->character->step : 1;
						}
					}
					if (guardian->guardian->rect.y.length() >= rect.y.length())
					{
						rect.y.min += guardian->at.y;
						rect.y.max += guardian->at.y;
						if (rect.y.min < guardian->guardian->rect.y.min * info->tileSize.y)
						{
							allOk = false;
							guardian->at.y += guardian->currentMovement.y != 0 ? guardian->character->step : 1;
						}
						if (rect.y.max >= (guardian->guardian->rect.y.max + 1) * info->tileSize.y)
						{
							allOk = false;
							guardian->at.y -= guardian->currentMovement.y != 0 ? guardian->character->step : 1;
						}
					}
				}
				if (allOk)
				{
					break;
				}
				--triesLeft;
			}
		}
	}
}

void Game::on_change_room(Room* _nextRoom)
{
	if (inRoom)
	{
		find_visited_room(inRoom)->itemsTaken = itemsTakenInThisRoom;
	}
	if (_nextRoom)
	{
		itemsTakenInThisRoom = find_visited_room(_nextRoom)->itemsTaken;
	}
	else
	{
		itemsTakenInThisRoom.clear();
	}
}

Game::VisitedRoomInfo* Game::find_visited_room(Room* _room)
{
	VisitedRoomInfo* visitedRoom = nullptr;
	for_every(vr, visitedRooms)
	{
		if (vr->room == _room)
		{
			visitedRoom = vr;
			break;
		}
	}
	if (!visitedRoom)
	{
		visitedRooms.push_back(VisitedRoomInfo());
		visitedRoom = &(visitedRooms.get_last());
		visitedRoom->room = _room;
		while (visitedRoom->itemsTaken.get_size() < _room->itemsAt.get_size())
		{
			visitedRoom->itemsTaken.push_back(false);
		}
	}
	return visitedRoom;
}

bool Game::start_death()
{
	if (!godMode)
	{
		deathFrame = 0;
		return true;
	}
	else
	{
		return false;
	}
}

bool Game::does_allow_to_auto_exit()
{
	if (atTitleScreen && timesPlayed == 0)
	{
		return true;
	}
	asksForAutoExit = true;
	if (asksForAutoExitFor > 5.0f)
	{
		return true;
	}
	return false;
}

#endif
