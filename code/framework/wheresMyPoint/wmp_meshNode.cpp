#include "wmp_meshNode.h"

#include "..\appearance\meshNode.h"

#include "..\meshGen\meshGenMeshNodeContext.h"

using namespace Framework;

//

MeshNode * MeshNodeBase::access_node(WheresMyPoint::Context & _context) const
{
	if (auto * meshNodeContext = fast_cast<MeshGeneration::MeshNodeContext>(_context.get_owner()))
	{
		return meshNodeContext->access_mesh_node();
	}
	else
	{
		WheresMyPoint::IOwner* wmpOwner = _context.get_owner();
		while (wmpOwner)
		{
			if (auto * context = fast_cast<MeshGeneration::GenerationContext>(wmpOwner))
			{
				if (!context->access_mesh_nodes().is_empty())
				{
					return context->access_mesh_nodes().get_last().get();
				}
			}
			wmpOwner = wmpOwner->get_wmp_owner();
		}
	}
	return nullptr;
}

//

bool MeshNodeName::update(REF_ WheresMyPoint::Value & _value, WheresMyPoint::Context & _context) const
{
	bool result = true;

	if (MeshNode const * meshNode = access_node(_context))
	{
		_value.set<Name>(meshNode->name);
	}
	else
	{
		error_processing_wmp(TXT("no current mesh node provided!"));
		result = false;
	}

	return result;
}

//

bool MeshNodePlacement::update(REF_ WheresMyPoint::Value & _value, WheresMyPoint::Context & _context) const
{
	bool result = true;

	if (MeshNode const * meshNode = access_node(_context))
	{
		_value.set<Transform>(meshNode->placement);
	}
	else
	{
		error_processing_wmp(TXT("no current mesh node provided!"));
		result = false;
	}

	return result;
}

//

bool MeshNodeStore::load_from_xml(IO::XML::Node const * _node)
{
	bool result = base::load_from_xml(_node);

	name = _node->get_name_attribute(TXT("as"), name);

	return result;
}

bool MeshNodeStore::update(REF_ WheresMyPoint::Value & _value, WheresMyPoint::Context & _context) const
{
	bool result = true;

	if (name.is_valid())
	{
		if (MeshNode * meshNode = access_node(_context))
		{
			SimpleVariableInfo const & info = meshNode->variables.find(name, _value.get_type());
			an_assert(RegisteredType::is_plain_data(_value.get_type()));
			memory_copy(info.access_raw(), _value.get_raw(), RegisteredType::get_size_of(_value.get_type()));
		}
		else
		{
			error_processing_wmp(TXT("no current mesh node provided!"));
			result = false;
		}
	}
	else
	{
		error_processing_wmp(TXT("no name"));
		result = false;
	}

	return result;
}

//

bool MeshNodeRestore::load_from_xml(IO::XML::Node const * _node)
{
	bool result = base::load_from_xml(_node);

	name = _node->get_name_attribute(TXT("from"), name);

	return result;
}

bool MeshNodeRestore::update(REF_ WheresMyPoint::Value & _value, WheresMyPoint::Context & _context) const
{
	bool result = true;

	if (name.is_valid())
	{
		if (MeshNode * meshNode = access_node(_context))
		{
			TypeId id;
			void const * value;
			if (meshNode->variables.get_existing_of_any_type_id(name, OUT_ id, OUT_ value))
			{
				_value.set_raw(id, value);
			}
			else
			{
				error_processing_wmp(TXT("could not find mesh node variable \"%S\""), name.to_char());
				result = false;
			}
		}
		else
		{
			error_processing_wmp(TXT("no current mesh node provided!"));
			result = false;
		}
	}
	else
	{
		error_processing_wmp(TXT("no name"));
		result = false;
	}

	return result;
}
