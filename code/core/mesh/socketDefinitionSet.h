#pragma once

#include "socketDefinition.h"
#include "..\serialisation\serialisableResource.h"
#include "..\serialisation\importer.h"

namespace Meshes
{
	struct SocketDefinitionSetImportOptions
	{
		String importFromNode; // fbx node
		Matrix33 importBoneMatrix;
		Optional<Transform> importFromPlacement;
		Optional<Transform> placeAfterImport;
		Vector3 importScale;

		String socketNodePrefix;

		SocketDefinitionSetImportOptions();

		bool load_from_xml(IO::XML::Node const * _node);
	};

	class SocketDefinitionSet
	: public SerialisableResource
	{
	public:
		static SocketDefinitionSet* create_new();

		SocketDefinitionSet();
		virtual ~SocketDefinitionSet() {}

		static Importer<SocketDefinitionSet, SocketDefinitionSetImportOptions> & importer() { return s_importer; }

		bool load_from_xml(IO::XML::Node const * _node);
		static IO::XML::Node const * find_xml_socket_node(IO::XML::Node const * _node, Name const & _socketName);

		Array<SocketDefinition> const & get_sockets() const { return sockets; }
		Array<SocketDefinition> & access_sockets() { return sockets; }

		SocketDefinition* find_socket(Name const & _socketName);
		SocketDefinition* find_or_create_socket(Name const & _socketName);

		int find_socket_index(Name const & _socketName) const;

		void add_sockets_from(SocketDefinitionSet const & _set, Optional<Transform> const& _placement = NP);

		void debug_draw_pose_MS(Array<Matrix44> const & _matMS, Skeleton* _skeleton) const;

	public: // SerialisableResource
		virtual bool serialise(Serialiser & _serialiser);

	private:
		static Importer<SocketDefinitionSet, SocketDefinitionSetImportOptions> s_importer;

		Array<SocketDefinition> sockets;
	};
};
