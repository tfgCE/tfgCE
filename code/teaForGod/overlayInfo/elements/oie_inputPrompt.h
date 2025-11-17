#pragma once

#include "..\overlayInfoElement.h"
#include "..\..\..\core\types\hand.h"
#include "..\..\..\framework\library\usedLibraryStored.h"
#include "..\..\..\framework\text\localisedString.h"

namespace Framework
{
	class TexturePart;
	class MeshLibInstance;
};

namespace TeaForGodEmperor
{
	namespace OverlayInfo
	{
		namespace Elements
		{
			struct InputPrompt
			: public Element
			{
				FAST_CAST_DECLARE(InputPrompt);
				FAST_CAST_BASE(Element);
				FAST_CAST_END();

				typedef Element base;
			public:
				InputPrompt() {}
				virtual ~InputPrompt();
				InputPrompt(Name const& _gameInputDefinition, Name const& _gameInput) { with_game_input(_gameInputDefinition, _gameInput); }
				InputPrompt(Name const& _gameInputDefinition, Array<Name> const& _gameInput) { with_game_input(_gameInputDefinition, _gameInput); }

				InputPrompt* with_game_input(Name const& _gameInputDefinition, Name const& _gameInput) { gameInput.push_back(_gameInput); gameInputDefinition = _gameInputDefinition; return this; }
				InputPrompt* with_game_input(Name const& _gameInputDefinition, Array<Name> const& _gameInput) { gameInput.add_from(_gameInput); gameInputDefinition = _gameInputDefinition; return this; }
				InputPrompt* with_scale(float _scale) { scale = _scale; return this; }
				InputPrompt* with_use_additional_scale(Optional<float> const& _scale) { useAdditionalScale = _scale; return this; }
				InputPrompt* with_additional_info(String const& _additionalInfo) { additionalInfo = _additionalInfo; requiresAdditionalInfoLinesUpdate = true; return this; }
				InputPrompt* for_hand(Optional<Hand::Type> const & _hand = NP) { forHand = _hand; return this; }

			public:
				implement_ void advance(System const* _system, float _deltaTime);
				implement_ void request_render_layers(OUT_ ArrayStatic<int, 8>& _renderLayers) const;
				implement_ void render(System const* _system, int _renderLayer) const;

			protected:
				Array<Name> gameInput;
				Name gameInputDefinition;

				Optional<int> vrControllersIteration;
				
				Optional<Hand::Type> forHand; // by default or meshes and textures are considered to be for right hand

				float scale = 1.0f;
				Optional<float> useAdditionalScale;

				String additionalInfo;
				List<String> additionalInfoLines;
				bool requiresAdditionalInfoLinesUpdate = false;

				struct Prompt
				{
					Name prompt;
					String text;
					Framework::UsedLibraryStored<Framework::TexturePart> texturePart;
					Framework::UsedLibraryStored<Framework::MeshLibInstance> meshLibInstance;
					Optional<Hand::Type> forHand;
				};
				Array<Prompt> prompts;
			};
		}
	};
};
