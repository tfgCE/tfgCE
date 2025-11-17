#pragma once

#include "..\vrSystem.h"

#ifdef AN_WITH_OCULUS_MOBILE
namespace VR
{
	class OculusMobileImpl;

	class OculusMobile
	: public VRSystem
	{
		typedef VRSystem base;
	protected: // IVR
		static bool is_available();

	private:
		OculusMobile();
		virtual ~OculusMobile();

		friend class IVR;
	};

};
#endif