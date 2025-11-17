#pragma once

#include "..\..\core\memory\softPooledObject.h"
#include "..\..\core\mesh\mesh3dInstance.h"
#include "..\..\core\math\math.h"

namespace Meshes
{
	struct Mesh3DRenderingBuffer;
};

namespace Framework
{
	class DoorInRoom;
	class Room;
	class PresenceLink;
	struct LightSource;

	namespace CustomModules
	{
		class LightSources;
	};

	namespace Render
	{
		class Context;
		class SceneBuildContext;

		struct LightSourceProxy
		{
			// light itself
			Vector3 location = Vector3::zero;
			Vector3 stick = Vector3::zero;
			Vector3 coneDir = Vector3::zero; // if cone light
			Colour colour = Colour::white;
			float distance = 1.0f;
			float power = 1.0f;
			float coneInnerAngle = 0.0f;
			float coneOuterAngle = 0.0f;
			int priority = 0;

			// information about this proxy - we may only have certain amount of lights
			float distanceToOwner = 0.0f;

			void copy_from(LightSource const & _light);

			void turn_into_matrix(OUT_ Matrix44 & _matrix44, Matrix44 const& _referenceModelViewMatrix) const;
		};

		/*
		 *	Represents lights stored per object
		 */
		struct LightSourceSetForPresenceLinkProxy
		{
		public:
			LightSourceSetForPresenceLinkProxy();

			void clear() { lights.clear(); }
			void add(LightSource const & _light, float _distanceToOwner);

		public:
			void setup_shader_program(::System::ShaderProgram* _shaderProgram, Matrix44 const & _referenceModelViewMatrix, Optional<int> const & _maxLights, Optional<int> const & _maxConeLights) const;

		public:
			void on_release();

		private:
			// all lights are stored in room matrix, in setup_shader_program method called reference
			ArrayStatic<LightSourceProxy, 64> lights;
		};


		/*
		 *	Represents lights stored per object
		 */
		struct LightSourceSetForRoomProxy
		{
		public:
			LightSourceSetForRoomProxy();

			void clear() { lights.clear(); }
			void add(LightSource const & _light, Transform const & _placement); // light is placed using _placement 
			void add(CustomModules::LightSources const * _lightSources, Transform const & _placement); // light is placed using _placement 

		public:
			void prepare(Optional<int> const & _maxLights = NP, Optional<int> const & _maxConeLights = NP);
			void setup_shader_binding_context_program_with_prepared(::System::ShaderProgramBindingContext& _shaderProgramBindingContext, System::VideoMatrixStack const& _matrixStack) const;

		public:
			void on_release();

		private:
			// all lights are stored in room matrix, in setup_shader_program method called reference
			ArrayStatic<LightSourceProxy, 64> lights;
			
			bool prepared = false;
			ArrayStatic<Matrix44, 64> preparedLights;
			ArrayStatic<Matrix44, 64> preparedConeLights;
		};

	};
};
