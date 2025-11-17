#include "meshNode.h"

#include "..\..\core\debug\debugRenderer.h"

using namespace Framework;

void MeshNode::apply(Matrix44 const & _mat)
{
	Vector3 newLoc = _mat.location_to_world(placement.get_translation());
	Vector3 newFwd = _mat.vector_to_world(placement.get_axis(Axis::Y));
	Vector3 newUp = _mat.vector_to_world(placement.get_axis(Axis::Z));
	placement = look_at_matrix(newLoc, newLoc + newFwd, newUp).to_transform();
}

void MeshNode::apply(Transform const & _transform)
{
	placement = _transform.to_world(placement);
}

MeshNode* MeshNode::create_copy() const
{
	MeshNode* copy = new MeshNode();

	*copy = *this;

	return copy;
}

void MeshNode::remove_dropped_from(REF_ Array<MeshNodePtr>& _in)
{
	for (int i = 0; i < _in.get_size(); ++i)
	{
		if (_in[i]->dropped)
		{
			_in.remove_at(i);
		}
	}
}

void MeshNode::add_not_dropped_to(Array<MeshNodePtr> const& _from, OUT_ Array<MeshNodePtr>& _to)
{
	_to.make_space_for_additional(_from.get_size());
	for_every(from, _from)
	{
		if (!from->get()->is_dropped())
		{
			_to.push_back(*from);
		}
	}
}

void MeshNode::add_not_dropped_except_to(Array<MeshNodePtr> const& _from, Array<MeshNodePtr> const& _exceptNodes, OUT_ Array<MeshNodePtr>& _to)
{
	_to.make_space_for_additional(_from.get_size());
	for_every(from, _from)
	{
		if (!from->get()->is_dropped())
		{
			bool add = true;
			for_every(except, _exceptNodes)
			{
				if (*from == *except)
				{
					add = false;
					break;
				}
			}
			if (add)
			{
				_to.push_back(*from);
			}
		}
	}
}

void MeshNode::copy_not_dropped_to(Array<MeshNodePtr> const& _from, OUT_ Array<MeshNodePtr>& _to, Optional<Transform> const& _at)
{
	_to.make_space_for_additional(_from.get_size());
	Transform at = _at.get(Transform::identity);
	for_every_ref(from, _from)
	{
		if (! from->is_dropped())
		{
			MeshNodePtr copy(from->create_copy());
			copy->placement = at.to_world(copy->placement);
			_to.push_back(copy);
		}
	}
}

bool MeshNode::load_from_xml(IO::XML::Node const * _node, LibraryLoadingContext & _lc)
{
	bool result = true;

	name = _node->get_name_attribute(TXT("name"), name);
	tags.load_from_xml_attribute_or_child_node(_node);
	placement.load_from_xml_child_node(_node, TXT("placement"));
	variables.load_from_xml_child_node(_node, TXT("variables"));
	variables.load_from_xml_child_node(_node, TXT("parameters"));

	return result;
}

void MeshNode::log(LogInfoContext& _log, Array<MeshNodePtr> const& _nodes)
{
	_log.log(TXT("mesh nodes (%i)"), _nodes.get_size());
	for_every_ref(node, _nodes)
	{
		_log.log(TXT("%02i. %p, %S"), for_everys_index(node), node, node->name.to_char());
	}
}

void MeshNode::debug_draw(Transform const& _upperPlacement)
{
#ifdef AN_DEBUG_RENDERER
	Transform p = _upperPlacement.to_world(placement);
	Optional<Colour> colour = NP;
	if (is_dropped())
	{
		colour = Colour::grey;
	}
	debug_draw_transform_size(true, p, 0.3f);
	debug_draw_text(true, colour, p.get_translation(), NP, true, NP, false, name.to_char());
#endif
}
