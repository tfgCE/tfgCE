#include "meshGenElementRayCast.h"

#include "meshGenGenerationContext.h"
#include "meshGenUtils.h"

#include "..\..\core\collision\model.h"
#include "..\..\core\collision\checkCollisionContext.h"
#include "..\..\core\collision\checkSegmentResult.h"
#include "..\..\core\mesh\mesh3dPart.h"
#include "..\..\core\system\video\indexFormat.h"
#include "..\..\core\system\video\indexFormatUtils.h"

#include "..\..\core\debug\debugRenderer.h"

//

#ifdef AN_ALLOW_OPTIMIZE_OFF
#pragma optimize("", off)
#endif

//

using namespace Framework;
using namespace MeshGeneration;

//

DEFINE_STATIC_NAME(start);
DEFINE_STATIC_NAME(end);

//

REGISTER_FOR_FAST_CAST(ElementRayCast);

ElementRayCast::~ElementRayCast()
{
}

bool ElementRayCast::load_from_xml(IO::XML::Node const * _node, LibraryLoadingContext & _lc)
{
	bool result = base::load_from_xml(_node, _lc);

	{
		String againstAttr = _node->get_string_attribute_or_from_child(TXT("against"));
		if (!againstAttr.is_empty())
		{
			if (againstAttr == TXT("mesh")) against = Target::Mesh; else
			if (againstAttr == TXT("movement collision") || againstAttr == TXT("movement")) against = Target::MovementCollision; else
			if (againstAttr == TXT("precise collision") || againstAttr == TXT("precise")) against = Target::PreciseCollision; else
				error_loading_xml(_node, TXT("invalid against for ray cast \"%S\""), againstAttr.to_char());
		}
	}
	againstAll = _node->get_bool_attribute_or_from_child_presence(TXT("againstAll"), againstAll);
	againstNamedCheckpoint = _node->get_name_attribute(TXT("againstNamedCheckpoint"), againstNamedCheckpoint);
	againstCheckpointsUp = _node->get_int_attribute_or_from_child(TXT("againstCheckpointsUp"), againstCheckpointsUp);
	debugDraw = _node->get_bool_attribute_or_from_child_presence(TXT("debugDraw"), debugDraw);
	debugDrawDetailed = _node->get_bool_attribute_or_from_child_presence(TXT("debugDrawDetailed"), debugDrawDetailed);

	outParam = _node->get_name_attribute_or_from_child(TXT("outParam"), outParam);
	outNormalParam = _node->get_name_attribute_or_from_child(TXT("outNormalParam"), outNormalParam);
	outHitParam = _node->get_name_attribute_or_from_child(TXT("outHitParam"), outHitParam);

	if (!outParam.is_valid())
	{
		error_loading_xml(_node, TXT("no \"outParam\" provided for rayCast"));
		result = false;
	}

	return result;
}

bool ElementRayCast::prepare_for_game(Library* _library, LibraryPrepareContext& _pfgContext)
{
	bool result = base::prepare_for_game(_library, _pfgContext);

	return result;
}

bool ElementRayCast::process(GenerationContext & _context, ElementInstance & _instance) const
{
	bool result = true;

	Vector3 rayStart = Vector3::zero;
	FOR_PARAM(_context, Vector3, start)
	{
		rayStart = *start;
	}
	else
	{
		error_generating_mesh(_instance, TXT("no \"start\" param for ray cast"));
		result = false;
	}

	Vector3 rayEnd = rayStart + Vector3::yAxis;
	FOR_PARAM(_context, Vector3, end)
	{
		rayEnd = *end;
	}
	else
	{
		error_generating_mesh(_instance, TXT("no \"end\" param for ray cast"));
		result = false;
	}

	Checkpoint startCP;
	Checkpoint endCP = _context.get_checkpoint();

	if (! againstAll)
	{
		startCP = _context.get_checkpoint(1 + againstCheckpointsUp); // of parent
	}
	if (againstNamedCheckpoint.is_valid())
	{
		auto& nc = _context.access_named_checkpoint(againstNamedCheckpoint);
		{
			startCP = nc.start;
			if (nc.end.is_set())
			{
				endCP = nc.end.get();
			}
		}
	}

#ifdef AN_DEVELOPMENT
	if (debugDraw && _context.debugRendererFrame)
	{
		_context.debugRendererFrame->add_arrow(true, Colour::blue, rayStart, rayEnd, 0.05f);
	}
#endif

	Segment segment(rayStart, rayEnd);
	Vector3 hitNormal = Vector3::zero;
	bool hit = false;
	if (against == Target::Mesh)
	{
		for (int i = startCP.partsSoFarCount; i < endCP.partsSoFarCount; ++i)
		{
			auto * part = _context.get_parts()[i].get();
			if (part->get_index_format().get_element_size() != ::System::IndexElementSize::None)
			{
				int vertexStride = part->get_vertex_format().get_stride();
				int locationOffset = part->get_vertex_format().get_location_offset();
				if (int triCount = part->get_number_of_tris())
				{
					for (int i = 0; i < triCount; ++i)
					{
						int t[] = { (int)::System::IndexFormatUtils::get_value(part->get_index_format(), part->get_element_data().get_data(), i * 3 + 0),
							(int)::System::IndexFormatUtils::get_value(part->get_index_format(), part->get_element_data().get_data(), i * 3 + 1),
							(int)::System::IndexFormatUtils::get_value(part->get_index_format(), part->get_element_data().get_data(), i * 3 + 2) };
						Vector3 v[3];
						for_count(int, n, 3)
						{
							v[n] = *(Vector3*)(part->get_vertex_data().get_data() + vertexStride * t[n] + locationOffset);
						}
						bool hitThis = segment.check_against_triangle(v[0], v[1], v[2], hitNormal);
#ifdef AN_DEVELOPMENT

						if (debugDrawDetailed && _context.debugRendererFrame)
						{
							_context.debugRendererFrame->add_triangle(true, hitThis ? Colour::red.with_alpha(0.5f) : Colour::blue.with_alpha(0.1f), v[0], v[1], v[2]);
							if (hitThis)
							{
								_context.debugRendererFrame->add_triangle_border(true, Colour::red, v[0], v[1], v[2]);
							}
						}
#endif
						hit |= hitThis;
					}
				}
				else if (part->get_number_of_vertices() != 0)
				{
					todo_important(TXT("implement_ raycast for such case!"));
				}
			}
			else
			{
				todo_important(TXT("implement_ raycast for such case!"));
			}
		}
	}
	else
	{
		for (int i = (against == Target::MovementCollision ? startCP.movementCollisionPartsSoFarCount : startCP.preciseCollisionPartsSoFarCount);
				 i < (against == Target::MovementCollision ? endCP.movementCollisionPartsSoFarCount : endCP.preciseCollisionPartsSoFarCount); ++i)
		{
			auto * part = against == Target::MovementCollision? _context.get_movement_collision_parts()[i] : _context.get_precise_collision_parts()[i];
			Collision::CheckSegmentResult result;
			Collision::CheckCollisionContext context;
			bool hitThis = part->check_segment(REF_ segment, REF_ result, context);
#ifdef AN_DEVELOPMENT
			if (debugDrawDetailed && _context.debugRendererFrame)
			{
				if (hitThis && result.model)
				{
					result.model->debug_draw(true, true, Colour::red, 0.5f);
				}
			}
#endif
			if (hitThis)
			{
				hitNormal = result.hitNormal;
			}
			hit |= hitThis;
		}
	}

#ifdef AN_DEVELOPMENT
	if (hit && debugDraw && _context.debugRendererFrame)
	{
		_context.debugRendererFrame->add_arrow(true, Colour::red, segment.get_hit(), segment.get_hit() + hitNormal, 0.05f);
	}
#endif

	{
		_context.set_parameter(outParam, type_id<Vector3>(), void_const_ptr(segment.get_hit()), 1); // in parent
	}
	if (outNormalParam.is_valid())
	{
		_context.set_parameter(outNormalParam, type_id<Vector3>(), &hitNormal, 1); // in parent
	}
	if (outHitParam.is_valid())
	{
		_context.set_parameter(outHitParam, type_id<bool>(), &hit, 1);
	}

	return result;
}
