#pragma once

#include "..\vrSystem.h"

#ifdef AN_WITH_OPENXR
namespace VR
{
	class OpenXRImpl;

	class OpenXR
	: public VRSystem
	{
		typedef VRSystem base;
	protected: // IVR
		static bool is_available();

	private:
		OpenXR(bool _firstChoice);
		virtual ~OpenXR();

		friend class IVR;
	};

};
#endif