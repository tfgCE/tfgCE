#pragma once

#include "..\presence\relativeToPresencePlacement.h"
#include "..\world\worldCamera.h"

namespace Framework
{
	class DoorInRoom;
	class ModulePresence;

	namespace Sound
	{
		class Context;
		class Scene;

		class Camera
		: public ::Framework::Camera
		{
		public:
			void base_on(::Framework::Camera const & _camera) { ::Framework::Camera::operator =(_camera); }

			void set_additional_offset(Transform const & _additionalOffset) { additionalOffset = _additionalOffset; }
			Transform const & get_additional_offset() const { return additionalOffset; }

			Transform get_total_placement() const { return get_placement().to_world(additionalOffset); }

		protected:
			Transform additionalOffset = Transform::identity; // this is sort of hack, it is used to avoid problems related to breaking presence paths - until it is solved, we will use hacky approach

		private: friend class Scene;
			ModulePresence * ownerForSoundSourcePresencePath = nullptr; // this is used only internally by sound scene
			RelativeToPresencePlacement fromOwnerToCamera; // used with above to get doors

			void update_from_owner_to_camera();
		};

	};
};
