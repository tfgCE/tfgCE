#pragma once

#include "..\moduleEquipment.h"
#include "..\..\shield.h"

#include "..\..\..\..\core\math\math.h"
#include "..\..\..\..\core\vr\iVR.h"
#include "..\..\..\..\framework\appearance\socketID.h"
#include "..\..\..\..\framework\module\moduleData.h"

namespace Framework
{
	class TemporaryObjectType;
};

namespace TeaForGodEmperor
{
	class ModulePilgrim;

	namespace ModuleEquipments
	{
		class ShieldData;

		class Shield
		: public ModuleEquipment
		, public IShield
		{
			FAST_CAST_DECLARE(Shield);
			FAST_CAST_BASE(ModuleEquipment);
			FAST_CAST_BASE(IShield);
			FAST_CAST_END();

			typedef ModuleEquipment base;
		public:
			static Framework::RegisteredModule<Framework::ModuleGameplay> & register_itself();

			Shield(Framework::IModulesOwner* _owner);
			virtual ~Shield();

		public: // IShield
			implement_ Vector3 get_cover_dir_os() const;

		public: // ModuleEquipment
			override_ void advance_post_move(float _deltaTime);
			override_ void advance_user_controls();

			override_ void ready_energy_quantum_context(EnergyQuantumContext & _eqc) const;

			override_ Optional<Energy> get_primary_state_value() const;
			override_ Optional<Energy> get_energy_state_value() const { return get_primary_state_value(); }

		protected: // Module
			override_ void reset();
			override_ void setup_with(Framework::ModuleData const * _moduleData);

			override_ void initialise();

		protected:
			ShieldData const * shieldData;

			SimpleVariableInfo beRetractedVar;
			SimpleVariableInfo beShieldVar;
			SimpleVariableInfo beShieldLeftHandVar;
			SimpleVariableInfo beShieldRightHandVar;

			Framework::SocketID shieldCoverSocket; // forward dir is cover
		};

		class ShieldData
		: public ModuleEquipmentData
		{
			FAST_CAST_DECLARE(ShieldData);
			FAST_CAST_BASE(ModuleEquipmentData);
			FAST_CAST_END();

			typedef ModuleEquipmentData base;
		public:
			ShieldData(Framework::LibraryStored* _inLibraryStored);
			virtual ~ShieldData();

		public: // Framework::ModuleData
			override_ bool load_from_xml(IO::XML::Node const * _node, Framework::LibraryLoadingContext & _lc);
			override_ bool prepare_for_game(Framework::Library* _library, Framework::LibraryPrepareContext& _pfgContext);
			override_ void prepare_to_unload();
		};
	};
};

