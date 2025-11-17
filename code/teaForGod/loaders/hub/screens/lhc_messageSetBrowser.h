#pragma once

#include "..\loaderHubScreen.h"
#include "..\..\..\library\messageSet.h"

#include "..\..\..\..\core\functionParamsStruct.h"

namespace Loader
{
	namespace HubScreens
	{
		struct MessageSetBrowser
		: public HubScreen
		{
			FAST_CAST_DECLARE(MessageSetBrowser);
			FAST_CAST_BASE(HubScreen);
			FAST_CAST_END();

			typedef HubScreen base;

		public:
			struct BrowseParams
			{
				ADD_FUNCTION_PARAM_DEF(BrowseParams, bool, randomOrder, in_random_order, true);
				ADD_FUNCTION_PARAM_DEF(BrowseParams, bool, forLoading, for_loading, true);
				ADD_FUNCTION_PARAM(BrowseParams, Name, showMessage, start_with_message);
				ADD_FUNCTION_PARAM_DEF(BrowseParams, bool, startWithRandomMessage, start_with_random_message, true);
				ADD_FUNCTION_PARAM(BrowseParams, String, title, with_title);
				ADD_FUNCTION_PARAM_DEF(BrowseParams, bool, cantBeClosed, cant_be_closed, true);
			};
		public:
			static MessageSetBrowser* browse(TeaForGodEmperor::MessageSet* _messageSet, BrowseParams const & _params, Hub* _hub, Name const & _id, bool _beModal = true, Optional<Vector2> const & _size = NP, Optional<Rotator3> const & _at = NP, Optional<float> const & _radius = NP, Optional<bool> const & _beVertical = NP, Optional<Rotator3> const & _verticalOffset = NP, Optional<Vector2> const & _pixelsPerAngle = NP, Optional<float> const & _scaleAutoButtons = NP, Optional<Vector2> const & _textAlignPt = NP);
			static MessageSetBrowser* browse(Array<TeaForGodEmperor::MessageSet*> const & _messageSets, BrowseParams const& _params, Hub* _hub, Name const & _id, bool _beModal = true, Optional<Vector2> const & _size = NP, Optional<Rotator3> const & _at = NP, Optional<float> const & _radius = NP, Optional<bool> const & _beVertical = NP, Optional<Rotator3> const & _verticalOffset = NP, Optional<Vector2> const & _pixelsPerAngle = NP, Optional<float> const & _scaleAutoButtons = NP, Optional<Vector2> const & _textAlignPt = NP);
		
			void show_message(tchar const * _id);
			void show_message(Name const & _id);

			void show_random_message();

			void show_message(int idx);

			void may_close(); // if loading has ended

		public: // HubScreen
			implement_ void process_input(int _handIdx, Framework::GameInput const & _input, float _deltaTime);
			implement_ void on_deactivate();

		private:
			MessageSetBrowser(Hub* _hub, Name const & _id, Vector2 const & _size, Rotator3 const & _at, float _radius, Optional<bool> const & _beVertical = NP, Optional<Rotator3> const & _verticalOffset = NP, Optional<Vector2> const & _pixelsPerAngle = NP, Optional<float> const& _scaleAutoButtons = NP, Optional<Vector2> const& _textAlignPt = NP);

		private:
			TeaForGodEmperor::MessageSet* messageSet = nullptr;
			Array<TeaForGodEmperor::MessageSet::Message const*> messages;

			String title;

			int messageIdx = 0;
			bool randomOrder = false;
			bool forLoading = false; // if set to true, if we switch to a different message, we will have to press "continue" when loading is done
			bool mayClose = true; // if for loading, won't be able to close
			bool requiresExplicitClose = false;
			bool cantBeClosed = false;

			float scaleAutoButtons = 2.0f;
			Optional<Vector2> textAlignPt;

			static MessageSetBrowser* browse(BrowseParams const& _params, Hub* _hub, Name const& _id, bool _beModal = true, Optional<Vector2> const& _size = NP, Optional<Rotator3> const& _at = NP, Optional<float> const& _radius = NP, Optional<bool> const& _beVertical = NP, Optional<Rotator3> const& _verticalOffset = NP, Optional<Vector2> const& _pixelsPerAngle = NP, Optional<float> const& _scaleAutoButtons = NP, Optional<Vector2> const& _textAlignPt = NP);

			void prepare_messages();
		};
	};
};
