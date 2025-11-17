#pragma once

#include "..\vrSystem.h"

#ifdef AN_WITH_OCULUS
namespace VR
{
	class OculusImpl;

	class Oculus
	: public VRSystem
	{
		typedef VRSystem base;
	protected: // IVR
		static bool is_available();

	private:
		Oculus();
		virtual ~Oculus();

		friend class IVR;
	};

};
#endif