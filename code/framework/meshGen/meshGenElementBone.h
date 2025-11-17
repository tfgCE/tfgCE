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
	class Skeleton;
};

// _boneIdx is more required for end than start!
#define ELEMENT_BONE_GENERATE_FN(_var) bool (*_var)(GenerationContext & _context, ElementInstance & _instance, ::Framework::MeshGeneration::ElementBoneData const * _data, int _boneIdx)
#define ELEMENT_BONE_CREATE_DATA_FN(_var) ::Framework::MeshGeneration::ElementBoneData* (*_var)()

#define REGISTER_MESH_GEN_BONE(name, generate_bone_start, generate_bone_end, create_data) \
	RegisteredElementBones::add(RegisteredElementBone(Name(name), generate_bone_start, generate_bone_end, create_data).temp_ref())

namespace Framework
{
	namespace MeshGeneration
	{
		class ElementBoneData
		{
			FAST_CAST_DECLARE(ElementBoneData);
			FAST_CAST_END();
		public:
			virtual ~ElementBoneData();

			void set_element(Element* _element) { element = _element; }
			Element* get_element() const { return element; }

			virtual bool is_xml_node_handled(IO::XML::Node const * _node) const { return false; }
			virtual bool load_from_xml(IO::XML::Node const * _node, LibraryLoadingContext & _lc);
			virtual bool prepare_for_game(Library* _library, LibraryPrepareContext& _pfgContext);

		private:
			Element* element = nullptr;
		};

		class ElementBone
		: public Element
		{
			FAST_CAST_DECLARE(ElementBone);
			FAST_CAST_BASE(Element);
			FAST_CAST_END();

			typedef Element base;
		public:
			virtual ~ElementBone();

			ElementBoneData const * get_data() const { return data; }

		private:
			override_ bool load_from_xml(IO::XML::Node const * _node, LibraryLoadingContext & _lc);
			override_ bool prepare_for_game(Library* _library, LibraryPrepareContext& _pfgContext);
#ifdef AN_DEVELOPMENT
			override_ void for_included_mesh_generators(std::function<void(MeshGenerator*)> _do) const;
#endif

			override_ bool process(GenerationContext & _context, ElementInstance & _instance) const;

		private:
			Name boneType;
			ELEMENT_BONE_GENERATE_FN(generate_bone_start) = nullptr;
			ELEMENT_BONE_GENERATE_FN(generate_bone_end) = nullptr;

			ElementBoneData* data = nullptr;
			Array<RefCountObjectPtr<Element>> elements;
		};

		struct RegisteredElementBone
		{
			Name name;
			bool isDefault = false;
			ELEMENT_BONE_GENERATE_FN(generate_bone_start) = nullptr;
			ELEMENT_BONE_GENERATE_FN(generate_bone_end) = nullptr;
			ELEMENT_BONE_CREATE_DATA_FN(create_data) = nullptr;

			RegisteredElementBone(Name const & _name, ELEMENT_BONE_GENERATE_FN(_generate_bone_start), ELEMENT_BONE_GENERATE_FN(_generate_bone_end), ELEMENT_BONE_CREATE_DATA_FN(_create_data) = nullptr) : name(_name), generate_bone_start(_generate_bone_start), generate_bone_end(_generate_bone_end), create_data(_create_data) {}
			void be_default() { isDefault = true; }

			RegisteredElementBone& temp_ref() { return *this; }
		};

		class RegisteredElementBones
		{
		public:
			static void initialise_static();
			static void close_static();

			static RegisteredElementBone const * find(Name const & _bone);

			static RegisteredElementBone& add(RegisteredElementBone & _bone);

		private:
			static RegisteredElementBones* s_bones;

			Array<RegisteredElementBone> bones;
		};
	};
};
