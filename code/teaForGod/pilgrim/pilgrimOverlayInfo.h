#pragma once

#include "..\..\core\functionParamsStruct.h"
#include "..\..\core\concurrency\spinLock.h"
#include "..\..\core\math\math.h"
#include "..\..\core\memory\safeObject.h"
#include "..\..\core\memory\softPooledObject.h"
#include "..\..\core\system\video\vertexFormat.h"
#include "..\..\core\types\optional.h"

#include "..\..\framework\interfaces\presenceObserver.h"
#include "..\..\framework\library\usedLibraryStored.h"
#include "..\..\framework\modulesOwner\modulesOwner.h"
#include "..\..\framework\presence\presencePath.h"
#include "..\..\framework\video\texturePartUse.h"

#include "..\teaForGod.h"

#include "..\game\energy.h"
#include "..\game\missionStateObjectives.h"
#include "..\modules\custom\mc_investigatorInfoProvider.h"
#include "..\modules\gameplay\equipment\me_gun.h"
#include "..\overlayInfo\overlayInfoElement.h"
#include "..\overlayInfo\elements\oie_text.h"

namespace System
{
	struct ShaderProgramInstance;
};

namespace Framework
{
	interface_class IModulesOwner;
	class Room;
	class Font;
	class Sample;
	class TexturePart;
};

namespace TeaForGodEmperor
{
	class EXMType;
	class LineModel;
	class PilgrimageInstanceOpenWorld;

	namespace OverlayInfo
	{
		class System;
	};

	namespace PilgrimOverlayInfoTip
	{
		enum Type
		{
			InputOpen,
			InputLock,
			InputActivateEXM,
			InputDeployWeapon,
			InputHoldWeapon,
			InputRemoveDamagedWeaponRight,
			InputRemoveDamagedWeaponLeft,
			InputUseEnergyCoil,
			InputHowToUseEnergyCoil,
			InputRealMovement,
			GuidanceFollowDot,
			UpgradeSelection,
			Custom, // provided with params, game script should be the only user

			MAX
		};
	};

	namespace PilgrimOverlayInfoSpeakLine
	{
		enum Type
		{
			Success,
			ReceivingData,
			TransferComplete,
			InterfaceBoxFound,
			InfestationEnded,
			SystemStabilised,
			MAX
		};
	};

	struct PilgrimOverlayInfoRenderableData
	{
		bool is_empty() const { return linesData.is_empty(); }
		void mark_new_data_required();
		void mark_new_data_done();
		void mark_data_available_to_render();
		bool should_do_new_data_to_render();

		PilgrimOverlayInfoRenderableData();

		void clear();
		void add_line(Vector3 const& _a, Vector3 const& _b, Colour const& _colour);
		void add_line_clipped(Vector3 const& _a, Vector3 const& _b, Range3 const & _to, Colour const& _colour);
		void add_triangle(Vector3 const& _a, Vector3 const& _b, Vector3 const& _c, Colour const& _colour); // filled

		void render(float _active, float _pulse, Colour const& _colour, Optional<Colour> const& _overlayOverrideColour, ::System::ShaderProgramInstance* _withShaderProgramInstance,
			Optional<Colour> const& _cooldownColour = NP, Optional<float> const& _cooldownY = NP);

		void set_scale(float _scale) { scale = _scale; }

	private:
		Concurrency::Counter newRequired;
		::System::VertexFormat vertexFormat;
		Concurrency::SpinLock dataFlagsLock = Concurrency::SpinLock(TXT("TeaForGodEmperor.PilgrimOverlayInfoRenderableData.dataFlagsLock"));
		Concurrency::Counter dataAvailableToRender = false;
		bool dataBeingRendered = false;
		float scale = 1.0f;
		Array<byte> linesData;
		Array<byte> trianglesData;
	};

	struct PilgrimOverlayInfo
	: public SafeObject<PilgrimOverlayInfo>
	, public Framework::IPresenceObserver
	{
		FAST_CAST_DECLARE(PilgrimOverlayInfo);
		FAST_CAST_BASE(Framework::IPresenceObserver);
		FAST_CAST_END();

	public:
		static bool should_collect_health_damage_for_local_player() { return s_collectHealthDamageForLocalPlayer; }
	public:
		PilgrimOverlayInfo();
		virtual ~PilgrimOverlayInfo();

		void set_owner(Framework::IModulesOwner* _owner);

		void advance(float _deltaTime, bool _controlActive, Vector2 const& _controlStick);

		void new_map_available(bool _knownLocation = false, bool _inform = false, Optional<VectorInt2> const& _showKnownLocation = NP);
		void update_map_if_visible();
		void clear_map_highlights();
		void highlight_map_at(VectorInt3 const& _at);

		bool is_active() const { return currentlyShowing != None; }
		bool is_showing_main() const { return currentlyShowing == Main; }
		bool is_actually_showing_map() const { return showMap.visible && ! showMap.rd.is_empty(); }
		bool is_trying_to_show_map() const { return currentlyShowing == Main || requestedToShow == Main || showMap.visible; }
		void show_main(bool _showMain) { requestedToShow = _showMain? Main : None; }
		void switch_show_investigate();

		bool is_showing_info(Name const& _id);
		void show_info(Optional<Name> const& _id, Transform const& _at, std::function<OverlayInfo::Element* ()> _setup_element, Optional<bool> const& _allowAutoHide = NP);
		void show_info(Optional<Name> const & _id, Transform const& _at, String const& _info, Optional<OverlayInfo::TextColours> const& _colours = NP, Optional<float> const & _size = NP, Optional<float> const & _width = NP, Optional<bool> const & _allowAutoHide = NP, Optional<Vector2> const & _pt = NP, Optional<bool> const & _allowSmallerDecimals = NP);
		void show_info_ex(Optional<Name> const & _id, Transform const& _at, String const& _info, std::function<OverlayInfo::Element* ()> _setup_element, Optional<OverlayInfo::TextColours> const& _colours = NP, Optional<float> const & _size = NP, Optional<float> const & _width = NP, Optional<bool> const & _allowAutoHide = NP, Optional<Vector2> const & _pt = NP, Optional<bool> const & _allowSmallerDecimals = NP);
		bool update_info(Name const & _id, String const& _info);
		void hide_show_info(Optional<Name> const& _id = NP);

		void show_symbol_replacement(Framework::TexturePart* _from, Framework::TexturePart* _to);

		void set_may_request_looking_for_pickups(bool _lookForPickups);

		// if no reason provided, will show always and will hide if no other reason exists
		void show_exms(Name const & _reason, bool _onlyActive = false);
		void hide_exms(Name const & _reason);
		void keep_showing_exms(Name const & _reason); // keeps showing until we decide to hide

		// permanent
		void show_permanent_exms(Name const& _reason);
		void hide_permanent_exms(Name const& _reason);
		void keep_showing_permanent_exms(Name const & _reason); // keeps showing until we decide to hide

		// there should be just one proposal per slot, so we don't have reason here
		void show_exm_proposal(Hand::Type _hand, bool _passiveEXM, Name const & _exm);
		void clear_exm_proposal(Hand::Type _hand, bool _passiveEXM) { show_exm_proposal(_hand, _passiveEXM, Name::invalid()); }

		void clear_in_hand_equipment_stats(Hand::Type _hand); // in case we switch weapon etc
		void show_in_hand_equipment_stats(Hand::Type _hand, Name const& _reason, Optional<float> const& _timeLeft = NP);
		void hide_in_hand_equipment_stats(Hand::Type _hand, Name const& _reason);
		void recreate_in_hand_equipment_stats(Optional<Hand::Type> const& _hand = NP);

		struct ShowTipParams
		{
			ADD_FUNCTION_PARAM(ShowTipParams, float, timeToDeactivateTip, with_time_to_deactivate_tip);
			ADD_FUNCTION_PARAM_TRUE(ShowTipParams, force, be_forced);
			ADD_FUNCTION_PARAM(ShowTipParams, float, pitchOffset, with_pitch_offset);
			ADD_FUNCTION_PARAM(ShowTipParams, float, scale, with_scale);
			ADD_FUNCTION_PARAM(ShowTipParams, Hand::Type, forHand, for_hand);
			ADD_FUNCTION_PARAM_TRUE(ShowTipParams, followCameraPitch, follow_camera_pitch);
			ADD_FUNCTION_PARAM(ShowTipParams, Name, locStrId, with_localised_string_id); // used for custom tips (or for input)
			ADD_FUNCTION_PARAM(ShowTipParams, String, text, with_text); // used for custom tips (or for input)
		};
		bool is_showing_tip(PilgrimOverlayInfoTip::Type _tip) const;
		void show_tip(PilgrimOverlayInfoTip::Type _tip, ShowTipParams const& _params);
		void hide_tips();
		void hide_tip(PilgrimOverlayInfoTip::Type _tip);
		void done_tip(PilgrimOverlayInfoTip::Type _tip, Optional<Hand::Type> const & _forHand = NP);

		void show_main_log_at_camera(bool _showMainLogAtCamera);
		void add_main_log(String const & _text, Optional<Colour> const & _colour = NP, Optional<int> const& _atLine = NP);
		void clear_main_log();
		void clear_main_log_line_indicator();
		void clear_main_log_line_indicator_on_no_voiceover_actor(Name const & _voActor);

		void set_boot_level(int _bootLevel) { bootLevelRequested = _bootLevel; }
		bool is_at_boot_level(int _bootLevel) { return bootLevel == 0 || bootLevel >= _bootLevel; }

		void on_upgrade_installed();

		void tried_to_shoot_empty(Hand::Type _hand);

		void log(LogInfoContext& _log);

		::System::ShaderProgramInstance* get_overlay_shader_instance() const { return overlayShaderInstance; }

	public:
		void on_transform_vr_anchor(Transform const& _localToVRAnchorTransform); // useful for sliding locomotion, if vr anchor has moved we want to move some things related to that (like disableWhenMovesTooFarFrom etc)

	public: // IPresenceObserver
		implement_ void on_presence_changed_room(Framework::ModulePresence* _presence, Framework::Room* _intoRoom, Transform const& _intoRoomTransform, Framework::DoorInRoom* _exitThrough, Framework::DoorInRoom* _enterThrough, Framework::DoorInRoomArrayPtr const* _throughDoors);
		implement_ void on_presence_added_to_room(Framework::ModulePresence* _presence, Framework::Room* _room, Framework::DoorInRoom* _enteredThroughDoor);
		implement_ void on_presence_removed_from_room(Framework::ModulePresence* _presence, Framework::Room* _room);
		implement_ void on_presence_destroy(Framework::ModulePresence* _presence);

	public:
		struct SpeakParams
		{
			ADD_FUNCTION_PARAM(SpeakParams, Colour, colour, with_colour);
			ADD_FUNCTION_PARAM_PLAIN_INITIAL_DEF(SpeakParams, bool, allowAutoClear, allow_auto_clear, true, true);
			ADD_FUNCTION_PARAM(SpeakParams, float, speakDelay, speak_delay);
			ADD_FUNCTION_PARAM(SpeakParams, bool, speakClearLogOnSpeak, clear_log_on_speak);
			ADD_FUNCTION_PARAM(SpeakParams, int, atLine, at_line);
			ADD_FUNCTION_PARAM_PLAIN(SpeakParams, std::function<void()>, performOnSpeak, perform_on_speak);

			SpeakParams() : allowAutoClear(true) {}
		};
		void speak(Framework::Sample* _line, SpeakParams const & _params = SpeakParams()); // adds to the queue (adds also to main log)
		void speak(PilgrimOverlayInfoSpeakLine::Type _line, SpeakParams const & _params = SpeakParams());
		bool is_speaking_or_pending() const;
		void set_auto_energy_speak_blocked(bool _blocked);

	public:
		void collect_health_damage(Framework::IModulesOwner* _imo, Energy const& _damage);

	private:
		struct ReportedDamage
		{
			Framework::IModulesOwner* unsafeIMOForReference = nullptr; // should not be accessed, just as a reference
			Energy damage = Energy::zero();
			float timeLeft = 0.0f;
		};
		static bool s_collectHealthDamageForLocalPlayer; // one local player
		Concurrency::SpinLock collectedHealthDamageLock;
		ArrayStatic<ReportedDamage, 128> collectedHealthDamage;

	private:
		Optional<float> controlActiveFor = 0.0f;
		bool controlConsumed = false;

		float keepShowingFor = 0.0f;

		float globalTintFade = 0.0f;

		bool noExplicitTutorials = false;

		::SafePtr<Framework::IModulesOwner> owner;
		Framework::ModulePresence* observingPresence = nullptr;

		float statsDist = 0.6f;

		int bootLevel = 0; // if non zero, special rules apply
		int bootLevelRequested = 0; 

		Collision::Flags pilgrimOverlayInfoTrace;

		String stringStatOf;

		::System::TimeStamp followGuidanceSpokenTS;
		::System::TimeStamp goingInWrongDirectionTipTS;
		int goingInWrongDirectionCount = 0;
		::System::TimeStamp goingInWrongDirectionTS;
		struct PilgrimEnergyState
		{
			bool ammoCouldBeTopped = false;
			bool ammoHalf = false;
			bool ammoLow = false; // just very low, should find a dispenser
			bool ammoCritical = false; // almost depleted
			bool healthCouldBeTopped = false;
			bool healthHalf = false;
			bool healthLow = false;
			bool healthVeryLow = false; // just very low, should find a dispenser
			bool healthCritical = false; // almost depleted

			bool playNoAmmo[Hand::MAX];
			System::TimeStamp playNoAmmoTS[Hand::MAX];
			
			// to avoid repeated play
			System::TimeStamp healthFullTS;
			System::TimeStamp ammoFullTS[Hand::MAX];

			PilgrimEnergyState()
			{
				for_count(int, i, Hand::MAX) playNoAmmo[i] = false;
			}
		} pilgrimEnergyState;

		enum WhatToShow
		{
			None,
			Main,
			ShowInfo,
			Investigate
		};
		WhatToShow currentlyShowing = None;
		WhatToShow requestedToShow = None;
		bool allowShowInfoInMain = true;
		bool lockedToHead = false;
		bool lockedToHeadForced = false; // if set to always lock to head
		struct ReferencePlacement
		{
			Transform ownerPlacement = Transform::identity;
			Transform eyesRelative = Transform::identity;
			Transform vrAnchorPlacement = Transform::identity;
		};
		bool showMainHealthStatus = true;
		bool showHealthStatus = true;
		bool showAmmoStatus = true;
		bool fullUpdateEquipmentStatsRequired = true;
		Optional<ReferencePlacement> referencePlacement;
		bool showOpenWorldCellInfo = false;
		Optional<Transform> disableWhenMovesTooFarFrom;
		float disableWhenMovesTooFarFromDist = 0.0f;
		Optional<Transform> disableMapWhenMovesTooFarFrom;
		float disableMapWhenMovesTooFarFromDist = 0.0f;
		Concurrency::SpinLock keepOverlayInfoElementsLock = Concurrency::SpinLock(TXT("TeaForGodEmperor.PilgrimOverlayInfo.keepOverlayInfoElementsLock"));
		Array<Name> keepOverlayInfoElements;

		template <typename Class>
		struct VisibleValue
		{
			Class value;
			RefCountObjectPtr<OverlayInfo::Element> element;
		};

		struct ShowMap
		{
			bool newAvailable = false;
			bool knownLocation = false;
			Optional<VectorInt2> showKnownLocation;
			Optional<VectorInt2> lastKnownLocation;
			Optional<VectorInt2> lastLocation;
			Optional<VectorInt2> lastValidLocation;
			bool visible = false;
			PilgrimOverlayInfoRenderableData rd;
			Concurrency::SpinLock highlightAtLock = Concurrency::SpinLock(TXT("TeaForGodEmperor.PilgrimOverlayInfo.ShowMap.highlightAtLock"));
			struct HighlightAt
			{
				Optional<VectorInt3> from; // only used if autoMapOnly
				VectorInt3 to;
			};
			Array<HighlightAt> highlightAt;
			::System::TimeStamp highlightsLastVisibleAt;
			bool resetHighlightsLastVisibleAt = false;
		} showMap;

		struct ShowEXMs
		{
			Concurrency::SpinLock showReasonsLock = Concurrency::SpinLock(TXT("TeaForGodEmperor.PilgrimOverlayInfo.ShowEXMs.showReasonsLock"));
			ArrayStatic<Name, 8> showReasons;
			ArrayStatic<Name, 8> showActiveReasons;
			ArrayStatic<Name, 8> keepShowingReasons;

			struct ShowEXM
			{
				CACHED_ EXMType const * currentEXMType = nullptr;
				CACHED_ EXMType const * proposalEXMType = nullptr;
				// reverse is used to show on the other side
				VisibleValue<Name> visible;
				VisibleValue<Name> visibleReverse;
				VisibleValue<Name> info;
				VisibleValue<Name> infoReverse;
				Name proposalEXM;

				PilgrimOverlayInfoRenderableData rd;
			};

			struct ShowHand
			{
				ArrayStatic<ShowEXM, MAX_PASSIVE_EXMS> passiveEXMs;
				ShowEXM activeEXM;

				ShowHand()
				{
					SET_EXTRA_DEBUG_INFO(passiveEXMs, TXT("PilgrimOverlayInfo::ShowEXMs::ShowHand.passiveEXMs"));
				}
			};
			ShowHand hands[Hand::MAX];

			ShowEXMs()
			{
				SET_EXTRA_DEBUG_INFO(showReasons, TXT("PilgrimOverlayInfo::ShowEXMs.showReasons"));
				SET_EXTRA_DEBUG_INFO(showActiveReasons, TXT("PilgrimOverlayInfo::ShowEXMs.showActiveReasons"));
				SET_EXTRA_DEBUG_INFO(keepShowingReasons, TXT("PilgrimOverlayInfo::ShowEXMs.keepShowingReasons"));
			}
		} showEXMs;

		struct ShowPermanentEXMs
		{
			Concurrency::SpinLock showReasonsLock = Concurrency::SpinLock(TXT("TeaForGodEmperor.PilgrimOverlayInfo.ShowPermanentEXMs.showReasonsLock"));
			ArrayStatic<Name, 8> showReasons;
			ArrayStatic<Name, 8> keepShowingReasons;

			struct ShowEXM
			{
				EXMType const * currentEXMType = nullptr;
				VisibleValue<Name> visible;
				VisibleValue<Name> info;
				int count = 1;
				int visibleCount = 1;

				PilgrimOverlayInfoRenderableData rd;
			};

			ArrayStatic<ShowEXM, 32> exms;
			ArrayStatic<Name, 32> pilgrimEXMs;

			ShowPermanentEXMs()
			{
				SET_EXTRA_DEBUG_INFO(showReasons, TXT("PilgrimOverlayInfo::ShowPermanentEXMs.showReasons"));
				SET_EXTRA_DEBUG_INFO(keepShowingReasons, TXT("PilgrimOverlayInfo::ShowPermanentEXMs.keepShowingReasons"));
				SET_EXTRA_DEBUG_INFO(exms, TXT("PilgrimOverlayInfo::ShowPermanentEXMs.exms"));
				SET_EXTRA_DEBUG_INFO(pilgrimEXMs, TXT("PilgrimOverlayInfo::ShowPermanentEXMs.pilgrimEXMs"));
			}
		} showPermanentEXMs;

		struct ShowInfoInfo
		{
			bool visible = false;
			RefCountObjectPtr<OverlayInfo::Element> element;
			bool allowAutoHide = true;
			Name id;
			Transform at;
			String info;
			std::function<OverlayInfo::Element*()> setup_element;
			OverlayInfo::TextColours colours;
			Optional<float> size;
			Optional<float> width;
			Optional<Vector2> pt;
			Optional<bool> allowSmallerDecimals;

			Optional<Transform> hideWhenMovesTooFarFrom;
			float hideWhenMovesTooFarFromDist = 0.0f;
		};
		Concurrency::SpinLock showInfosLock = Concurrency::SpinLock(TXT("TeaForGodEmperor.PilgrimOverlayInfo.showInfosLock"));
		Array<ShowInfoInfo> showInfos;
		bool updateShowInfos = false;

		Framework::Room* inRoom = nullptr;
		Optional<VectorInt2> inOpenWorldCell;
		Optional<Vector3> inOpenWorldCellVisible;
		PilgrimageInstanceOpenWorld* inOpenWorldCellForPilgrimageRef = nullptr;
		VectorInt2 mapOffset = VectorInt2::zero;
		VectorInt2 mapStick = VectorInt2::zero;
		float mapStickSameFor = 0.0f;
		float mapStickSameTimeLimit = 0.0f;

		struct TipsDone
		{
			struct TipDone
			{
				bool done = false;
				::System::TimeStamp when;
				float interval = 60.0f * 10.0f;
			};
			TipDone tips[PilgrimOverlayInfoTip::MAX];
		} tipsDone;
		Optional<PilgrimOverlayInfoTip::Type> currentTip;
		Optional<Hand::Type> currentTipForHand;
		Optional<float> timeToDeactivateCurrentTip;

		struct InOpenWorldCellInfoTexture
		{
			Vector2 offset = Vector2::zero;
			Framework::TexturePartUse texturePartUse;
			Framework::TexturePartUse altTexturePartUse;
			Optional<Colour> colourOverride;
			Optional<Colour> altColourOverride;
			Vector2 altOffsetPt = Vector2::zero;
			float periodTime = 0.0f;
			float altPt = 0.5f;
			bool altFirst = false;
			bool altOnTop = false;
		};
		Array<InOpenWorldCellInfoTexture> inOpenWorldCellInfoTextures;

		struct ShowLog
		{
			struct Element
			{
				bool multiLineContinuation = false;
				String text;
				Optional<Colour> colour;
				String elementText;
				RefCountObjectPtr<OverlayInfo::Element> element;
				RefCountObjectPtr<OverlayInfo::Element> indicatorElement;
			};
			Array<Element> lines;
			int displayedLines = 0;
			Name clearMainLogLineIndicatorOnNoVoiceoverActor;
			bool withLineIndicator = true;
		};
		ShowLog showMainLog;
		ShowLog showInvestigateLog;

		// this is global
		bool currentShowMainLogAtCamera = false; // this is sort of hack for when we control turret
		bool showMainLogAtCamera = false; // this is sort of hack for when we control turret

		bool mayRequestLookingForPickups = false; // this means that won't be just clearing stuff but may look for pickups actively

		struct ShowSubtitleLog
		{
			int ver = NONE;
			Array<String> lines;
		} showSubtitleLog;

		::System::ShaderProgramInstance* overlayShaderInstance = nullptr;

		Colour colourHere = Colour::red;
		Colour colourHighlight = Colour::yellow;
		Colour colourKnownExits = Colour::grey;
		Colour colourKnownExitsAutoMapOnly = Colour::grey;
		Colour colourKnownDevices = Colour::grey;
		Colour colourKnownDevicesAutoMapOnly = Colour::grey;
		Colour colourVisited = Colour::grey;
		Colour colourLineModel = Colour::grey;
		Colour colourLongDistanceDetection = Colour::green;
		Colour colourLongDistanceDetectionByDirection = Colour::red;
		Colour colourReplaceEXM = Colour::red;
		Colour colourInstallEXM = Colour::greyLight;
		Colour colourActiveEXM = Colour::red;
		Colour colourActiveEXMCooldown = Colour::blue;
		
		Colour colourCellInfoEnterFrom = Colour::cyan;

		Colour colourInvestigateDamage = Colour::red;
		
		Colour colourMissionType = Colour::white;

		struct InvestigateInfo
		{
			struct Icon
			{
				tchar icon; // gets the first character from a localised string
				::Framework::LocalisedString info;

				void setup(Name const& _lsIcon, Name const& _info);
			};
			Icon health;
			Icon healthRule;
			Icon proneToExplosions;
			Icon noSight;
			Icon sightMovementBased;
			Icon noHearing;
			Icon ally;
			Icon enemy;
			Icon shield;
			Icon confused;
			Icon ricochet;
			// will choose which one to use
			Icon armour100; // use this for info
			Icon armour75;
			Icon armour50;
			Icon armour25;
		} investigateInfo;

		struct InvestigateLogInfo
		{
			struct LogLineElement
			{
				int lineIdx = NONE;
				CustomModules::InvestigatorInfoCachedData cachedData;

				LogLineElement() {}
				explicit LogLineElement(int _lineIdx) : lineIdx(_lineIdx) {}
			};

			::SafePtr<Framework::IModulesOwner> imo;
			LogLineElement health;
			Array<LogLineElement> healthRules;
			LogLineElement proneToExplosions;
			LogLineElement shield;
			LogLineElement armour;
			LogLineElement ricochet;
			LogLineElement noSight;
			LogLineElement noHearing;
			LogLineElement social;
			LogLineElement confused;
			ArrayStatic<LogLineElement,4> extraEffects; // no more than this should be ever active at the same time
			Array<LogLineElement> investigatorInfoProvider;
		} investigateLogInfo;

		struct TimedReason
		{
			Name reason;
			Optional<float> timeLeft;

			TimedReason() {}
			TimedReason(Name const & _reason, Optional<float> const & _timeLeft = NP) : reason(_reason), timeLeft(_timeLeft) {}

			bool operator==(TimedReason const& _reason) const { return reason == _reason.reason; }
		};

		struct PilgrimStats
		{
			float timeSinceUpdateGameStats = 0.0f;
			RefCountObjectPtr<OverlayInfo::Element> experienceGainedElement;
			int experienceGainedVer = NONE;
			VisibleValue<Energy> experienceToSpend;
			VisibleValue<int> meritPointsToSpend; // only if missions are active
			VisibleValue<int> realTime; // seconds from midnight
			VisibleValue<bool> lockedToHead;
			VisibleValue<int> turnCounter;
			VisibleValue<int> time;
			VisibleValue<float> distance;
			VisibleValue<Energy> healthMain;
			VisibleValue<Energy> health;
			VisibleValue<Energy> leftHand;
			VisibleValue<Energy> rightHand;
			VisibleValue<Energy> maxHealth;
			VisibleValue<Energy> maxAmmoLeft;
			VisibleValue<Energy> maxAmmoRight;
			VisibleValue<Energy> meleeDamage;
			VisibleValue<Energy> meleeKillHealth;
			VisibleValue<Energy> meleeKillAmmo;
#ifdef WITH_CRYSTALITE
			VisibleValue<int> crystalite;
#endif
			ArrayStatic<Name, 8> extraEXMs;

			PilgrimStats()
			{
				SET_EXTRA_DEBUG_INFO(extraEXMs, TXT("PilgrimOverlayInfo::PilgrimStats.extraEXMs"));
			}
		} pilgrimStats;

		bool wideStats = false;

		struct EquipmentStats
		{
			::SafePtr<Framework::IModulesOwner> equipment;
			ModuleEquipments::GunChosenStats gunStats;
			VisibleValue<int> shots;
			VisibleValue<Energy> health;
			ArrayStatic<RefCountObjectPtr<WeaponPart>, 16> weaponParts;
			bool operator ==(EquipmentStats const& _gs) const;
			bool operator !=(EquipmentStats const& _gs) const { return !(operator ==(_gs)); }
			void copy_stats_from(EquipmentStats const& _stats);
			void update_stats(bool _fillAffection = false, bool _fillWeaponParts = false);

			void reset(bool _keepEquipment = false);

			EquipmentStats& operator=(EquipmentStats const& _source) { copy_stats_from(_source); return *this; }
		};
		struct EquipmentsStats
		{
#ifdef WITH_SHORT_EQUIPMENT_STATS
			EquipmentStats mainEquipment[Hand::MAX];
#endif
			EquipmentStats usedEquipment[Hand::MAX];
			EquipmentStats usedEquipmentInHand[Hand::MAX]; // copy, used as visible value storage
#ifdef WITH_SHORT_EQUIPMENT_STATS
			ArrayStatic<EquipmentStats, 16> pockets[Hand::MAX];
#endif
			Concurrency::SpinLock showInHandReasonsLock = Concurrency::SpinLock(TXT("TeaForGodEmperor.PilgrimOverlayInfo.EquipmentStats.showInHandReasonsLock"));
			ArrayStatic<TimedReason, 8> showInHandReasons[Hand::MAX];

			void reset();

			bool operator ==(EquipmentsStats const& _gs) const;
			bool operator !=(EquipmentsStats const& _gs) const { return !(operator ==(_gs)); }

			EquipmentsStats()
			{
				for_count(int, hIdx, Hand::MAX)
				{
#ifdef WITH_SHORT_EQUIPMENT_STATS
					SET_EXTRA_DEBUG_INFO(pockets[hIdx], TXT("PilgrimOverlayInfo::EquipmentsStats.pockets"));
#endif
					SET_EXTRA_DEBUG_INFO(showInHandReasons[hIdx], TXT("PilgrimOverlayInfo::EquipmentsStats.showInHandReasons"));
				}
			}
			void copy_equipment_stats_from(EquipmentsStats const& _source);
			void copy_in_hand_equipment_stats_from(EquipmentsStats const& _source);

		private:
			EquipmentsStats& operator=(EquipmentsStats const& _source); // do not implement
		};
		EquipmentsStats equipmentsStats;
		
		EquipmentStats workingEquipmentStats;
		EquipmentsStats workingEquipmentsStats; // it's a big structure to keep it on stack

		struct MissionUtils;
		friend struct MissionUtils;

		struct MissionInfo
		{
			RefCountObjectPtr<OverlayInfo::Element> missionTypeElement;
			struct Objective
			{
				RefCountObjectPtr<OverlayInfo::Element> requirement;
				RefCountObjectPtr<OverlayInfo::Element> reward;
				VisibleValue<int> resultInt;
			};
			Objective visitedCells;
			Objective visitedInterfaceBoxes;
			Objective bringItems;
			Objective hackBoxes;
			Objective infestations;
			Objective refills;
			PilgrimOverlayInfoRenderableData missionItemsRenderableData;
			MissionStateObjectives missionItemsDoneFor;
		} missionInfo;

		struct SymbolReplacement
		{
			Framework::UsedLibraryStored<Framework::TexturePart> from;
			Framework::UsedLibraryStored<Framework::TexturePart> to;
			float time = 0.0f;
			RefCountObjectPtr<OverlayInfo::Element> element;
		};
		Concurrency::SpinLock symbolReplacementsLock = Concurrency::SpinLock(TXT("TeaForGodEmperor.PilgrimOverlayInfo.symbolReplacementsLock"));
		Array<SymbolReplacement> symbolReplacements;

		RefCountObjectPtr<OverlayInfo::Element> turnCounterInfo;
		int turnCounterInfoShownForTurnCount = 0;

		Framework::UsedLibraryStored<Framework::Font> oiFont;
		Framework::UsedLibraryStored<Framework::Font> oiFontStats;

		Framework::UsedLibraryStored<Framework::TexturePart> oipLeft;
		Framework::UsedLibraryStored<Framework::TexturePart> oipRight;
		Framework::UsedLibraryStored<Framework::TexturePart> oipHealth;
		Framework::UsedLibraryStored<Framework::TexturePart> oipAmmo;
		Framework::UsedLibraryStored<Framework::TexturePart> oipStorage;
#ifdef WITH_CRYSTALITE
		Framework::UsedLibraryStored<Framework::TexturePart> oipCrystalite;
#endif
		Framework::UsedLibraryStored<Framework::TexturePart> oipExperience;
		Framework::UsedLibraryStored<Framework::TexturePart> oipMeritPoints;
		Framework::UsedLibraryStored<Framework::TexturePart> oipTime;
		Framework::UsedLibraryStored<Framework::TexturePart> oipDistance;

		Framework::UsedLibraryStored<Framework::TexturePart> oieHealth;
		
		Framework::UsedLibraryStored<Framework::TexturePart> oigShotCost;
		Framework::UsedLibraryStored<Framework::TexturePart> oigDamage;
		Framework::UsedLibraryStored<Framework::TexturePart> oigArmourPiercing;
		Framework::UsedLibraryStored<Framework::TexturePart> oigAntiDeflection;
		Framework::UsedLibraryStored<Framework::TexturePart> oigMagazineSize;
		Framework::UsedLibraryStored<Framework::TexturePart> oigSpread;
		
		Framework::UsedLibraryStored<Framework::TexturePart> oiKill;
		Framework::UsedLibraryStored<Framework::TexturePart> oiMelee;

		Framework::UsedLibraryStored<LineModel> oiPermanentEXM;
		Framework::UsedLibraryStored<LineModel> oiPermanentEXMp1;
		Framework::UsedLibraryStored<LineModel> oiPassiveEXM;
		Framework::UsedLibraryStored<LineModel> oiActiveEXM;
		Framework::UsedLibraryStored<LineModel> oiIntegralEXM;
		Framework::UsedLibraryStored<LineModel> oiInstallEXM;
		Framework::UsedLibraryStored<LineModel> oiMoveEXMThroughStack;
		Framework::UsedLibraryStored<LineModel> oiEXMToReplace;
		
		Framework::UsedLibraryStored<Framework::Sample> speakNewMapAvailable;
		Framework::UsedLibraryStored<Framework::Sample> speakSymbolUnobfuscated;
		Framework::UsedLibraryStored<Framework::Sample> speakUpgradeInstalled;
		Framework::UsedLibraryStored<Framework::Sample> speakHealthCritical;
		Framework::UsedLibraryStored<Framework::Sample> speakLowHealth;
		Framework::UsedLibraryStored<Framework::Sample> speakHalfHealth;
		Framework::UsedLibraryStored<Framework::Sample> speakFullHealth;
		Framework::UsedLibraryStored<Framework::Sample> speakLowAmmo;
		Framework::UsedLibraryStored<Framework::Sample> speakHalfAmmo;
		Framework::UsedLibraryStored<Framework::Sample> speakLowAmmoHand[Hand::MAX];
		Framework::UsedLibraryStored<Framework::Sample> speakHalfAmmoHand[Hand::MAX];
		Framework::UsedLibraryStored<Framework::Sample> speakAmmoCritical;
		Framework::UsedLibraryStored<Framework::Sample> speakAmmoCriticalHand[Hand::MAX];
		Framework::UsedLibraryStored<Framework::Sample> speakFullAmmo;
		Framework::UsedLibraryStored<Framework::Sample> speakFullAmmoHand[Hand::MAX];
		Framework::UsedLibraryStored<Framework::Sample> speakFollowGuidance;
		Framework::UsedLibraryStored<Framework::Sample> speakFollowGuidanceAmmo;
		Framework::UsedLibraryStored<Framework::Sample> speakFollowGuidanceHealth;
		Framework::UsedLibraryStored<Framework::Sample> speakFindEnergySource;
		Framework::UsedLibraryStored<Framework::Sample> speakSuccess;
		Framework::UsedLibraryStored<Framework::Sample> speakReceivingData;
		Framework::UsedLibraryStored<Framework::Sample> speakTransferComplete;
		Framework::UsedLibraryStored<Framework::Sample> speakInterfaceBoxFound;
		Framework::UsedLibraryStored<Framework::Sample> speakInfestationEnded;
		Framework::UsedLibraryStored<Framework::Sample> speakSystemStabilised;
		Colour speakWarningColour = Colour::red;

		struct Speaking
		{
			bool shouldUpdateSpeaking = false;
			mutable Concurrency::SpinLock speakingLock = Concurrency::SpinLock(TXT("TeaForGodEmperor.PilgrimOverlayInfo.Speaking.speakingLock"));
			Framework::Sample* speakingLine = nullptr;
			struct LineToSpeak
			{
				Framework::Sample* line = nullptr;
				Optional<Colour> colour;
				Optional<float> delay;
				bool clearLogOnSpeak = false;
				Optional<int> atLine;
				std::function<void()> performOnSpeak;
			};
			Array<LineToSpeak> linesToSpeak;
			float timeSinceLastSpeak = 1000.0f;
			bool allowAutoClear = true;
		} speaking;

		struct SpeakingContext
		{
			mutable Concurrency::SpinLock speakingLock = Concurrency::SpinLock(TXT("TeaForGodEmperor.PilgrimOverlayInfo.SpeakingContext.speakingLock"));

			Optional<Energy> health;
			Optional<Energy> healthTotal;
			Optional<Energy> energyStorages[Hand::MAX];

			// this is kind of a hack, explained in the source file
			float healthAvailableFor = 0.0f;
			float energyStoragesAvailableFor = 0.0f;

			bool blocked = false;
		} speakingContext;

		struct ObjectDescriptor
		{
			static int const MAX_ROOM_DEPTH = 8;
			enum MaxRoomDepth
			{
				PickUp = 3,
				Investigate = MAX_ROOM_DEPTH,
				HealthDamage = MAX_ROOM_DEPTH,
				NoDoorMarkers = 3,
			};

			struct Object
			: public SoftPooledObject<Object>
			, public RefCountObject
			{
				typedef SoftPooledObject<Object> base;
			public:
				typedef RefCountObjectPtr<Object> Ptr;

				bool valid = true;
				Framework::PresencePath path; // object is whatever there is at the end of the path
				struct DescriptorElement
				{
					RefCountObjectPtr<OverlayInfo::Element> element;
					CustomModules::InvestigatorInfoCachedData cachedData;
					Optional<int> healthRuleIdx; // used for mix
					Optional<int> iconIdx; // used for mix

					DescriptorElement() {}
					explicit DescriptorElement(OverlayInfo::Element* _e) : element(_e) {}
					DescriptorElement& for_health_rule(int _idx) { healthRuleIdx = _idx; return *this; }
					DescriptorElement& for_icon(int _idx) { iconIdx = _idx; return *this; }
				};
				ArrayStatic<DescriptorElement, 20> descriptorElements; // icons and stuff, displayed
				enum DescriptorElementIdx
				{
					DEI_Health,
					DEI_Armour,
					DEI_Ricochet,
					DEI_Icons,
					DEI_ExtraEffects,
					DEI_Mix // for all below
				};
				struct StartIndices
				{
					int healthRules = 0;
					int icons = 0; // counts only if has icon and is at socket
					int end = 0;
				} startIndices;
				ArrayStatic<CustomModules::InvestigatorInfoCachedData, INVESTIGATOR_INFO_PROVIDER_INFO_LIMIT> investigatorInfoProviderCachedData; // per info

				enum Description
				{
					None,
					PickUp,
					Investigate,
					NoDoorMarker,
					
					// always on, non replacable, will fade away on its own
					HealthDamage
				};
				Description description = None;
				Transform vrPlacement = Transform(Vector3(0.0f, 0.0f, -1000.0f), Quat::identity);
				bool shouldDeactivate = false;
				float forceKeepTimeLeft = 0.0f;

				//
				Framework::IModulesOwner* unsafeIMOForReference = nullptr; // just as reference, should not be accessed
				Optional<Energy> collectedDamage;
				bool collectedDamageShouldBeVisible = false;

				void deactivate_descriptor_element();

			protected: friend class SoftPooledObject<Object>;
				override_ void on_get();
				override_ void on_release();

			protected: friend class RefCountObject;
				override_ void destroy_ref_count_object() { SoftPooledObject<Object>::release(); }
			};

			Array<RefCountObjectPtr<Object>> objects;
			float timeToUpdate = 0.0f;
			float timeSinceLastUpdate = 0.0f;
		} objectDescriptor;

		void add_main(Framework::IModulesOwner* _owner, OverlayInfo::System* _overlayInfoSystem);
		void add_info_about_cell(Framework::IModulesOwner* _owner, OverlayInfo::System* _overlayInfoSystem, float _time);
		void add_show_info(Framework::IModulesOwner* _owner, OverlayInfo::System* _overlayInfoSystem);
		void redo_main_overlay_elements();

		void show_map(Framework::IModulesOwner* _owner, OverlayInfo::System* _overlayInfoSystem);
		void hide_map(OverlayInfo::System* _overlayInfoSystem);
		void update_disable_map_when_moves_too_far_from(OverlayInfo::System* _overlayInfoSystem);

		void update_symbol_replacements(float _deltaTime);
		
		void update_turn_counter_info();

		void add_pilgrim_stats(Framework::IModulesOwner* _owner, OverlayInfo::System* _overlayInfoSystem);
		void update_pilgrim_stats(Framework::IModulesOwner* _owner);

		void add_equipment_stats(Framework::IModulesOwner* _owner, OverlayInfo::System* _overlayInfoSystem);
		void update_equipment_stats(Framework::IModulesOwner* _owner);
		
		void update_investigate_info();

		void update_log(WhatToShow _whichLog, bool _clear = false, Optional<bool> const & _withLineIndicator = NP);
		void update_main_log(bool _clear = false, Optional<bool> const & _withLineIndicator = NP);
		void update_investigate_log(bool _clear = false);
		
		void update_subtitle_log();

		void update_exm_infos(Framework::IModulesOwner* _owner);

		void update_in_hand_equipment_stats(float _deltaTime);

		static void update_stat_overlay_element(RefCountObjectPtr<OverlayInfo::Element>& element, String const& _text);
		static void update_stat_overlay_element_blinking_low(RefCountObjectPtr<OverlayInfo::Element>& element, Energy const & _current, Energy const & _max);
		void add_pilgrim_stat_at(OverlayInfo::System* _overlayInfoSystem, RefCountObjectPtr<OverlayInfo::Element> & element, int _side, Transform const& _placement, float _size,
			Framework::TexturePart* _left = nullptr, Framework::TexturePart* _centre = nullptr, Framework::TexturePart* _right = nullptr);

		void update_equipment_stats(EquipmentStats& _stats, int _padLeft);
		void add_one_equipment_stats(OverlayInfo::System* _overlayInfoSystem, EquipmentStats& _stats, Name const& _as, Transform const& _at, float _size);
		void add_in_hand_equipment_stats(OverlayInfo::System* _overlayInfoSystem, EquipmentStats& _stats, Name const& _as, Hand::Type _inHand);
		void add_stat_at(OverlayInfo::System* _overlayInfoSystem, Name const& _id, Optional<Hand::Type> _inHand, String const& _text, OUT_ RefCountObjectPtr<OverlayInfo::Element>* out_element, Transform const& _placement, float _size, Framework::TexturePart* _texturePart = nullptr, String const& _header = String::empty(), Optional<Colour> const & _colour = NP);

		Transform get_vr_placement_for_world(Framework::IModulesOwner* _owner, Transform const& _localPlacement) const;
		Transform get_vr_placement_for_local(Framework::IModulesOwner* _owner, Transform const& _localPlacement, ReferencePlacement const* _usingRefPlacement = nullptr);

		static void draw_permanent_exm_for_overlay(int _idx, PilgrimOverlayInfo* _poi, float _active, float _pulse, Colour const& _colour);

		static void draw_exm_for_overlay(Hand::Type _hand, Optional<int> const & _passiveEXMIdx, PilgrimOverlayInfo* _poi, float _active, float _pulse, Colour const& _colour);
		static Transform calculate_exm_placement_for_overlay(Hand::Type _hand, Optional<int> const& _passiveEXMIdx, PilgrimOverlayInfo* _poi, Transform const& _offset);
		static Transform calculate_equipment_placement_for_overlay(Hand::Type _hand, PilgrimOverlayInfo* _poi, Transform const& _offset);

#ifdef AN_ALLOW_EXTENSIVE_LOGS
		static tchar const* to_char(WhatToShow _what);
#endif

		void configure_tip_oie_element(OverlayInfo::Element* _element, Rotator3 const& _offset, Optional<float> const& _preferredDistance = NP, Optional<bool> const & _followPitch = NP) const;

		void internal_show_in_hand_equipment_stats(Hand::Type _hand, OverlayInfo::System* overlayInfoSystem);
		void internal_show_permanent_exms(OverlayInfo::System* overlayInfoSystem);

		void set_reference_placement();
		void set_reference_placement_if_not_set();
		void clear_reference_placement_if_possible();
		static bool setup_reference_placement(Framework::IModulesOwner* _owner, REF_ ReferencePlacement& _refPlace);

		void internal_hide_exms(Name const& _reason);
		void internal_hide_permanent_exms(Name const& _reason);

		void update_speaking(float _deltaTime);

		void setup_overlay_element_placement(OverlayInfo::Element* e, Transform const& _placement, bool _forceVRPlacement = false);

		bool should_use_wide_stats() const;

		struct ScanForObjectDescriptorContext
		{
			bool lookForGuns = false;
			bool lookForPickups = false;
			bool lookForInvestigate = false;
			bool lookForHealthDamage = false;
			bool lookForNoDoorMarkers = false;

			bool should_look_for_anything() const { return lookForGuns || lookForPickups || lookForInvestigate || lookForHealthDamage || lookForNoDoorMarkers; }

			Framework::Room* pilgrimInRoom;
			Transform originalPilgrimViewPlacement;
			int throughDoorsLimit = 3;

			struct ChooseOne
			{
				float bestMatch = 0.0f;
				Framework::IModulesOwner* chosen = nullptr;
				ArrayStatic<Framework::DoorInRoom*, ObjectDescriptor::MAX_ROOM_DEPTH > chosenThroughDoors;
			};

			ChooseOne investigate;
		};
		struct ScanForObjectDescriptorKeepData
		{
			struct ChooseOne
			{
				float bestMatch = 0.0f;
				::SafePtr<Framework::IModulesOwner> chosen;
			};

			ChooseOne investigate;
		} scanForObjectDescriptorKeepData;
		void clear_object_descriptions();
		void advance_object_descriptor(float _deltaTime);
		void scan_room_object_descriptor(REF_ ScanForObjectDescriptorContext & _context, ScanForObjectDescriptorKeepData const & _keepData, Framework::Room* room, Transform const& _pilgrimPlacement, ArrayStatic<Framework::DoorInRoom*, ObjectDescriptor::MAX_ROOM_DEPTH> & _throughDoors, System::ViewFrustum const& _frustum, Transform const& _thisRoomTransform);
		bool is_relevant_to_object_descriptor(REF_ ScanForObjectDescriptorContext & _context, ScanForObjectDescriptorKeepData const& _keepData, Framework::IModulesOwner* imo, Transform const& _pilgrimPlacement, ArrayStatic<Framework::DoorInRoom*, ObjectDescriptor::MAX_ROOM_DEPTH > const & _throughDoors, OUT_ ObjectDescriptor::Object::Description& _description) const; // if returns true, should be added using _description
		void use_object_descriptor(Framework::IModulesOwner* o, ObjectDescriptor::Object::Description desc, ArrayStatic<Framework::DoorInRoom*, ObjectDescriptor::MAX_ROOM_DEPTH > const& _throughDoors);
	};

};
