#pragma once

#include "..\..\globalDefinitions.h"
#include "..\..\math\math.h"

#include "video.h"

namespace System
{
#ifdef AN_GL
	namespace VideoMatrixMode
	{
		enum Type
		{
			NotSet		= 0,
			Projection	= GL_PROJECTION,
			ModelView	= GL_MODELVIEW,
		};

		inline int count() { return 2; }

		inline int to_index(Type _type)
		{
			if (_type == Projection) return 0;
			if (_type == ModelView) return 1;
			return NONE;
		}

		inline Type from_index(int _index)
		{
			if (_index == 0) return Projection;
			if (_index == 1) return ModelView;
			return NotSet;
		}
	};
#endif

	namespace VideoMatrixStackMode
	{
		enum Type
		{
			xRight_yUp,
			xRight_yForward_zUp,	// x right y forward z up
		};
	};

	/**
	 *	Always set mode when using stack to send data properly to system (opengl)
	 *	The first one (indexed 1!) (at the bottom) is assumed to be view
	 */
	class VideoMatrixStack
	{
	public:
		VideoMatrixStack(
#ifdef AN_GL
			VideoMatrixMode::Type _id = VideoMatrixMode::ModelView
#endif
		);

		void clear();

		void push_to_world(Matrix44 const & _mat);
		void push_set(Matrix44 const & _mat);
		void pop();

		void set_mode(VideoMatrixStackMode::Type _mode); // you have to clear (matrix stack and clip planes too!), send_current afterwards
		VideoMatrixStackMode::Type get_mode() const { return mode; }

		void ready_for_rendering(REF_ Matrix44 & _mat) const;

		void requires_send_all();
		void force_requires_send_all();

		bool is_empty() const { return stackSize <= 1; }

		inline Matrix44 const & get_current_for_rendering() const { return currentForRendering; }
		inline Matrix44 const & get_current() const { return current; }
		inline Matrix44 const & get_first() const { an_assert(stackSize > 1); return stack[1]; } // it's not actual first as it is always identity

		AN_NOT_CLANG_INLINE void set_vr_for_rendering(Optional<Matrix44> const & _mat = NP);
		inline Optional<Matrix44> const & get_vr_for_rendering() const { return vrForRendering; }
		inline Optional<Matrix44> const & get_vr() const { return vr; }

	private:
		static const int32 c_stackSize = 256;

#ifdef AN_GL
		VideoMatrixMode::Type id;
#endif
		VideoMatrixStackMode::Type mode;
		int32 stackSize;
		Matrix44 stack[c_stackSize]; // 0 is identity, 1 is view, rest is models
		Matrix44 current;
		Matrix44 currentForRendering;
#ifdef AN_DEVELOPMENT
		Matrix44 currentSent;
		Matrix44 currentSentForRendering;
#endif
		// below is assumed to be view (these are in vr space)
		Optional<Matrix44> vr;
		Optional<Matrix44> vrForRendering;

		inline void ready_current();
		inline void send_current_internal(bool _force = false);

	};

};
