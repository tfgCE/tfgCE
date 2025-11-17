#pragma once

#include "..\loaderHubScene.h"
#include "..\loaderHubWidget.h"

namespace TeaForGodEmperor
{
	class MessageSet;
};

namespace Loader
{
	namespace HubScenes
	{
		class BeAtRightPlace
		: public HubScene
		{
			FAST_CAST_DECLARE(BeAtRightPlace);
			FAST_CAST_BASE(HubScene);
			FAST_CAST_END();

			typedef HubScene base;

		public:
			static bool is_currently_active() { return s_current != nullptr; }

			static bool is_required(Hub* _hub); // in general
			bool is_required() const; // this one

			BeAtRightPlace(bool _requiresToBeDeactivated = false, bool _testShowBoundary = false);
			virtual ~BeAtRightPlace();

			BeAtRightPlace* with_go_back_button(bool _goBackButton = true) { goBackButton = _goBackButton; if (!_goBackButton) goBackDo = nullptr; return this; }
			BeAtRightPlace* with_go_back_button(std::function<void()> _goBackDo) { goBackButton = true; goBackDo = _goBackDo; return this; }
			BeAtRightPlace* ignore_rotation(bool _ignoreRotation = true) { ignoreRotation = _ignoreRotation; return this; }
			BeAtRightPlace* wait_for_be_at(bool _waitForBeAt = true) { waitForBeAt = _waitForBeAt; return this; }
			BeAtRightPlace* set_waiting_message(String const & _waitingMessage, Optional<float> const & _scale = NP) { waitingMessage = _waitingMessage; waitingMessageScale = _scale; return this; }
			BeAtRightPlace* set_info_message(String const& _infoMessage, Optional<float> const& _scale = NP) { infoMessage = _infoMessage; infoMessageScale = _scale; return this; }
			BeAtRightPlace* set_requires_to_be_deactivated(bool _requiresToBeDeactivated = true) { requiresToBeDeactivated = _requiresToBeDeactivated; return this; }
			BeAtRightPlace* with_help(TeaForGodEmperor::MessageSet* _helpMessageSet) { helpMessageSet = _helpMessageSet; return this; }
			BeAtRightPlace* with_special_button(String const& _label, std::function<bool()> _do) { specialButtons.push_front/*we're adding starting with the bottom*/(SpecialButton(_label, _do)); return this; }
			BeAtRightPlace* with_no_special_button() { specialButtons.clear(); return this; }
			BeAtRightPlace* allow_loading_tips(bool _allowLoadingTips = true) { allowLoadingTips = _allowLoadingTips; return this; }

			static void be_at(Optional<Transform> const& _beAt, bool _anyHeight = false); // in vr space!
			static void reset_be_at() { be_at(NP); }

			static void set_max_dist(Optional<float> const & _maxDist = NP); // in vr space!

			static Optional<Transform> get_be_at(OPTIONAL_ OUT_ bool* _waitForBeAt = nullptr, OPTIONAL_ OUT_ bool* _beAtAnyHeight = nullptr, OPTIONAL_ OUT_ bool* _ignoreRotation = nullptr);
			
		public: // HubScene
			implement_ void on_activate(HubScene* _prev);
			implement_ void on_update(float _deltaTime);
			implement_ void on_deactivate(HubScene* _next);
			implement_ void on_loader_deactivate();
			implement_ bool does_want_to_end() { return ((!requiresToBeDeactivated || loaderDeactivated) && isAtRightPlace) || backRequested; }

		private:
			RefCountObjectPtr<HubScene> goBackToScene;

			RefCountObjectPtr<HubWidget> textWidget;
			RefCountObjectPtr<HubWidget> imageWidget;

			TeaForGodEmperor::MessageSet* helpMessageSet = nullptr;

			struct SpecialButton
			{
				String label;
				std::function<bool()> perform; // returns true if continues, false to close
				SpecialButton() {}
				SpecialButton(String const& _label, std::function<bool()> _perform) : label(_label), perform(_perform) {}
			};
			Array<SpecialButton> specialButtons;

			// if is at right place, waiting message will appear
			String waitingMessage;
			String infoMessage;
			Optional<float> waitingMessageScale;
			Optional<float> infoMessageScale;

			float highlightInfoPt = 0.0f;
			float highlightInfoPeriod = 1.0f;
			float highlightInfoActivePt = 0.8f;

			bool goBackButton = false;
			std::function<void()> goBackDo;

			bool allowLoadingTips = false;

			bool testShowBoundary = false;
			bool requiresToBeDeactivated = false;
			bool loaderDeactivated = false;
			bool isAtRightPlace = false;
			bool forceRightPlace = false;
			bool backRequested = false;

			bool waitForBeAt = false;
			
			static BeAtRightPlace* s_current;
			static Optional<Transform> beAt; // in vr space
			static bool beAtAnyHeight;
			static Concurrency::SpinLock beAtLock;
			static Optional<float> maxDist;

			bool ignoreRotation = false;

			void show_info();

			void go_back();

			enum IsAtRightPlace
			{
				AtRightPlace	= 0,
				TurnRight		= 1,
				TurnLeft		= 2,
				Lower			= 4,
				Higher			= 8,
				MoveLeft		= 16,
				MoveRight		= 32,
				MoveForward		= 64,
				MoveBackward	= 128,
				Unknown			= 256
			};
			int lastIARS = Unknown;

			struct AtRightPlaceInfo
			{
				Framework::TexturePart* texturePart = nullptr;
				Name locStrId;

				AtRightPlaceInfo() {}
				AtRightPlaceInfo(Name const & _globalReferenceId, Name const & _locStrId);
			};
			Array<AtRightPlaceInfo> atRightPlaceInfos;
			static int iars_to_index(int _iars);
			static int is_at_right_place(Hub* _hub, Optional<bool> const & _checkRotation = NP, Optional<bool> const & _waitForBeAt = NP, OPTIONAL_ OUT_ bool* _beaconRActive = nullptr, OPTIONAL_ OUT_ float* _beaconZ = nullptr, OPTIONAL_ OUT_ Optional<float>* _beAtExplitYaw = nullptr);
			static Optional<Transform> get_start_movement_centre(Hub* _hub, Optional<bool> const& _waitForBeAt = NP);

			void show_help();
		};
	};
};
