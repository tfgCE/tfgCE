#pragma once

#include "..\..\core\memory\softPooledObject.h"
#include "..\..\core\mesh\mesh3dInstanceSetInlined.h"

#include "..\world\environmentParams.h"

namespace System
{
	class ShaderProgramBindingContext;
};

namespace Framework
{
	class DoorInRoom;
	class Room;
	class Environment;

	namespace Render
	{
		class Context;
		class SceneBuildContext;
		class PresenceLinkProxy;
		class RoomProxy;

		class EnvironmentProxy
		: public SoftPooledObject<EnvironmentProxy>
		{
		public:
			static EnvironmentProxy* build(SceneBuildContext & _context, RoomProxy * _forRoom, RoomProxy * _fromRoomProxy = nullptr, DoorInRoom const * _throughDoorFromOuter = nullptr); // _throughDoorFromOuter is door in other room that we entered through 

			void setup_shader_binding_context(::System::ShaderProgramBindingContext * _bindingContext, ::System::VideoMatrixStack const & _matrixStack) const;
			static void setup_shader_binding_context_no_proxy(::System::ShaderProgramBindingContext * _bindingContext, ::System::VideoMatrixStack const & _matrixStack);

			void add_to(SceneBuildContext & _context, REF_ Meshes::Mesh3DRenderingBufferSet & _renderingBuffers) const;

			Room* get_for_room() { return forRoom; }
			RoomProxy* get_for_room_proxy() { return forRoomProxy; }
			Environment* get_environment() { return environment; }

			void log(LogInfoContext& _log) const;

		public:
			Meshes::Mesh3DInstanceSetInlined & access_apearance() { return appearanceCopy; }
			EnvironmentParams &access_params() { return params; }
			Matrix44 const& get_matrix_from_room_proxy() const { return environmentMatrixFromRoomProxy; } // this is placement
			Matrix44 & access_matrix_from_room_proxy() { return environmentMatrixFromRoomProxy; } // this is placement

		protected:
			override_ void on_release();

		private:
			Room * forRoom;
			Environment * environment; // for reference (and debugger)

			RoomProxy* forRoomProxy;

			Matrix44 environmentMatrixFromRoomProxy;
			
			EnvironmentParams params;

			EnvironmentProxy* next;

			// stored appearance
			Meshes::Mesh3DInstanceSetInlined appearanceCopy;

			PresenceLinkProxy* presenceLinks;
		};

	};
};

TYPE_AS_CHAR(Framework::Render::EnvironmentProxy);
