#pragma once

#include "..\..\core\system\video\shaderProgramBindingContext.h"
#include "..\..\core\mesh\mesh3dInstanceSetInlined.h"
#include "..\..\core\mesh\mesh3dRenderingBufferSet.h"
#include "..\..\core\memory\softPooledObject.h"
#ifdef AN_CLANG
#include "..\world\doorHole.h" // DoorSide enum
#endif
#include "..\world\predefinedRoomOcclusionCulling.h"
#include "..\framework.h"

#include "lightSourceProxy.h"

// settings
#define USE_DEPTH_RANGE
//#define USE_DEPTH_CLAMP


// #define DEBUG_COMPLETE_FRONT_PLANE

#define DEBUG_DRAW_FRUSTUM

#ifdef AN_DEVELOPMENT_OR_PROFILER
#define DEBUG_RENDER_DOOR_HOLES
#endif

namespace Meshes
{
	class Mesh3D;
};

namespace Framework
{
	class DoorHole;
	class DoorInRoom;
	class Room;

#ifndef AN_CLANG
	namespace DoorSide
	{
		enum Type;
	};
#endif

	namespace Render
	{
		class Context;
		class SceneBuildContext;
		class SceneRenderContext;
		class PresenceLinkProxy;
		class EnvironmentProxy;
		class DoorInRoomProxy;
		class Scene;
		class Camera;

		struct RoomProxyRenderingBuffers
		{
			Meshes::Mesh3DRenderingBufferSet buffers;
			Meshes::Mesh3DRenderingBuffer& scene; // main scene
			Meshes::Mesh3DRenderingBuffer& skyBox; // sky box, rendered before background or rendering, whatever comes first
#ifdef AN_USE_BACKGROUND_RENDERING_BUFFER
			Meshes::Mesh3DRenderingBuffer& background; // rendered before rendering
#endif

			RoomProxyRenderingBuffers();

			void clear()
			{
				buffers.clear_each_buffer();
			}

			void sort(Matrix44 const & _currentViewMatrix)
			{
				buffers.sort_each_buffer(_currentViewMatrix);
			}
		};

		class RoomProxy
		: public SoftPooledObject<RoomProxy>
		{
		public:
#ifdef DEBUG_RENDER_DOOR_HOLES
			static bool debugRenderDoorHolesCloseAlwaysDepth;
			static bool debugRenderDoorHolesDontUseDepthRange;
			static bool debugRenderDoorHolesPaddedDepthRange;
			static bool debugRenderDoorHolesPartially;
			static bool debugRenderDoorHolesPartiallyOnlyNormal;
			static bool debugRenderDoorHolesPartiallyOnlyNear;
			static bool debugRenderDoorHolesNoExtend;
			static bool debugRenderDoorHolesNegativeExtend;
			static bool debugRenderDoorHolesFrontCapZSubZero;
			static bool debugRenderDoorHolesFrontCapZZero;
			static bool debugRenderDoorHolesFrontCapZSmall;
			static bool debugRenderDoorHolesFrontCapZLarger;
			static bool debugRenderDoorHolesFrontCapZSmaller;
			static bool debugRenderDoorHolesSmallerCap;
#endif

		public:
			struct DoorInfo
			{
				// rendering portal (only if plane is not zero) (for doorOuter: from outer roomproxy - outboundMatrix is door-in-outer's get_outbound_matrix() stored)
				Plane plane; // in this one
				float distance;
				Matrix44 outboundMatrix; // used only when there's door (door plane is not zero)
				Matrix44 outboundMatrixWithScale; //
				Matrix44 otherRoomMatrix;
				Transform otherRoomTransform;
				Meshes::Mesh3D const* holeOutboundMesh;
				::System::ShaderProgramInstance const* holeShaderProgramInstance;
				DoorSide::Type side;
				DoorHole const* hole;
				Vector2 holeOutboundMeshScale;
#ifdef DEBUG_DRAW_FRUSTUM
				DoorInRoom const* doorInRoomRef;
#endif
			};

		public:
			static RoomProxy* build(SceneBuildContext & _context, Room * _room, RoomProxy * _fromRoomProxy = nullptr, DoorInRoom const * _throughDoorFromOuter = nullptr); // _throughDoorFromOuter is door in other room that we entered through 

			static void adjust_for_camera_offset(REF_ RoomProxy * & _room, REF_ Render::Camera & _renderCamera, Transform const & _offset, OUT_ Transform & _intoRoomTransform);
			static void render(RoomProxy* _room, Context & _context, SceneRenderContext & _sceneRenderContext); // may swap starting room!

			static void set_rendering_depth_range_front(::System::Video3D* v3d);
			static void set_rendering_depth_range_back(::System::Video3D* v3d);
			static void set_rendering_depth_range(::System::Video3D* v3d);
			static void clear_rendering_depth_range(::System::Video3D* v3d);

			EnvironmentProxy* get_environment() { return environment; }

			LightSourceSetForRoomProxy const & get_lights() const { return lights; }
			LightSourceSetForRoomProxy & access_lights() { return lights; }

			DoorInfo const& get_door_we_entered_trough_info() const { return doorLeave; } //  this room space

			PredefinedRoomOcclusionCulling const& get_predefined_occlusion_culling() const { return predefinedRoomOcclusionCulling; }

			void log(LogInfoContext& _log) const;

		protected: friend class SoftPool<RoomProxy>;
			RoomProxy();
			override_ void on_release();

		private:
			Room * room; // for reference (and debugger)
			PredefinedRoomOcclusionCulling predefinedRoomOcclusionCulling;

			int depth;

			DoorInfo doorOuter; // door through which we entered in previous room
			DoorInfo doorLeave; // if we would require to go back from this one to outer
			// door leave is used when we swap two rooms it goes like this
			//
			//								before
			//				room main (1)			room inside (2)
			//				no door outer			door outer (A)
			//				no door leave			door leave (B)
			//
			//							    swapped
			//			   new room main (2)		new room inside (1)
			//				no door outer			door outer (B)
			//				no door leave			door leave (A)
			//

#ifdef RENDER_SCENE_EXTERNAL_VIEW_SUPPORT
			::System::CustomVideoClipPlaneSet<32> externalViewDoorClipPlaneSetRS; // the door we entered through, to clip everything inside, in room space
#endif

			bool softDoorRendering = false;
			bool skipCloseDoorRendering = false;

			RoomProxy* rooms = nullptr;
			RoomProxy* next = nullptr;
			EnvironmentProxy* environment = nullptr;
			DoorInRoomProxy* doors = nullptr;

			LightSourceSetForRoomProxy lights;

			::System::ShaderProgramBindingContext shaderProgramBindingContextCopy;

			// stored appearance
			Meshes::Mesh3DInstanceSetInlined appearanceCopy;

			PresenceLinkProxy* presenceLinks = nullptr;

			RoomProxyRenderingBuffers renderingBuffers;

			void render_worker(Context & _context, SceneRenderContext & _sceneRenderContext, Matrix44 const & _currentViewMatrix) const;

			inline void render_front_plane(Context & _context, ::System::Video3D* _v3d, bool _useFrontPlane, Vector3 const * _frontPlanePoints
#ifdef DEBUG_COMPLETE_FRONT_PLANE
				, Vector3* _completeFrontPlanePoints
#endif
				) const;
			inline void render_far_plane(Context & _context, ::System::Video3D* _v3d, bool _background = false, Optional<Range3> const & _onlyForRange = NP) const;

			void entered_through_door_from_outer(DoorInRoom const * _throughDoorFromOuter, float _distanceToCamera);

			bool does_enter_through_door(Vector3 const & _currLocation, Vector3 const & _nextLocation, OUT_ Vector3 & _destLocation, float _minInFront = -0.01f) const;

			static void swap_main(REF_ RoomProxy * _room, REF_ RoomProxy * _with);

		private:
			struct EnvironmentProxySetupScope
			{
				Context & context;
				::System::ShaderProgramBindingContext prevShaderProgramBindingContext;

				EnvironmentProxySetupScope(Context & _context, ::System::Video3D* _v3d, RoomProxy const * _roomProxy);
				~EnvironmentProxySetupScope();
			};

			friend class EnvironmentProxy;
		};

	};
};

TYPE_AS_CHAR(Framework::Render::RoomProxy);
