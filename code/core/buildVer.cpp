#include "buildVer.h"

// validators for defines

#ifdef BUILD_PUBLIC_RELEASE
	#pragma message(" + building: public release")
	#ifdef BUILD_NON_RELEASE
		#error public release should not have BUILD_NON_RELEASE set
	#endif
	#ifdef BUILD_PREVIEW
		#error public release should not have BUILD_PREVIEW set
	#endif
	#ifdef BUILD_PREVIEW_PUBLIC_RELEASE_CANDIDATE
		#error public release should not have BUILD_PREVIEW_PUBLIC_RELEASE_CANDIDATE set
	#endif
#else
	#ifdef BUILD_PREVIEW_RELEASE
		#ifdef BUILD_NON_RELEASE
			#error preview release should not have BUILD_NON_RELEASE set
		#endif
		#ifdef BUILD_PREVIEW_PUBLIC_RELEASE_CANDIDATE
			#pragma message(" + building: public release candidate (preview release)")
		#else
			#ifdef BUILD_PREVIEW
				#pragma message(" + building: preview release")
			#else
				#error preview release should have BUILD_PREVIEW set
			#endif
		#endif
	#else
		#ifdef BUILD_PREVIEW_PUBLIC_RELEASE_CANDIDATE
			#ifdef BUILD_NON_RELEASE
				#error public release candidate should not have BUILD_NON_RELEASE set
			#endif
			#pragma message(" + building: public release candidate (preview)")
		#else
			#ifndef BUILD_NON_RELEASE
				#error non non release build sohuld have BUILD_NON_RELEASE set
			#endif
			#ifdef BUILD_PREVIEW
				#pragma message(" + building: preview")
			#else
				#pragma message(" + building: development/non-preview")
			#endif
		#endif
	#endif
#endif
