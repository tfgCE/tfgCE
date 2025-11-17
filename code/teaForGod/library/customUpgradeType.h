#pragma once

#include "..\game\energy.h"
#include "..\game\weaponSetup.h"

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
	class MissionGeneralProgress;
	struct UnlockableCustomUpgradesContext;

	/**
	 *	Custom upgrade type.
	 *  These can be used for multiple purposes:
	 *		unlocking mission types
	 *		unlocking biomes
	 *		enlarging armoury
	 *		buying weapons (added to armoury)
	 */
	class CustomUpgradeType
	: public Framework::LibraryStored
	{
		FAST_CAST_DECLARE(CustomUpgradeType);
		FAST_CAST_BASE(Framework::LibraryStored);
		FAST_CAST_END();
		LIBRARY_STORED_DECLARE_TYPE();

		typedef Framework::LibraryStored base;
	public:
		enum UpgradeType
		{
			Unknown,
			ArmouryEnlargement,
			MissionGeneralProgress,
			WeaponForArmoury,
		};

	public:
		CustomUpgradeType(Framework::Library * _library, Framework::LibraryName const & _name);

		static CustomUpgradeType const* find(Name const& _id);
		static void get_all(OUT_ Array< CustomUpgradeType const*>& _all); // sorted by name

		Name const& get_id() const { return id; }
		int get_order() const { return order; }

		UpgradeType get_upgrade_type() const { return upgradeType; }

		bool is_for_buying() const { return upgradeType == WeaponForArmoury; }

		Framework::LocalisedString const& get_ui_full_name() const { return uiFullName; }
		String const & get_ui_desc() const { return uiDesc.get(); }
		Name const& get_ui_desc_id() const { return uiDesc.get_id(); }

		Energy calculate_unlock_xp_cost() const;
		int calculate_unlock_merit_points_cost() const;

		int get_unlocked_count() const;
		int get_unlock_limit() const { return unlockLimit; }

		bool is_available_to_unlock(OPTIONAL_ UnlockableCustomUpgradesContext const * _context = nullptr) const; // with context it is more strict, checks more requirements

		bool process_unlock() const;

	public:
		LineModel* get_line_model() const { return lineModel.get(); }
		LineModel* get_line_model_for_display() const { return lineModelForDisplay.get()? lineModelForDisplay.get() : lineModel.get(); }
		LineModel* get_outer_line_model() const { return outerLineModel.get(); }

	public: // LibraryStored
		override_ bool load_from_xml(IO::XML::Node const * _node, Framework::LibraryLoadingContext & _lc);
		override_ bool prepare_for_game(Framework::Library* _library, Framework::LibraryPrepareContext& _pfgContext);

	protected:
		virtual ~CustomUpgradeType();

	private:
		Name id; // just makes it more convenient, also to process an unlock
		int order = 0; // to group stuff together, higher order means it will be earlier on the list
		UpgradeType upgradeType = UpgradeType::Unknown;
		int unlockLimit = 1;

		TagCondition forMissionsDefinitionTagged;

		struct CostMeritPoints
		{
			int flat = 0;
			// if flat is non 0, only consecutive will get added linear/square
			// if flat is 0, first unlock will get linear/square
			int linear = 0;
			int square = 0;
			int limit = 0;
		} costMeritPoints;

		Framework::LocalisedString uiFullName; // full, longer name
		Framework::LocalisedString uiDesc; // info in UI
		Framework::UsedLibraryStored<LineModel> lineModel;
		Framework::UsedLibraryStored<LineModel> lineModelForDisplay; // it might be slightly different
		Framework::UsedLibraryStored<LineModel> outerLineModel; // outer/secondary
		
		Framework::UsedLibraryStored<TeaForGodEmperor::MissionGeneralProgress> missionGeneralProgress; // for mgp it will use its cost
		
		WeaponSetupTemplate weaponSetupTemplate;
		TagCondition useWeaponPartTypesTagged;
	};
};
