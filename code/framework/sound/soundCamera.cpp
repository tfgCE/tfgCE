#include "soundCamera.h"

#include "..\module\modulePresence.h"

using namespace Framework;
using namespace Framework::Sound;

void Sound::Camera::update_from_owner_to_camera()
{
	fromOwnerToCamera.be_temporary_snapshot();
	fromOwnerToCamera.reset();
	if (! ownerForSoundSourcePresencePath ||
		ownerForSoundSourcePresencePath->get_in_room() == get_in_room())
	{
		return;
	}
	fromOwnerToCamera.find_path(ownerForSoundSourcePresencePath->get_owner(), get_in_room(), get_placement(), true);
}
