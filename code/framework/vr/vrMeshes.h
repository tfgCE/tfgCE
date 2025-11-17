#pragma once

#include "..\library\libraryLoadLevel.h"
#include "..\library\usedLibraryStored.h"

#include "..\..\core\containers\map.h"
#include "..\..\core\math\math.h"
#include "..\..\core\mesh\mesh3dInstance.h"
#include "..\..\core\types\name.h"

namespace Concurrency
{
	class JobExecutionContext;
};

namespace Framework
{
	class Library;
	class Mesh;

	namespace Render
	{
		class Scene;
	};

	class VRMeshes
	{
	public:
		VRMeshes(Library* _library);
		Mesh* get_mesh_for_hand(int _handIndex);

		bool unload(LibraryLoadLevel::Type _libraryLoadLevel);

		void add_hands_to_render_scenes(Render::Scene** _scenes, int _sceneCount, Transform const & _vrAnchor = Transform::identity);

	private:
		Library* library = nullptr;
		Concurrency::SpinLock meshesLock = Concurrency::SpinLock(TXT("Framework.VRMeshes.meshesLock"));
		Map<Name, UsedLibraryStored<Mesh>> meshes;

		Meshes::Mesh3DInstance instances[2];
		static void load_render_model(Concurrency::JobExecutionContext const * _executionContext, void* _data);
	};
};
