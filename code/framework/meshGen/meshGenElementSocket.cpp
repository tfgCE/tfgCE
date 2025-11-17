#include "meshGenElementSocket.h"

#include "meshGenGenerationContext.h"

#include "meshGenUtils.h"

#include "meshGenValueDefImpl.inl"

#include "..\..\core\io\xml.h"
#include "..\..\core\mesh\socketDefinitionSet.h"

//

using namespace Framework;
using namespace MeshGeneration;

//

REGISTER_FOR_FAST_CAST(ElementSocket);

ElementSocket::~ElementSocket()
{
}

bool ElementSocket::load_from_xml(IO::XML::Node const * _node, LibraryLoadingContext & _lc)
{
	bool result = base::load_from_xml(_node, _lc);

	socketName.load_from_xml_child_node(_node, TXT("name"));
	parentBoneName.load_from_xml_child_node(_node, TXT("parent"));
	parentBoneName.load_from_xml_child_node(_node, TXT("parentBone"));
	parentBoneName.load_from_xml_child_node(_node, TXT("bone"));
	parentBoneName.load_from_xml_child_node(_node, TXT("boneName"));
	placementFromMeshNode.load_from_xml_child_node(_node, TXT("placementFromMeshNode"));
	dropMeshNode = _node->get_bool_attribute_or_from_child_presence(TXT("dropMeshNode"), dropMeshNode);
	location.load_from_xml_child_node(_node, TXT("location"));
	relativeLocation.load_from_xml_child_node(_node, TXT("relativeLocation"));
	placement.load_from_xml_child_node(_node, TXT("placement"));
	relativePlacement.load_from_xml_child_node(_node, TXT("relativePlacement"));

	if (!socketName.is_set())
	{
		error_loading_xml(_node, TXT("socket name not provided"));
		result = false;
	}

	return result;
}

bool ElementSocket::prepare_for_game(Library* _library, LibraryPrepareContext& _pfgContext)
{
	bool result = base::prepare_for_game(_library, _pfgContext);

	return result;
}

bool ElementSocket::process(GenerationContext & _context, ElementInstance & _instance) const
{
	Name actualSocketName = socketName.get(_context);
	if (actualSocketName.is_valid())
	{
		auto * socketDefinition = _context.access_sockets().find_or_create_socket(actualSocketName);
		socketDefinition->set_bone_name(_context.get_current_parent_bone());
		if (parentBoneName.is_set())
		{
			socketDefinition->set_bone_name(parentBoneName.get(_context));
		}
		socketDefinition->set_generation_id(_context.get_new_sockets_generation_id());
		if (placement.is_set() || location.is_set())
		{
			int boneIdx = _context.find_bone_index(socketDefinition->get_bone_name());
			Transform place = placement.is_set()? placement.get(_context) : Transform(location.get(_context), Quat::identity);
			if (!place.get_orientation().is_normalised())
			{
				error_generating_mesh(_instance, TXT("placement's orientation not normalised! %S"), place.get_orientation().to_string().to_char());
				return false;
			}
			if (boneIdx != NONE)
			{
				socketDefinition->set_relative_placement(_context.get_bone_placement_MS(boneIdx).to_local(place));
			}
			else if (_instance.get_context().is_mesh_static())
			{
				socketDefinition->set_placement_MS(place);
			}
			else
			{
				error_generating_mesh(_instance, TXT("no bone \"%S\" found for socket"), socketDefinition->get_bone_name().to_char());
				return false;
			}
		}
		if (relativePlacement.is_set() || relativeLocation.is_set())
		{
			socketDefinition->set_relative_placement(relativePlacement.is_set()? relativePlacement.get(_context) : Transform(relativeLocation.get(_context), Quat::identity));
		}
		if (placementFromMeshNode.is_set())
		{
			bool found = false;
			Name mnName = placementFromMeshNode.get(_context);
			for (int i = _context.get_checkpoint(1).meshNodesSoFarCount; i < _context.access_mesh_nodes().get_size(); ++i)
			{
				auto& mn = _context.access_mesh_nodes()[i];
				if (mn->name == mnName)
				{
					socketDefinition->set_placement_MS(mn->placement);
					if (dropMeshNode)
					{
						mn->drop();
					}
					found = true;
					break;
				}
			}
			if (!found)
			{
				// check all previous, at higher levels
				for (int i = _context.get_checkpoint(1).meshNodesSoFarCount - 1; i >= 0; --i)
				{
					auto& mn = _context.access_mesh_nodes()[i];
					if (mn->name == mnName)
					{
						socketDefinition->set_placement_MS(mn->placement);
						if (dropMeshNode)
						{
							mn->drop();
						}
						found = true;
						break;
					}
				}
			}
			if (!found)
			{
				error_generating_mesh(_instance, TXT("no mesh node \"%S\" found for socket"), mnName.to_char());
				return false;
			}
		}
		return true;
	}
	else
	{
		error(TXT("no valid socket name!"));
		return false;
	}
}
