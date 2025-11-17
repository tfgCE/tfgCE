#include "meshGenElementBone.h"

#include "meshGenGenerationContext.h"

#include "..\..\core\io\xml.h"

using namespace Framework;
using namespace MeshGeneration;

namespace Framework
{
	namespace MeshGeneration
	{
		namespace Bones
		{
			void add_bones();
		};
	};
};

//

REGISTER_FOR_FAST_CAST(ElementBoneData);

ElementBoneData::~ElementBoneData()
{
}

bool ElementBoneData::load_from_xml(IO::XML::Node const * _node, LibraryLoadingContext & _lc)
{
	return true;
}

bool ElementBoneData::prepare_for_game(Library* _library, LibraryPrepareContext& _pfgContext)
{
	return true;
}

//

REGISTER_FOR_FAST_CAST(ElementBone);

ElementBone::~ElementBone()
{
	delete_and_clear(data);
}

bool ElementBone::load_from_xml(IO::XML::Node const * _node, LibraryLoadingContext & _lc)
{
	bool result = base::load_from_xml(_node, _lc);

	boneType = _node->get_name_attribute(TXT("type"), boneType);
	if (RegisteredElementBone const * registeredElementBone = RegisteredElementBones::find(boneType))
	{
		generate_bone_start = registeredElementBone->generate_bone_start;
		generate_bone_end = registeredElementBone->generate_bone_end;
		if (registeredElementBone->create_data)
		{
			an_assert(data == nullptr);
			data = registeredElementBone->create_data();
			data->set_element(this);
			result &= data->load_from_xml(_node, _lc);
		}
	}
	else
	{
		error_loading_xml(_node, TXT("could not recognise bone type \"%S\""), boneType.to_char());
	}

	for_every(node, _node->all_children())
	{
		if (!base::is_xml_node_handled(node) &&
			(!data || !data->is_xml_node_handled(node)))
		{
			if (Element* element = Element::create_from_xml(node, _lc))
			{
				if (element->load_from_xml(node, _lc))
				{
					elements.push_back(RefCountObjectPtr<Element>(element));
				}
				else
				{
					error_loading_xml(node, TXT("problem loading one element of set"));
					result = false;
				}
			}
		}
	}

	return result;
}

#ifdef AN_DEVELOPMENT
void ElementBone::for_included_mesh_generators(std::function<void(MeshGenerator*)> _do) const
{
	for_every(element, elements)
	{
		element->get()->for_included_mesh_generators(_do);
	}
}
#endif

bool ElementBone::prepare_for_game(Library* _library, LibraryPrepareContext& _pfgContext)
{
	bool result = base::prepare_for_game(_library, _pfgContext);

	if (data)
	{
		result &= data->prepare_for_game(_library, _pfgContext);
	}
	
	for_every(element, elements)
	{
		result &= element->get()->prepare_for_game(_library, _pfgContext);
	}

	return result;
}

bool ElementBone::process(GenerationContext & _context, ElementInstance & _instance) const
{
	if (generate_bone_start)
	{
		bool result = true;
		
		int boneIdx = _context.get_generated_bones().get_size();

		result &= generate_bone_start(_context, _instance, data, boneIdx);
		
		for_every(element, elements)
		{
			result &= _context.process(element->get(), &_instance, for_everys_index(element));
		}

		if (generate_bone_end)
		{
			result &= generate_bone_end(_context, _instance, data, boneIdx);
		}

		return result;
	}
	else
	{
		error_generating_mesh(_instance, TXT("could not generate bone, no function available"));
		return false;
	}
}

//

RegisteredElementBones* RegisteredElementBones::s_bones = nullptr;

void RegisteredElementBones::initialise_static()
{
	an_assert(s_bones == nullptr);
	s_bones = new RegisteredElementBones();

	// add default bones
	Bones::add_bones();
}

void RegisteredElementBones::close_static()
{
	an_assert(s_bones != nullptr);
	delete_and_clear(s_bones);
}

RegisteredElementBone const * RegisteredElementBones::find(Name const & _bone)
{
	an_assert(s_bones);
	for_every_const(bone, s_bones->bones)
	{
		if (bone->name == _bone ||
			(! _bone.is_valid() && bone->isDefault))
		{
			return bone;
		}
	}
	error(TXT("could not find bone named \"%S\""), _bone.to_char());
	return nullptr;
}

RegisteredElementBone& RegisteredElementBones::add(RegisteredElementBone & _bone)
{
	an_assert(s_bones);
	s_bones->bones.push_back(_bone);
	return s_bones->bones.get_last();
}
