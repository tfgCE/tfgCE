#pragma once

#include "..\vrSystem.h"

#ifdef AN_WITH_OPENVR
namespace VR
{
	class OpenVRImpl;

	class OpenVR
	: public VRSystem
	{
		typedef VRSystem base;
	protected: // IVR
		static bool is_available();

		static void create_config_files();

	private:
		OpenVR();
		virtual ~OpenVR();

		friend class IVR;
	};

};
#endif