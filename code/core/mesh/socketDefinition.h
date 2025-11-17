#pragma once

#include "..\math\math.h"

namespace Meshes
{
	struct Pose;
	class Skeleton;

	struct SocketDefinition
	{
	public:
		SocketDefinition();

		bool load_from_xml(IO::XML::Node const * _node);
		static Name load_name_from_xml(IO::XML::Node const * _node);

		void set_name(Name const & _name) { name = _name; }
		void set_bone_name(Name const & _boneName) { boneName = _boneName; }
		void set_placement_MS(Transform const & _placementMS) { placementMS = _placementMS; }
		void clear_placement_MS() { placementMS.clear(); }
		void set_relative_placement(Transform const & _relativePlacement) { an_assert(_relativePlacement.get_orientation().is_normalised()); relativePlacement = _relativePlacement; }
		void clear_relative_placement() { relativePlacement.clear(); }
		void set_generation_id(int _generationId) { generationId = _generationId; }

		Name const & get_name() const { return name; }
		Name const & get_bone_name() const { return boneName; }
		Optional<Transform> const & get_placement_MS() const { return placementMS; }
		Optional<Transform> const & get_relative_placement() const { return relativePlacement; }
		int get_generation_id() const { return generationId; }

		bool serialise(Serialiser & _serialiser);

		void apply_to_placement_MS(Matrix44 const & _mat);

	public:
		static Transform calculate_socket_using_ls(Meshes::SocketDefinition const * _socket, Meshes::Pose const* _poseLS, Meshes::Skeleton const * _skeleton);
		static Transform calculate_socket_using_ms(Meshes::SocketDefinition const * _socket, Meshes::Pose const* _poseMS, Meshes::Skeleton const * _skeleton);
		static Transform calculate_socket_using_ms(Meshes::SocketDefinition const * _socket, Array<Matrix44> const & _matMS, Meshes::Skeleton const* _skeleton);
		static bool get_socket_info(Meshes::SocketDefinition const * _socket, Meshes::Skeleton const * _skeleton, OUT_ int & _boneIdx, OUT_ Transform & _relativePlacement);

	private:
		Name name;
		Name boneName;
		Optional<Transform> placementMS; // model/mesh space (when using bone, it will auto calculate relative - if provided, if not, there's no relative placement)
		Optional<Transform> relativePlacement;
		int generationId = NONE; // to allow for mesh generation to alter right sockets, for game it doesn't matter (it just takes place in memory!)
	};
};
