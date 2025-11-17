#pragma once

#include "..\..\..\framework\library\usedLibraryStored.h"
#include "..\..\..\framework\module\moduleCustom.h"

namespace Framework
{
	class TexturePart;
};

namespace TeaForGodEmperor
{
	namespace CustomModules
	{
		/**
		 *	This is a module that sets gun/projectile related variables to a temporary object for appearance, resets etc
		 */
		class GunTemporaryObject
		: public Framework::ModuleCustom
		{
			FAST_CAST_DECLARE(GunTemporaryObject);
			FAST_CAST_BASE(Framework::ModuleCustom);
			FAST_CAST_END();

			typedef Framework::ModuleCustom base;
		public:
			static Framework::RegisteredModule<Framework::ModuleCustom> & register_itself();

			GunTemporaryObject(Framework::IModulesOwner* _owner);
			virtual ~GunTemporaryObject();

		protected: // Module
			override_ void reset();
			override_ void setup_with(Framework::ModuleData const* _moduleData);
			override_ void activate();

		private:
			bool autoGetEmissiveColours = true;
			Colour projectileEmissiveColour = Colour::none;
			Colour projectileEmissiveBaseColour = Colour::none;
		};
	};
};

