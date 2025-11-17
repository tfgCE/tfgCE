#pragma once

#include "..\system\video\video.h"
#include "..\system\video\video3d.h"
#include "..\system\video\vertexFormat.h"
#include "..\system\video\indexFormat.h"

#include "iMesh3d.h"
#include "primitiveType.h"
#include "usage.h"

#include "mesh3dPart.h"
#include "mesh3dInstanceSet.h"

#include "..\serialisation\serialiserBasics.h"

namespace System
{
	struct MaterialInstance;
};

namespace Meshes
{
	class Mesh3DInstanceSet;

	/**
	 *	Combined set of mesh3d instances (as multiple meshes) and actual mesh3d instance set.
	 *
	 *	It is actually both mesh3d instance set...
	 *	and mesh3d data used by that instance set (might be multiple meshes)
	 *
	 *	At the moment it can be only based on mesh3d (cannot be combination of combined mesh3d).
	 *	Holds mesh3dInstanceSet that contains materials and meshes that couldn't be combined.
	 *
	 *	It gets mesh parts that are setup in same way, they are combined into one entry in this class.
	 *	Each entry is new mesh that is actually skinned-to-single-bone mesh.
	 *	Each entry is for separate material (which means that if we have 10 3-part meshes in source and meshes are identical except for that each part has different material setup,
	 *	we will end with 3 entries per material).
	 *
	 *	It also stores array that points from original Mesh3dInstanceSet to entries. This is used to allow moving things around from something external (for example: room).
	 *
	 *	Is implementation of IMesh3D allowing it to be used by Mesh3dInstance
	 */
	class CombinedMesh3DInstanceSet
	{
	public:
		CombinedMesh3DInstanceSet();
		~CombinedMesh3DInstanceSet();

		void clear(); // should be called only from main thread when actually being destroyed

		void combine(Mesh3DInstanceSet const & _sourceSet);

		Mesh3DInstanceSet & access_as_instance_set() { return instanceSet; }
		Mesh3DInstanceSet const & get_as_instance_set() const { return instanceSet; }

		void set_instance_placement(int _meshIndex, Transform const & _placement);

	private:
		/** Instance set that is used when creating proxy for rendering.
		 *	If there was no mesh created, it is normal instance here.
		 */
		Mesh3DInstanceSet instanceSet;

		struct CombinedMesh
		{
			Mesh3D* mesh; // each mesh has just one part - because otherwise we could waste bones (I guess that in many cases)
			Array<Transform> placements;
			// relevant only while building - source
			RefCountObjectPtr<Mesh3DPart> firstSourceMeshPart; // pointer to first encountered
			::System::MaterialInstance const * sourceMaterialInstance; // pointer to first encountered
			bool immovable;

			~CombinedMesh();
		};
		// meshes that were created by combining source instance set
		Array<CombinedMesh*> combinedMeshes;

		/**
		 *	Two classes below are used to point from source mesh 3d instance set index to parts in this combined mesh and bone index.
		 *	This is because original mesh might be put into separate parts (due to different materials used for different parts).
		 */
		struct SourceMesh3DInstancePartRedirector
		{
			int meshIndex; // index of mesh
			int partIndex; // part within mesh
			int boneIndex; // bone in mesh/part - as each source mesh is bound to single bone
		};
		struct SourceMesh3DInstanceRedirector
		{
			bool wasCombined; // if was combined, use partRedirectors; if wasn't combined, just use instance placement
			int meshIndex; // to be used when wasn't combined
			Array<SourceMesh3DInstancePartRedirector> partRedirectors; // to be used when was combined, index is index of part within instance in source mesh3dInstanceSet

			~SourceMesh3DInstanceRedirector();
		};
		Array<SourceMesh3DInstanceRedirector*> instanceRedirectors; // index is index of instance in source mesh3dInstanceSet

	};

};
