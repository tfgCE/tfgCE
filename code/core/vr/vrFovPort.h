#pragma once

#include "..\globalDefinitions.h"

struct ovrFovPort_; 

struct Vector2;

struct VRFovPort
{
	// angles!
	DEG_ float left = 0.0f; // should be negative
	DEG_ float right = 0.0f; // should be positive
	DEG_ float up = 0.0f; // should be positive
	DEG_ float down = 0.0f; // should be negative

	VRFovPort() {}
#ifdef AN_WINDOWS
	VRFovPort(ovrFovPort_ const & _ovrFovPort);
#endif

	bool is_sane() const { return left < 0.0f && right > 0.0f && down < 0.0f && up > 0.0f; }

	DEG_ float get_fov() const;
	Vector2 get_lens_centre_offset() const;

	void tan_to_deg();
};