#pragma once

#include "schematicElement.h"

#include "..\..\core\containers\array.h"
#include "..\..\core\mesh\mesh3dInstance.h"
#include "..\..\core\system\video\indexFormat.h"
#include "..\..\core\system\video\vertexFormat.h"

namespace Meshes
{
	interface_class IMesh3D;
};

namespace TeaForGodEmperor
{
	struct SchematicLine;

	class Schematic
	: public RefCountObject
	{
	public:
		Schematic();
		~Schematic();

		void add(SchematicElement* _element);
		void cut_outer_lines_with(SchematicLine const & _convexLine);
		void cut_inner_lines_with(SchematicLine const & _convexLine);
		
		void sort_elements();

		void build_mesh();

		Range2 const & get_size() const { return size; }

		Meshes::Mesh3DInstance const& get_mesh_instance() const { return meshInstance; }
		Meshes::Mesh3DInstance const& get_mesh_instance_outline() const { return meshInstanceOutline; }
		Meshes::Mesh3DInstance const& get_mesh_instance_for_overlay() const { return meshInstanceForOverlay; }

		bool load_from_xml(IO::XML::Node const* _node, Framework::LibraryLoadingContext& _lc);

	private:
		Array<RefCountObjectPtr<SchematicElement>> elements;

		Range2 size = Range2::empty;

		Meshes::IMesh3D* mesh3d = nullptr;
		Meshes::IMesh3D* mesh3dOutline = nullptr;
		Meshes::Mesh3DInstance meshInstance;
		Meshes::Mesh3DInstance meshInstanceForOverlay;
		Meshes::Mesh3DInstance meshInstanceOutline;

	};
}

