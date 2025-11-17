#pragma once

#include "moduleData.h"

#include "..\..\core\math\math.h"

// check moduleMovement.h
#define LOAD_GAITS_PROP(type, propName, defaultValue) \
	if (_node->get_name() == NAME(propName).to_char()) \
	{ \
		bool provideDefault = (!_gait.params.find_existing<type>(NAME(propName))); \
		auto & param = _gait.params.access<type>(NAME(propName)); \
		if (provideDefault) param = defaultValue; \
		return RegisteredType::load_from_xml(_node, ::type_id<type>(), &param); \
	} \
	if (_gait.isBase) \
	{ \
		if (!_gait.params.find_existing<type>(NAME(propName))) \
		{ \
			auto & param = _gait.params.access<type>(NAME(propName)); \
			param = defaultValue; \
		} \
	}

#define LOAD_GAITS_PROP_BOOL_SET(propName) \
	if (_node->get_name() == NAME(propName).to_char()) \
	{ \
		_gait.params.access<bool>(NAME(propName)) = true; \
		return true; \
	}

namespace Framework
{
	class ModuleMovementData
	: public ModuleData
	{
		FAST_CAST_DECLARE(ModuleMovementData);
		FAST_CAST_BASE(ModuleData);
		FAST_CAST_END();
		typedef ModuleData base;
	public:
		struct Gait
		{
			bool isBase = false;
			Name name;
			SimpleVariableStorage params;

			Gait();
			bool load_from_xml(IO::XML::Node const * _node, LibraryLoadingContext & _lc, ModuleMovementData* _forData);
		};

	public:
		ModuleMovementData(LibraryStored* _inLibraryStored);
		virtual ~ModuleMovementData();

		Gait const * find_gait(Name const & _gaitName) const;
		
#ifdef AN_DEVELOPMENT_OR_PROFILER
		Array<Name> list_gaits() const;
#endif

	public: // ModuleData
		override_ bool read_parameter_from(IO::XML::Node const * _node, LibraryLoadingContext & _lc);
		override_ bool read_parameter_from(IO::XML::Attribute const * _attr, LibraryLoadingContext & _lc);
		override_ bool prepare_for_game(Library* _library, LibraryPrepareContext& _pfgContext);

	protected:
		virtual bool load_gait_param_from_xml(ModuleMovementData::Gait & _gait, IO::XML::Node const * _node, LibraryLoadingContext & _lc);

	protected:	friend class ModuleMovement;
		Map<Name, Gait*> gaits;
		Gait* defaultGait;
		Gait baseGait; // definition for fallbacks etc

	protected:
		Name movementName;

		struct AlignWithGravity
		{
			Name type;
			// will sum up starting with highest priority and all weights left (if any) will be used by lower priorities
			int priority = 0;
			float weight = 1.0f;
			bool requiresPresenceGravityTraces = false;

			bool load_from_xml(IO::XML::Node const * _node, LibraryLoadingContext & _lc);
		};
		Map<Name, AlignWithGravity> alignWithGravity; // this might be used by other stuff, like walkers, etc. "walker" stores its own align with gravity
		AlignWithGravity moduleAlignWithGravity; // as module movement provides 
		// when we have all "align with gravity" stored, we may calculate the actual align with gravity

		AlignWithGravity const * find_align_with_gravity(Name const & _type) const;

		Array<Name> disallowMovementLinearVars;
	};
};


