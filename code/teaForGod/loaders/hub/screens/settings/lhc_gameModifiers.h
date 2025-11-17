#pragma once

#include "..\..\loaderHubScreen.h"
#include "..\..\..\..\game\runSetup.h"

namespace TeaForGodEmperor
{
	class GameModifier;
};

namespace Loader
{
	namespace HubScreens
	{
		class GameModifiers
		: public HubScreen
		{
			FAST_CAST_DECLARE(GameModifiers);
			FAST_CAST_BASE(HubScreen);
			FAST_CAST_END();

			typedef HubScreen base;
		public:
			static HubScreen* show(Hub* _hub, bool _beStandAlone, Name const& _id, TeaForGodEmperor::RunSetup& _runSetup, std::function<void()> _initialSetup = nullptr, std::function<void()> _onApply = nullptr);

		public:
			GameModifiers(Hub* _hub, bool _beStandAlone, Name const& _id, TeaForGodEmperor::RunSetup& _runSetup, std::function<void()> _initialSetup, Optional<Name> const & _onApplyButton, std::function<void()> _onApply, Optional<Rotator3> const& _atDir, Vector2 const& _size, float _radius, Vector2 const& _ppa, bool _forHeightCalculation = false);

		private:
			TeaForGodEmperor::RunSetup& runSetupSource;
			TeaForGodEmperor::RunSetup runSetup;

			Colour colourTitleOn;
			Colour colourTitleOff;

			Colour colourDescOn;
			Colour colourDescOff;

			bool standAlone = false;

			struct GameModifierWidget
			{
				bool blocked = false;
				Optional<VectorInt2> at;
				TeaForGodEmperor::GameModifier const* gameModifier = nullptr;
				RefCountObjectPtr<HubWidget> buttonWidget;
				RefCountObjectPtr<HubWidget> titleWidget;
				RefCountObjectPtr<HubWidget> descriptionWidget;
			};
			Array<GameModifierWidget> widgets;
			RefCountObjectPtr<HubWidget> experienceModifierWidget;

			void change_modifier(Name const & _modifier, Optional<bool> const & _set = NP);
			void update_widgets();

			void show_help();
		};
	};
};
