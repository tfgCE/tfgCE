#pragma once

#include "..\..\loaderHubScreen.h"
#include "..\..\..\..\game\playerSetup.h"

namespace TeaForGodEmperor
{
	class GameDefinition;
	class GameSubDefinition;
};

namespace Loader
{
	namespace HubScreens
	{
		class GameDefinitionSelection
		: public HubScreen
		{
			FAST_CAST_DECLARE(GameDefinitionSelection);
			FAST_CAST_BASE(HubScreen);
			FAST_CAST_END();

			typedef HubScreen base;
		public:
			enum WhatToShow
			{
				ShowAll,
				SelectGameDefinition,
				SelectGameSubDefinition,
				ShowSummary
			};

		public:
			static HubScreen* show(Hub* _hub, Name const& _id, TeaForGodEmperor::PlayerGameSlot* _gameSlot, WhatToShow _whatToShow, std::function<void()> _onPrev = nullptr, std::function<void(bool _startImmediately)> _onNextSelect = nullptr);

		public: // HubScreen
			override_ void advance(float _deltaTime, bool _beyondModal);

		private:
			GameDefinitionSelection(Hub* _hub, Name const& _id, TeaForGodEmperor::PlayerGameSlot* _gameSlot,
				WhatToShow _whatToShow,
				std::function<void(bool _startImmediately)> _onNextSelect,
				Array<TeaForGodEmperor::GameDefinition*> const & _gameDefinitions,
				Array<TeaForGodEmperor::GameSubDefinition*> const & _gameSubDefinitions,
				Rotator3 const& _atDir, Vector2 const& _size, float _radius, Vector2 const& _ppa);

		private:
			RefCountObjectPtr<TeaForGodEmperor::PlayerGameSlot> gameSlot;

			RefCountObjectPtr<HubWidget> generalDescriptionWidget;
			
			Array<RefCountObjectPtr<HubWidget>> gameDefinitionWidgets;
			
			Array<TeaForGodEmperor::GameDefinition*> gameDefinitions;
			Array<TeaForGodEmperor::GameSubDefinition*> gameSubDefinitions;
			Array<RefCountObjectPtr<HubWidget>> gameDefinitionButtons;
			Array<RefCountObjectPtr<HubWidget>> gameDefinitionMoreInfoButtons;
			Array<RefCountObjectPtr<HubWidget>> gameSubDefinitionButtons;

			TeaForGodEmperor::GameDefinition* selectedGameDefinition = nullptr;
			Array<TeaForGodEmperor::GameSubDefinition*> selectedGameSubDefinitions;

			void select_game_definition(TeaForGodEmperor::GameDefinition* gd);
			void select_game_sub_definition(TeaForGodEmperor::GameSubDefinition* gsd);
			
			void update_chosen_game_definition();
			
			void update_selection();

		};
	};
};
