#pragma once

#include "pilgrimSetup.h"

#include "..\teaForGod.h"

#include "..\game\energy.h"
#include "..\game\gameSettings.h"
#include "..\game\weaponPart.h"

#include "..\..\core\memory\refCountObject.h"

#include "..\..\framework\library\libraryStored.h"

struct Tags;

namespace IO
{
	namespace XML
	{
		class Node;
	};
};

namespace Framework
{
	struct LibraryLoadingContext;
};

namespace TeaForGodEmperor
{
	class Pilgrimage;
	class WeaponCoreModifiers;
	struct PersistenceProgressLevel;

	struct PilgrimGear
	: public RefCountObject
	{
	public:
		struct ItemSetup
		{
			bool mainWeapon = false; // used only for inHand

			Random::Generator individualRandomGenerator;
			// weapon
			WeaponSetup weaponSetup; // one or another
			WeaponSetupTemplate weaponSetupTemplate;
			// item/actor
			Framework::UsedLibraryStored<Framework::ItemType> itemType;
			Framework::UsedLibraryStored<Framework::ActorType> actorType;
			SimpleVariableStorage variables; // there should be a way to store things, specific modules (gameplay, health) + ai logic, should have interfaces for load/save

			ItemSetup();

			void clear();
			bool is_used() const;

			bool load_from_xml(IO::XML::Node const* _node);
			bool resolve_library_links();
			bool save_to_xml(IO::XML::Node * _node) const;

			Framework::IModulesOwner* async_create_object(Framework::IModulesOwner* ownerAsObject) const;
			void store_object(Framework::IModulesOwner*);
		};

		struct HandSetup
		{
			ArrayStatic<PilgrimHandEXMSetup, MAX_PASSIVE_EXMS> passiveEXMs;
			PilgrimHandEXMSetup activeEXM;
			WeaponSetup weaponSetup; // one or another
			WeaponSetupTemplate weaponSetupTemplate;
			ItemSetup inHand;

			HandSetup();

			void clear();
			bool load_from_xml(IO::XML::Node const* _node);
			bool resolve_library_links();
			bool save_to_xml(IO::XML::Node* _node) const;
		};

	public:
		bool unlockAllEXMs = false;
		Array<Name> unlockedEXMs;
		Array<Name> permanentEXMs;
		HandSetup hands[Hand::MAX];
		ItemSetup pockets[MAX_POCKETS];
		PilgrimInitialEnergyLevels initialEnergyLevels;

		void clear();
		bool load_from_xml(IO::XML::Node const* _node);
		bool resolve_library_links();
		bool save_to_xml(IO::XML::Node* _node) const;
	};
};
