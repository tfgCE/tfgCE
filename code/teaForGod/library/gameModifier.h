#pragma once

#include "..\game\energy.h"
#include "..\game\gameSettings.h"

#include "..\..\framework\library\libraryStored.h"

namespace TeaForGodEmperor
{
	class GameModifier
	: public Framework::LibraryStored
	{
		FAST_CAST_DECLARE(GameModifier);
		FAST_CAST_BASE(Framework::LibraryStored);
		FAST_CAST_END();
		LIBRARY_STORED_DECLARE_TYPE();

		typedef Framework::LibraryStored base;
	public:
		static void initialise_static();
		static void close_static();

		static ArrayStatic<Name, 64> get_all_game_modifiers();
		static GameModifier const* get(Name const & _id);

		static void apply(Tags const& _gameModifers);

		static VectorInt2 get_game_modifiers_grid_size();

	public:
		GameModifier(Framework::Library* _library, Framework::LibraryName const& _name);
		virtual ~GameModifier();

		Name const& get_id() const { return id; }
		VectorInt2 const& get_at() const { return at; }

		Array<Name> const& get_disabled_when() const { return disabledWhen; }

		EnergyCoef const& get_experience_modifier() const { return experienceModifier; }

		GameSettings::IncompleteDifficulty const& get_difficulty_modifier() const { return difficultyModifier; }
		bool should_disallow_crouch() const { return disallowCrouch; }

		Framework::TexturePart const* get_texture_part_on() const { return texturePartOn.get(); }
		Framework::TexturePart const* get_texture_part_off() const { return texturePartOff.get(); }

		Framework::LocalisedString const& get_title() const { return lsTitle; }
		Framework::LocalisedString const& get_desc() const { return lsDesc; }

	public: // LibraryStored
		override_ bool load_from_xml(IO::XML::Node const* _node, Framework::LibraryLoadingContext& _lc);
		override_ bool prepare_for_game(Framework::Library* _library, Framework::LibraryPrepareContext& _pfgContext);

	private:
		Name id;
		VectorInt2 at = VectorInt2::zero; // top left
		TagCondition forGameDefinitionTagged;

		EnergyCoef experienceModifier = EnergyCoef::zero();

		GameSettings::IncompleteDifficulty difficultyModifier;
		bool disallowCrouch = false;

		Array<Name> disabledWhen;

		Framework::UsedLibraryStored<Framework::TexturePart> texturePartOn;
		Framework::UsedLibraryStored<Framework::TexturePart> texturePartOff;

		Framework::LocalisedString lsTitle;
		Framework::LocalisedString lsDesc;
	};
};
