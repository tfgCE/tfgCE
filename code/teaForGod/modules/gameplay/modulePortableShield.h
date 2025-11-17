#pragma once

#include "..\moduleGameplay.h"
#include "..\shield.h"

#include "..\..\teaEnums.h"
#include "..\..\..\framework\appearance\socketID.h"

namespace TeaForGodEmperor
{
	class ModulePortableShieldData;

	class ModulePortableShield
	: public ModuleGameplay
	, public IShield
	{
		FAST_CAST_DECLARE(ModulePortableShield);
		FAST_CAST_BASE(ModuleGameplay);
		FAST_CAST_BASE(IShield);
		FAST_CAST_END();

		typedef ModuleGameplay base;
	public:
		static Framework::RegisteredModule<Framework::ModuleGameplay> & register_itself();

		ModulePortableShield(Framework::IModulesOwner* _owner);
		virtual ~ModulePortableShield();

	public: // IShield
		implement_ Vector3 get_cover_dir_os() const;

	protected: // Module
		override_ void setup_with(Framework::ModuleData const* _moduleData);

		override_ void initialise();

	protected: // ModuleGameplay
		override_ void advance_post_move(float _deltaTime);

	protected:
		Framework::SocketID shieldCoverSocket; // forward dir is cover

		Energy healthDepletionPerSecond = Energy::zero();
		float healthDepletionPerSecondMissingBit = 0.0f;

		bool playedStart = false;
		bool playedStop = false;

		float shieldOn = 0.0f;
		float shieldOnTarget = 1.0f;
		float prevShieldOn = -1.0f;

		Colour shieldStartColour = Colour::orange;
		float shieldStartLength = 0.5f;
		Colour shieldDepletedColour = Colour::white;
		float shieldDepletedLength = 0.2f;

		Colour shieldOverrideColour = Colour::white;
		float useShieldOverrideColour = 0.0f;
		float prevShieldOverrideColour = -1.0f;
		float useShieldOverrideColourLength = 0.0f;
		bool updateShieldOverrideColour = false;
	};

	class ModulePortableShieldData
	: public Framework::ModuleData
	{
		FAST_CAST_DECLARE(ModulePortableShieldData);
		FAST_CAST_BASE(Framework::ModuleData);
		FAST_CAST_END();

		typedef Framework::ModuleData base;
	public:
		ModulePortableShieldData(Framework::LibraryStored* _inLibraryStored);
		virtual ~ModulePortableShieldData();

	};
};

