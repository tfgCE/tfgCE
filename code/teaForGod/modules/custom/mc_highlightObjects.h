#pragma once

#include "..\..\..\core\system\video\materialInstance.h"
#include "..\..\..\core\types\hand.h"
#include "..\..\..\framework\module\moduleCustom.h"
#include "..\..\..\framework\module\moduleData.h"

namespace Framework
{
	class Material;
	namespace Render
	{
		class SceneBuildContext;
	};
};

namespace TeaForGodEmperor
{
	namespace CustomModules
	{
		class HighlightObjectsData;

		class HighlightObjects
		: public Framework::ModuleCustom
		{
			FAST_CAST_DECLARE(HighlightObjects);
			FAST_CAST_BASE(Framework::ModuleCustom);
			FAST_CAST_END();

			typedef Framework::ModuleCustom base;
		public:
			static Framework::RegisteredModule<Framework::ModuleCustom> & register_itself();

			HighlightObjects(Framework::IModulesOwner* _owner);
			virtual ~HighlightObjects();

			void setup_scene_build_context(Framework::Render::SceneBuildContext& _context);

		public:	// Module
			override_ void reset();
			override_ void setup_with(Framework::ModuleData const * _moduleData);

			override_ void initialise();

		protected: // ModuleCustom
			implement_ void advance_post(float _deltaTime);

		protected:
			HighlightObjectsData const * moduleData = nullptr;

			struct ObjectToHighlight
			{
				SafePtr<Framework::IModulesOwner> imo; // if not set - not used, if set - used
				::System::MaterialInstance materialInstance;
				Hand::Type hand;
				Name reason;

				float individualOffset = 0.0f;
				float state = 0.0f;
				float target = 1.0f;
			};
			static const int MaxObjectsToHighlight = 16;
			ObjectToHighlight objectsToHighlight[MaxObjectsToHighlight];
		};

		class HighlightObjectsData
		: public Framework::ModuleData
		{
			FAST_CAST_DECLARE(HighlightObjectsData);
			FAST_CAST_BASE(Framework::ModuleData);
			FAST_CAST_END();

			typedef Framework::ModuleData base;
		public:
			HighlightObjectsData(Framework::LibraryStored* _inLibraryStored);
			virtual ~HighlightObjectsData();

		protected: // Framework::ModuleData
			override_ bool read_parameter_from(IO::XML::Attribute const * _attr, Framework::LibraryLoadingContext & _lc);
			override_ bool prepare_for_game(Framework::Library* _library, Framework::LibraryPrepareContext& _pfgContext);
			override_ void prepare_to_unload();

		private:
			Framework::UsedLibraryStored<Framework::Material> handMaterials[Hand::MAX];
			Framework::UsedLibraryStored<Framework::Material> damagedMaterial;
			Name highlightParamName;
			Name instanceOffsetParamName;

			friend class HighlightObjects;
		};
	};
};

