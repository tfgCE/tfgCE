#include "meshGenUtils.h"

#include "meshGenConfig.h"
#include "meshGenCreateCollisionInfo.h"
#include "meshGenCreateSpaceBlockerInfo.h"
#include "meshGenGenerationContext.h"
#include "meshGenElement.h"
#include "meshGenElementSocket.h"

#include "..\collision\physicalMaterial.h"
#include "..\world\pointOfInterest.h"

#include "..\..\core\mainSettings.h"
#include "..\..\core\collision\model.h"
#include "..\..\core\mesh\mesh3d.h"
#include "..\..\core\mesh\socketDefinition.h"
#include "..\..\core\mesh\builders\mesh3dBuilder_IPU.h"
#include "..\..\core\system\video\vertexFormatUtils.h"
#include "..\..\core\types\vectorPacked.h"

//

#ifdef AN_ALLOW_OPTIMIZE_OFF
#pragma optimize("", off)
#endif

//

using namespace Framework;
using namespace MeshGeneration;

//

DEFINE_STATIC_NAME(autoPlacement);
DEFINE_STATIC_NAME(autoScale);
DEFINE_STATIC_NAME(autoColour);
DEFINE_STATIC_NAME(autoU);
DEFINE_STATIC_NAME(autoUV);
DEFINE_STATIC_NAME(autoUVW);
DEFINE_STATIC_NAME(autoForceSkinning);
DEFINE_STATIC_NAME(autoSkinningBone);
DEFINE_STATIC_NAME(autoSkinningBones);
DEFINE_STATIC_NAME(autoSkinningWeights);
DEFINE_STATIC_NAME(smoothingDotLimit);
DEFINE_STATIC_NAME(smoothingAngleLimit);
DEFINE_STATIC_NAME(forceSmoothingDotLimit);
DEFINE_STATIC_NAME(forceSmoothingAngleLimit);
DEFINE_STATIC_NAME(ipuWeldingDistance);

//

void make_range_valid(REF_ Range3& range)
{
	if (range.x.min == range.x.max)
	{
		range.x.expand_by(0.001f);
	}
	if (range.y.min == range.y.max)
	{
		range.y.expand_by(0.001f);
	}
	if (range.z.min == range.z.max)
	{
		range.z.expand_by(0.001f);
	}
}

//

bool MeshGeneration::get_auto_placement_and_auto_scale(GenerationContext & _context, ElementInstance & _instance, OUT_ Transform & _autoPlacement, OUT_ Vector3 & _autoScale)
{
	Transform usePlacement = Transform::identity;
	bool applyPlacement = false;
	{
		FOR_PARAM(_context, Transform, autoPlacement)
		{
			usePlacement = *autoPlacement;
			applyPlacement = true;
		}
	}
	{
		FOR_PARAM(_context, Vector3, autoPlacement)
		{
			if (applyPlacement)
			{
				error_generating_mesh(_instance, TXT("we have autoPlacement as both transform and vector!"));
			}
			usePlacement.set_orientation(Quat::identity);
			usePlacement.set_translation(*autoPlacement);
			applyPlacement = true;
		}
	}

	Vector3 useScale = Vector3::one;
	bool applyScale = false;
	{
		FOR_PARAM(_context, float, autoScale)
		{
			useScale.x = *autoScale;
			useScale.y = *autoScale;
			useScale.z = *autoScale;
			applyScale = true;
		}
	}
	{
		FOR_PARAM(_context, Vector3, autoScale)
		{
			useScale = *autoScale;
			applyScale = true;
		}
	}

	_autoPlacement = usePlacement;
	_autoScale = useScale;

	return applyPlacement || applyScale;
}

void MeshGeneration::apply_standard_parameters(REF_ Array<int8> & vertexData, int _vertexCount, GenerationContext & _context, ElementInstance & _instance)
{
	ApplyTo applyTo(vertexData, _vertexCount, nullptr);
	apply_placement_and_scale_from_parameters(applyTo, _context, _instance);
	apply_colour_and_uv_from_parameters(applyTo, _context, _instance);
	apply_skinning_from_parameters(applyTo, _context, _instance);
}

void MeshGeneration::apply_standard_parameters(Collision::Model* _model, GenerationContext & _context, ElementInstance & _instance)
{
	ApplyTo applyTo(_model);
	apply_placement_and_scale_from_parameters(applyTo, _context, _instance);
	apply_colour_and_uv_from_parameters(applyTo, _context, _instance);
	apply_skinning_from_parameters(applyTo, _context, _instance);
}

void MeshGeneration::apply_standard_parameters(REF_ Bone & _bone, GenerationContext & _context, ElementInstance & _instance)
{
	ApplyTo applyTo(_bone);
	apply_placement_and_scale_from_parameters(applyTo, _context, _instance);
	apply_skinning_from_parameters(applyTo, _context, _instance);
}

void MeshGeneration::apply_standard_parameters(AppearanceControllerData * _appearanceController, GenerationContext & _context, ElementInstance & _instance)
{
	ApplyTo applyTo(_appearanceController);
	apply_placement_and_scale_from_parameters(applyTo, _context, _instance);
	apply_skinning_from_parameters(applyTo, _context, _instance);
}

void MeshGeneration::apply_standard_parameters(Meshes::SocketDefinition * _socket, GenerationContext & _context, ElementInstance & _instance)
{
	ApplyTo applyTo(_socket);
	apply_placement_and_scale_from_parameters(applyTo, _context, _instance);
	apply_skinning_from_parameters(applyTo, _context, _instance);
}

void MeshGeneration::apply_standard_parameters(MeshNode * _meshNode, GenerationContext & _context, ElementInstance & _instance)
{
	ApplyTo applyTo(_meshNode);
	apply_placement_and_scale_from_parameters(applyTo, _context, _instance);
}

void MeshGeneration::apply_standard_parameters(PointOfInterest * _poi, GenerationContext & _context, ElementInstance & _instance)
{
	ApplyTo applyTo(_poi);
	apply_placement_and_scale_from_parameters(applyTo, _context, _instance);
}

void MeshGeneration::apply_standard_parameters(SpaceBlocker * _sb, GenerationContext& _context, ElementInstance& _instance)
{
	ApplyTo applyTo(_sb);
	apply_placement_and_scale_from_parameters(applyTo, _context, _instance);
}

void MeshGeneration::invalidate_standard_parameters(GenerationContext & _context, ElementInstance & _instance)
{
	invalidate_placement_and_scale_parameters(_context, _instance);
	invalidate_colour_and_uv_parameters(_context, _instance);
	invalidate_skinning_parameters(_context, _instance);
}

void MeshGeneration::invalidate_placement_and_scale_parameters(GenerationContext & _context, ElementInstance & _instance)
{
	INVALIDATE_PARAM(_context, Transform, autoPlacement);
	INVALIDATE_PARAM(_context, Vector3, autoPlacement);
	INVALIDATE_PARAM(_context, float, autoScale);
	INVALIDATE_PARAM(_context, Vector3, autoScale);
}

void MeshGeneration::apply_placement_and_scale_from_parameters(ApplyTo const & _applyTo, GenerationContext & _context, ElementInstance & _instance)
{
	Transform usePlacement = Transform::identity;
	Vector3 useScale = Vector3::one;

	if (!get_auto_placement_and_auto_scale(_context, _instance, OUT_ usePlacement, OUT_ useScale))
	{
		return;
	}

	if (useScale != Vector3::one)
	{
		apply_scale(_applyTo, _context, useScale);
	}

	if (! Transform::same_with_orientation(usePlacement, Transform::identity))
	{
		apply_placement(_applyTo, _context, usePlacement);
	}
}

void MeshGeneration::apply_scale(ApplyTo const & _applyTo, GenerationContext & _context, Vector3 const & _scale)
{
	if (_applyTo.vertexData)
	{
		int8* currentVertexData = _applyTo.vertexData->get_data();
		auto const& vertexFormat = _applyTo.vertexFormat ? *_applyTo.vertexFormat : _context.get_vertex_format();
		for (int vrtIdx = 0; vrtIdx < _applyTo.vertexCount; ++vrtIdx, currentVertexData += vertexFormat.get_stride())
		{
			Vector3* out = (Vector3*)(currentVertexData + vertexFormat.get_location_offset());
			*out = *out * _scale;
		}
		for_every(cd, vertexFormat.get_custom_data())
		{
			if (cd->attribType == ::System::VertexFormatAttribType::Location ||
				cd->attribType == ::System::VertexFormatAttribType::Vector)
			{
				int8* currentVertexData = _applyTo.vertexData->get_data();
				int offset = vertexFormat.get_custom_data_offset(cd->name);
				for (int vrtIdx = 0; vrtIdx < _applyTo.vertexCount; ++vrtIdx, currentVertexData += vertexFormat.get_stride())
				{
					Vector3* out = (Vector3*)(currentVertexData + offset);
					*out = *out * _scale;
				}
			}
		}
	}
	if (_applyTo.model)
	{
		Matrix44 transform = scale_matrix(_scale);
		apply_transform_to_collision_model(_applyTo.model, transform);
	}
	if (_applyTo.bone)
	{
		_applyTo.bone->placement.set_translation(_applyTo.bone->placement.get_translation() * _scale);
	}
	if (_applyTo.socket)
	{
		if (_applyTo.socket->get_placement_MS().is_set())
		{
			Transform placement = _applyTo.socket->get_placement_MS().get();
			placement.set_translation(placement.get_translation() * _scale);
			_applyTo.socket->set_placement_MS(placement);
		}
	}
	if (_applyTo.meshNode)
	{
		_applyTo.meshNode->placement.set_translation(_applyTo.meshNode->placement.get_translation() * _scale);
	}
	if (_applyTo.poi)
	{
		_applyTo.poi->offset.set_translation(_applyTo.poi->offset.get_translation() * _scale);
	}
	if (_applyTo.spaceBlocker)
	{
		_applyTo.spaceBlocker->box.apply_transform(Transform(Vector3::zero, Quat::identity, _scale.x));
	}
	if (_applyTo.appearanceController)
	{
		todo_note(TXT("is it required?"));
	}
}

void MeshGeneration::apply_placement(ApplyTo const & _applyTo, GenerationContext & _context, Transform const & _placement)
{
	if (_applyTo.vertexData)
	{
		int8* currentVertexData = _applyTo.vertexData->get_data();
		auto const& vertexFormat = _applyTo.vertexFormat ? *_applyTo.vertexFormat : _context.get_vertex_format();
		for (int vrtIdx = 0; vrtIdx < _applyTo.vertexCount; ++vrtIdx, currentVertexData += vertexFormat.get_stride())
		{
			if (vertexFormat.get_location() == ::System::VertexFormatLocation::XYZ)
			{
				Vector3* out = (Vector3*)(currentVertexData + vertexFormat.get_location_offset());
				*out = _placement.location_to_world(*out);
			}

			if (vertexFormat.get_normal() == ::System::VertexFormatNormal::Yes)
			{
				if (vertexFormat.is_normal_packed())
				{
					VectorPacked* out = (VectorPacked*)(currentVertexData + vertexFormat.get_normal_offset());
					out->set_as_vertex_normal(_placement.vector_to_world(out->get_as_vertex_normal()).normal());
				}
				else
				{
					Vector3* out = (Vector3*)(currentVertexData + vertexFormat.get_normal_offset());
					*out = _placement.vector_to_world(*out);
				}
			}

			for_every(cd, vertexFormat.get_custom_data())
			{
				if (cd->attribType == ::System::VertexFormatAttribType::Location ||
					cd->attribType == ::System::VertexFormatAttribType::Vector)
				{
					int8* currentVertexData = _applyTo.vertexData->get_data();
					int offset = vertexFormat.get_custom_data_offset(cd->name);
					for (int vrtIdx = 0; vrtIdx < _applyTo.vertexCount; ++vrtIdx, currentVertexData += vertexFormat.get_stride())
					{
						Vector3* out = (Vector3*)(currentVertexData + offset);
						if (cd->attribType == ::System::VertexFormatAttribType::Location)
						{
							*out = _placement.location_to_world(*out);
						}
						if (cd->attribType == ::System::VertexFormatAttribType::Vector)
						{
							*out = _placement.vector_to_world(*out);
						}
					}
				}
			}
		}
	}
	if (_applyTo.model)
	{
		apply_transform_to_collision_model(_applyTo.model, _placement.to_matrix());
	}
	if (_applyTo.bone)
	{
		_applyTo.bone->placement = _placement.to_world(_applyTo.bone->placement);
	}
	if (_applyTo.socket)
	{
		if (_applyTo.socket->get_placement_MS().is_set())
		{
			Transform placement = _applyTo.socket->get_placement_MS().get();
			_applyTo.socket->set_placement_MS(_placement.to_world(placement));
		}
		else if (!_applyTo.socket->get_bone_name().is_valid() || _context.find_bone_index(_applyTo.socket->get_bone_name()) == NONE || fast_cast<ElementSocket>(_context.get_current_element()))
		{
			// allow manipulation with auto placement only if not added to bone yet or no bone specified at all - because if there is bone, our placement is settled
			// and allow setting for element socket
			if (_applyTo.socket->get_relative_placement().is_set())
			{
				Transform placement = _applyTo.socket->get_relative_placement().get();
				_applyTo.socket->set_relative_placement(_placement.to_world(placement));
			}
			else
			{
				_applyTo.socket->set_relative_placement(_placement);
			}
		}
	}
	if (_applyTo.meshNode)
	{
		_applyTo.meshNode->apply(_placement);
	}
	if (_applyTo.poi)
	{
		_applyTo.poi->apply(_placement);
	}
	if (_applyTo.spaceBlocker)
	{
		_applyTo.spaceBlocker->box.apply_transform(_placement);
	}
	if (_applyTo.appearanceController)
	{
		todo_note(TXT("is it required?"));
	}
}

void MeshGeneration::invalidate_colour_and_uv_parameters(GenerationContext & _context, ElementInstance & _instance)
{
	INVALIDATE_PARAM(_context, Colour, autoColour);
	INVALIDATE_PARAM(_context, float, autoU);
	INVALIDATE_PARAM(_context, Vector2, autoUV);
	INVALIDATE_PARAM(_context, Vector3, autoUVW);
}

void MeshGeneration::apply_colour_and_uv_from_parameters(ApplyTo const & _applyTo, GenerationContext & _context, ElementInstance & _instance)
{
	Colour useColour = Colour::white;
	bool applyColour = false;
	{
		FOR_PARAM(_context, Colour, autoColour)
		{
			useColour = *autoColour;
			applyColour = true;
		}
	}

	Vector3 useUV = Vector3::zero;
	bool applyUV = false;
	{
		FOR_PARAM(_context, float, autoU)
		{
			useUV.x = *autoU;
			useUV.y = 0.0f;
			useUV.z = 0.0f;
			applyUV = true;
		}
	}
	{
		FOR_PARAM(_context, Vector2, autoUV)
		{
			useUV.x = autoUV->x;
			useUV.y = autoUV->y;
			useUV.z = 0.0f;
			applyUV = true;
		}
	}
	{
		FOR_PARAM(_context, Vector3, autoUVW)
		{
			useUV = *autoUVW;
			applyUV = true;
		}
	}

	if (!applyColour &&
		!applyUV)
	{
		return;
	}

	if (_applyTo.vertexData)
	{
		int8* currentVertexData = _applyTo.vertexData->get_data();
		auto const& vertexFormat = _applyTo.vertexFormat ? *_applyTo.vertexFormat : _context.get_vertex_format();
		for (int vrtIdx = 0; vrtIdx < _applyTo.vertexCount; ++vrtIdx, currentVertexData += vertexFormat.get_stride())
		{
			if (applyColour)
			{
				if (vertexFormat.get_colour() == ::System::VertexFormatColour::RGB)
				{
					Colour* out = (Colour*)(currentVertexData + vertexFormat.get_colour_offset());
					out->r = useColour.r;
					out->g = useColour.g;
					out->b = useColour.b;
				}
				if (vertexFormat.get_colour() == ::System::VertexFormatColour::RGBA)
				{
					Colour* out = (Colour*)(currentVertexData + vertexFormat.get_colour_offset());
					out->r = useColour.r;
					out->g = useColour.g;
					out->b = useColour.b;
					out->a = useColour.a;
				}
			}

			if (applyUV)
			{
				if (vertexFormat.get_texture_uv() == ::System::VertexFormatTextureUV::U)
				{
					::System::VertexFormatUtils::store(currentVertexData + vertexFormat.get_texture_uv_offset(), vertexFormat.get_texture_uv_type(), useUV.x);
				}
				else if (vertexFormat.get_texture_uv() == ::System::VertexFormatTextureUV::UV)
				{
					::System::VertexFormatUtils::store(currentVertexData + vertexFormat.get_texture_uv_offset(), vertexFormat.get_texture_uv_type(), Vector2(useUV.x, useUV.y));
				}
				else if (vertexFormat.get_texture_uv() == ::System::VertexFormatTextureUV::UVW)
				{
					::System::VertexFormatUtils::store(currentVertexData + vertexFormat.get_texture_uv_offset(), vertexFormat.get_texture_uv_type(), useUV);
				}
				else
				{
					todo_important(TXT("implement_ other texture uv type"));
				}
			}
		}
	}
}

void MeshGeneration::invalidate_skinning_parameters(GenerationContext & _context, ElementInstance & _instance)
{
	INVALIDATE_PARAM(_context, bool, autoForceSkinning);
	INVALIDATE_PARAM(_context, int, autoSkinningBone);
	INVALIDATE_PARAM(_context, VectorInt4, autoSkinningBones);
	INVALIDATE_PARAM(_context, Vector4, autoSkinningWeights);
	INVALIDATE_PARAM(_context, Name, autoSkinningBone);
}

void MeshGeneration::apply_skinning_from_parameters(ApplyTo const & _applyTo, GenerationContext & _context, ElementInstance & _instance)
{
	auto const& vertexFormat = _applyTo.vertexFormat ? *_applyTo.vertexFormat : _context.get_vertex_format();
	if (!vertexFormat.has_skinning_data())
	{
		return;
	}

	bool forceSkinning = false;
	{
		FOR_PARAM(_context, bool, autoForceSkinning)
		{
			forceSkinning = *autoForceSkinning;
		}
	}

	uint32 useIndices[4] = { 0, 0, 0, 0 };
	float useWeights[4] = { 1.0f, 0.0f, 0.0f, 0.0f };
	bool applySkinning = false;
	{
		FOR_PARAM(_context, int, autoSkinningBone)
		{
			useIndices[0] = *autoSkinningBone;
			useWeights[0] = 1.0f;
			useWeights[1] = 0.0f;
			useWeights[2] = 0.0f;
			useWeights[3] = 0.0f;
			applySkinning = true;
		}
	}

	{
		FOR_PARAM(_context, VectorInt4, autoSkinningBones)
		{
			useIndices[0] = autoSkinningBones->x;
			useIndices[1] = autoSkinningBones->y;
			useIndices[2] = autoSkinningBones->z;
			useIndices[3] = autoSkinningBones->w;
			applySkinning = true;
		}
	}

	{
		FOR_PARAM(_context, Vector4, autoSkinningWeights)
		{
			useWeights[0] = autoSkinningWeights->x;
			useWeights[1] = autoSkinningWeights->y;
			useWeights[2] = autoSkinningWeights->z;
			useWeights[3] = autoSkinningWeights->w;
			applySkinning = true;
		}
	}

	{
		FOR_PARAM(_context, Name, autoSkinningBone)
		{
			int32 resolvedBone = _context.find_bone_index(*autoSkinningBone);
			if (resolvedBone != NONE)
			{
				useIndices[0] = resolvedBone;
				useIndices[1] = 0;
				useIndices[2] = 0;
				useIndices[3] = 0;
				useWeights[0] = 1.0f;
				useWeights[1] = 0.0f;
				useWeights[2] = 0.0f;
				useWeights[3] = 0.0f;
				applySkinning = true;
			}
		}
	}

	if (!applySkinning)
	{
		return;
	}

	if (_applyTo.vertexData)
	{
		int8* currentVertexData = _applyTo.vertexData->get_data();
		int stride = vertexFormat.get_stride();
		int skinningOffset = vertexFormat.get_skinning_element_count() > 1 ? vertexFormat.get_skinning_indices_offset() : vertexFormat.get_skinning_index_offset();
		int bonesRequired = vertexFormat.get_skinning_element_count();
		if (bonesRequired > 4)
		{
			error_generating_mesh(_instance, TXT("no more than 4 bones supported!"));
			todo_important(TXT("add support for more bones"));
			bonesRequired = 4;
		}
		for (int vrtIdx = 0; vrtIdx < _applyTo.vertexCount; ++vrtIdx, currentVertexData += stride)
		{
			if (vertexFormat.get_skinning_index_type() == ::System::DataFormatValueType::Uint8)
			{
				if (!forceSkinning)
				{
					bool somethingSet = false;
					uint8* out = (uint8*)(currentVertexData + skinningOffset);
					for (int idx = 0; idx < bonesRequired; ++idx, ++out)
					{
						if (*out != (uint8)NONE)
						{
							somethingSet = true;
						}
					}
					if (somethingSet)
					{
						continue;
					}
				}
				{
					uint8* out = (uint8*)(currentVertexData + skinningOffset);
					for (int idx = 0; idx < bonesRequired; ++idx, ++out)
					{
						*out = (uint8)useIndices[idx];
					}
				}
			}
			else if (vertexFormat.get_skinning_index_type() == ::System::DataFormatValueType::Uint16)
			{
				if (!forceSkinning)
				{
					bool somethingSet = false;
					uint16* out = (uint16*)(currentVertexData + skinningOffset);
					for (int idx = 0; idx < bonesRequired; ++idx, ++out)
					{
						if (*out != (uint16)NONE)
						{
							somethingSet = true;
						}
					}
					if (somethingSet)
					{
						continue;
					}
				}
				{
					uint16* out = (uint16*)(currentVertexData + skinningOffset);
					for (int idx = 0; idx < bonesRequired; ++idx, ++out)
					{
						*out = (uint16)useIndices[idx];
					}
				}
			}
			else if (vertexFormat.get_skinning_index_type() == ::System::DataFormatValueType::Uint32)
			{
				if (!forceSkinning)
				{
					bool somethingSet = false;
					uint32* out = (uint32*)(currentVertexData + skinningOffset);
					for (int idx = 0; idx < bonesRequired; ++idx, ++out)
					{
						if (*out != (uint32)NONE)
						{
							somethingSet = true;
						}
					}
					if (somethingSet)
					{
						continue;
					}
				}
				{
					uint32* out = (uint32*)(currentVertexData + skinningOffset);
					for (int idx = 0; idx < bonesRequired; ++idx, ++out)
					{
						*out = (uint32)useIndices[idx];
					}
				}
			}
			else
			{
				todo_important(TXT("implement_ other skinning index type"));
			}

			if (bonesRequired > 1) // no need to provide weight for single bone
			{
				float* out = (float*)(currentVertexData + vertexFormat.get_skinning_weights_offset());
				for (int idx = 0; idx < bonesRequired; ++idx, ++out)
				{
					*out = useWeights[idx];
				}
			}
		}
	}
	if (_applyTo.model || _applyTo.bone || _applyTo.socket)
	{
		int highestWeightIdx = 0;
		float highestWeight = 0.0f;
		for_count(int, i, 4)
		{
			if (highestWeight < useWeights[i])
			{
				highestWeight = useWeights[i];
				highestWeightIdx = i;
			}
		}
		Name skinToBone = _context.find_skeleton_bone_name(useIndices[highestWeightIdx]);
		if (_applyTo.model)
		{
			_applyTo.model->skin_to(skinToBone, !forceSkinning); // this way we go through any replacer and resolve to skeleton's bone
		}
		if (_applyTo.bone)
		{
			if (forceSkinning || (!_applyTo.bone->parentName.is_valid() || _applyTo.bone->parentName == _context.get_current_parent_bone()))
			{
				if (_applyTo.bone->boneName != skinToBone)
				{
					_applyTo.bone->parentName = skinToBone;
				}
			}
		}
		if (_applyTo.socket)
		{
			if (forceSkinning || (!_applyTo.socket->get_bone_name().is_valid() || _applyTo.socket->get_bone_name() == _context.get_current_parent_bone()))
			{
				_applyTo.socket->set_bone_name(skinToBone); // this way we go through any replacer and resolve to skeleton's bone
			}
		}
	}
	if (_applyTo.appearanceController)
	{
		todo_note(TXT("is it required?"));
	}
}

Optional<float> MeshGeneration::get_smoothing_dot_limit(GenerationContext & _context)
{
	Optional<float> result;
	FOR_PARAM(_context, float, smoothingDotLimit)
	{
		result = (*smoothingDotLimit);
	}
	FOR_PARAM(_context, float, smoothingAngleLimit)
	{
		result = (cos_deg<float>(*smoothingAngleLimit));
	}
	if (MeshGeneration::Config::get().should_smooth_everything_at_lod(_context.get_for_lod()))
	{
		result = (-2.0f);
	}
	if (!MainSettings::global().get_mesh_gen_smoothing())
	{
		result = (1.0f);
	}
	// this is a way to always smooth, otherwise it depends on main settings (if we allow to smooth or not)
	FOR_PARAM(_context, float, forceSmoothingDotLimit)
	{
		result = (*forceSmoothingDotLimit);
	}
	FOR_PARAM(_context, float, forceSmoothingAngleLimit)
	{
		result = (cos_deg<float>(*forceSmoothingAngleLimit));
	}
	return result;
}

void MeshGeneration::load_ipu_parameters(::Meshes::Builders::IPU & _ipu, GenerationContext & _context)
{
	Optional<float> smoothingDotLimit = get_smoothing_dot_limit(_context);
	if (smoothingDotLimit.is_set())
	{
		_ipu.set_smoothing_dot_limit(smoothingDotLimit.get());
	}
	{
		float v = 0.0f;
		if (_context.restore_value<float>(NAME(ipuWeldingDistance), v))
		{
			_ipu.set_welding_distance(v);
		}
	}
}

void MeshGeneration::apply_transform_to_vertex_data(Array<int8> & _vertexData, int _numberOfVertices, ::System::VertexFormat const & _vertexFormat, Matrix44 const & _transform)
{
	int stride = _vertexFormat.get_stride();
	{
		int8* currentVertexData = _vertexData.get_data() + _vertexFormat.get_location_offset();
		for (int vrtIdx = 0; vrtIdx < _numberOfVertices; ++vrtIdx, currentVertexData += stride)
		{
			if (_vertexFormat.get_location() == ::System::VertexFormatLocation::XYZ)
			{
				Vector3* location = (Vector3*)(currentVertexData);
				*location = _transform.location_to_world(*location);
			}
		}

		for_every(cd, _vertexFormat.get_custom_data())
		{
			if (cd->attribType == ::System::VertexFormatAttribType::Location ||
				cd->attribType == ::System::VertexFormatAttribType::Vector)
			{
				int8* currentVertexData = _vertexData.get_data();
				int offset = _vertexFormat.get_custom_data_offset(cd->name);
				for (int vrtIdx = 0; vrtIdx < _numberOfVertices; ++vrtIdx, currentVertexData += _vertexFormat.get_stride())
				{
					Vector3* out = (Vector3*)(currentVertexData + offset);
					if (cd->attribType == ::System::VertexFormatAttribType::Location)
					{
						*out = _transform.location_to_world(*out);
					}
					if (cd->attribType == ::System::VertexFormatAttribType::Vector)
					{
						*out = _transform.vector_to_world(*out);
					}
				}
			}
		}
	}
	if (_vertexFormat.get_normal() == ::System::VertexFormatNormal::Yes)
	{
		int8* currentVertexData = _vertexData.get_data() + _vertexFormat.get_normal_offset();
		if (_vertexFormat.is_normal_packed())
		{
			for (int vrtIdx = 0; vrtIdx < _numberOfVertices; ++vrtIdx, currentVertexData += stride)
			{
				Vector3 normal = ((VectorPacked*)(currentVertexData))->get_as_vertex_normal();
				normal = _transform.vector_to_world(normal).normal();
				((VectorPacked*)(currentVertexData))->set_as_vertex_normal(normal);
			}
		}
		else
		{
			for (int vrtIdx = 0; vrtIdx < _numberOfVertices; ++vrtIdx, currentVertexData += stride)
			{
				Vector3* normal = (Vector3*)(currentVertexData);
				*normal = _transform.vector_to_world(*normal);
				normal->normalise();
			}
		}
	}
}

void MeshGeneration::apply_transform_to_vertex_data_per_vertex(Array<int8>& _vertexData, int _numberOfVertices, ::System::VertexFormat const& _vertexFormat, std::function<Matrix44(int, Vector3 const&)> _get_transform)
{
	int stride = _vertexFormat.get_stride();
	{
		int8* currentVertexData = _vertexData.get_data() + _vertexFormat.get_location_offset();
		for (int vrtIdx = 0; vrtIdx < _numberOfVertices; ++vrtIdx, currentVertexData += stride)
		{
			if (_vertexFormat.get_location() == ::System::VertexFormatLocation::XYZ)
			{
				Vector3* location = (Vector3*)(currentVertexData);
				Matrix44 transform = _get_transform(vrtIdx, *location);
				*location = transform.location_to_world(*location);
			}
		}

		if (_vertexFormat.get_location() == ::System::VertexFormatLocation::XYZ)
		{
			for_every(cd, _vertexFormat.get_custom_data())
			{
				if (cd->attribType == ::System::VertexFormatAttribType::Location ||
					cd->attribType == ::System::VertexFormatAttribType::Vector)
				{
					int locationOffset = _vertexFormat.get_location_offset();
					int8* currentVertexData = _vertexData.get_data();
					int offset = _vertexFormat.get_custom_data_offset(cd->name);
					for (int vrtIdx = 0; vrtIdx < _numberOfVertices; ++vrtIdx, currentVertexData += _vertexFormat.get_stride())
					{
						Vector3* loc = (Vector3*)(currentVertexData + locationOffset);
						Vector3* out = (Vector3*)(currentVertexData + offset);
						Matrix44 transform = _get_transform(vrtIdx, *loc);
						if (cd->attribType == ::System::VertexFormatAttribType::Location)
						{
							*out = transform.location_to_world(*out);
						}
						if (cd->attribType == ::System::VertexFormatAttribType::Vector)
						{
							*out = transform.vector_to_world(*out);
						}
					}
				}
			}
		}
		else
		{
			todo_implement;
		}
	}
	if (_vertexFormat.get_normal() == ::System::VertexFormatNormal::Yes)
	{
		if (_vertexFormat.get_location() == ::System::VertexFormatLocation::XYZ)
		{
			int8* currentVertexData = _vertexData.get_data();
			int locationOffset = _vertexFormat.get_location_offset();
			int normalOffset = _vertexFormat.get_normal_offset();
			if (_vertexFormat.is_normal_packed())
			{
				for (int vrtIdx = 0; vrtIdx < _numberOfVertices; ++vrtIdx, currentVertexData += stride)
				{
					Vector3* loc = (Vector3*)(currentVertexData + locationOffset);
					Vector3 normal = ((VectorPacked*)(currentVertexData + normalOffset))->get_as_vertex_normal();
					Matrix44 transform = _get_transform(vrtIdx, *loc);
					normal = transform.vector_to_world(normal).normal();
					((VectorPacked*)(currentVertexData + normalOffset))->set_as_vertex_normal(normal);
				}
			}
			else
			{
				for (int vrtIdx = 0; vrtIdx < _numberOfVertices; ++vrtIdx, currentVertexData += stride)
				{
					Vector3* loc = (Vector3*)(currentVertexData + locationOffset);
					Vector3* normal = (Vector3*)(currentVertexData + normalOffset);
					Matrix44 transform = _get_transform(vrtIdx, *loc);
					*normal = transform.vector_to_world(*normal);
					normal->normalise();
				}
			}
		}
		else
		{
			todo_implement;
		}
	}
}

void MeshGeneration::reverse_triangles_on_collision_model(Collision::Model * _model)
{
	_model->reverse_triangles();
}

void MeshGeneration::apply_transform_to_collision_model(Collision::Model * _model, Matrix44 const & _transform)
{
	_model->apply_transform(_transform);
}

void MeshGeneration::apply_transform_to_bone(REF_ Bone & _bone, Matrix44 const & _transform)
{
	apply_transform_to(ApplyTo(_bone), _transform);
}

void MeshGeneration::apply_transform_to(ApplyTo const & _applyTo, Matrix44 const & _transform)
{
	if (_applyTo.bone)
	{
		Matrix44 boneMat = _transform.to_world(_applyTo.bone->placement.to_matrix());
		boneMat.fix_axes_forward_up_right();
		_applyTo.bone->placement = boneMat.to_transform();
	}
}

Collision::Model* MeshGeneration::create_collision_box_from(::Meshes::Builders::IPU const & _ipu, GenerationContext const & _context, ElementInstance const & _instance, CreateCollisionInfo const & _ccInfo, int _startingAtPolygon, int _polygonCount)
{
	if (_ccInfo.createCondition.is_set() &&
		!_ccInfo.createCondition.get(_context))
	{
		return nullptr;
	}

	Collision::Model* model = new Collision::Model();
	Range3 range = Range3::empty;
	PhysicalMaterial* providedPhysicalMaterial = _ccInfo.get_provided_physical_material(_context);
	PhysicalMaterial* physicalMaterial = providedPhysicalMaterial;
	int skinToBoneIdx = NONE;
	ArrayStatic<float, 64> us; SET_EXTRA_DEBUG_INFO(us, TXT("MeshGeneration::create_collision_box_from[1].us"));
	_ipu.for_every_triangle_simple_skinning_and_u([&range, &skinToBoneIdx, &us](Vector3 const & _a, Vector3 const & _b, Vector3 const & _c, int _skinToBoneIdx, float _u)
	{
		range.include(_a);
		range.include(_b);
		range.include(_c);
		if (skinToBoneIdx == NONE)
		{
			skinToBoneIdx = _skinToBoneIdx;
		}
		us.push_back_unique(_u);
	}
	, _startingAtPolygon, _polygonCount
	);
	if (!providedPhysicalMaterial)
	{
		int materialIndex = _instance.get_material_index_from_params();
		for_every(u, us)
		{
			auto * newPhysicalMaterial = _ccInfo.get_physical_material(_context, materialIndex, *u);
			if (newPhysicalMaterial != physicalMaterial && physicalMaterial)
			{
				warn_generating_mesh(_instance, TXT("different physical materials when creating multipart collision, provide physical material implicitly or break down into smaller collision objects"));
			}
			if (!physicalMaterial)
			{
				physicalMaterial = newPhysicalMaterial;
			}
		}
	}

	if (range.is_empty())
	{
		delete model;
		return nullptr;
	}

	Vector3 sizeCoef = _ccInfo.get_size_coef(_context);
	range.scale_relative_to_centre(sizeCoef);

	if (_ccInfo.makeSizeValid)
	{
		make_range_valid(REF_ range);
	}

	Collision::Box box;
	if (range.x.length() == 0.0f) range.x.expand_by(0.001f);
	if (range.y.length() == 0.0f) range.y.expand_by(0.001f);
	if (range.z.length() == 0.0f) range.z.expand_by(0.001f);
	box.setup(range);

	if (_context.has_skeleton())
	{
		if (skinToBoneIdx != NONE)
		{
			box.set_bone(_context.get_bone_name(skinToBoneIdx));
		}
	}
	if (physicalMaterial)
	{
		box.use_material(physicalMaterial);
	}
	model->add(box);
	return model;
}

Collision::Model* MeshGeneration::create_collision_box_from(GenerationContext const & _context, CreateCollisionInfo const & _ccInfo, int _startAtPart, int _partCount)
{
	if (_ccInfo.createCondition.is_set() &&
		!_ccInfo.createCondition.get(_context))
	{
		return nullptr;
	}

	int endPart = _partCount == NONE ? _context.get_parts().get_size() - 1 : min(_context.get_parts().get_size() - 1, _startAtPart + _partCount);
	Range3 range = Range3::empty;
	int skinToBoneIdx = NONE;
	PhysicalMaterial* providedPhysicalMaterial = _ccInfo.get_provided_physical_material(_context);
	PhysicalMaterial* physicalMaterial = providedPhysicalMaterial;
	for_range(int, i, _startAtPart, endPart)
	{
		auto const & part = _context.get_parts()[i];
		ArrayStatic<float, 64> us; SET_EXTRA_DEBUG_INFO(us, TXT("MeshGeneration::create_collision_box_from[2].us"));
		part->for_every_triangle_simple_skinning_and_u([&range, &skinToBoneIdx, &us](Vector3 const & _a, Vector3 const & _b, Vector3 const & _c, int _skinToBoneIdx, float _u)
		{
			range.include(_a);
			range.include(_b);
			range.include(_c);
			if (skinToBoneIdx == NONE)
			{
				skinToBoneIdx = _skinToBoneIdx;
			}
			us.push_back_unique(_u);
		});
		if (! providedPhysicalMaterial)
		{
			int materialIndex = _context.get_material_index_for_part(part.get());
			for_every(u, us)
			{
				auto * newPhysicalMaterial = _ccInfo.get_physical_material(_context, materialIndex, *u);
				if (newPhysicalMaterial != physicalMaterial && physicalMaterial)
				{
					warn_generating_mesh_element(nullptr, TXT("different physical materials when creating multipart collision (defined for whole object), provide physical material implicitly or break down into smaller collision objects"));
				}
				if (!physicalMaterial)
				{
					physicalMaterial = newPhysicalMaterial;
				}
			}
		}
	}
	if (range.is_empty())
	{
		return nullptr;
	}

	if (_ccInfo.makeSizeValid)
	{
		make_range_valid(REF_ range);
	}

	Collision::Model* model = new Collision::Model();
	Collision::Box box;
	if (range.x.length() == 0.0f)
	{
		range.x.expand_by(0.001f);
	}
	if (range.y.length() == 0.0f)
	{
		range.y.expand_by(0.001f);
	}
	if (range.z.length() == 0.0f)
	{
		range.z.expand_by(0.001f);
	}
	box.setup(range);
	if (_context.has_skeleton())
	{
		if (skinToBoneIdx != NONE)
		{
			box.set_bone(_context.get_bone_name(skinToBoneIdx));
		}
	}
	if (physicalMaterial)
	{
		box.use_material(physicalMaterial);
	}
	model->add(box);
	return model;
}

Collision::Model* MeshGeneration::create_collision_box_at(Matrix44 const & _at, ::Meshes::Builders::IPU const & _ipu, GenerationContext const & _context, ElementInstance const & _instance, CreateCollisionInfo const & _ccInfo, int _startingAtPolygon, int _polygonCount)
{
	if (_ccInfo.createCondition.is_set() &&
		!_ccInfo.createCondition.get(_context))
	{
		return nullptr;
	}

	Collision::Model* model = new Collision::Model();
	Range3 range = Range3::empty;
	PhysicalMaterial* providedPhysicalMaterial = _ccInfo.get_provided_physical_material(_context);
	PhysicalMaterial* physicalMaterial = providedPhysicalMaterial;
	int skinToBoneIdx = NONE;
	ArrayStatic<float, 64> us; SET_EXTRA_DEBUG_INFO(us, TXT("MeshGeneration::create_collision_box_at.us"));
	_ipu.for_every_triangle_simple_skinning_and_u([_at, &range, &skinToBoneIdx, &us](Vector3 const & _a, Vector3 const & _b, Vector3 const & _c, int _skinToBoneIdx, float _u)
	{
		range.include(_at.location_to_local(_a));
		range.include(_at.location_to_local(_b));
		range.include(_at.location_to_local(_c));
		if (skinToBoneIdx == NONE)
		{
			skinToBoneIdx = _skinToBoneIdx;
		}
		us.push_back_unique(_u);
	}
		, _startingAtPolygon, _polygonCount
		);
	if (!providedPhysicalMaterial)
	{
		int materialIndex = _instance.get_material_index_from_params();
		for_every(u, us)
		{
			auto * newPhysicalMaterial = _ccInfo.get_physical_material(_context, materialIndex, *u);
			if (newPhysicalMaterial != physicalMaterial && physicalMaterial)
			{
				warn_generating_mesh(_instance, TXT("different physical materials when creating multipart collision, provide physical material implicitly or break down into smaller collision objects"));
			}
			if (!physicalMaterial)
			{
				physicalMaterial = newPhysicalMaterial;
			}
		}
	}

	if (_ccInfo.makeSizeValid)
	{
		make_range_valid(REF_ range);
	}

	Collision::Box box;
	{
		if (range.x.length() == 0.0f) range.x.expand_by(0.001f);
		if (range.y.length() == 0.0f) range.y.expand_by(0.001f);
		if (range.z.length() == 0.0f) range.z.expand_by(0.001f);
		box.setup(_at.get_translation(), range, _at.get_x_axis(), _at.get_y_axis(), _at.get_z_axis());
	}

	if (_context.has_skeleton())
	{
		if (skinToBoneIdx != NONE)
		{
			box.set_bone(_context.get_bone_name(skinToBoneIdx));
		}
	}
	if (physicalMaterial)
	{
		box.use_material(physicalMaterial);
	}
	model->add(box);
	return model;
}

struct PhysicalMaterialLookUp
{
	int partIdx = 0;
	int materialIndex = 0;
	int skinToBoneIdx = NONE;
	float u = 0.0f;
	PhysicalMaterial* physicalMaterial = nullptr;

	PhysicalMaterialLookUp() {}
	PhysicalMaterialLookUp(int _partIdx, int _materialIndex, int _skinToBoneIdx, float _u, PhysicalMaterial* _physicalMaterial) : partIdx(_partIdx), materialIndex(_materialIndex), skinToBoneIdx(_skinToBoneIdx), u(_u), physicalMaterial(_physicalMaterial) {}

	static PhysicalMaterialLookUp* get(Array<PhysicalMaterialLookUp> & _in, int _partIdx, int _materialIndex, int _skinToBoneIdx, float _u)
	{
		for_every(pml, _in)
		{
			if (pml->partIdx == _partIdx &&
				pml->materialIndex == _materialIndex &&
				pml->skinToBoneIdx == _skinToBoneIdx &&
				pml->u == _u)
			{
				return pml;
			}
		}
		return nullptr;
	}

	static PhysicalMaterialLookUp const * get(Array<PhysicalMaterialLookUp> const & _in, int _partIdx, int _materialIndex, int _skinToBoneIdx, float _u)
	{
		for_every(pml, _in)
		{
			if (pml->partIdx == _partIdx &&
				pml->materialIndex == _materialIndex &&
				pml->skinToBoneIdx == _skinToBoneIdx &&
				pml->u == _u)
			{
				return pml;
			}
		}
		return nullptr;
	}

	static PhysicalMaterial* find(Array<PhysicalMaterialLookUp> const & _in, int _partIdx, int _materialIndex, int _skinToBoneIdx, float _u)
	{
		if (auto* pml = get(_in, _partIdx, _materialIndex, _skinToBoneIdx, _u))
		{
			return pml->physicalMaterial;
		}
		return nullptr;
	}
};

struct PhysicalMaterialSkinned
{
	PhysicalMaterial* physicalMaterial = nullptr;
	int skinToBoneIdx = NONE;

	PhysicalMaterialSkinned() {}
	PhysicalMaterialSkinned(PhysicalMaterial* _physicalMaterial, int _skinToBoneIdx) : physicalMaterial(_physicalMaterial), skinToBoneIdx(_skinToBoneIdx) {}

	bool operator==(PhysicalMaterialSkinned const & _other) const { return physicalMaterial == _other.physicalMaterial && skinToBoneIdx == _other.skinToBoneIdx; }
};

Array<Collision::Model*> MeshGeneration::create_collision_meshes_from(::Meshes::Builders::IPU const & _ipu, GenerationContext const & _context, ElementInstance const & _instance, CreateCollisionInfo const & _ccInfo, int _startingAtPolygon, int _polygonCount)
{
	Array<Collision::Model*> models;

	if (_ccInfo.createCondition.is_set() &&
		!_ccInfo.createCondition.get(_context))
	{
		return models;
	}

	if (!_ipu.is_empty())
	{
		// gather information about physical materials (how many etc)
		Array<PhysicalMaterialSkinned> physicalMaterialSkinneds;
		Array<PhysicalMaterialLookUp> physicalMaterialLookUps;

		int partIdx = 0;
		int materialIndex = _instance.get_material_index_from_params();
		_ipu.for_every_triangle_simple_skinning_and_u([&_context, &_ccInfo, &physicalMaterialSkinneds, &physicalMaterialLookUps, partIdx, materialIndex](Vector3 const & _a, Vector3 const & _b, Vector3 const & _c, int _skinToBoneIdx, float _u)
		{
			if (Plane(_a, _b, _c).get_normal().is_almost_zero())
			{
				// skip it
				return;
			}
			if (!PhysicalMaterialLookUp::get(physicalMaterialLookUps, partIdx, materialIndex, _skinToBoneIdx, _u))
			{
				PhysicalMaterial* pm = _ccInfo.get_physical_material(_context, materialIndex, _u);
				physicalMaterialSkinneds.push_back_unique(PhysicalMaterialSkinned(pm, _skinToBoneIdx));
				physicalMaterialLookUps.push_back(PhysicalMaterialLookUp(partIdx, materialIndex, _skinToBoneIdx, _u, pm));
			}
		}
		, _startingAtPolygon, _polygonCount
		);

		// build actual meshes, combined by physical material
		for_every(physicalMaterialSkinned, physicalMaterialSkinneds)
		{
			Collision::Mesh mesh;
			_ipu.for_every_triangle_simple_skinning_and_u([&physicalMaterialSkinned, &physicalMaterialLookUps, partIdx, materialIndex, &mesh](Vector3 const & _a, Vector3 const & _b, Vector3 const & _c, int _skinToBoneIdx, float _u)
			{
				if (Plane(_a, _b, _c).get_normal().is_almost_zero())
				{
					// skip it
					return;
				}
				if (physicalMaterialSkinned->skinToBoneIdx == _skinToBoneIdx)
				{
					auto* pm = PhysicalMaterialLookUp::find(physicalMaterialLookUps, partIdx, materialIndex, _skinToBoneIdx, _u);
					if (physicalMaterialSkinned->physicalMaterial == pm)
					{
						mesh.add(_a, _b, _c);
					}
				}
			}
			, _startingAtPolygon, _polygonCount
			);
			if (_context.has_skeleton())
			{
				if (physicalMaterialSkinned->skinToBoneIdx != NONE)
				{
					mesh.set_bone(_context.get_bone_name(physicalMaterialSkinned->skinToBoneIdx));
				}
			}
			Collision::Model* model = new Collision::Model();
			if (physicalMaterialSkinned->physicalMaterial)
			{
				mesh.use_material(physicalMaterialSkinned->physicalMaterial);
			}
			model->add(mesh);
			models.push_back(model);
		}
	}

	return models;
}

Array<Collision::Model*> MeshGeneration::create_collision_meshes_from(GenerationContext const & _context, CreateCollisionInfo const & _ccInfo, int _startAtPart, int _partCount)
{
	Array<Collision::Model*> models;

	if (_ccInfo.createCondition.is_set() &&
		!_ccInfo.createCondition.get(_context))
	{
		return models;
	}

	// well, it's a copy of above, gotta think about generalisation
	{
		// gather information about physical materials (how many etc)
		Array<PhysicalMaterialSkinned> physicalMaterialSkinneds;
		Array<PhysicalMaterialLookUp> physicalMaterialLookUps;

		int endPart = _partCount == NONE ? _context.get_parts().get_size() - 1 : min(_context.get_parts().get_size() - 1, _startAtPart + _partCount);
		for_range(int, partIdx, _startAtPart, endPart)
		{
			auto const & part = _context.get_parts()[partIdx].get();
			if (_context.should_use_part_for_collision(part))
			{
				int materialIndex = _context.get_material_index_for_part(part);
				part->for_every_triangle_simple_skinning_and_u([&_context, &_ccInfo, &physicalMaterialSkinneds, &physicalMaterialLookUps, partIdx, materialIndex](Vector3 const & _a, Vector3 const & _b, Vector3 const & _c, int _skinToBoneIdx, float _u)
				{
					if (Plane(_a, _b, _c).get_normal().is_almost_zero())
					{
						// skip it
						return;
					}
					if (!PhysicalMaterialLookUp::get(physicalMaterialLookUps, partIdx, materialIndex, _skinToBoneIdx, _u))
					{
						PhysicalMaterial* pm = _ccInfo.get_physical_material(_context, materialIndex, _u);
						physicalMaterialSkinneds.push_back_unique(PhysicalMaterialSkinned(pm, _skinToBoneIdx));
						physicalMaterialLookUps.push_back(PhysicalMaterialLookUp(partIdx, materialIndex, _skinToBoneIdx, _u, pm));
					}
				});
			}
		}

		// build actual meshes, combined by physical material
		for_every(physicalMaterialSkinned, physicalMaterialSkinneds)
		{
			Collision::Mesh mesh;
			for_range(int, partIdx, _startAtPart, endPart)
			{
				auto const & part = _context.get_parts()[partIdx].get();
				if (_context.should_use_part_for_collision(part))
				{
					int materialIndex = _context.get_material_index_for_part(part);

					part->for_every_triangle_simple_skinning_and_u([&physicalMaterialSkinned, &physicalMaterialLookUps, partIdx, materialIndex, &mesh](Vector3 const & _a, Vector3 const & _b, Vector3 const & _c, int _skinToBoneIdx, float _u)
					{
						if (Plane(_a, _b, _c).get_normal().is_almost_zero())
						{
							// skip it
							return;
						}
						if (physicalMaterialSkinned->skinToBoneIdx == _skinToBoneIdx)
						{
							auto* pm = PhysicalMaterialLookUp::find(physicalMaterialLookUps, partIdx, materialIndex, _skinToBoneIdx, _u);
							if (physicalMaterialSkinned->physicalMaterial == pm)
							{
								mesh.add(_a, _b, _c);
							}
						}
					});
				}
			}

			if (_context.has_skeleton())
			{
				if (physicalMaterialSkinned->skinToBoneIdx != NONE)
				{
					mesh.set_bone(_context.get_bone_name(physicalMaterialSkinned->skinToBoneIdx));
				}
			}
			Collision::Model* model = new Collision::Model();
			if (physicalMaterialSkinned->physicalMaterial)
			{
				mesh.use_material(physicalMaterialSkinned->physicalMaterial);
			}
			model->add(mesh);
			models.push_back(model);
		}
	}

	return models;
}

Array<Collision::Model*> MeshGeneration::create_collision_skinned_meshes_from(GenerationContext const & _context, CreateCollisionInfo const & _ccInfo, int _startAtPart, int _partCount)
{
	Array<Collision::Model*> models;

	if (_ccInfo.createCondition.is_set() &&
		!_ccInfo.createCondition.get(_context))
	{
		return models;
	}

	// well, it's a copy of above, gotta think about generalisation
	{
		// gather information about physical materials (how many etc)
		Array<PhysicalMaterialSkinned> physicalMaterialSkinneds;
		Array<PhysicalMaterialLookUp> physicalMaterialLookUps;

		int endPart = _partCount == NONE ? _context.get_parts().get_size() - 1 : min(_context.get_parts().get_size() - 1, _startAtPart + _partCount);
		for_range(int, partIdx, _startAtPart, endPart)
		{
			auto const & part = _context.get_parts()[partIdx].get();
			if (_context.should_use_part_for_collision(part))
			{
				int materialIndex = _context.get_material_index_for_part(part);
				part->for_every_triangle_and_u([&_context, &_ccInfo, &physicalMaterialSkinneds, &physicalMaterialLookUps, partIdx, materialIndex](Vector3 const & _a, Vector3 const & _b, Vector3 const & _c, float _u)
				{
					if (Plane(_a, _b, _c).get_normal().is_almost_zero())
					{
						// skip it
						return;
					}
					if (!PhysicalMaterialLookUp::get(physicalMaterialLookUps, partIdx, materialIndex, NONE, _u))
					{
						PhysicalMaterial* pm = _ccInfo.get_physical_material(_context, materialIndex, _u);
						physicalMaterialSkinneds.push_back_unique(PhysicalMaterialSkinned(pm, NONE));
						physicalMaterialLookUps.push_back(PhysicalMaterialLookUp(partIdx, materialIndex, NONE, _u, pm));
					}
				});
			}
		}

		// build actual meshes, combined by physical material
		for_every(physicalMaterialSkinned, physicalMaterialSkinneds)
		{
			Collision::MeshSkinned mesh;
			for_range(int, partIdx, _startAtPart, endPart)
			{
				auto const & part = _context.get_parts()[partIdx].get();
				if (_context.should_use_part_for_collision(part))
				{
					int materialIndex = _context.get_material_index_for_part(part);

					part->for_every_triangle_full_skinning_and_u([&physicalMaterialSkinned, &physicalMaterialLookUps, partIdx, materialIndex, &mesh](Vector3 const & _a, Vector3 const & _b, Vector3 const & _c, Meshes::VertexSkinningInfo const & _aSkinning, Meshes::VertexSkinningInfo const & _bSkinning, Meshes::VertexSkinningInfo const & _cSkinning, float _u)
					{
						if (Plane(_a, _b, _c).get_normal().is_almost_zero())
						{
							// skip it
							return;
						}
						if (physicalMaterialSkinned->skinToBoneIdx == NONE)
						{
							auto* pm = PhysicalMaterialLookUp::find(physicalMaterialLookUps, partIdx, materialIndex, NONE, _u);
							if (physicalMaterialSkinned->physicalMaterial == pm)
							{
								mesh.add(_a, _b, _c, _aSkinning, _bSkinning, _cSkinning);
							}
						}
					});
				}
			}

			Collision::Model* model = new Collision::Model();
			if (physicalMaterialSkinned->physicalMaterial)
			{
				mesh.use_material(physicalMaterialSkinned->physicalMaterial);
			}
			model->add(mesh);
			models.push_back(model);
		}
	}

	return models;
}

SpaceBlocker MeshGeneration::create_space_blocker_from(::Meshes::Builders::IPU const& _ipu, GenerationContext const& _context, ElementInstance const& _instance, CreateSpaceBlockerInfo const& _csbInfo, int _startingAtPolygon, int _polygonCount)
{
	if (_csbInfo.createCondition.is_set() &&
		!_csbInfo.createCondition.get(_context))
	{
		return SpaceBlocker::none();
	}

	Range3 range = Range3::empty;
	_ipu.for_every_triangle_simple_skinning_and_u([&range](Vector3 const& _a, Vector3 const& _b, Vector3 const& _c, int _skinToBoneIdx, float _u)
	{
		range.include(_a);
		range.include(_b);
		range.include(_c);
	}
	, _startingAtPolygon, _polygonCount
	);

	if (range.is_empty())
	{
		return SpaceBlocker::none();
	}

	SpaceBlocker sb;
	sb.box.setup(range);
	sb.blocks = _csbInfo.get_blocks(_context);
	sb.requiresToBeEmpty = _csbInfo.get_requires_to_be_empty(_context);
	return sb;
}

SpaceBlocker MeshGeneration::create_space_blocker_from(GenerationContext const& _context, CreateSpaceBlockerInfo const& _csbInfo, int _startAtPart, int _partCount)
{
	if (_csbInfo.createCondition.is_set() &&
		!_csbInfo.createCondition.get(_context))
	{
		return SpaceBlocker::none();
	}

	int endPart = _partCount == NONE ? _context.get_parts().get_size() - 1 : min(_context.get_parts().get_size() - 1, _startAtPart + _partCount);
	Range3 range = Range3::empty;
	for_range(int, i, _startAtPart, endPart)
	{
		auto const& part = _context.get_parts()[i];
		part->for_every_triangle_simple_skinning_and_u([&range](Vector3 const& _a, Vector3 const& _b, Vector3 const& _c, int _skinToBoneIdx, float _u)
		{
			range.include(_a);
			range.include(_b);
			range.include(_c);
		});
	}
	if (range.is_empty())
	{
		return SpaceBlocker::none();
	}
	SpaceBlocker sb;
	sb.box.setup(range);
	sb.blocks = _csbInfo.get_blocks(_context);
	sb.requiresToBeEmpty = _csbInfo.get_requires_to_be_empty(_context);
	return sb;
}
