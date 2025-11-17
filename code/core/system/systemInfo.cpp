#include "systemInfo.h"

#include "..\utils.h"
#include "..\debug\logInfoContext.h"

#ifdef AN_DEVELOPMENT
#include "..\mesh\mesh3d.h"
#endif

//

#ifdef AN_ALLOW_OPTIMIZE_OFF
#pragma optimize("", off)
#endif

//

void store_draw_info(tchar const * _info, ...)
{
	va_list list;
	va_start(list, _info);
#ifndef AN_CLANG
	int textsize = (int)(tstrlen(_info) + 1);
	int memsize = textsize * sizeof(tchar);
	allocate_stack_var(tchar, format, textsize);
	memory_copy(format, _info, memsize);
	sanitise_string_format(format);
#else
	tchar const* format = _info;
#endif
	tchar buf[2048];
	tvsprintf(buf, 2048-1, format, list);
	System::SystemInfo::store_draw_info(buf);
}

void store_draw_object_name(tchar const * _whatsBeingDrawn)
{
	System::SystemInfo::store_draw_object_name(_whatsBeingDrawn);
}

void store_direct_gl_call()
{
	System::SystemInfo::store_direct_gl_call();
}

void store_draw_call()
{
	System::SystemInfo::store_draw_call();
}

void store_draw_triangles(int _count)
{
	System::SystemInfo::store_draw_triangles(_count);
}

void store_draw_call_render_target(bool _otherDraw)
{
	System::SystemInfo::store_draw_call_render_target(_otherDraw);
}

void suspend_store_direct_gl_call(bool _suspend)
{
	System::SystemInfo::suspend_store_direct_gl_call(_suspend);
}

void suspend_store_draw_call(bool _suspend)
{
	System::SystemInfo::suspend_store_draw_call(_suspend);
}

void store_new_call()
{
	System::SystemInfo::store_new_call();
}

void store_delete_call()
{
	System::SystemInfo::store_delete_call();
}

using namespace System;

SystemInfo* SystemInfo::s_systemInfo = nullptr;
Concurrency::Counter SystemInfo::s_allocated = 0;

SystemInfo::SystemInfo()
{
#ifdef AN_SYSTEM_INFO_DRAW_DETAILS
	SET_EXTRA_DEBUG_INFO(drawCallsDetailed, TXT("SystemInfo.drawCallsDetailed"));
#endif
}

void SystemInfo::initialise_static()
{
#ifdef AN_SYSTEM_INFO
	an_assert(!s_systemInfo);
	s_systemInfo = new SystemInfo();
	s_systemInfo->allocated = s_allocated;
#endif
}

void SystemInfo::close_static()
{
#ifdef AN_SYSTEM_INFO
	delete_and_clear(s_systemInfo);
#endif
}

void SystemInfo::store_direct_gl_call()
{
#ifdef AN_SYSTEM_INFO
	an_assert(s_systemInfo);
	if (!s_systemInfo->suspendDirectGLCalls)
	{
		++s_systemInfo->directGLCalls;
	}
#endif
}

void SystemInfo::suspend_store_direct_gl_call(bool _suspend)
{
#ifdef AN_SYSTEM_INFO
	an_assert(s_systemInfo);
	s_systemInfo->suspendDirectGLCalls = s_systemInfo->suspendDirectGLCalls + (_suspend? 1 : -1);
#endif
}

void SystemInfo::store_draw_info(tchar const* _info)
{
#ifdef AN_SYSTEM_INFO
	an_assert(s_systemInfo);
	if (!s_systemInfo->suspendDrawCalls)
	{
#ifdef AN_SYSTEM_INFO_DRAW_DETAILS
		if (s_systemInfo->drawCallsDetailed.get_space_left() > 0)
		{
			s_systemInfo->drawCallsDetailed.grow_size(1);
			s_systemInfo->drawCallsDetailed.get_last().type = ' ';
			s_systemInfo->drawCallsDetailed.get_last().triangles = 0;
			memory_copy(s_systemInfo->drawCallsDetailed.get_last().drawObjectInfo, _info, (tstrlen(_info) + 1) * sizeof(tchar));
		}
#endif
	}
#endif
}

void SystemInfo::store_draw_object_name(tchar const* _whatsBeingDrawn)
{
#ifdef AN_SYSTEM_INFO
	an_assert(s_systemInfo);
	if (!s_systemInfo->suspendDrawCalls)
	{
#ifdef AN_SYSTEM_INFO_DRAW_DETAILS
		if (_whatsBeingDrawn)
		{
			memory_copy(s_systemInfo->drawObjectInfo, _whatsBeingDrawn, (tstrlen(_whatsBeingDrawn) + 1) * sizeof(tchar));
		}
		else
		{
			memory_copy(s_systemInfo->drawObjectInfo, TXT("??"), 3 * sizeof(tchar));
		}
#endif
	}
#endif
}

void SystemInfo::store_draw_call_render_target(bool _otherDraw)
{
#ifdef AN_SYSTEM_INFO
	an_assert(s_systemInfo);
	if (!s_systemInfo->suspendDrawCalls)
	{
		if (_otherDraw)
		{
			++s_systemInfo->otherCallsRenderTargets;
		}
		else
		{
			++s_systemInfo->drawCallsRenderTargets;
#ifdef AN_SYSTEM_INFO_DRAW_DETAILS
			if (s_systemInfo->drawCallsDetailed.get_space_left() > 0)
			{
				s_systemInfo->drawCallsDetailed.grow_size(1);
				s_systemInfo->drawCallsDetailed.get_last().type = 'R';
				s_systemInfo->drawCallsDetailed.get_last().triangles = 0;
				memory_copy(s_systemInfo->drawCallsDetailed.get_last().drawObjectInfo, s_systemInfo->drawObjectInfo, (tstrlen(s_systemInfo->drawObjectInfo) + 1) * sizeof(tchar));
			}
#endif
		}
	}
#endif
}

void SystemInfo::store_draw_call()
{
#ifdef AN_SYSTEM_INFO
	an_assert(s_systemInfo);
	if (!s_systemInfo->suspendDrawCalls)
	{
		++s_systemInfo->drawCalls;
#ifdef AN_SYSTEM_INFO_DRAW_DETAILS
		if (s_systemInfo->drawCallsDetailed.get_space_left() > 0)
		{
			s_systemInfo->drawCallsDetailed.grow_size(1);
			s_systemInfo->drawCallsDetailed.get_last().type = 'm';
			s_systemInfo->drawCallsDetailed.get_last().triangles = 0;
			memory_copy(s_systemInfo->drawCallsDetailed.get_last().drawObjectInfo, s_systemInfo->drawObjectInfo, (tstrlen(s_systemInfo->drawObjectInfo) + 1) * sizeof(tchar));
		}
#endif
	}
#endif
}

void SystemInfo::store_draw_triangles(int _count)
{
#ifdef AN_SYSTEM_INFO
	an_assert(s_systemInfo);
	if (!s_systemInfo->suspendDrawCalls)
	{
#ifdef INSPECT_MESH_RENDERING
		output(TXT("     store draw triangles +%i"), _count);
#endif
		s_systemInfo->drawTriangles += _count;
#ifdef AN_SYSTEM_INFO_DRAW_DETAILS
		if (!s_systemInfo->drawCallsDetailed.is_empty())
		{
			s_systemInfo->drawCallsDetailed.get_last().triangles += _count;
		}
#endif
	}
#endif
}

void SystemInfo::suspend_store_draw_call(bool _suspend)
{
#ifdef AN_SYSTEM_INFO
	an_assert(s_systemInfo);
	s_systemInfo->suspendDrawCalls = s_systemInfo->suspendDrawCalls + (_suspend ? 1 : -1);
#endif
}

void SystemInfo::store_new_call()
{
#ifdef AN_SYSTEM_INFO
	s_allocated.increase();
	if (s_systemInfo)
	{
		s_systemInfo->newCalls.increase();
		s_systemInfo->allocated.increase();
	}
#endif
}

void SystemInfo::store_delete_call()
{
#ifdef AN_SYSTEM_INFO
	s_allocated.decrease();
	if (s_systemInfo)
	{
		s_systemInfo->deleteCalls.increase();
		s_systemInfo->allocated.decrease();
	}
#endif
}

void SystemInfo::created_vertex_buffer()
{
#ifdef AN_SYSTEM_INFO
	an_assert(s_systemInfo);
	++s_systemInfo->vertexBufferCount;
	++s_systemInfo->vertexBufferCreated;
#endif
}

void SystemInfo::created_element_buffer()
{
#ifdef AN_SYSTEM_INFO
	an_assert(s_systemInfo);
	++ s_systemInfo->elementBufferCount;
	++s_systemInfo->elementBufferCreated;
#endif
}

void SystemInfo::created_render_target()
{
#ifdef AN_SYSTEM_INFO
	an_assert(s_systemInfo);
	++s_systemInfo->renderTargetCount;
	++s_systemInfo->renderTargetsCreated;
#endif
}

void SystemInfo::destroyed_vertex_buffer()
{
#ifdef AN_SYSTEM_INFO
	an_assert(s_systemInfo);
	--s_systemInfo->vertexBufferCount;
#endif
}

void SystemInfo::destroyed_element_buffer()
{
#ifdef AN_SYSTEM_INFO
	an_assert(s_systemInfo);
	--s_systemInfo->elementBufferCount;
#endif
}

void SystemInfo::destroyed_render_target()
{
#ifdef AN_SYSTEM_INFO
	an_assert(s_systemInfo);
	--s_systemInfo->renderTargetCount;
#endif
}

void SystemInfo::loaded_vertex_buffer(int _dataSize, float _time)
{
#ifdef AN_SYSTEM_INFO
	Loading loaded;
	loaded.dataSize = _dataSize;
	loaded.time = _time;

	an_assert(s_systemInfo);
	s_systemInfo->loadedVertexBuffer.push_back(loaded);
	++ s_systemInfo->loadedVertexBufferCount;
#endif
}

void SystemInfo::loaded_element_buffer(int _dataSize, float _time)
{
#ifdef AN_SYSTEM_INFO
	Loading loaded;
	loaded.dataSize = _dataSize;
	loaded.time = _time;

	an_assert(s_systemInfo);
	s_systemInfo->loadedElementBuffer.push_back(loaded);
	++ s_systemInfo->loadedElementBufferCount;
#endif
}

void SystemInfo::store_in(SystemInfo & _SystemInfo)
{
#ifdef AN_SYSTEM_INFO
	an_assert(s_systemInfo);
	_SystemInfo = *s_systemInfo;
#endif
}

void SystemInfo::clear_temporary()
{
#ifdef AN_SYSTEM_INFO
#ifdef INSPECT_MESH_RENDERING
	output(TXT("rendered %i triangles"), s_systemInfo->drawTriangles);
	output(TXT("in %i draw calls"), s_systemInfo->drawCalls);
#endif
	an_assert(s_systemInfo);
	s_systemInfo->newCalls = 0;
	s_systemInfo->deleteCalls = 0;
	s_systemInfo->directGLCalls = 0;
	s_systemInfo->drawTriangles = 0;
	s_systemInfo->drawCalls = 0;
	s_systemInfo->drawCallsRenderTargets = 0;
	s_systemInfo->otherCallsRenderTargets = 0;
	s_systemInfo->vertexBufferCreated = 0;
	s_systemInfo->elementBufferCreated = 0;
	s_systemInfo->renderTargetsCreated = 0;
	s_systemInfo->loadedVertexBuffer.clear();
	s_systemInfo->loadedElementBuffer.clear();
#ifdef AN_SYSTEM_INFO_DRAW_DETAILS
	s_systemInfo->drawCallsDetailed.clear();
#endif
#endif
}

void SystemInfo::log(LogInfoContext & _log, bool _withDetails) const
{
	_log.log(TXT("temporary/frame"));
	{
		LOG_INDENT(_log);
		_log.log(TXT("gl finish time: %.3fms"), glFinishTime * 1000.0f);
		_log.log(TXT("gl finished interval: %.3fms"), glFinishedInterval * 1000.0f);
		if (int calls = newCalls.get()) _log.log(TXT("new calls %i"), calls);
		if (int calls = deleteCalls.get()) _log.log(TXT("delete calls %i"), calls);
		if (directGLCalls) _log.log(TXT("direct GL calls %i"), directGLCalls);
		if (drawTriangles) _log.log(TXT("draw tris %i"), drawTriangles);
		if (drawCalls) _log.log(TXT("draw calls %i"), drawCalls);
		if (drawCallsRenderTargets) _log.log(TXT("draw calls (render targets) %i"), drawCallsRenderTargets);
		if (otherCallsRenderTargets) _log.log(TXT("other calls (render targets) %i"), otherCallsRenderTargets);
#ifdef AN_SYSTEM_INFO_DRAW_DETAILS
		if (_withDetails)
		{
			if (!drawCallsDetailed.is_empty())
			{
				_log.log(TXT("draw calls detailed %i"), drawCallsDetailed.get_size());
				{
					LOG_INDENT(_log);
					for_every(det, drawCallsDetailed)
					{
						_log.log(TXT("%03i %08i %c %S"), for_everys_index(det), det->triangles, det->type, det->drawObjectInfo);
					}
				}
			}
		}
#endif
		if (vertexBufferCreated) _log.log(TXT("vertex buffers created %i"), vertexBufferCreated);
		if (elementBufferCreated) _log.log(TXT("element buffers created %i"), elementBufferCreated);
		if (renderTargetsCreated) _log.log(TXT("render targets created %i"), renderTargetsCreated);
		if (!loadedVertexBuffer.is_empty() ||
			!loadedElementBuffer.is_empty())
		{
			_log.log(TXT("meshes loading times"));
			LOG_INDENT(_log);
			log(_log, loadedVertexBuffer, TXT("vertex buffer loading times"));
			log(_log, loadedElementBuffer, TXT("element buffer loading times"));
		}
	}
	_log.log(TXT("all time"));
	{
		LOG_INDENT(_log);
		_log.log(TXT("allocated %i"), allocated.get());
		_log.log(TXT("vertex buffer count %i"), vertexBufferCount);
		_log.log(TXT("element buffer count %i"), elementBufferCount);
		_log.log(TXT("loaded vertex buffer count %i"), loadedVertexBufferCount);
		_log.log(TXT("loaded element buffer count %i"), loadedElementBufferCount);
		_log.log(TXT("render target count %i"), renderTargetCount);
	}
}

void SystemInfo::log(LogInfoContext & _log, Array<Loading> const & _buffer, tchar const * _txt) const
{
	if (_buffer.is_empty())
	{
		return;
	}
	_log.log(_txt);
	LOG_INDENT(_log);
	float time = 0.0f;
	for_every(loaded, _buffer)
	{
		time += loaded->time;
	}
	_log.log(TXT("%i in %.3fms"), _buffer.get_size(), time * 1000.0f);
	for_every(loaded, _buffer)
	{
		_log.log(TXT(" +- %ib : %.3fms"), loaded->dataSize, loaded->time * 1000.0f);
	}
}

void SystemInfo::display_buffer_gl_finish_start()
{
#ifdef AN_SYSTEM_INFO
	an_assert(s_systemInfo);
	s_systemInfo->glFinishTS.reset();
#endif
}

void SystemInfo::display_buffer_gl_finish_end()
{
#ifdef AN_SYSTEM_INFO
	an_assert(s_systemInfo);
	s_systemInfo->glFinishTime = s_systemInfo->glFinishTS.get_time_since();
	s_systemInfo->glFinishedInterval = s_systemInfo->glFinishedTS.get_time_since();
	s_systemInfo->glFinishedTS.reset();
#endif
}
