#include "spaceBlocker.h"

#include "..\..\core\debug\debugRenderer.h"
#include "..\..\core\debug\debugVisualiser.h"

//

#ifdef AN_OUTPUT_WORLD_GENERATION
//#define AN_INSPECT_SPACE_BLOCKERS
#endif

#ifdef AN_ALLOW_OPTIMIZE_OFF
#pragma optimize("", off)
#endif

//

using namespace Framework;

//

bool SpaceBlocker::load_from_xml(IO::XML::Node const* _node)
{
	bool result = true;

	blocks = _node->get_bool_attribute_or_from_child_presence(TXT("blocks"), blocks);
	blocks = ! _node->get_bool_attribute_or_from_child_presence(TXT("doesNotBlock"), ! blocks);

	requiresToBeEmpty = _node->get_bool_attribute_or_from_child_presence(TXT("requiresToBeEmpty"), requiresToBeEmpty);
	requiresToBeEmpty = ! _node->get_bool_attribute_or_from_child_presence(TXT("doesNotRequireToBeEmpty"), !requiresToBeEmpty);

	result &= box.load_from_xml(_node);

	error_loading_xml_on_assert(blocks || requiresToBeEmpty, result, _node, TXT("should have \"blocks\" or \"requiresToBeEmpty\" set"));

	return result;
}

void SpaceBlocker::log(LogInfoContext& _context) const
{
	_context.log(blocks ? TXT("blocks") : TXT("does not block"));
	_context.log(requiresToBeEmpty ? TXT("requires to be empty") : TXT("does not require to be empty"));
#ifdef AN_DEVELOPMENT
	_context.log(TXT("di: %S"), debugInfo.to_char());
#endif
	box.log(_context);
}

bool SpaceBlocker::check(SpaceBlocker const& _blocker) const
{
	if ((requiresToBeEmpty && _blocker.blocks) ||
		(blocks && _blocker.requiresToBeEmpty))
	{
		if (box.does_overlap(_blocker.box))
		{
			return false;
		}
	}
	return true;
}

#ifdef AN_DEVELOPMENT
void SpaceBlocker::debug_visualise(DebugVisualiser* dv, Transform const& _placement) const
{
	ArrayStatic<Vector3, 8> corners;
	corners.set_size(8);
	corners[0] = box.get_point_at_pt(Vector3(0.0f, 0.0f, 0.0f));
	corners[1] = box.get_point_at_pt(Vector3(1.0f, 0.0f, 0.0f));
	corners[2] = box.get_point_at_pt(Vector3(0.0f, 1.0f, 0.0f));
	corners[3] = box.get_point_at_pt(Vector3(1.0f, 1.0f, 0.0f));
	corners[4] = box.get_point_at_pt(Vector3(0.0f, 0.0f, 1.0f));
	corners[5] = box.get_point_at_pt(Vector3(1.0f, 0.0f, 1.0f));
	corners[6] = box.get_point_at_pt(Vector3(0.0f, 1.0f, 1.0f));
	corners[7] = box.get_point_at_pt(Vector3(1.0f, 1.0f, 1.0f));

	for_every(c, corners)
	{
		*c = _placement.location_to_world(*c);
	}

	Colour colour = Colour::magenta;
	if (!blocks && requiresToBeEmpty)
	{
		colour = Colour::mint;
	}
	if (blocks && !requiresToBeEmpty)
	{
		colour = Colour::red;
	}

	// z == 0
	dv->add_line_3d(corners[0], corners[1], colour);
	dv->add_line_3d(corners[0], corners[2], colour);
	dv->add_line_3d(corners[1], corners[3], colour);
	dv->add_line_3d(corners[2], corners[3], colour);

	// z == 1
	dv->add_line_3d(corners[4], corners[5], colour);
	dv->add_line_3d(corners[4], corners[6], colour);
	dv->add_line_3d(corners[5], corners[7], colour);
	dv->add_line_3d(corners[6], corners[7], colour);

	// z == 0->1
	dv->add_line_3d(corners[0], corners[4], colour);
	dv->add_line_3d(corners[1], corners[5], colour);
	dv->add_line_3d(corners[2], corners[6], colour);
	dv->add_line_3d(corners[3], corners[7], colour);
}
#endif

//

bool SpaceBlockers::load_from_xml(IO::XML::Node const* _node)
{
	bool result = true;

	for_every(node, _node->children_named(TXT("blocker")))
	{
		SpaceBlocker b;
		if (b.load_from_xml(node))
		{
			blockers.push_back(b);
		}
		else
		{
			result = false;
		}
	}

	return result;
}

bool SpaceBlockers::load_from_xml_child_node(IO::XML::Node const* _node, tchar const * _childNodeName)
{
	bool result = true;

	for_every(node, _node->children_named(_childNodeName))
	{
		result &= load_from_xml(node);
	}

	return result;
}

bool SpaceBlockers::check(SpaceBlockers const& _blockers, Transform const& _placement) const
{
	for_every(_sb, _blockers.blockers)
	{
		SpaceBlocker sb = *_sb;
		sb.box.apply_transform(_placement);
		for_every(esb, blockers)
		{
			if ((sb.requiresToBeEmpty && esb->blocks) ||
				(sb.blocks && esb->requiresToBeEmpty))
			{
				if (sb.box.does_overlap(esb->box))
				{
#ifdef AN_INSPECT_SPACE_BLOCKERS
					LogInfoContext log;
					log.log(TXT("failed blocker: tried placement %S %S"), _placement.get_translation().to_string().to_char(), _placement.get_orientation().to_rotator().to_string().to_char());
					log.log(TXT("              : %i(%i) with existing blocker %i(%i)"), for_everys_index(_sb), _blockers.blockers.get_size(), for_everys_index(esb), blockers.get_size());
					log.log(TXT("to check"));
					{	LOG_INDENT(log);	_blockers.log(log); }
					log.log(TXT("existing"));
					{	LOG_INDENT(log);	this->log(log); }
					log.output_to_output();
#endif
					return false;
				}
			}
		}
	}

#ifdef AN_INSPECT_SPACE_BLOCKERS
	LogInfoContext log;
	log.log(TXT("ok blocker: good placement %S %S"), _placement.get_translation().to_string().to_char(), _placement.get_orientation().to_rotator().to_string().to_char());
	log.log(TXT("          : (%i) existing blockers (%i)"), _blockers.blockers.get_size(), blockers.get_size());
	log.log(TXT("to check"));
	{	LOG_INDENT(log);	_blockers.log(log); }
	log.log(TXT("existing"));
	{	LOG_INDENT(log);	this->log(log); }
	log.output_to_output();
#endif
	return true;
}

void SpaceBlockers::add(SpaceBlockers const& _blockers, Transform const& _placement)
{
	for_every(_sb, _blockers.blockers)
	{
		SpaceBlocker sb = *_sb;
		sb.box.apply_transform(_placement);
		blockers.push_back(sb);
	}
}

void SpaceBlockers::debug_draw() const
{
	for_every(sb, blockers)
	{
		Colour c = Colour::magenta;
		if (!sb->blocks && sb->requiresToBeEmpty)
		{
			c = Colour::mint;
		}
		if (sb->blocks && !sb->requiresToBeEmpty)
		{
			c = Colour::red;
		}
		debug_draw_box(true, false, c, 0.2f, sb->box);
	}
}

void SpaceBlockers::log(LogInfoContext& _context) const
{
	if (!blockers.is_empty())
	{
		_context.log(TXT("blockers:"));
		LOG_INDENT(_context);
		for_every(blocker, blockers)
		{
			_context.log(TXT("+- blocker"));
			{
				LOG_INDENT(_context);
				blocker->log(_context);
			}
		}
	}
}

#ifdef AN_DEVELOPMENT
void SpaceBlockers::debug_visualise(DebugVisualiser* dv, Transform const& _placement) const
{
	for_every(b, blockers)
	{
		b->debug_visualise(dv, _placement);
	}
}
#endif

//

