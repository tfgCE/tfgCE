#pragma once

#include "..\..\functionParamsStruct.h"

#include "..\..\math\math.h"

struct String;

namespace System
{
	class RenderTarget;

#ifdef BUILD_NON_RELEASE
	struct RenderTargetSaveSpecialOptions
	{
		static RenderTargetSaveSpecialOptions const & get() { return s_renderTargetSaveSpecialOptions; }
		static RenderTargetSaveSpecialOptions& access() { return s_renderTargetSaveSpecialOptions; }

		bool transparent = false;
		void* onlyObject = nullptr;

	private:
		static RenderTargetSaveSpecialOptions s_renderTargetSaveSpecialOptions;
	};
#endif

	struct RenderTargetSave
	{
		RenderTarget* rt = nullptr;

		ADD_FUNCTION_PARAM_PLAIN_INITIAL_DEF(RenderTargetSave, bool, storeRGBA, store_as_rgba, false, true);
		ADD_FUNCTION_PARAM_PLAIN_INITIAL(RenderTargetSave, RangeInt2, frameTo, frame_to, RangeInt2::empty);

		RenderTargetSave(RenderTarget* _rt) : rt(_rt) {}

		bool save(String const& _fileName, bool _addExtension = false);
	};
};
