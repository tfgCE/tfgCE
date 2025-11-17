#pragma once

#include "..\io\file.h"

#include "..\system\video\video3d.h"
#include "..\system\video\vertexFormat.h"

#include "iMesh3d.h"
#include "primitiveType.h"
#include "usage.h"

namespace Meshes
{
	struct Mesh3DImportOptions;

	/**
	 *	Simple 3d mesh builder (plain vertices)
	 *	Data to load or to render can be prepared by hand although it is advised to create it with builder
	 *
	 *	Usage:
	 *		setup
	 *		add everything
	 *		build
	 */
	class Mesh3DBuilder
	: public IMesh3D
	{
		FAST_CAST_DECLARE(Mesh3DBuilder);
		FAST_CAST_BASE(IMesh3D);
		FAST_CAST_END();
	public:
		Mesh3DBuilder();
		virtual ~Mesh3DBuilder();

		void setup(Primitive::Type _primitiveType, ::System::VertexFormat const & _vertexFormat, uint32 _reserveSpaceForPrimitivesCount = 0);

		bool load_from_file(String const & _fileName, Meshes::Mesh3DImportOptions const & _options);
		bool load_wavefront_obj(IO::File * _file, Meshes::Mesh3DImportOptions const & _options);

		void add_point();
		void add_triangle();

		void location(Vector3 const & _location);
		void normal(Vector3 const & _normal);
		void colour(Colour const & _colour);
		void uv(Vector2 const & _uv);
		void skin_to_single(int _boneIdx);

		void done_point();
		void done_triangle();

		void done();

		void build();

		void clear();
		
		// usage
		int get_number_of_parts() const { return parts.get_size(); }
		void const * get_data(int _part) const;
		uint32 get_number_of_vertices(int _part) const;
		uint32 get_number_of_primitives(int _part) const;
		Primitive::Type get_primitive_type() const { return primitiveType; }
		::System::VertexFormat const & get_vertex_format() const { return vertexFormat; }

		inline Usage::Type get_usage() const { return usage; }
		inline void set_usage(Usage::Type _usage) { usage = _usage; }

	public: // IMesh3D
		implement_ void render(::System::Video3D * _v3d, ::System::IMaterialInstanceProvider const * _materialProvider = nullptr, Mesh3DRenderingContext const & _renderingContext = Mesh3DRenderingContext::none()) const;
		implement_ ::System::MaterialShaderUsage::Type get_material_usage() const { return Usage::usage_to_material_usage(usage); }
		implement_ int calculate_triangles() const;
		implement_ void log(LogInfoContext& _log) const;

	private:
		struct Part
		{
			// just points at data inside builder
			uint32 address;
			uint32 numberOfVertices;
		};

		Usage::Type usage;

		Array<Part*> parts;
		Primitive::Type primitiveType;
		::System::VertexFormat vertexFormat;

		Array<int8> vertexData;
		uint32 numberOfVertices;

		bool openPoint;
		bool openTriangle;

		// current values
		Vector3 inputLocation;
		Vector3 inputNormal;
		Colour inputColour;
		Vector2 inputUV;
		Optional<int> skinToSingle;

		inline int8* get_addr_for(uint32 _idx, uint32 _offset);

		void reset_input();
		void make_space_for(uint32 _for);
		void set_data_size(uint32 _for);

		void* get_current_vertex_data();

		void open_point();

		void load_location(Vector2 const & _location);
		void load_location(Vector3 const & _location);
		void load_normal(Vector3 const & _normal);
		void load_colour(Colour const & _colour);
		void load_uv(Vector2 const & _uv);
		void load_no_skinning();
		void load_skinning_single(int _boneIdx);
	};

};
