#pragma once

#include "..\..\core\math\math.h"

#include "..\library\usedLibraryStored.h"
#include "..\video\material.h"

namespace Framework
{
	class Library;
	class Mesh;
	struct LibraryLoadingContext;
	struct LibraryPrepareContext;
};

namespace Framework
{
	class DisplayHardwareMeshGenerator;
	struct DisplayHardwareMeshSetup;
	
	//

	/**
	 *	Requires one thing to be provided:
	 *		size	- how much space on screen display should take? this can be different to actual mesh, as mesh due to perspective might be bigger or smaller
	 */
	struct DisplayHardwareMeshSetup
	{
		DisplayHardwareMeshSetup();

		DisplayHardwareMeshSetup & with_size_2d(Vector2 const & _size2d) { size2d = _size2d; return *this; }
		DisplayHardwareMeshSetup & with_size_3d(Vector2 const & _size3d) { size3d = _size3d; return *this; }

		bool load_from_xml(IO::XML::Node const * _node, LibraryLoadingContext & _lc);
		bool prepare_for_game(Library* _library, LibraryPrepareContext& _pfgContext);

		Vector2 const & get_size_2d() const { return size2d; }
		Vector2 const & get_size_3d() const { return size3d; }

	public: // allow anyone to alter them
		float sizeMultiplier = 1.0f;

		float cameraAngle = 80.0f; // only for 2d meshes to emulate projection

		// these should be loaded
		Vector2 casingInside = Vector2(0.1f, 0.1f); // (pt!) how much it is inside relative to whole size, in the end it is about how much outside is in relation to diagonal of screen
		float innerCornerInside = 0.1f; // how much of inner space is cut with corner (percentage from corner to centre)
		float outerCornerInside = 0.02f; // how much of outer space is cut with corner (percentage from corner to centre)

		// to calculate curve of screen and casing
		float diagonal = 0.20f; // should be always in meters (this is relative to innerCorner affected by inner corner inside!)
		float depth = 0.005f; // should be always in meters (absolute! centre of screen is always at depth 0, positive, more far away from camera)
		float casingDepth = -0.01f; // (absolute! as above)

		float innerBorderTakes = 0.04f; // how much of display does take border (on edge of display and casing)

		float reflectUVDistanceCoef = 5.0f; // how far into screen do we extend with uv reflection
		float reflectUVPerpendicularCoef = 0.3f; // how we modify uv reflection perpendicular to reflection dir
		
		float darkerReflectionInCornersCoef = 8.0f; // lower, it gets wider, higher, it gets closer to corner

		// density of mesh
		int xCurvePointCount = 17;
		int yCurvePointCount = 17;
		int innerRingCount = 10;
		int outerRingCount = 8;

		Colour screenColour = Colour(1.0f, 1.0f, 1.0f, 1.0f);
		Colour casingColour = Colour(0.5f, 0.5f, 0.5f, 0.5f);

		UsedLibraryStored<Material> material; // has default settings

	private:
		// those have to be set in runtime
		Vector2 size2d = Vector2::zero; // 
		Vector2 size3d = Vector2::zero; // 3d if not provided is calculated from size2d proportion and diagonal

		friend class DisplayHardwareMeshGenerator;
	};

	class DisplayHardwareMeshGenerator
	{
	public:
		// size is in pixels on final screen
		static Mesh* create_2d_mesh(DisplayHardwareMeshSetup const & _setup, Library* _library = nullptr, LibraryName const & _useLibraryName = LibraryName::invalid());
		
		// size is how much physical space it takes
		static Mesh* create_3d_mesh(DisplayHardwareMeshSetup const & _setup, Library* _library = nullptr, LibraryName const & _useLibraryName = LibraryName::invalid(), OPTIONAL_ OUT_ Vector2 * _meshSize = nullptr);

		// create vr scene mesh
		static Mesh* create_vr_scene_mesh(float _radius, Library* _library = nullptr, LibraryName const & _useLibraryName = LibraryName::invalid(), LibraryName const & _useMeshGenerator = LibraryName::invalid());

	private:
		static Mesh* create_mesh(DisplayHardwareMeshSetup const & _setup, bool _2d, Library* _library, LibraryName const & _useLibraryName, OPTIONAL_ OUT_ Vector2 * _meshSize = nullptr);

	};
};
