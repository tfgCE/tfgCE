#pragma once

#include "timeStamp.h"

#include "..\debugSettings.h"

#include "..\containers\array.h"
#include "..\containers\arrayStatic.h"
#include "..\concurrency\counter.h"

struct LogInfoContext;

void store_draw_info(tchar const* _info, ...);
void store_draw_object_name(tchar const* _whatsBeingDrawn);
void store_direct_gl_call();
void store_draw_triangles(int _count);
void store_draw_call();
void store_draw_call_render_target(bool _otherDraw);

namespace System
{
	struct SystemInfo
	{
		static void initialise_static();
		static void close_static();

		SystemInfo();

		static SystemInfo* get() { return s_systemInfo; }

		static void created_vertex_buffer();
		static void created_element_buffer();
		static void created_render_target();
		static void destroyed_vertex_buffer();
		static void destroyed_element_buffer();
		static void destroyed_render_target();
		static void loaded_vertex_buffer(int _dataSize, float _time);
		static void loaded_element_buffer(int _dataSize, float _time);
		static void store_direct_gl_call();
		static void suspend_store_direct_gl_call(bool _suspend);
		static void store_draw_triangles(int _count);
		static void store_draw_call();
		static void suspend_store_draw_call(bool _suspend);
		static void store_draw_call_render_target(bool _otherDraw);
		static void store_new_call();
		static void store_delete_call();
		static void store_draw_info(tchar const* _info);
		static void store_draw_object_name(tchar const* _whatsBeingDrawn);

		static void display_buffer_gl_finish_start();
		static void display_buffer_gl_finish_end();

		static void store_in(SystemInfo & _SystemInfo);
		static void clear_temporary();

		void log(LogInfoContext & _log, bool _withDetails) const;

		int get_direct_gl_calls() const { return directGLCalls; }
		int get_draw_calls() const { return drawCalls; }
		int get_draw_calls_render_targets() const { return drawCallsRenderTargets; }
		int get_other_calls_render_targets() const { return otherCallsRenderTargets; }
		int get_draw_triangles() const { return drawTriangles; }
		int get_allocated() const { return allocated.get(); }

	private:
		static SystemInfo* s_systemInfo;

		struct Loading
		{
			int dataSize = 0;
			float time = 0.0f;
		};

#ifdef AN_SYSTEM_INFO_DRAW_DETAILS
		struct DrawInfo
		{
			tchar drawObjectInfo[1024];
			tchar type = ' ';
			int triangles = 0;
		};
#endif
		// temporary
		Concurrency::Counter newCalls = 0;
		Concurrency::Counter deleteCalls = 0;
		int directGLCalls = 0;
		int drawTriangles = 0;
		int drawCalls = 0;
		int drawCallsRenderTargets = 0;
		int otherCallsRenderTargets = 0;
#ifdef AN_SYSTEM_INFO_DRAW_DETAILS
#ifdef AN_SYSTEM_INFO
		tchar drawObjectInfo[1024];
#endif
		ArrayStatic<DrawInfo, 1024> drawCallsDetailed;
#endif
#ifdef AN_SYSTEM_INFO
		int suspendDirectGLCalls = 0;
		int suspendDrawCalls = 0;
#endif
		int vertexBufferCreated = 0;
		int elementBufferCreated = 0;
		int renderTargetsCreated = 0;
		Array<Loading> loadedVertexBuffer;
		Array<Loading> loadedElementBuffer;
		TimeStamp glFinishTS;
		TimeStamp glFinishedTS;
		float glFinishTime = 0.0f;
		float glFinishedInterval = 0.0f;

		// all
		static Concurrency::Counter s_allocated; // to track allocations before initialise_static
		Concurrency::Counter allocated = 0;
		int vertexBufferCount = 0;
		int elementBufferCount = 0;
		int renderTargetCount = 0;
		int loadedVertexBufferCount = 0;
		int loadedElementBufferCount = 0;

		void log(LogInfoContext & _log, Array<Loading> const & _buffer, tchar const * _txt) const;
	};
};

