#pragma once

#include "..\vrSystem.h"

#ifdef AN_WITH_WAVEVR
namespace VR
{
	class WaveVRImpl;

	class WaveVR
	: public VRSystem
	{
		typedef VRSystem base;
	protected: // IVR
		static bool is_available();

		static void create_config_files();

	private:
		WaveVR();
		virtual ~WaveVR();

		friend class IVR;
	};

};
#endif