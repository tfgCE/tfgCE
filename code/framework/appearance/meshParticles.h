#pragma once

#include "meshSkinned.h"
#include "..\..\core\wheresMyPoint\iWMPTool.h"

namespace Framework
{
	class Skeleton;
	struct MeshSkinnedMeshImporter;

	/**
	 *	This is special case of skinned mesh (skinned to single bone).
	 *	It should be used by particles.
	 *	It gets single mesh and multiplies it as many times as required (number of particles) and skins to bones.
	 *	Input mesh may be static, skinning information will be added.
	 */
	class MeshParticles
	: public MeshSkinned
	{
		FAST_CAST_DECLARE(MeshParticles);
		FAST_CAST_BASE(MeshSkinned);
		FAST_CAST_END();
		LIBRARY_STORED_DECLARE_TYPE();

		typedef MeshSkinned base;
	public:
		MeshParticles(Library * _library, LibraryName const & _name);

		void get_particle_indices(Name const & _boneParticleType, OUT_ Array<int> & _particleIndices) const;

	public: // LibraryStored
		override_ bool load_from_xml(IO::XML::Node const * _node, LibraryLoadingContext & _lc);
		override_ bool prepare_for_game(Library* _library, LibraryPrepareContext& _pfgContext);

	protected: // Mesh
		override_ bool load_mesh_from_xml(IO::XML::Node const * _node, LibraryLoadingContext & _lc);

	protected:
		virtual ~MeshParticles();

	protected:
		struct Element
		{
			RangeInt count = RangeInt(1, 1);
			Range scale = Range(1.0f, 1.0f);
			Name boneParticleType;
			struct Option
			{
				float probabilityCoef = 1.0f;
				UsedLibraryStored<Mesh> particleMesh;
				WheresMyPoint::ToolSet instanceCustomDataWMP;

				bool load_from_xml(IO::XML::Node const * _node, LibraryLoadingContext & _lc);
				bool prepare_for_game(Library* _library, LibraryPrepareContext& _pfgContext);
			};
			Array<Option> options;

			bool load_from_xml(IO::XML::Node const * _node, LibraryLoadingContext & _lc);
			bool prepare_for_game(Library* _library, LibraryPrepareContext& _pfgContext);
		};
		Array<Element> particleMeshes;
		Array<Name> boneParticleTypes;
		CACHED_ int particleCount = 1; // sum
		bool forceSingleMaterial = false;
	};
};
