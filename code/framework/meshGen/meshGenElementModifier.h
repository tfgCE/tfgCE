#pragma once

#include "meshGenElement.h"

namespace IO
{
	namespace XML
	{
		class Node;
	};
};

namespace Meshes
{
	class Mesh3D;
	struct Mesh3DPart;
};

#define ELEMENT_MESH_MODIFIER_PROCESS_FN(_var) bool (*_var)(GenerationContext & _context, ElementInstance & _instigatorInstance, Element const * _subject, ::Framework::MeshGeneration::ElementModifierData const * _data)
#define ELEMENT_MESH_MODIFIER_CREATE_DATA_FN(_var) ::Framework::MeshGeneration::ElementModifierData* (*_var)()

#define REGISTER_MESH_GEN_MODIFIER(name, updateWMPPhase, process_modifier, create_data) \
	RegisteredElementModifiers::add(RegisteredElementModifier(Name(TXT(#name)), updateWMPPhase, process_modifier, create_data).temp_ref());

namespace Framework
{
	namespace MeshGeneration
	{
		class ElementModifierData
		{
			FAST_CAST_DECLARE(ElementModifierData);
			FAST_CAST_END();
		public:
			virtual ~ElementModifierData() {}

			virtual bool load_from_xml(IO::XML::Node const * _node, LibraryLoadingContext & _lc) = 0;
			virtual bool prepare_for_game(Library* _library, LibraryPrepareContext& _pfgContext) { return true; }
		};

		class ElementModifier
		: public Element
		{
			FAST_CAST_DECLARE(ElementModifier);
			FAST_CAST_BASE(Element);
			FAST_CAST_END();

			typedef Element base;
		public:
			virtual ~ElementModifier();

			ElementModifierData const * get_data() const { return data; }

		private:
			override_ bool load_from_xml(IO::XML::Node const * _node, LibraryLoadingContext & _lc);
			override_ bool prepare_for_game(Library* _library, LibraryPrepareContext& _pfgContext);
#ifdef AN_DEVELOPMENT
			override_ void for_included_mesh_generators(std::function<void(MeshGenerator*)> _do) const;
#endif

			override_ bool process(GenerationContext & _context, ElementInstance & _instance) const;

		private:
			Name modifier;
			ELEMENT_MESH_MODIFIER_PROCESS_FN(process_modifier) = nullptr;
			ElementModifierData* data = nullptr;

			RefCountObjectPtr<Element> element; // if no element, will modify everything that was done at this level before it
			RefCountObjectPtr<Element> elementOnResult; // "element on result", we will create a checkpoint and go deeper processing in elementOnResult elements newly created by this modifier
		};

		struct RegisteredElementModifier
		{
			Name name;
			Element::UpdateWMPPhase updateWMPPhase;
			ELEMENT_MESH_MODIFIER_PROCESS_FN(process_modifier) = nullptr;
			ELEMENT_MESH_MODIFIER_CREATE_DATA_FN(create_data) = nullptr;

			RegisteredElementModifier(Name const & _name, Element::UpdateWMPPhase _updateWMPPhase, ELEMENT_MESH_MODIFIER_PROCESS_FN(_process_modifier), ELEMENT_MESH_MODIFIER_CREATE_DATA_FN(_create_data) = nullptr) : name(_name), updateWMPPhase(_updateWMPPhase), process_modifier(_process_modifier), create_data(_create_data) {}
			RegisteredElementModifier & temp_ref() { return *this; }
		};

		class RegisteredElementModifiers
		{
		public:
			static void initialise_static();
			static void close_static();

			static RegisteredElementModifier const * find(Name const & _modifier);

			static void add(RegisteredElementModifier & _modifier);

		private:
			static RegisteredElementModifiers* s_modifiers;

			Array<RegisteredElementModifier> modifiers;
		};
	};
};
