#pragma once

#include "..\game\energy.h"

#include "..\..\core\mesh\mesh3dInstance.h"
#include "..\..\core\types\hand.h"

#include "..\..\framework\library\libraryStored.h"
#include "..\..\framework\text\localisedString.h"

namespace Framework
{
	class Actor;
	class ActorType;
	class Mesh;
	class MeshGenerator;

	namespace AI
	{
		class Mind;
	};
};

namespace TeaForGodEmperor
{
	class LineModel;
	class ModulePilgrim;

	namespace EXMID
	{
		namespace Passive
		{
			Name const& weapon_collector();
			Name const& long_range_automap();
			Name const& energy_dispenser_drain();
			Name const& upgrade_machine_refill();
			Name const& energy_dispatcher();
			Name const& super_health_storage();
			Name const& instant_weapon_charge();
			Name const& vampire_fists();
			Name const& beggar_fists();
			Name const& projectile_energy_absorber();
			Name const& energy_retainer();
			Name const& another_chance();
		};
		namespace Active
		{
			Name const& investigator();
			Name const& reroll();
		};
		namespace Integral
		{
			Name const& anti_motion_sickness();
		};
	};

	/**
	 *	EXtension Module
	 *  Parameters are copied to actor.
	 *	Z is towards user
	 *	To get how it should be placed:
	 *		various sockets are used (from ModulePilgrimData::EXMSetup leftBottomAnchorSocket, centreAnchorSocket)
	 *		0,0,0 point as the centre of an exm
	 */
	class EXMType
	: public Framework::LibraryStored
	{
		FAST_CAST_DECLARE(EXMType);
		FAST_CAST_BASE(Framework::LibraryStored);
		FAST_CAST_END();
		LIBRARY_STORED_DECLARE_TYPE();

		typedef Framework::LibraryStored base;
	public:
		static bool does_player_have_equipped(Name const& _exm);

	public:
		EXMType(Framework::Library * _library, Framework::LibraryName const & _name);

		static EXMType const* find(Name const& _id);
		static Array<Name> get_all();
		static Array<Name> get_all(TagCondition const & _tagCondition, Optional<RangeInt> const& _allowedEXMLevels = NP);
		static Array<EXMType*> get_all_exms(TagCondition const& _tagCondition, Optional<RangeInt> const & _allowedEXMLevels = NP);

		Name const& get_id() const { return id; }

		Framework::LocalisedString const& get_ui_name() const { return uiName; }
		Framework::LocalisedString const& get_ui_full_name() const { return uiFullName; }
		String get_ui_desc() const;
		Name const& get_ui_desc_id() const { return uiDesc.get_id(); }

		bool is_permanent() const { return isPermanent; }
		bool is_active() const { return isActive; }
		bool is_integral() const { return isIntegral; }
		bool can_be_activated_at_any_time() const { return isActive && canBeActivatedAtAnyTime; }

		int get_level() const { return level; }
		
		Array<Name> const& get_permanent_prerequisites() const { return permanentPrerequisites; }
		int get_permanent_limit() const { return permanentLimit; }

		void request_input_tip(ModulePilgrim* mp, Hand::Type heldByHand) const;

	public:
		Framework::Actor * create_actor(Framework::Actor* _owner, Hand::Type _hand) const;
		Framework::ActorType* get_actor_type() const { return actorType.get(); }

		LineModel* get_line_model() const { return lineModel.get(); }
		LineModel* get_line_model_for_display() const { return lineModelForDisplay.get()? lineModelForDisplay.get() : lineModel.get(); }

	public: // LibraryStored
		override_ bool load_from_xml(IO::XML::Node const * _node, Framework::LibraryLoadingContext & _lc);
		override_ bool prepare_for_game(Framework::Library* _library, Framework::LibraryPrepareContext& _pfgContext);

	protected:
		virtual ~EXMType();

	private:
		Framework::LocalisedString uiName; // how it should be displayed on ui
		Framework::LocalisedString uiFullName; // full, longer name
		Framework::LocalisedString uiDesc; // info in UI
		Framework::LocalisedString uiInputTip; // if not empty, used as an input tip
		Name id; // used to identify on the installed list and also to provide sorting for UI (the best is to have a hierarchical system for that)
				 // NEVER CHANGE THAT WHEN PUBLISHED AN EXM
		Framework::UsedLibraryStored<Framework::ActorType> actorType;
		Framework::UsedLibraryStored<Framework::AI::Mind> aiMind; // overrides one in actor type
		bool isPermanent = false; // different kind of exm, can't be removed
		bool isActive = false; // active means that requires to be chosen and activated with "useEXM" input
		bool isIntegral = false; // through options
		bool canBeActivatedAtAnyTime = true; // if active and can't be activated at any time (because it doesn't make sense to do so), this should be set to false
		int level = -1; // if -2 or -1 then always unlocked, if -2 doesn't even mention need to unlock
		Framework::UsedLibraryStored<LineModel> lineModel;
		Framework::UsedLibraryStored<LineModel> lineModelForDisplay; // it might be slightly different
		Array<Name> permanentPrerequisites; // exms one should have to allow for permanent to be available
		int permanentLimit = 1; // how many of the same can there be
	};
};
