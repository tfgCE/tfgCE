#pragma once

#include "..\moduleCustom.h"
#include "..\..\display\displayType.h"
#include "..\..\display\displayTypes.h"
#include "..\..\video\material.h"

namespace Framework
{
	namespace CustomModules
	{
		class DisplayData;

		class Display
		: public ModuleCustom
		{
			FAST_CAST_DECLARE(Display);
			FAST_CAST_BASE(ModuleCustom);
			FAST_CAST_END();

			typedef Framework::ModuleCustom base;
		public:
			static Framework::RegisteredModule<Framework::ModuleCustom> & register_itself();

			Display(Framework::IModulesOwner* _owner);
			virtual ~Display();

			Framework::Display* get_display() const { return display.get(); }

			void update_display_in(::System::MaterialInstance & _materialInstance) const;

		public:	// Module
			override_ void reset();
			override_ void setup_with(::Framework::ModuleData const * _moduleData);

			override_ void initialise();

		protected: // ModuleCustom
			override_ void advance_post(float _deltaTime);

		protected:
			DisplayData const * displayData = nullptr;

			// these are used to replace with actual display
			UsedLibraryStored<Material> useWithMaterial;
			Name textureParamName;

			UsedLibraryStored<Framework::DisplayType> displayType;
			VectorInt2 displayMaxOutputSize = VectorInt2(1024, 1024);
			float displayOutputSizeMultiplier = 1.0f;
			VectorInt2 displayScreenSize = VectorInt2(-1,-1);
			VectorInt2 displayBorderSize = VectorInt2(-1, -1);
			float displayStartingRetraceAtPct = 0.0f;
			Optional<Colour> displayInk;
			Optional<Colour> displayPaper;

#ifdef USE_DISPLAY_SHADOW_MASK
			UsedLibraryStored<Framework::Texture> shadowMask;
			Optional<Vector2> shadowMaskPerPixel;
			Name shadowMaskTextureParamName;
			Name shadowMaskSizeParamName;
#endif

			mutable bool displayVisible = false;
			mutable UsedLibraryStored<Framework::Texture> displayOffTexture;

			Framework::DisplayPtr display;

			struct InitDisplay;
		};

		class DisplayData
		: public ModuleData
		{
			FAST_CAST_DECLARE(DisplayData);
			FAST_CAST_BASE(ModuleData);
			FAST_CAST_END();
			typedef ModuleData base;
		public:
			DisplayData(LibraryStored* _inLibraryStored);

		public: // ModuleData
			override_ bool load_from_xml(IO::XML::Node const * _node, LibraryLoadingContext & _lc);
			override_ bool prepare_for_game(Library* _library, LibraryPrepareContext& _pfgContext);

		protected: // ModuleData
			override_ bool read_parameter_from(IO::XML::Attribute const * _attr, LibraryLoadingContext & _lc);
			override_ bool read_parameter_from(IO::XML::Node const * _node, LibraryLoadingContext & _lc);

		protected:	friend class Display;
			UsedLibraryStored<Material> useWithMaterial;
			Name textureParamName;
			UsedLibraryStored<Framework::DisplayType> displayType;
			VectorInt2 displayMaxOutputSize = VectorInt2(1024, 1024);
			float displayOutputSizeMultiplier = 1.0f;
			VectorInt2 displayScreenSize = VectorInt2(-1, -1);
			VectorInt2 displayBorderSize = VectorInt2(-1, -1);
			float displayStartingRetraceAtPct = 0.0f;
			Optional<Colour> displayInk;
			Optional<Colour> displayPaper;

#ifdef USE_DISPLAY_SHADOW_MASK
			UsedLibraryStored<Framework::Texture> shadowMask;
			Vector2 shadowMaskPerPixel = Vector2::zero;
			Name shadowMaskTextureParamName;
			Name shadowMaskSizeParamName;
#endif
		};
	};
};

