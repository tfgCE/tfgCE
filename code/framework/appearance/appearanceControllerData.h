#pragma once

#include "..\meshGen\meshGenParam.h"

#include "..\..\core\fastCast.h"
#include "..\..\core\mesh\boneRenameFunc.h"
#include "..\..\core\memory\refCountObject.h"

#define GET_VALUE_FROM_APPEARANCE_CONTROLLER_DATA(variable) variable = _data->variable.get();
#define GET_VALUE_FROM_APPEARANCE_CONTROLLER_DATA_OPTIONAL(variable) if (_data->variable.is_set()) { variable = _data->variable.get(); }

struct Name;
struct Matrix44;

namespace IO
{
	namespace XML
	{
		class Node;
	};
};

namespace WheresMyPoint
{
	struct Value;
};

namespace Framework
{
	class AppearanceController;
	class Library;
	struct LibraryLoadingContext;
	class ModuleAppearance;

	struct LibraryPrepareContext;

	namespace MeshGeneration
	{
		struct GenerationContext;
	};

	class AppearanceControllerData
	: public RefCountObject
	{
		FAST_CAST_DECLARE(AppearanceControllerData);
		FAST_CAST_END();
	public:
		AppearanceControllerData();
		virtual ~AppearanceControllerData();

#ifdef AN_DEVELOPMENT
	public:
		tchar const * get_location_info() const { return locationInfo.to_char(); }
		tchar const * get_desc() const { return desc.to_char(); }
		int get_process_at_level() const { return processAtLevel.get(); }

		static int compare_process_at_level(void const * _a, void const * _b);
#endif

	public: // loading, preparing, creating controller
		virtual bool load_from_xml(IO::XML::Node const * _node, LibraryLoadingContext & _lc);
		virtual bool prepare_for_game(Library* _library, LibraryPrepareContext& _pfgContext);

		virtual void apply_transform(Matrix44 const & _transform) {}
		virtual void reskin(Meshes::BoneRenameFunc) {} // it's called reskin but may be used to rename variables too!
		virtual void apply_mesh_gen_params(MeshGeneration::GenerationContext const & _context);
		virtual AppearanceControllerData* create_copy() const;

		virtual AppearanceController* create_controller();

	private:
#ifdef AN_DEVELOPMENT
		String locationInfo;
		String desc;
#endif
		// float only vars!
		Array<MeshGenParam<Name>> activeVars;
		Array<MeshGenParam<Name>> blockActiveVars; // to block being active
		Array<MeshGenParam<Name>> holdVars; // don't do anything new, just hold last result - this may be not implemented by everyone
		MeshGenParam<int> processAtLevel; // see AppearanceController
		MeshGenParam<Name> activeBlendCurve = Name::invalid();
		MeshGenParam<float> activeMask = 1.0f;
		MeshGenParam<float> activeInitial = 1.0f;
		MeshGenParam<float> activeBlendInTime = 0.0f;
		MeshGenParam<float> activeBlendOutTime = 0.0f;
		MeshGenParam<Name> activeBlendTimeVar;
		MeshGenParam<Name> outActiveVar; // to have feedback about the state of being active or not

		friend class AppearanceController;
	};
};
