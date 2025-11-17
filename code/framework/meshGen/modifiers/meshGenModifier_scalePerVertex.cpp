#include "meshGenModifiers.h"

#include "..\meshGenGenerationContext.h"
#include "..\meshGenUtils.h"
#include "..\meshGenElementModifier.h"
#include "..\meshGenValueDef.h"
#include "..\meshGenValueDefImpl.inl"

#include "..\..\world\pointOfInterest.h"

#include "..\..\..\core\mesh\mesh3d.h"
#include "..\..\..\core\mesh\mesh3dPart.h"
#include "..\..\..\core\mesh\socketDefinitionSet.h"
#include "..\..\..\core\random\randomNumber.h"

//

#ifdef AN_ALLOW_OPTIMIZE_OFF
#pragma optimize("", off)
#endif

//

using namespace Framework;
using namespace MeshGeneration;
using namespace Modifiers;

/**
 *	Allows:
 *		skew using anchor point and source point and destination point (all can be params)
 */
class ScalePerVertexData
: public ElementModifierData
{
	FAST_CAST_DECLARE(ScalePerVertexData);
	FAST_CAST_BASE(ElementModifierData);
	FAST_CAST_END();

	typedef ElementModifierData base;
public:
	override_ bool load_from_xml(IO::XML::Node const * _node, LibraryLoadingContext & _lc);

public: // we won't be exposed anywhere else, so we may be public here!
	ValueDef<Transform> anchor;
	Random::Number<float> scale;
	ValueDef<Vector3> applyScale;
	ValueDef<Vector3> scaleVector;
	ValueDef<Vector3> fullScaleAt;
	ValueDef<Vector3> noScaleAt;

	ValueDef<Vector3> alongDir;
	ValueDef<Vector3> alongDirAnchor;
	ValueDef<Range> alongDirDist;
	ValueDef<Range> alongDirRadius;
};

//

bool Modifiers::scale_per_vertex(GenerationContext & _context, ElementInstance & _instigatorInstance, Element const * _subject, ::Framework::MeshGeneration::ElementModifierData const * _data)
{
	bool result = true;

	Checkpoint checkpoint(_context);

	if (_subject)
	{
		result &= _context.process(_subject, &_instigatorInstance);
	}
	else
	{
		checkpoint = _context.get_checkpoint(1); // of parent
	}

	Checkpoint now(_context);
	
	if (checkpoint != now)
	{
		if (ScalePerVertexData const * data = fast_cast<ScalePerVertexData>(_data))
		{
			if (! data->scale.is_empty())
			{
				Transform anchor = data->anchor.is_set()? data->anchor.get(_context) : Transform::identity;
				Vector3 applyScale = data->applyScale.get_with_default(_context, Vector3::one);
				Vector3 scaleVector = data->scaleVector.get_with_default(_context, Vector3::one);
				Optional<Vector3> fullScaleAt;
				Optional<Vector3> noScaleAt;
				if (data->fullScaleAt.is_set()) fullScaleAt = data->fullScaleAt.get(_context);
				if (data->noScaleAt.is_set()) noScaleAt = data->noScaleAt.get(_context);
				float fullScaleDist = 0.0f;
				Vector3 fullScaleDir = Vector3::zero;
				if (fullScaleAt.is_set() || noScaleAt.is_set())
				{
					noScaleAt.set_if_not_set(Vector3::zero);
					fullScaleAt.set_if_not_set(Vector3::zero);
					fullScaleDir = (fullScaleAt.get() - noScaleAt.get()).normal();
					fullScaleDist = (fullScaleAt.get() - noScaleAt.get()).length();
				}
				Optional<Vector3> alongDir;
				Optional<Vector3> alongDirAnchor;
				Optional<Range> alongDirDist;
				Optional<Range> alongDirRadius;
				if (data->alongDir.is_set()) alongDir = data->alongDir.get(_context);
				if (data->alongDirAnchor.is_set()) alongDirAnchor = data->alongDirAnchor.get(_context);
				if (data->alongDirDist.is_set()) alongDirDist = data->alongDirDist.get(_context);
				if (data->alongDirRadius.is_set()) alongDirRadius = data->alongDirRadius.get(_context);
				Random::Generator rg = _context.access_random_generator().spawn();
				{
					Matrix44 sourceMat = anchor.to_matrix();
					if (checkpoint.partsSoFarCount != now.partsSoFarCount)
					{
						RefCountObjectPtr<::Meshes::Mesh3DPart> const* pPart = &_context.get_parts()[checkpoint.partsSoFarCount];
						for (int i = checkpoint.partsSoFarCount; i < now.partsSoFarCount; ++i, ++pPart)
						{
							::Meshes::Mesh3DPart* part = pPart->get();
							apply_transform_to_vertex_data_per_vertex(part->access_vertex_data(), part->get_number_of_vertices(), part->get_vertex_format(),
								[data, rg, scaleVector, applyScale, sourceMat, noScaleAt, fullScaleAt, fullScaleDist, fullScaleDir, alongDir, alongDirAnchor, alongDirDist, alongDirRadius](int _idx, Vector3 const& _v)
							{
								Random::Generator useRg = rg;
								useRg.advance_seed(_idx * 4, 3458 + _idx * 3);
								Matrix44 destMat = sourceMat;
								float scale = data->scale.get(useRg);
								float applyScalingNow = 1.0f;
								if (fullScaleAt.is_set() && noScaleAt.is_set() && fullScaleDist != 0.0f)
								{
									applyScalingNow *= Vector3::dot(_v - noScaleAt.get(), fullScaleDir) / fullScaleDist;
								}
								if (alongDir.is_set() && alongDirRadius.is_set())
								{
									Vector3 dir = alongDir.get();
									Vector3 v = _v;
									if (alongDirAnchor.is_set())
									{
										v = v - alongDirAnchor.get();
										if (alongDirDist.is_set())
										{
											float distAlong = Vector3::dot(v, dir.normal());
											if (!alongDirDist.get().does_contain(distAlong))
											{
												applyScalingNow = 0.0f;
											}
										}
									}
									Vector3 perp = v.drop_using(dir);
									float dist = perp.length();
									float distPt = alongDirRadius.get().get_pt_from_value(dist);
									distPt = clamp(distPt, 0.0f, 1.0f);
									applyScalingNow *= 1.0f - distPt;
								}
								destMat.set_x_axis(destMat.get_x_axis() * ((1.0f - applyScale.x * applyScalingNow) + scale * scaleVector.x * applyScale.x * applyScalingNow));
								destMat.set_y_axis(destMat.get_y_axis() * ((1.0f - applyScale.y * applyScalingNow) + scale * scaleVector.y * applyScale.y * applyScalingNow));
								destMat.set_z_axis(destMat.get_z_axis() * ((1.0f - applyScale.z * applyScalingNow) + scale * scaleVector.z * applyScale.z * applyScalingNow));
								Matrix44 transformMat = destMat.to_world(sourceMat.inverted());
								return transformMat;
							});
						}
					}
				}
			}
		}
	}
	
	return result;
}

::Framework::MeshGeneration::ElementModifierData* Modifiers::create_scale_per_vertex_data()
{
	return new ScalePerVertexData();
}

//

REGISTER_FOR_FAST_CAST(ScalePerVertexData);

bool ScalePerVertexData::load_from_xml(IO::XML::Node const * _node, LibraryLoadingContext & _lc)
{
	bool result = true;

	anchor.load_from_xml_child_node(_node, TXT("anchor"));
	scale.load_from_xml(_node, TXT("scale"));
	scaleVector.load_from_xml_child_node(_node, TXT("scaleVector"));
	applyScale.load_from_xml_child_node(_node, TXT("applyScale"));
	fullScaleAt.load_from_xml_child_node(_node, TXT("fullScaleAt"));
	noScaleAt.load_from_xml_child_node(_node, TXT("noScaleAt"));
	alongDir.load_from_xml_child_node(_node, TXT("alongDir"));
	alongDirAnchor.load_from_xml_child_node(_node, TXT("alongDirAnchor"));
	alongDirDist.load_from_xml_child_node(_node, TXT("alongDirDist"));
	alongDirRadius.load_from_xml_child_node(_node, TXT("alongDirRadius"));

	if (scale.is_empty())
	{
		scale.add(1.0f);
	}

	return result;
}
