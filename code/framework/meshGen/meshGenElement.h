#pragma once

#include "..\..\core\fastCast.h"
#include "..\..\core\containers\array.h"
#include "..\..\core\other\simpleVariableStorage.h"
#include "..\..\core\wheresMyPoint\wmp.h"
#include "..\..\core\memory\refCountObject.h"
#include "..\..\core\memory\refCountObjectPtr.h"

#include "meshGenCheckpoint.h"
#include "meshGenGenerationCondition.h"
#include "meshGenParam.h"

namespace IO
{
	namespace XML
	{
		class Node;
	};
};

#define error_generating_mesh(_instance, msg, ...) { { ScopedOutputLock outputLock; output_colour(1,0,0,1); trace_func(TXT("[ERROR in mesh generation] %S : "), (_instance).get_element()->get_location_info().to_char()); report_func(msg, ##__VA_ARGS__); output_colour(); } debug_on_error(); }
#define warn_generating_mesh(_instance, msg, ...) { { ScopedOutputLock outputLock; output_colour(1,1,0,1); trace_func(TXT("[warning in mesh generation] %S : "), (_instance).get_element()->get_location_info().to_char()); report_func(msg, ##__VA_ARGS__); output_colour(); } /*debug_on_warn();*/ }
#define error_generating_mesh_element(_element, msg, ...) { { ScopedOutputLock outputLock; output_colour(1,0,0,1); Framework::MeshGeneration::Element const * element = _element; trace_func(TXT("[ERROR in mesh generation] %S : "), element? element->get_location_info().to_char(): TXT("??")); report_func(msg, ##__VA_ARGS__); output_colour(); } debug_on_error(); }
#define warn_generating_mesh_element(_element, msg, ...) { { ScopedOutputLock outputLock; output_colour(1,1,0,1); Framework::MeshGeneration::Element const * element = _element; trace_func(TXT("[warning in mesh generation] %S : "), element? element->get_location_info().to_char(): TXT("??")); report_func(msg, ##__VA_ARGS__); output_colour(); } /*debug_on_warn();*/ }

namespace Framework
{
	class MeshGenerator;
	class Library;
	struct LibraryLoadingContext;
	struct LibraryPrepareContext;

	namespace MeshGeneration
	{
		class Element;

		struct GenerationContext;

		/**
		 *	Instance for duration of generation
		 */
		struct ElementInstance
		: public WheresMyPoint::IOwner
		{
			FAST_CAST_DECLARE(ElementInstance);
			FAST_CAST_BASE(WheresMyPoint::IOwner);
			FAST_CAST_END();

			typedef WheresMyPoint::IOwner base;
		public:
			GenerationContext & access_context() { return context; }
			GenerationContext const & get_context() const { return context; }
			Element const * get_element() const { return element; }

			int get_material_index_from_params() const;

		public: // WheresMyPoint::IOwner
			override_ bool store_value_for_wheres_my_point(Name const & _byName, WheresMyPoint::Value const & _value);
			override_ bool restore_value_for_wheres_my_point(Name const & _byName, OUT_ WheresMyPoint::Value & _value) const;
			override_ bool store_convert_value_for_wheres_my_point(Name const& _byName, TypeId _to);
			override_ bool rename_value_forwheres_my_point(Name const& _from, Name const& _to);
			override_ bool store_global_value_for_wheres_my_point(Name const & _byName, WheresMyPoint::Value const & _value);
			override_ bool restore_global_value_for_wheres_my_point(Name const & _byName, OUT_ WheresMyPoint::Value & _value) const;
			override_ bool get_point_for_wheres_my_point(Name const & _byName, OUT_ Vector3 & _point) const; // shouldn't be called!

			override_ WheresMyPoint::IOwner* get_wmp_owner();

		public:
			bool update_wmp_directly(); // function for those that want to update directly

		private: friend struct GenerationContext;
			ElementInstance(GenerationContext & _context, Element const * _element, ElementInstance* _parent = nullptr); // use GenerationContext's process

			bool process(); // this calls element's process but also applies all standard parameters!

		private:
			GenerationContext & context;
			Element const * element = nullptr;
			ElementInstance * parent = nullptr;

#ifdef AN_DEVELOPMENT
			bool updatedWMPDirectly;
#endif
		};

		class Element
		: public RefCountObject
		{
			FAST_CAST_DECLARE(Element);
			FAST_CAST_END();

		public:
			enum UpdateWMPPhase
			{
				UpdateWMPPreProcess,
				UpdateWMPPostProcess,
				UpdateWMPDirectly,
			};

			virtual ~Element() {}

			virtual bool is_stateless() const { return true; }

		public:
			static bool load_single_element_from_xml(IO::XML::Node const * _node, LibraryLoadingContext & _lc, tchar const * const _childContainerName, RefCountObjectPtr<Element> & _element);
			static bool load_single_element_from_xml(IO::XML::Node const * _node, LibraryLoadingContext & _lc, RefCountObjectPtr<Element> & _element);

			static Element* create_from_xml(IO::XML::Node const * _node, LibraryLoadingContext & _lc); // handles different types

		public:
			virtual bool is_xml_node_handled(IO::XML::Node const * _node) const; // to allow having everything at same level (without need to have "elements" node inside)
			virtual bool load_from_xml(IO::XML::Node const * _node, LibraryLoadingContext & _lc);
			virtual bool prepare_for_game(Library* _library, LibraryPrepareContext& _pfgContext);
#ifdef AN_DEVELOPMENT
			virtual void for_included_mesh_generators(std::function<void(MeshGenerator*)> _do) const {}
#endif
			String const & get_location_info() const;

			virtual bool should_use_random_generator_stack() const { return true /* true means: use stack when processing this element */; }

		private: friend struct ElementInstance;
			virtual bool process(GenerationContext & _context, ElementInstance & _instance) const; // process through generation context! (it will go through element instance!)
			virtual bool post_process(GenerationContext & _context, ElementInstance & _instance) const; // post process, post last WMP

		protected:
			UpdateWMPPhase updateWMPPhase = UpdateWMPPreProcess;
			bool useStack = true; // if true, will push and pop stack - use this to have access to parameters (altering them) from previous elements
			bool useCheckpoint = true; // if true will push and pop checkpoint - use this to have access to parts etc from previous elements, use with care as it may break applying of standard parameters (autoPlacement etc), consider using rayCast's againstCheckpointsUp

			bool update_wmp(ElementInstance & _instance, WheresMyPoint::ToolSet const * _processor = nullptr /* &wheresMyPointProcessor */) const;

		public:
			// these should be only called by something that is going to manage checkpoint/stack on its own (like modifier elementOnResult)
			void skip_using_stack() { useStack = false; }
			void skip_using_checkpoint() { useCheckpoint = false; }

		protected:
			SimpleVariableStorage parameters;
			SimpleVariableStorage requiredParameters;
			SimpleVariableStorage optionalParameters; // if not provided will enter with default value, won't issue error nor warning
			struct OutputParameter
			{
				TypeId type;
				Name name;
				Name storeAs;
			};
			Array<OutputParameter> outputParameters; // will be provided up, after reverting stack


		private:
			mutable Concurrency::SpinLock processLock/* = do not check limits, this may take a while Concurrency::SpinLock(TXT("Framework.MeshGeneration.Element.processLock")) */; // allow only one to do at the same time

			RefCountObjectPtr<Element> basedOn;

			bool randomiseSeed = false;

			bool dropMovementCollision = false;
			bool dropPreciseCollision = false;

			GenerationCondition generationCondition;

			WheresMyPoint::ToolSet wheresMyPointProcessor; // allows to modify or set or even add values
			WheresMyPoint::ToolSet wheresMyPointProcessorPost;

			MeshGenParam<int> forceUseRandomSeed;

#ifdef AN_DEVELOPMENT
			String locationInfo;
#endif
		};

	};
};
