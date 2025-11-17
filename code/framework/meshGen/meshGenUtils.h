#pragma once

#include "..\..\core\containers\array.h"
#include "..\..\core\containers\arrayStack.h"
#include "..\..\core\types\optional.h"

#include <functional>

#define FOR_PARAM(context, type, name) \
	if (type const * name = context.get_parameter<type>(NAME(name)))

#define INVALIDATE_PARAM(context, type, param) \
	context.invalidate_parameter<type>(NAME(param));

struct Vector3;
struct Transform;
struct Matrix44;

namespace Collision
{
	class Model;
};

namespace Meshes
{
	struct SocketDefinition;
	namespace Builders
	{
		class IPU;
	};
};

namespace System
{
	struct VertexFormat;
};

namespace Framework
{
	struct MeshNode;
	struct PointOfInterest;
	struct SpaceBlocker;
	class AppearanceControllerData;
	class PhysicalMaterial;

	namespace MeshGeneration
	{
		struct Bone;
		struct CreateCollisionInfo;
		struct CreateSpaceBlockerInfo;

		struct ApplyTo
		{
			Array<int8>* vertexData = nullptr;
			int vertexCount = 0;
			System::VertexFormat const * vertexFormat = nullptr;
			Collision::Model* model = nullptr;
			Bone* bone = nullptr;
			AppearanceControllerData* appearanceController = nullptr;
			Meshes::SocketDefinition* socket = nullptr;
			MeshNode* meshNode = nullptr;
			PointOfInterest* poi = nullptr;
			SpaceBlocker* spaceBlocker = nullptr;

			ApplyTo() {}
			explicit ApplyTo(Array<int8> & _vertexData, int _vertexCount, System::VertexFormat const * _vertexFormat) : vertexData(&_vertexData), vertexCount(_vertexCount), vertexFormat(_vertexFormat) {}
			explicit ApplyTo(Collision::Model* _model) : model(_model) {}
			explicit ApplyTo(Meshes::SocketDefinition * _socket) : socket(_socket) {}
			explicit ApplyTo(Bone& _bone) : bone(&_bone) {}
			explicit ApplyTo(AppearanceControllerData* _appearanceController) : appearanceController(_appearanceController) {}
			explicit ApplyTo(MeshNode * _meshNode) : meshNode(_meshNode) {}
			explicit ApplyTo(PointOfInterest * _poi) : poi(_poi) {}
			explicit ApplyTo(SpaceBlocker * _sb) : spaceBlocker(_sb) {}
		};

		/**
		 *	Auto skinning works only if is forced or there is no skinning info set (all bones set to NONE)
		 */
		struct GenerationContext;
		struct ElementInstance;
		// all of below
		void apply_standard_parameters(REF_ Array<int8> & vertexData, int _vertexCount, GenerationContext & _context, ElementInstance & _instance);
		void apply_standard_parameters(Collision::Model* _model, GenerationContext & _context, ElementInstance & _instance);
		void apply_standard_parameters(Bone& _bone, GenerationContext & _context, ElementInstance & _instance);
		void apply_standard_parameters(AppearanceControllerData* _appearanceController, GenerationContext & _context, ElementInstance & _instance);
		void apply_standard_parameters(Meshes::SocketDefinition * _socket, GenerationContext & _context, ElementInstance & _instance);
		void apply_standard_parameters(MeshNode * _meshNode, GenerationContext & _context, ElementInstance & _instance);
		void apply_standard_parameters(PointOfInterest * _poi, GenerationContext & _context, ElementInstance & _instance);
		void apply_standard_parameters(SpaceBlocker * _sb, GenerationContext& _context, ElementInstance& _instance);
		//
		void invalidate_standard_parameters(GenerationContext & _context, ElementInstance & _instance);
		//
		// and individual
		void apply_scale(ApplyTo const & _applyTo, GenerationContext & _context, Vector3 const & _scale);
		void apply_placement(ApplyTo const & _applyTo, GenerationContext & _context, Transform const & _placement);
		void apply_placement_and_scale_from_parameters(ApplyTo const & _applyTo, GenerationContext & _context, ElementInstance & _instance);
		void apply_colour_and_uv_from_parameters(ApplyTo const & _applyTo, GenerationContext & _context, ElementInstance & _instance);
		void apply_skinning_from_parameters(ApplyTo const & _applyTo, GenerationContext & _context, ElementInstance & _instance);
		//
		void invalidate_placement_and_scale_parameters(GenerationContext & _context, ElementInstance & _instance);
		void invalidate_colour_and_uv_parameters(GenerationContext & _context, ElementInstance & _instance);
		void invalidate_skinning_parameters(GenerationContext & _context, ElementInstance & _instance);
		//
		
		bool get_auto_placement_and_auto_scale(GenerationContext & _context, ElementInstance & _instance, OUT_ Transform & _autoPlacement, OUT_ Vector3 & _autoScale);

		//
		Optional<float> get_smoothing_dot_limit(GenerationContext & _context);
		//

		void load_ipu_parameters(::Meshes::Builders::IPU & _ipu, GenerationContext & _context);

		void apply_transform_to_vertex_data(Array<int8> & _vertexData, int _numberOfVertices, ::System::VertexFormat const & _vertexFormat, Matrix44 const & _transform);
		void apply_transform_to_vertex_data_per_vertex(Array<int8> & _vertexData, int _numberOfVertices, ::System::VertexFormat const & _vertexFormat, std::function<Matrix44(int _idx, Vector3 const&)> _get_transform);
		void apply_transform_to_collision_model(Collision::Model * _model, Matrix44 const & _transform);
		void apply_transform_to_bone(REF_ Bone & _bone, Matrix44 const & _transform);
		void apply_transform_to(ApplyTo const & _applyTo, Matrix44 const & _transform);

		void reverse_triangles_on_collision_model(Collision::Model* _model);

		Collision::Model* create_collision_box_from(::Meshes::Builders::IPU const & _ipu, GenerationContext const & _context, ElementInstance const & _instance, CreateCollisionInfo const & _ccInfo, int _startingAtPolygon = 0, int _polygonCount = NONE);
		Collision::Model* create_collision_box_from(GenerationContext const & _context, CreateCollisionInfo const & _ccInfo, int _startAtPart = 0, int _partCount = NONE);
		Collision::Model* create_collision_box_at(Matrix44 const & _at, ::Meshes::Builders::IPU const & _ipu, GenerationContext const & _context, ElementInstance const & _instance, CreateCollisionInfo const & _ccInfo, int _startingAtPolygon = 0, int _polygonCount = NONE);

		// multiple meshes may be created
		Array<Collision::Model*> create_collision_meshes_from(::Meshes::Builders::IPU const & _ipu, GenerationContext const & _context, ElementInstance const & _instance, CreateCollisionInfo const & _ccInfo, int _startingAtPolygon = 0, int _polygonCount = NONE);
		Array<Collision::Model*> create_collision_meshes_from(GenerationContext const & _context, CreateCollisionInfo const & _ccInfo, int _startAtPart = 0, int _partCount = NONE);
		Array<Collision::Model*> create_collision_skinned_meshes_from(GenerationContext const & _context, CreateCollisionInfo const & _ccInfo, int _startAtPart = 0, int _partCount = NONE);

		SpaceBlocker create_space_blocker_from(::Meshes::Builders::IPU const& _ipu, GenerationContext const& _context, ElementInstance const& _instance, CreateSpaceBlockerInfo const& _csbInfo, int _startingAtPolygon = 0, int _polygonCount = NONE);
		SpaceBlocker create_space_blocker_from(GenerationContext const& _context, CreateSpaceBlockerInfo const& _csbInfo, int _startAtPart = 0, int _partCount = NONE);
	};
};