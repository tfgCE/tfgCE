#pragma once

#include "..\..\core\types\hand.h"

#include "..\..\framework\library\libraryStored.h"
#include "..\..\framework\text\localisedString.h"

namespace Framework
{
	class Actor;
	class ActorType;

	namespace AI
	{
		class Mind;
	};
};

namespace TeaForGodEmperor
{
	/**
	 *	Fore Arm Display
	 *  Parameters are copied to actor.
	 *	FAD should have precise collision bounding box in appearance - it is used to get the size of a FAD.
	 *	Forward is towards a user, up is user's up.
	 *	It has a variable fadHand (int) to indicate at which hand it is.
	 */
	class FADType
	: public Framework::LibraryStored
	{
		FAST_CAST_DECLARE(FADType);
		FAST_CAST_BASE(Framework::LibraryStored);
		FAST_CAST_END();
		LIBRARY_STORED_DECLARE_TYPE();

		typedef Framework::LibraryStored base;
	public:
		FADType(Framework::Library * _library, Framework::LibraryName const & _name);

		static FADType const * find(Name const & _id);
		Name const & get_id() const { return id; }

		Framework::LocalisedString const & get_ui_name() const { return uiName; }
		String const & get_ui_desc() const { return uiDesc.get(); }
		Name const & get_ui_desc_id() const { return uiDesc.get_id(); }

	public:
		Framework::Actor * create_actor(Framework::Actor* _owner, Hand::Type _hand) const;

	public: // LibraryStored
		override_ bool load_from_xml(IO::XML::Node const * _node, Framework::LibraryLoadingContext & _lc);
		override_ bool prepare_for_game(Framework::Library* _library, Framework::LibraryPrepareContext& _pfgContext);

	protected:
		virtual ~FADType();

	private:
		Framework::LocalisedString uiName; // how it should be displayed on ui
		Framework::LocalisedString uiDesc; // info in UI
		Name id; // more friendly identifier, this should allow to override_ FAD types (basing on priority)
		int priority = 0;
		Framework::UsedLibraryStored<Framework::ActorType> actorType;
		Framework::UsedLibraryStored<Framework::AI::Mind> aiMind; // overrides one in actor type
	};
};
