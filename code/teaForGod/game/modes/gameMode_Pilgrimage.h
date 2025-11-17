#pragma once

#include "..\..\teaEnums.h"
#include "..\..\..\framework\game\gameMode.h"
#include "..\..\..\framework\library\usedLibraryStored.h"

namespace Framework
{
	class RegionType;
	class SubWorld;
};

namespace TeaForGodEmperor
{
	class Game;
	class ItemCollection;
	struct ItemVariant;
	struct GameState;

	class Pilgrimage;
	class PilgrimageInstance;

	namespace GameModes
	{
		class PilgrimageSetup
		: public Framework::GameModeSetup
		{
			FAST_CAST_DECLARE(PilgrimageSetup);
			FAST_CAST_BASE(GameModeSetup);
			FAST_CAST_END();
			typedef GameModeSetup base;
		public:
			PilgrimageSetup(Game* _game, TeaForGodEmperor::Pilgrimage* _pilgrimage, GameState* _gameState);

#ifdef AN_DEVELOPMENT
			virtual tchar const * get_mode_name() const { return TXT("pilgrimage"); }
#endif

			TeaForGodEmperor::Pilgrimage* get_pilgrimage() const { return pilgrimage; }
			TeaForGodEmperor::GameState* get_game_state() const { return gameState; }

		public: // GameModeSetup
			override_ Framework::GameMode* create_mode();

		protected:
			TeaForGodEmperor::Pilgrimage* pilgrimage = nullptr;
			TeaForGodEmperor::GameState* gameState = nullptr;
		};

		class Pilgrimage
		: public Framework::GameMode
		{
			FAST_CAST_DECLARE(Pilgrimage);
			FAST_CAST_BASE(Framework::GameMode);
			FAST_CAST_END();
			typedef Framework::GameMode base;
		public:
			Pilgrimage(Framework::GameModeSetup* _setup);
			virtual ~Pilgrimage();

			bool has_pilgrimage_instance() { return pilgrimageInstance.get() != nullptr; }
			PilgrimageInstance & access_pilgrimage_instance() { return *pilgrimageInstance.get(); }
			template <typename InstanceClass>
			InstanceClass * access_pilgrimage_instance_as() { return fast_cast<InstanceClass>(pilgrimageInstance.get()); }

			void set_pilgrimage_result(PilgrimageResult::Type _pilgrimageResult);

		public: // linear
			bool is_linear() const;

			bool linear__is_station(Framework::Room* _room) const;
			Framework::Region* linear__get_current_region() const;
			int linear__get_current_region_idx() const;
			bool linear__is_current_station(Framework::Room* _room) const;
			bool linear__is_next_station(Framework::Room* _room) const;
			Framework::Room* linear__get_current_station() const;
			Framework::Room* linear__get_next_station() const;
			bool linear__has_reached_end() const;

		public:
			override_ void on_start();
			override_ void on_end(bool _abort = false);

			override_ void pre_advance(Framework::GameAdvanceContext const & _advanceContext);
			override_ void pre_game_loop(Framework::GameAdvanceContext const & _advanceContext);

		public:
			override_ void log(LogInfoContext & _log) const;

		protected:
			override_ Framework::GameSceneLayer* create_layer_to_replace(Framework::GameSceneLayer* _replacingLayer);
			override_ bool does_use_auto_layer_management() const { return true; }

		protected:
			RefCountObjectPtr<PilgrimageSetup> pilgrimageSetup;

			RefCountObjectPtr<TeaForGodEmperor::PilgrimageInstance> pilgrimageInstance;

			Framework::LibraryName testRoom;
			bool testRoomCreated = false;

			Optional<PilgrimageResult::Type> pilgrimageResult;
		};
	};
};
