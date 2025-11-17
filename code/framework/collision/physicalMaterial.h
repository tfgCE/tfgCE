#pragma once

#include "..\..\core\io\xml.h"
#include "..\..\core\collision\physicalMaterial.h"

#include "..\library\libraryStored.h"

namespace Framework
{
	interface_class IModulesOwner;
	class Room;

	class PhysicalMaterial
	: public LibraryStored
	, public Collision::PhysicalMaterial
	{
		FAST_CAST_DECLARE(PhysicalMaterial);
		FAST_CAST_BASE(LibraryStored);
		FAST_CAST_BASE(Collision::PhysicalMaterial);
		FAST_CAST_END();
		LIBRARY_STORED_DECLARE_TYPE();

		typedef LibraryStored base;
	public:
		PhysicalMaterial(Library * _library, LibraryName const & _name);

		void play_collision(IModulesOwner * _owner, Room const * _inRoom = nullptr, Optional<Vector3> const & _location = NP, Optional<float> const & _volume = NP) const;
		void play_foot_step(IModulesOwner * _owner, Room const * _inRoom, Transform const & _placement, Optional<float> const & _volume = NP) const;

	public:
		static PhysicalMaterial * cast(Collision::PhysicalMaterial * _pm) { return fast_cast<PhysicalMaterial>(_pm); }
		static PhysicalMaterial const * cast(Collision::PhysicalMaterial const * _pm) { return fast_cast<PhysicalMaterial>(_pm); }
		static PhysicalMaterial * get(Name const & _for = Name::invalid());
		static PhysicalMaterial * get(Collision::PhysicalMaterial * _pm, Name const & _for = Name::invalid());
		static PhysicalMaterial const * get(Collision::PhysicalMaterial const * _pm, Name const & _for = Name::invalid());

	public: // LibraryStored
		override_ bool load_from_xml(IO::XML::Node const * _node, LibraryLoadingContext & _lc);
		override_ bool prepare_for_game(Library* _library, LibraryPrepareContext& _pfgContext);

	public: // Collision::PhysicalMaterial
		override_ bool load_from_xml(IO::XML::Node const * _node, Collision::LoadingContext const & _loadingContext);
		override_ bool load_from_xml(IO::XML::Attribute const * _attr, Collision::LoadingContext const & _loadingContext);
		override_ void log_usage_info(LogInfoContext & _context) const;

	protected:
		virtual ~PhysicalMaterial();

	protected:
		Array<Name> collisionSounds; // as below
		Array<Name> footStepSounds; // will try playing one after another (using names, playing through stepping person's module sound), this is foot step based on the material, there might be also foot step played by character itself through walker sound (event)

	private:
		static Map<Name, PhysicalMaterial*>* s_defaults;
	};
};
