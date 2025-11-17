#pragma once

#include "..\..\game\gameInput.h"

#include "..\..\..\core\math\math.h"
#include "..\..\..\core\system\video\renderTargetPtr.h"

#include "platformerRoom.h"

namespace System
{
	class RenderTarget;
};

#ifdef AN_MINIGAME_PLATFORMER
namespace Platformer
{
	class Info;
	class Room;
	class Character;

	class Game
	{
	public:
		Game(Info* _info, VectorInt2 const & _wholeDisplayResolution);
		~Game();

		bool does_allow_to_auto_exit();

		void advance(float _deltaTime, ::System::RenderTarget* _outputRT);

		void title_screen();

		void start_game();

		void game_over();

		void provide_input(::Framework::IGameInput* _providedInput) { providedInput = _providedInput; useProvidedInput = true; }
		void default_input() { useProvidedInput = false; }

	private:
		struct VisitedRoomInfo
		{
			Room const * room = nullptr;
			Array<bool> itemsTaken;
		};

		struct Object
		{
			Character* character = nullptr; // if nullptr, doesn't exist
			Room::Guardian const * guardian = nullptr;
			int frame = 0; // frame for noDir
			VectorInt2 at;
			VectorInt2 dir = VectorInt2(1, 0); // can't be zero
			VectorInt2 currentMovement = VectorInt2::zero;
		};
		
		float asksForAutoExitFor = 0.0f;
		bool asksForAutoExit = false;

		Info* info = nullptr;
		int gameFrameCheck = 0;
		int timeHours = 8;
		int timeMinutes = 0;
		int timeSeconds = 0;
		int subSecondFrame = 0;

		bool atTitleScreen = true;
		bool atGameOverScreen = false;
		int gameOverFrame = 0;
		int timesPlayed = 0;

		::Framework::GameInput playerInput;
		::Framework::IGameInput* providedInput = nullptr;
		bool useProvidedInput = false; // may use null input
		
		bool jumpButtonPressed = false;
		Vector2 movementStick = Vector2::zero;
		bool supressJumpButton = false;

		int livesLeftCount = NONE; // none - not in game
		bool godMode = false;
		float timeMultiplier = 1.0f;

		VectorInt2 screenOffset;
		VectorInt2 useBorderSize;

		::System::RenderTargetPtr borderGraphicsRT; // border uses colour clash only to get into proper colours
		::System::RenderTargetPtr borderColourGridRT;

		::System::RenderTargetPtr backgroundGraphicsRT;
		::System::RenderTargetPtr backgroundColourGridRT;

		::System::RenderTargetPtr mainGraphicsRT;
		::System::RenderTargetPtr mainColourGridRT;

		float timeLeftThisFrame = 0.0f;

		Room* renderedRoom = nullptr;
		Room* inRoom = nullptr;
		int inRoomFrame = 0;

		int deathFrame = NONE;
		bool forceRerender = false;
		bool renderItemsAndTime = false;

		int livesAnimateCounter = 0;
		int livesFrame = 0;

		Object player;
		int playerJumpFrame = NONE;
		int playerAutoMovement = 0;
		bool playerFalling = false;
		int playerFallsFor = 0;
		bool playerJumpFailedImmediately = false;
		bool playerAgainstAutoMovement = false;
		bool playerAgainstAutoMovementStill = false;
		Array<Object> guardians;
		
		Array<VisitedRoomInfo> visitedRooms;
		Array<bool> itemsTakenInThisRoom;
		int itemsTaken = 0;

		Object checkpoint;
		int checkpointJumpFrame = NONE;
		bool checkpointFalling = false;
		int checkpointFallsFor = 0;

		void advance_frame();

		void switch_to_room(Room* _room, int _doorIndex, int _leftThroughDoorIndex);

		bool start_death();

		void spawn_guardians();

		void do_checkpoint();

		void render_room_name(String const & _name);
		void render_items_and_time();

		void on_change_room(Room* _nextRoom);
		Game::VisitedRoomInfo* find_visited_room(Room* _room);
	};
};
#endif
