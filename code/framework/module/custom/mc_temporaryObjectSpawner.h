#pragma once

#include "..\moduleCustom.h"
#include "..\..\presence\relativeToPresencePlacement.h"
#include "..\..\sound\soundSource.h"

#include "..\..\..\core\functionParamsStruct.h"
#include "..\..\..\core\vr\iVR.h"
#include "..\..\..\core\types\hand.h"

namespace Framework
{
	namespace CustomModules
	{
		class TemporaryObjectSpawnerData;

		/*
		 *	Use it where we can afford it as it will advance every frame
		 */
		class TemporaryObjectSpawner
		: public ModuleCustom
		{
			FAST_CAST_DECLARE(TemporaryObjectSpawner);
			FAST_CAST_BASE(ModuleCustom);
			FAST_CAST_END();

			typedef Framework::ModuleCustom base;
		public:
			static Framework::RegisteredModule<Framework::ModuleCustom> & register_itself();

			TemporaryObjectSpawner(Framework::IModulesOwner* _owner);
			virtual ~TemporaryObjectSpawner();

		public:	// Module
			override_ void reset();
			override_ void setup_with(::Framework::ModuleData const * _moduleData);

			override_ void initialise();
			override_ void activate();

			override_ void on_owner_destroy();

		protected: // ModuleCustom
			override_ void advance_post(float _deltaTime);

		protected:
			TemporaryObjectSpawnerData const * temporaryObjectSpawnerData = nullptr;

			struct Instance
			{
				Range interval = Range::empty;
				Range duration = Range::empty; // if empty/zero trigger single

				float intervalLeft = 0.0f;
				float durationLeft = 0.0f;
				
				Name temporaryObject;
				Name socket; // if provided

				Array<SafePtr<Framework::IModulesOwner>> spawned; // in case we have to stop them
			};
			Array<Instance> instances;
		};

		class TemporaryObjectSpawnerData
		: public ModuleData
		{
			FAST_CAST_DECLARE(TemporaryObjectSpawnerData);
			FAST_CAST_BASE(ModuleData);
			FAST_CAST_END();
			typedef ModuleData base;
		public:
			TemporaryObjectSpawnerData(LibraryStored* _inLibraryStored);

		public: // ModuleData
			override_ bool load_from_xml(IO::XML::Node const * _node, LibraryLoadingContext & _lc);
			override_ bool prepare_for_game(Library* _library, LibraryPrepareContext& _pfgContext);

		protected: // ModuleData
			override_ bool read_parameter_from(IO::XML::Attribute const * _attr, LibraryLoadingContext & _lc);
			override_ bool read_parameter_from(IO::XML::Node const * _node, LibraryLoadingContext & _lc);

		protected:	friend class TemporaryObjectSpawner;
	
			struct Element
			{
				Range interval = Range::empty;
				Range duration = Range::empty; // if empty/zero trigger single

				Name temporaryObject;
				Name socket; // if provided
			};
			Array<Element> elements;
		};
	};
};
