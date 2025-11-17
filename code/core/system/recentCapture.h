#pragma once

#include "video/renderTargetPtr.h"

#include "..\concurrency\spinLock.h"
#include "..\math\math.h"

namespace System
{
	struct RecentCapture
	{
	public:
		enum RecentCaptureIndex
		{
			Last,
			FullInGame, // not faded etc

			MAX
		};

		// this is done via Video3D's init and close
		static void initialise_static(); // works as reinitialise
		static void close_static();

		static void store(System::RenderTarget* _what, bool _fullInGame = true);
		
		static void force_next_store_pixel_data();

		static void store_pixel_data(); // copy data to local array - will block CPU waiting for GPU!
		
		static bool read_pixel_data(RecentCaptureIndex _what, OUT_ VectorInt2 & _resolution, OUT_ Array<byte> & _upsideDownPixels, OUT_ int & _bytesPerPixel, OUT_ bool & _convertSRGB);

	private:
		static RecentCapture* s_recentCapture;

		static const int MAX_SIZE = 800; // in any direction

#ifdef WITH_RECENT_CAPTURE
		bool forceStorePixelData = false;
#endif

		Concurrency::SpinLock lock = Concurrency::SpinLock(TXT("System.RecentCapture.lock"));

		::System::RenderTargetPtr recentCapture[MAX]; // anything goes here

		Array<byte> recentCaptureRGBAUpsideDownPixels[MAX];
		VectorInt2 recentCaptureRGBAUpsideDownPixelsSize[MAX];
	};

};
