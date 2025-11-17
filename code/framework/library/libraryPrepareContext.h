#pragma once

#include "..\..\core\globalDefinitions.h"
#include "..\..\core\containers\array.h"
#include "..\..\core\concurrency\spinLock.h"

#include "libraryStoredRenamer.h"

#define IF_PREPARE_LEVEL(_pfgContext, _level) \
	if (_pfgContext.get_current_level() < _level) \
	{ \
		_pfgContext.request_level(_level); \
	} \
	if (_pfgContext.get_current_level() == _level)

#define IF_AFTER_PREPARE_LEVEL(_pfgContext, _level) \
	if (_pfgContext.get_current_level() > _level)

namespace Framework
{
	namespace LibraryPrepareLevel
	{
		enum Type
		{
			StartLevel							=   0,		// very first stage
			  CreateFromTemplates				=	0,		// for objects created from templates this step will be skipped and we will continue with following
															// note: objects created at one level may be not prepared during this level!
			Resolve								=  10,		// resolve, find all objects, some initial stuff that can be done anywhere might be also done here
			  InitialPrepare					=  25,
			  CopySockets						=  35,
			  CombineVoxelMoldLayers			=  40,
			  CombineSpriteLayers				=  40,		// before Auto_SpriteTextureProcessor, provides data for sprite-texture processor
			  Auto_SpriteTextureProcessor		=  45,		// before LoadTexture
			Generate							=  50,
			  GenerateSkeletons					=  60,
			  GenerateMeshes					=  70,
			  GenerateParticleMeshes			=  80,		// has to be done after meshes are generated
			  GenerateAutoCollisionMeshes		=  90,
			LoadFirsts							= 100,
			  LoadTexture						= 100,
			  LoadShader						= 100,
			TexturesLoaded						= 150,
			  LoadFont							= 150,
			LoadShaderProgram					= 200,
			LoadParamsIntoShaders				= 300,
			ShaderProgramsLoaded				= 400,
			  LoadShaderIntoMaterial			= 400,
			LinkMaterialParent					= 500,
			LoadMaterialIntoMesh				= 600,
			  LoadMaterialIntoMeshLibInstance	= 650,
			MaterialsDone						= 700,
			  SetupRoomPieceCombiner			= 700,
			  ConnectNodesInPostProcess			= 700,
			GraphicsLoaded						= 800,
			  BakeWalkersGaits					= 800,
			  InitialiseMeshLibInstance			= 800,
			//
			LevelCount
		};
	};

	struct LibraryPrepareContext
	{
	public:
		LibraryPrepareContext();

		int32 get_current_level() const { return currentLevel; }

		void request_level(int32 _level); // also marks current object as it requires more work
		bool start_next_requested_level();

		// this is done per thread!
		void ready_for_next_object();
		bool does_current_object_require_more_work() const;

		void mark_fail() { failed = true; }
		bool has_failed() const { return failed; }

#ifdef AN_DEVELOPMENT
		bool should_allow_failing() const;
#endif

	private:
		Concurrency::SpinLock levelLock = Concurrency::SpinLock(TXT("Framework.LibraryPrepareContext.levelLock"), 0.1f /* used only during loading, we can live with a longer wait */);

		int32 currentLevel;
		int32 requestedLevel;

		mutable Concurrency::SpinLock currentObjectRequiresMoreWorkLock = Concurrency::SpinLock(TXT("Framework.LibraryPrepareContext.currentObjectRequiresMoreWorkLock"), 0.1f /* used only during loading, we can live with a longer wait */);
		Array<bool> currentObjectRequiresMoreWork;
		bool failed;

		LibraryPrepareContext(LibraryPrepareContext const & _other); // inaccessible
		LibraryPrepareContext& operator=(LibraryPrepareContext const & _other); // inaccessible
	};

};
