#pragma once

#include "..\..\globalDefinitions.h"
#include "..\..\math\math.h"

#include "video.h"

namespace System
{
	class VideoMatrixStack;
	class ViewFrustum;

	template<unsigned int PlaneCount>
	struct CustomVideoClipPlaneSet
	{
	public:
		ArrayStatic<Plane, PlaneCount> planes; // max plane count defined, for actual video clip plane set, these are in camera space (transformed by model view matrix), but can be in any other space

		inline void clear();
		inline void add(Plane const& _plane, VideoMatrixStack const& _videoMatrixStack);
		inline void add_as_is(Plane const& _plane);

		inline void set_with(PlaneSet const& _planeSet, VideoMatrixStack const& _videoMatrixStack);
		inline void set_with_to_local(Matrix44 const& _viewMatrix, ViewFrustum const& _frustum);

		inline bool operator ==(CustomVideoClipPlaneSet const& _other) const;
	};

	struct VideoClipPlaneSet
	: public CustomVideoClipPlaneSet<PLANE_SET_PLANE_LIMIT>
	{
	};

	template<typename UseClipPlaneSet>
	class CustomVideoClipPlaneStack
	{
	public:
		CustomVideoClipPlaneStack();

		void connect_video_matrix_stack(VideoMatrixStack* _videoMatrixStack) { videoMatrixStack = _videoMatrixStack; }

		void clear();
		bool is_empty() const { return stack.is_empty(); }

		// remember that push and pop work in relation to current model view - always update model view before udpating this stack
		void push();
		void pop();

		void push_current_as_is(); // push current as is, to have current and to be able to add more clip planes

		void clear_current();
		void set_current(PlaneSet const & _planes);
		void add_to_current(Plane const & _plane);
		void add_to_current(PlaneSet const & _planes);
		void add_as_is_to_current(ViewFrustum const& _viewFrustum);

		void ready_current(bool _force = false); // for rendering
		inline UseClipPlaneSet const & get_current_for_rendering() const { an_assert(isReadyForRendering);  return setReadyForRendering; }

	protected:
		VideoMatrixStack * videoMatrixStack = nullptr;

		ArrayStatic<UseClipPlaneSet, 256> stack;
		UseClipPlaneSet setReady; // set that was used to ready for rendering
		UseClipPlaneSet setReadyForRendering;
		bool isReadyForRendering = false;

		int clipPlanesSent;
		int clipPlanesEnabled;

		inline void send_current_internal(bool _force = false);
	};

	class VideoClipPlaneStack
	: public CustomVideoClipPlaneStack<VideoClipPlaneSet>
	{
	public:
		void open_clip_planes();
		void close_clip_planes();

		void open_close_clip_planes_for_rendering();

	};
};
