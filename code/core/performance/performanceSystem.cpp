#include "performanceSystem.h"

#include "..\concurrency\threadManager.h"
#include "..\concurrency\threadSystemUtils.h"
#include "..\concurrency\scopedMRSWLock.h"
#include "..\concurrency\scopedSpinLock.h"
#include "performanceUtils.h"
#include "..\random\random.h"

#include "..\system\core.h"
#include "..\system\input.h"
#include "..\system\faultHandler.h"
#include "..\system\video\font.h"
#include "..\system\video\video3d.h"
#include "..\system\video\video3dPrimitives.h"

#include <time.h>

#ifdef AN_WINDOWS
#include <windows.h>
#include <shobjidl.h> 

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <windows.h>
#include <commctrl.h>
#include <SHELLAPI.H>
#endif

//

#ifdef AN_ALLOW_OPTIMIZE_OFF
#pragma optimize("", off)
#endif

#ifdef AUTO_SAVE_FRAME_PERFORMANCE_INFO_ON_HITCH
//#define STORE_FRAME_INFO_ON_END_FRAME
#endif

//

using namespace Performance;

//

Performance::System* Performance::System::s_system = nullptr;

ThreadInfo::ThreadInfo()
{
#ifdef AN_CONCURRENCY_STATS
	gatherBlockLock.do_not_report_stats();
	gatherMarkerLock.do_not_report_stats();
#endif
	for_count(int, i, MAX_PREV_FRAMES + 1)
	{
		blocks[i].make_space_for(MAX_SIZE);
		markers[i].make_space_for(MAX_SIZE_MARKERS);
	}
	for_count(int, i, MAX_PREV_FRAMES)
	{
		displayBlocks[i] = &blocks[i];
		displayMarkers[i] = &markers[i];
	}
	gatherBlocks = &blocks[MAX_PREV_FRAMES];
	gatherMarkers = &markers[MAX_PREV_FRAMES];
}

void ThreadInfo::add_marker(Timer const & _timer)
{
	if (!Performance::is_enabled())
	{
		return;
	}
	Concurrency::ScopedSpinLock lock(gatherMarkerLock);
	if (gatherMarkers->get_size() < MAX_SIZE_MARKERS)
	{
		gatherMarkers->push_back(_timer);
	}
}

Block* ThreadInfo::add_block(BlockTag const & _tag)
{
	if (!Performance::is_enabled())
	{
		return nullptr;
	}
	Concurrency::ScopedSpinLock lock(gatherBlockLock);
	if (gatherBlocks->get_size() < MAX_SIZE)
	{
		gatherBlocks->push_back(Block(_tag));
		return &gatherBlocks->get_last();
	}
	else
	{
		return nullptr;
	}
}

Block* ThreadInfo::find_block(BlockToken const & _token)
{
	if (!Performance::is_enabled())
	{
		return nullptr;
	}
	Concurrency::ScopedSpinLock lock(gatherBlockLock);
	for_every(block, *gatherBlocks)
	{
		if (block->get_id() == _token.id)
		{
			return block;
		}
	}
	return nullptr;
}

void ThreadInfo::swap_blocks()
{
	Concurrency::ScopedSpinLock lockB(gatherBlockLock);
	Concurrency::ScopedSpinLock lockM(gatherMarkerLock);

	// swap blocks
	{
		auto* temp = gatherBlocks;
		gatherBlocks = displayBlocks[MAX_PREV_FRAMES - 1];
		for (int i = MAX_PREV_FRAMES - 1; i > 0; --i)
		{
			displayBlocks[i] = displayBlocks[i - 1];
		}
		displayBlocks[0] = temp;
	}
	// swap markers
	{
		auto* temp = gatherMarkers;
		gatherMarkers = displayMarkers[MAX_PREV_FRAMES - 1];
		for (int i = MAX_PREV_FRAMES - 1; i > 0; --i)
		{
			displayMarkers[i] = displayMarkers[i - 1];
		}
		displayMarkers[0] = temp;
	}

	// remove all gather blocks
	gatherBlocks->clear();
	gatherMarkers->clear();

	// if there is any unfinished block, add it to gatherBlocks and turn it into copy in displayBlocks
	for_every(block, *displayBlocks[CURR_FRAME_INFO_IDX])
	{
		if (!block->has_finished())
		{
			an_assert(gatherBlocks->get_size() < MAX_SIZE);
			if (gatherBlocks->get_size() < MAX_SIZE)
			{
				gatherBlocks->push_back(*block);
			}
			block->turn_into_copy(); // make displayed block a copy, original should keep its id
		}
	}
}

void ThreadInfo::clear_gatherer()
{
	Concurrency::ScopedSpinLock lock(gatherBlockLock);

	// remove all gather blocks
	gatherBlocks->clear();
	gatherMarkers->clear();
}

void Performance::System::initialise_static()
{
#ifdef AN_PERFORMANCE_MEASURE
	new System();
#endif
}

void Performance::System::close_static()
{
	delete_and_clear(s_system);
}

Performance::System::System()
{
	an_assert(s_system == nullptr);
	s_system = this;

#ifdef AN_CONCURRENCY_STATS
	processLock.do_not_report_stats();
#endif

	while (threads.get_size() < Concurrency::ThreadSystemUtils::get_number_of_cores())
	{
		threads.push_back(new ThreadInfo());
	}
}

Performance::System::~System()
{
	an_assert(s_system == this);
	s_system = nullptr;

	for_every_ptr(thread, threads)
	{
		delete thread;
	}
}

int Performance::System::get_performance_frame_no()
{
	return s_system ? s_system->performanceFrameNo : 0;
}

void Performance::System::add_marker(Timer const & _timer)
{
	if (!s_system)
	{
		return;
	}
	//Concurrency::ScopedMRSWLockRead processLock(s_system->processLock);
	int threadID = Concurrency::ThreadManager::get_current_thread_id(true);
	if (threadID != NONE)
	{
		ThreadInfo* thread = s_system->get_thread(threadID);
		thread->add_marker(_timer);
	}
}

void Performance::System::add_block(BlockTag const & _tag, Timer const & _timer, OUT_ BlockToken & _token)
{
	if (!s_system)
	{
		return;
	}
	//Concurrency::ScopedMRSWLockRead processLock(s_system->processLock);
	int threadID = Concurrency::ThreadManager::get_current_thread_id(true);
	if (threadID != NONE)
	{
		ThreadInfo* thread = s_system->get_thread(threadID);
		_token.block = thread->add_block(_tag);
	}
	if (_token.block)
	{
		_token.id = _token.block->get_id();
		_token.block->update(_timer);
	}
}

void Performance::System::update_block(BlockTag const & _tag, Timer const & _timer, BlockToken & _token)
{
	if (!s_system || !Performance::is_enabled())
	{
		return;
	}
	//Concurrency::ScopedMRSWLockRead processLock(s_system->processLock);
	if (!_token.block || _token.block->get_id() != _token.id)
	{
		// no block or id different? find it by id, we're updating! not adding new one!
		int threadID = Concurrency::ThreadManager::get_current_thread_id(true); 
		if (threadID != NONE)
		{
			ThreadInfo* thread = s_system->get_thread(threadID);
			_token.block = thread->find_block(_token);
		}
	}
	if (_token.block)
	{
		_token.id = _token.block->get_id();
		_token.block->update(_timer);
	}
}

ThreadInfo* Performance::System::get_thread(int32 _threadIdx)
{
	if (!threads.is_index_valid(_threadIdx))
	{
		an_assert(nullptr, TXT("not enough thread info"));
	}
	return threads[_threadIdx];
}

void Performance::System::start_frame()
{
	if (!s_system)
	{
		return;
	}
	s_system->frameTime.start();
}

void Performance::System::end_frame(bool _keepExistingResults)
{
	if (!s_system)
	{
		return;
	}
	s_system->frameTime.stop();

	Concurrency::Stats::next_frame();

	if (s_system->state == Saved)
	{
		output(TXT("allow storing performance frame info (saved) (state:%i)"), s_system->state);
		s_system->state = Normal;
#ifdef INSPECT_SAVE_FRAME
		s_system->savingState = -1;
		s_system->savingStateI = 0;
		s_system->savingStateISub = 0;
		s_system->savingStateCount = 0;
#endif
	}

	if (s_system->state == FailedToSave)
	{
		output(TXT("allow storing performance frame info (failed to save) (state:%i)"), s_system->state);
		s_system->state = Normal;
#ifdef INSPECT_SAVE_FRAME
		s_system->savingState = -1;
		s_system->savingStateI = 0;
		s_system->savingStateISub = 0;
		s_system->savingStateCount = 0;
#endif
	}

	//Concurrency::ScopedMRSWLockWrite processLock(s_system->processLock);

	if (_keepExistingResults || !can_gather_new_data())
	{
#ifdef STORE_FRAME_INFO_ON_END_FRAME
		output(TXT("new frame, ignore #%i (time %.3fms)"), s_system->performanceFrameNo, s_system->frameTime.get_time_ms());
#endif
		for_every_ptr(thread, s_system->threads)
		{
			thread->clear_gatherer();
		}
	}
	else
	{
#ifdef STORE_FRAME_INFO_ON_END_FRAME
		output(TXT("new frame, stored #%i (time %.3fms)"), s_system->performanceFrameNo, s_system->frameTime.get_time_ms());
#endif

		// push prev frames
		{
			Concurrency::ScopedSpinLock lockDAI(s_system->displayAdditionalInfosLock);

			for (int i = MAX_PREV_FRAMES - 1; i > 0; --i)
			{
				s_system->displayFrameInfo[i] = s_system->displayFrameInfo[i - 1];
			}
		}
		auto& currentDisplayFrameInfo = s_system->displayFrameInfo[CURR_FRAME_INFO_IDX];

		// store frame time
		currentDisplayFrameInfo.frameTime = s_system->frameTime;

		// store frame number
		currentDisplayFrameInfo.performanceFrameNo = s_system->performanceFrameNo;

		// store concurrency stats
		currentDisplayFrameInfo.concurrencyStats = Concurrency::Stats::get_last_frame();

		// rearrange blocks
		for_every_ptr(thread, s_system->threads)
		{
			thread->swap_blocks();
		}

#ifdef USE_PERFORMANCE_ADDITIONAL_INFO
		{
			Concurrency::ScopedSpinLock lockAI(s_system->additionalInfosLock);
			Concurrency::ScopedSpinLock lockDAI(s_system->displayAdditionalInfosLock);
			currentDisplayFrameInfo.displayAdditionalInfos = s_system->additionalInfos;
		}
#endif
	}

	++s_system->performanceFrameNo;
}

struct ExtraInfo
{
	String text;
	Colour colour = Colour::white;
	ExtraInfo() {}
	ExtraInfo(String const & _text, Colour const & _colour = Colour::white) : text(_text), colour(_colour) {}
};

#define CONCURRENCY_STAT_TO_TEXT(_a, _b) (_b != 0? ((float)_a / (float)_b) * 100.0f : 0.0f), _a, _b

struct OutTime
{
	Name name;
	float time = 0.0f;
};

struct TextStack
{
	String text;
	Colour colour = Colour::white;

	TextStack() {}
	TextStack(String const & _string) : text(_string) {}
	TextStack(Colour const & _colour, String const & _string) : text(_string), colour(_colour) {}
};

void Performance::System::display(::System::Font const * _font, bool _render, bool _outputLog, LogInfoContext* _logInto, SystemCustomDisplay const* _customDisplay)
{
	if (!s_system)
	{
		return;
	}

	SUSPEND_STORE_DIRECT_GL_CALL();
	SUSPEND_STORE_DRAW_CALL();

	//MEASURE_AND_OUTPUT_PERFORMANCE(TXT("display"));

	::System::Video3D * v3d = ::System::Video3D::get();
	Vector2 viewportSize;

	if (_render)
	{
		if (_customDisplay&& _customDisplay->viewportSize.is_set())
		{
			viewportSize = _customDisplay->viewportSize.get().to_vector2();
		}
		else
		{
			v3d->set_default_viewport();
			v3d->set_near_far_plane(0.02f, 100.0f);

			v3d->set_2d_projection_matrix_ortho();
			v3d->access_model_view_matrix_stack().clear();

			v3d->setup_for_2d_display();

			viewportSize = v3d->get_viewport_size().to_vector2();
		}
	}
	else
	{
		viewportSize = Vector2(100.0f, 100.0f);
	}

	int32 threadCount = Concurrency::ThreadManager::get()->get_thread_count();
	if (s_system->state == ShowLoaded)
	{
		threadCount = s_system->loadedThreadCount;
	}

	Range viewDisplayRange = Range(viewportSize.x * 0.1f, viewportSize.x * 0.9f);
	float vertOffset = viewportSize.y * 0.1f;
	float vertThreadOffset = (viewportSize.y * 0.9f - vertOffset) / float(threadCount);
	float vertHeight = vertThreadOffset * 0.9f;
	Range displayRange(0.0f, 1.0f);
	Vector2 mouse = Vector2::zero;
#ifdef AN_STANDARD_INPUT
	static Optional<float> mouseHeldAt;
	float mouseCurrAt = 0.0f;
#endif

	Array<TextStack> textStack;
	textStack.make_space_for(20);
	Array<OutTime> times; // used only when outputing log
	if (_outputLog || _logInto)
	{
		times.make_space_for(1000);
	}
	bool mousePressed = false;
	if (_render)
	{
		mouse = _customDisplay && _customDisplay->mouseAt.is_set()? _customDisplay->mouseAt.get() : ::System::Input::get()->get_mouse_window_location();
		float mouseAt = viewDisplayRange.get_pt_from_value(mouse.x);
		mouseAt = (mouseAt - s_system->centre) / s_system->zoom + s_system->centre;
		float mouseRel = viewDisplayRange.get_pt_from_value(::System::Input::get()->get_mouse_relative_location().x + viewportSize.x * 0.5f) - 0.5f;
		mouseRel = mouseRel / s_system->zoom;
		float factor = 1.0f;
		bool zoomUsingMouse = false;
#ifdef AN_STANDARD_INPUT
		if (::System::Input::get()->has_key_been_pressed(::System::Key::MouseWheelDown))
		{
			factor = 0.5f;
			zoomUsingMouse = true;
		}
		if (::System::Input::get()->has_key_been_pressed(::System::Key::MouseWheelUp))
		{
			factor = 2.0f;
			zoomUsingMouse = true;
		}
#endif
		if (_customDisplay && _customDisplay->zoom.is_set())
		{
			float z = _customDisplay->zoom.get();
			if (z > 0.0f)
			{
				factor = 1.0f * (1.0f + z * ::System::Core::get_raw_delta_time() * 1.0f);
				zoomUsingMouse = true;
			}
			if (z < 0.0f)
			{
				factor = 1.0f / (1.0f - z * ::System::Core::get_raw_delta_time() * 1.0f);
				zoomUsingMouse = true;
			}
		}
#ifdef AN_STANDARD_INPUT
		if (::System::Input::get()->has_mouse_button_been_pressed(0))
		{
			mousePressed = true;
		}
#endif
		if (_customDisplay && _customDisplay->inputActivate.get(false))
		{
			mousePressed = true;
		}
#ifdef AN_STANDARD_INPUT
		if (::System::Input::get()->is_mouse_button_pressed(2))
		{
			mouseHeldAt.set_if_not_set(mouseAt);
		}
		else
		{
			mouseHeldAt.clear();
		}
		mouseCurrAt = mouseAt;
#endif
#ifdef AN_STANDARD_INPUT
		if (auto* i = ::System::Input::get())
		{

			{
				float overrideZoom = 0.0f;
				if (i->is_key_pressed(::System::Key::W))
				{
					overrideZoom += 1.0f;
				}
				if (i->is_key_pressed(::System::Key::S))
				{
					overrideZoom -= 1.0f;
				}
				if (i->is_key_pressed(::System::Key::LeftShift) ||
					i->is_key_pressed(::System::Key::RightShift))
				{
					overrideZoom *= 3.0f;
				}
				if (overrideZoom != 0.0f)
				{
					float z = overrideZoom;
					if (z > 0.0f)
					{
						factor = 1.0f * (1.0f + z * ::System::Core::get_raw_delta_time() * 1.0f);
					}
					if (z < 0.0f)
					{
						factor = 1.0f / (1.0f - z * ::System::Core::get_raw_delta_time() * 1.0f);
					}
				}
			}
		}
#endif
		if (factor != 1.0f)
		{
			float prevZoom = s_system->zoom;
			s_system->zoom = max(s_system->zoom * factor, 1.0f);
			/*
			m = (ms - c0) / z0 + c0
			m = (ms - c1) / z1 + c1

			ms = (m - c0)* z0 + c0
			m = ((m - c0)* z0 + c0 - c1) / z1 + c1
			m * z1 = (m - c0)* z0 + c0 - c1 + c1 * z1
			m * z1 = (m - c0)* z0 + c0 + c1 * (-1 + z1)
			c1 = (m * z1 - c0 - (m-c0) * z0) / (z1 - 1)
			*/
			if (s_system->zoom <= 1.0f)
			{
				s_system->centre = 0.5f;
			}
			else
			{
				if (zoomUsingMouse)
				{
					s_system->centre = (mouseAt * s_system->zoom - s_system->centre - (mouseAt - s_system->centre) * prevZoom) / (s_system->zoom - 1.0f);
				}
				else
				{
					float fakeMouseAt = 0.5f;
					fakeMouseAt = (fakeMouseAt - s_system->centre) / s_system->zoom + s_system->centre;
					s_system->centre = (fakeMouseAt * s_system->zoom - s_system->centre - (fakeMouseAt - s_system->centre) * prevZoom) / (s_system->zoom - 1.0f);
				}
			}
		}
		if (::System::Input::get()->is_mouse_button_pressed(0))
		{
			s_system->centre -= mouseRel;
		}
		if (_customDisplay && _customDisplay->pan.is_set())
		{
			s_system->centre += (_customDisplay->pan.get() / s_system->zoom) * ::System::Core::get_raw_delta_time() * 1.0f;
		}
#ifdef AN_STANDARD_INPUT
		if (auto* i = ::System::Input::get())
		{
			{
				float overridePan = 0.0f;
				if (i->is_key_pressed(::System::Key::D))
				{
					overridePan += 1.0f;
				}
				if (i->is_key_pressed(::System::Key::A))
				{
					overridePan -= 1.0f;
				}
				if (i->is_key_pressed(::System::Key::LeftShift) ||
					i->is_key_pressed(::System::Key::RightShift))
				{
					overridePan *= 3.0f;
				}
				if (overridePan != 0.0f)
				{
					s_system->centre += (overridePan / s_system->zoom) * ::System::Core::get_raw_delta_time() * 1.0f;
				}
			}
		}
#endif
		displayRange.min = (displayRange.min - s_system->centre) * s_system->zoom + s_system->centre;
		displayRange.max = (displayRange.max - s_system->centre) * s_system->zoom + s_system->centre;
		displayRange.min = viewDisplayRange.get_at(displayRange.min);
		displayRange.max = viewDisplayRange.get_at(displayRange.max);
		auto& displayFrameTime = s_system->displayFrameInfo[CURR_FRAME_INFO_IDX].frameTime;
		textStack.push_back(String::printf(TXT("%8.5fms < at of %8.5fms : fps %5.2f"), mouseAt * displayFrameTime.get_time_ms(), displayFrameTime.get_time_ms(), displayFrameTime.get_time_ms() != 0.0f ? 1000.0f / displayFrameTime.get_time_ms() : 0.0f));
	}

	static float contextInfoTime = 0.0f;
	static const float contextInfoTimePeriod = 0.5f;
	contextInfoTime = mod(contextInfoTime + ::System::Core::get_raw_delta_time(), contextInfoTimePeriod);

	int levelsPrepared = 0;

	if (s_system->state == Loading)
	{
		if (_render && _font)
		{
			float sy = (viewportSize.y / 20.0f) / _font->get_height();
			Vector2 scale = Vector2(sy, sy);

			_font->draw_text_at_with_border(v3d, String(TXT("loading frame data")), Colour::white, Colour::black, 1.0f, viewportSize * 0.5f, scale, Vector2::half);
		}
	}
	else
	{
		for (int32 threadIdx = 0; threadIdx < threadCount; ++threadIdx)
		{
			ThreadInfo const* thread = s_system->threads[threadIdx];

			float vertLoc = vertOffset + vertThreadOffset * (float)(threadIdx);
			Range verticalRange = Range(viewportSize.y - vertLoc - vertHeight, viewportSize.y - vertLoc);
			if (_render)
			{
				::System::Video3DPrimitives::fill_rect_2d(Colour::whiteCold * Colour(1.0f, 1.0f, 1.0f, 0.2f), Vector2(displayRange.min, verticalRange.min), Vector2(displayRange.max, verticalRange.max));
			}
			if (displayRange.does_contain(mouse.x) &&
				verticalRange.does_contain(mouse.y) &&
				(thread->read_cpu != NONE ||
				 thread->read_id != NONE ||
				 thread->read_policy != NONE ||
				 thread->read_priority != NONE ||
				 thread->read_priorityNice != NONE))
			{
				textStack.push_back(String::printf(TXT("cpu:%i id:%i policy:%i prio:%i nice:%i"), thread->read_cpu, thread->read_id, thread->read_policy, thread->read_priority, thread->read_priorityNice));
			}

			verticalRange = Range(verticalRange.get_at(0.1f), verticalRange.get_at(0.9f));
			Range highlightRange = Range::empty;
			Range highlightVerticalRange = Range::empty;
			Colour highlightColour = Colour::white.with_alpha(0.3f);
			for_every(block, *thread->displayBlocks[CURR_FRAME_INFO_IDX])
			{
				if (_outputLog || _logInto)
				{
					bool found = false;
					for_every(time, times)
					{
						if (time->name == block->get_tag().name)
						{
							time->time += block->get_time_ms();
							found = true;
							break;
						}
					}
					if (!found)
					{
						OutTime time;
						time.name = block->get_tag().name;
						time.time = block->get_time_ms();
						times.push_back(time);
					}
				}
				auto& displayFrameTime = s_system->displayFrameInfo[CURR_FRAME_INFO_IDX].frameTime;
				Range blockRange = block->get_relative_range(displayFrameTime);
				int level = 0;
				if (block->get_tag().type == BlockType::Normal)
				{
					auto& blockLevel = block->access_level();
					if (!blockLevel.is_set() && levelsPrepared < 1000)
					{
						++levelsPrepared;
						for_every(otherBlock, *thread->displayBlocks[CURR_FRAME_INFO_IDX])
						{
							if (block != otherBlock)
							{
								if (otherBlock->get_relative_range(displayFrameTime).does_contain(blockRange))
								{
									++level;
								}
							}
						}
						blockLevel = level;
					}
					level = blockLevel.is_set() ? blockLevel.get() : 6; // will popup soon
				}
				float levelOffset = min(0.4f, log10f(((float)level) * 0.1f + 1.0f));
				Range blockVerticalRange = Range(verticalRange.get_at(levelOffset), verticalRange.get_at(1.0f - levelOffset));
				blockRange.min = displayRange.get_at(blockRange.min);
				blockRange.max = displayRange.get_at(blockRange.max);
				Colour blockColour = block->get_tag().colour;
				if (!block->has_finished())
				{
					// not finished, blink!
					blockColour = Colour(Random::get_float(0.0f, 1.0f), Random::get_float(0.0f, 1.0f), Random::get_float(0.0f, 1.0f), 0.5f);
				}
				if (blockRange.does_contain(mouse.x) &&
					blockVerticalRange.does_contain(mouse.y))
				{
					{
						if (block->get_tag().name.is_valid())
						{
							textStack.push_back(String::printf(TXT("%8.5fms : %S"), block->get_time_ms(), block->get_tag().name.to_char()));
						}
						else
						{
							textStack.push_back(TextStack(Colour::green, String::printf(TXT("%8.5fms :--> %S"), block->get_time_ms(), block->get_tag().contextInfo)));
							blockColour = Colour::green * (0.75f + 0.25f * sin_deg(360.0f * (contextInfoTime / contextInfoTimePeriod)));
						}
					}
					highlightRange = blockRange;
					highlightVerticalRange = blockVerticalRange;
				}
				if (_render)
				{
					::System::Video3DPrimitives::fill_rect_2d(blockColour, Vector2(blockRange.min, blockVerticalRange.min), Vector2(blockRange.max, blockVerticalRange.max));
				}
			}
			if (_render && !highlightRange.is_empty())
			{
				::System::Video3DPrimitives::fill_rect_2d(highlightColour, Vector2(highlightRange.min, highlightVerticalRange.min), Vector2(highlightRange.max, highlightVerticalRange.max));
			}
		}

		// markers etc
		for (int32 threadIdx = 0; threadIdx < threadCount; ++threadIdx)
		{
			ThreadInfo const* thread = s_system->threads[threadIdx];

			Range verticalRange = Range(0.0f, viewportSize.y);

			for_every(marker, *thread->displayMarkers[CURR_FRAME_INFO_IDX])
			{
				auto& displayFrameTime = s_system->displayFrameInfo[CURR_FRAME_INFO_IDX].frameTime;
				Range markerRange = marker->get_relative_range(displayFrameTime);
				float x = displayRange.get_at(markerRange.min);
				::System::Video3DPrimitives::line_2d(marker->get_colour().with_alpha(0.3f), Vector2(x, verticalRange.min), Vector2(x, verticalRange.max));
			}

#ifdef AN_STANDARD_INPUT
			if (_render && mouseHeldAt.is_set())
			{
				int off = 10;
				Range verticalRange = Range(0.0f, viewportSize.y);
				Colour lineColour = Colour::white;

				for (int i = -off; i <= off; ++i)
				{
					float x = displayRange.get_at(mouseHeldAt.get() + (float)i * (mouseCurrAt - mouseHeldAt.get()));
					::System::Video3DPrimitives::line_2d(lineColour, Vector2(x, verticalRange.min), Vector2(x, verticalRange.max));
				}
				{
					float y = ::System::Input::get()->get_mouse_window_location().y;
					float x0 = displayRange.get_at(mouseHeldAt.get() + 0.0f * (mouseCurrAt - mouseHeldAt.get()));
					float x1 = displayRange.get_at(mouseHeldAt.get() + 1.0f * (mouseCurrAt - mouseHeldAt.get()));
					::System::Video3DPrimitives::line_2d(lineColour, Vector2(x0, y), Vector2(x1, y));
				}

				if (_render && _font)
				{
					float sy = (viewportSize.y / 50.0f) / _font->get_height();
					Vector2 scale = Vector2(sy, sy);

					_font->draw_text_at_with_border(v3d, String::printf(TXT("%.3fms"),
						(mouseCurrAt - mouseHeldAt.get()) * s_system->displayFrameInfo[CURR_FRAME_INFO_IDX].frameTime.get_time_ms()),
						lineColour, Colour::black, 1.0f,
						::System::Input::get()->get_mouse_window_location(), scale, Vector2(0.0f, 0.0f));
				}
			}
#endif
		}

		if (_render && _font)
		{
			if (s_system->state == ShowLoaded)
			{
				float sy = (viewportSize.y / 50.0f) / _font->get_height();
				Vector2 scale = Vector2(sy, sy);
				_font->draw_text_at_with_border(v3d, String::printf(TXT("%S #%i"), s_system->showLoadedInfo.to_char(), s_system->displayFrameInfo[CURR_FRAME_INFO_IDX].performanceFrameNo), Colour::yellow, Colour::black, 1.0f, viewportSize* Vector2(1.0f, 0.1f), scale, Vector2(1.0f, 0.0f));
			}

			{
				float sy = (viewportSize.y / 50.0f) / _font->get_height();
				Vector2 scale = Vector2(sy, sy);
				Vector2 at = Vector2(0.0f, 0.0f);
				for_every(text, textStack)
				{
					_font->draw_text_at_with_border(v3d, text->text, for_everys_index(text) == 0 ? Colour::cyan : text->colour, Colour::black, 1.0f, at, scale);
					at.y += _font->get_height() * scale.y;
				}
			}

			{
				float sy = (viewportSize.y / 60.0f) / _font->get_height();
				Vector2 scale = Vector2(sy, sy);
				Array<ExtraInfo> extraInfos;
				extraInfos.push_back(ExtraInfo(String(TXT("concurrency")), Colour::grey));
				auto& concurrencyStats = s_system->displayFrameInfo[CURR_FRAME_INFO_IDX].concurrencyStats;
				extraInfos.push_back(ExtraInfo(String::printf(TXT("spinlock: %4.1f%% %06i/%06i"), CONCURRENCY_STAT_TO_TEXT(concurrencyStats.waitedInSpinLock, concurrencyStats.wasInSpinLock))));
				extraInfos.push_back(ExtraInfo(String::printf(TXT("counter: %4.1f%% %06i/%06i"), CONCURRENCY_STAT_TO_TEXT(concurrencyStats.waitedInCounter, concurrencyStats.wasInCounter))));
				extraInfos.push_back(ExtraInfo(String::printf(TXT("semaphore: %4.1f%% %06i/%06i"), CONCURRENCY_STAT_TO_TEXT(concurrencyStats.waitedInSemaphore, concurrencyStats.wasInSemaphore))));
				extraInfos.push_back(ExtraInfo(String::printf(TXT("mrsw-r: %4.1f%% %06i/%06i"), CONCURRENCY_STAT_TO_TEXT(concurrencyStats.waitedInMRSWLockRead, concurrencyStats.wasInMRSWLockRead))));
				extraInfos.push_back(ExtraInfo(String::printf(TXT("mrsw-w: %4.1f%% %06i/%06i"), CONCURRENCY_STAT_TO_TEXT(concurrencyStats.waitedInMRSWLockWrite, concurrencyStats.wasInMRSWLockWrite))));
				Vector2 at = Vector2(viewportSize.x, viewportSize.y);
				for_every(extraInfo, extraInfos)
				{
					_font->draw_text_at_with_border(v3d, extraInfo->text, extraInfo->colour, Colour::black, 1.0f, at, scale, Vector2(1.0f, 1.0f));
					at.y -= _font->get_height() * scale.y;
				}
			}

#ifdef USE_PERFORMANCE_ADDITIONAL_INFO
			{
				Concurrency::ScopedSpinLock lock(s_system->displayAdditionalInfosLock);

				float sy = (viewportSize.y / 60.0f) / _font->get_height();
				Vector2 scale = Vector2(sy, sy);
				Vector2 at = Vector2(0.0f, viewportSize.y);
				for_every(ai, s_system->displayFrameInfo[CURR_FRAME_INFO_IDX].displayAdditionalInfos)
				{
					_font->draw_text_at_with_border(v3d, ai->info, Colour::white, Colour::black, 1.0f, at, scale, Vector2(0.0f, 1.0f));
					at.y -= _font->get_height() * scale.y;
				}
			}
#endif

			{
				float sy = (viewportSize.y / 60.0f) / _font->get_height();
				Vector2 scale = Vector2(sy, sy);
				Range2 buttonRect = Range2::empty;
				buttonRect.include(viewportSize * Vector2(0.8f, 0.0f));
				buttonRect.include(viewportSize * Vector2(1.0f, 0.1f));
				Vector2 buttonSize = buttonRect.length();
				float buttonSpacing = max(round(buttonSize.x * 0.1f), round(buttonSize.y * 0.1f));
				Vector2 buttonOffset = buttonSize + Vector2::one * buttonSpacing;
				if (s_system->state == Saving)
				{
					::System::Video3DPrimitives::fill_rect_2d(Colour::greyLight.with_alpha(0.3f), buttonRect.bottom_left(), buttonRect.top_right());
					_font->draw_text_at_with_border(v3d, String(TXT("saving...")), Colour::black, Colour::black, 0.0f, buttonRect.get_at(Vector2(0.5f, 0.5f)), scale, Vector2(0.5f, 0.5f));
				}
				else if (s_system->state != Saving && s_system->state != Saved &&
					s_system->state != Loading && s_system->state != ShowLoaded &&
					(!s_system->savedFrameNo.is_set() || s_system->savedFrameNo.get() != s_system->displayFrameInfo[CURR_FRAME_INFO_IDX].performanceFrameNo))
				{
					::System::Video3DPrimitives::fill_rect_2d(Colour::greyLight.with_alpha(0.3f), buttonRect.bottom_left(), buttonRect.top_right());
					_font->draw_text_at_with_border(v3d, String(TXT("save frame")), Colour::black, Colour::black, 0.0f, buttonRect.get_at(Vector2(0.5f, 0.6f)), scale, Vector2(0.5f, 0.0f));
					_font->draw_text_at_with_border(v3d, String::printf(TXT("#%i"), s_system->displayFrameInfo[CURR_FRAME_INFO_IDX].performanceFrameNo), Colour::black, Colour::black, 0.0f, buttonRect.get_at(Vector2(0.5f, 0.4f)), scale, Vector2(0.5f, 1.0f));
					if (mousePressed && buttonRect.does_contain(mouse))
					{
						save_frame(true);
					}
				}
				buttonRect.x.min -= buttonOffset.x;
				buttonRect.x.max -= buttonOffset.x;
				if (s_system->state != Loading)
				{
					::System::Video3DPrimitives::fill_rect_2d(Colour::greyLight.with_alpha(0.3f), buttonRect.bottom_left(), buttonRect.top_right());
					_font->draw_text_at_with_border(v3d, String(TXT("load frame")), Colour::black, Colour::black, 0.0f, buttonRect.centre(), scale, Vector2(0.5f, 0.5f));
					if (mousePressed && buttonRect.does_contain(mouse))
					{
						load_frame();
					}
				}
				buttonRect.x.max -= buttonOffset.x;
				buttonRect.x.min -= buttonOffset.x;
				if (s_system->state == ShowLoaded)
				{
					::System::Video3DPrimitives::fill_rect_2d(Colour::greyLight.with_alpha(0.3f), buttonRect.bottom_left(), buttonRect.top_right());
					_font->draw_text_at_with_border(v3d, String(TXT("return to")), Colour::black, Colour::black, 0.0f, buttonRect.get_at(Vector2(0.5f, 0.6f)), scale, Vector2(0.5f, 0.0f));
					_font->draw_text_at_with_border(v3d, String(TXT("normal")), Colour::black, Colour::black, 0.0f, buttonRect.get_at(Vector2(0.5f, 0.4f)), scale, Vector2(0.5f, 1.0f));
					if (mousePressed && buttonRect.does_contain(mouse))
					{
						s_system->state = Normal;
					}
				}
			}
		}
		if (_outputLog)
		{
			ScopedOutputLock outputLock;
			output_colour(0, 0, 1, 1);
			output(TXT("PERFORMANCE"));
			output_colour();
			for_every(time, times)
			{
				output(TXT(" %10.5f ms : %S"), time->time, time->name.to_char());
			}
		}
		if (_logInto)
		{
			_logInto->log(TXT("PERFORMANCE"));
			_logInto->increase_indent();
			for_every(time, times)
			{
				_logInto->log(TXT(" %10.5f ms : %S"), time->time, time->name.to_char());
			}
			_logInto->decrease_indent();
		}

	}

	RESTORE_STORE_DIRECT_GL_CALL();
	RESTORE_STORE_DRAW_CALL();
}

void Performance::System::add_long_job(tchar const * _name, Timer const & _time)
{
	if (!s_system)
	{
		return;
	}

	Concurrency::ScopedSpinLock lock(s_system->longJobsLock);

	for_every(longJob, s_system->longJobs)
	{
		if (longJob->name == _name)
		{
			longJob->add_time(_time.get_time_ms());
			return;
		}
	}

	LongJob longJob;
	longJob.name = _name;
	longJob.add_time(_time.get_time_ms());

	s_system->longJobs.push_back(longJob);
}

void Performance::System::output_long_jobs()
{
	if (!s_system)
	{
		return;
	}

	Concurrency::ScopedSpinLock lock(s_system->longJobsLock);

	output(TXT("long jobs"));
	for_every(longJob, s_system->longJobs)
	{
		output(TXT("  %10.5f<%10.5f<%10.5f ms [%06i] %S"), longJob->timeRange.min, longJob->avg, longJob->timeRange.max, longJob->count, longJob->name.to_char());
	}
}

void Performance::System::set_additional_info(tchar const* _id, String const& _info)
{
#ifdef USE_PERFORMANCE_ADDITIONAL_INFO
	if (!s_system)
	{
		return;
	}

	Concurrency::ScopedSpinLock lock(s_system->additionalInfosLock);

	for_every(ai, s_system->additionalInfos)
	{
		if (String::compare_icase(_id, ai->id))
		{
			ai->info = _info;
			return;
		}
	}

	AdditionalInfo ai;
	ai.id = _id;
	ai.info = _info;
	s_system->additionalInfos.push_back(ai);

	sort(s_system->additionalInfos);
#endif
}

bool Performance::System::can_gather_new_data()
{
	if (!s_system)
	{
		return false;
	}
	return s_system->state == Normal;
}

void Performance::System::save_frame(Optional<bool> const& _force, Optional<String> const& _suffix, int _currFrameOffset)
{
	if (s_system->state != Normal)
	{
#ifdef INSPECT_SAVE_FRAME
		output(TXT("NOT NORMAL, don't save (state:%i, saving state:%i  %i.%i/%i)"), s_system->state, s_system->savingState, s_system->savingStateI, s_system->savingStateISub, s_system->savingStateCount);
#endif
		return;
	}
	if (! _force.get(false) &&
		(s_system->lastSaveTS.get_time_since() < 0.5f ||
		 s_system->savedCount > 100)) // to avoid storing too much
	{
#ifdef INSPECT_SAVE_FRAME
		output(TXT("don't save performance frame info due to:"));
		output(TXT("  s_system->lastSaveTS.get_time_since() %.3f"), s_system->lastSaveTS.get_time_since());
		output(TXT("  s_system->savedCount %i"), s_system->savedCount);
#endif
		return;
	}

	if (auto* threadManager = Concurrency::ThreadManager::get())
	{
		s_system->state = Saving;
#ifdef INSPECT_SAVE_FRAME
		s_system->savingState = 0;
		s_system->savingStateI = 0;
		s_system->savingStateISub = 0;
		s_system->savingStateCount = 0;
#endif
		s_system->savingExtraData.suffix = _suffix.get(String::empty());
		s_system->savingExtraData.currFrameOffset = _currFrameOffset;
		output(TXT("save frame performance info, frame #%i"), s_system->displayFrameInfo[CURR_FRAME_INFO_IDX].performanceFrameNo - _currFrameOffset);
		prepare_to_save_frame();
		threadManager->create_thread(save_frame_thread_func, nullptr, ThreadPriority::Lower, true);
	}
}

void Performance::System::prepare_to_save_frame()
{
	if (!s_system)
	{
		return;
	}
	if (s_system->savedFrameNo.is_set() &&
		s_system->savedFrameNo.get() == s_system->displayFrameInfo[CURR_FRAME_INFO_IDX].performanceFrameNo)
	{
		return;
	}
	// start with combined as it gives most of the information
	for(int saveFrameWorkerIdx = MAX_PREV_FRAMES ; saveFrameWorkerIdx >= 0; --saveFrameWorkerIdx)
	{
		int saveFrameIdx = saveFrameWorkerIdx;
		if (saveFrameWorkerIdx < 0 || saveFrameWorkerIdx >= MAX_PREV_FRAMES)
		{
			saveFrameIdx = -1;
		}

		int32 threadCount = Concurrency::ThreadManager::get()->get_thread_count();
		for (int32 threadIdx = 0; threadIdx < threadCount; ++threadIdx)
		{
			ThreadInfo const* thread = s_system->threads[threadIdx];

			for (int iframe = 0; iframe < (saveFrameIdx < 0? MAX_PREV_FRAMES : 1); ++ iframe)
			{
				int useFrameIdx = saveFrameIdx < 0? iframe : saveFrameIdx;
				for_every(block, *thread->displayBlocks[useFrameIdx])
				{
					block->prepare_hard_copy();
				}
			}
		}
	}
}

void Performance::System::save_frame_thread_func(void* _data)
{
	if (!s_system)
	{
		if (s_system->state == Saving)
		{
			s_system->lastSaveTS.reset();
			s_system->state = FailedToSave;
		}
		return;
	}
	if (s_system->savedFrameNo.is_set() &&
		s_system->savedFrameNo.get() == s_system->displayFrameInfo[CURR_FRAME_INFO_IDX].performanceFrameNo)
	{
		if (s_system->state == Saving)
		{
			s_system->lastSaveTS.reset();
			s_system->state = FailedToSave;
		}
		return;
	}
#ifdef INSPECT_SAVE_FRAME
	s_system->savingState = 1;
#endif
	s_system->savedFrameNo = s_system->displayFrameInfo[CURR_FRAME_INFO_IDX].performanceFrameNo;
	// start with combined as it gives most of the information
	for(int saveFrameWorkerIdx = MAX_PREV_FRAMES ; saveFrameWorkerIdx >= 0; --saveFrameWorkerIdx)
	{
#ifdef INSPECT_SAVE_FRAME
		int const savingStatePerFrame = 50;
		int const savingStatePerThread = 5;
		int const savingStateBase = savingStatePerFrame + savingStatePerFrame * (MAX_PREV_FRAMES - saveFrameWorkerIdx);
		s_system->savingState = savingStateBase;
#endif
		int saveFrameIdx = saveFrameWorkerIdx;
		if (saveFrameWorkerIdx < 0 || saveFrameWorkerIdx >= MAX_PREV_FRAMES)
		{
			saveFrameIdx = -1;
		}
		auto& saveFrameInfo = s_system->displayFrameInfo[max(0, saveFrameIdx)];
		auto& cstats = saveFrameInfo.concurrencyStats;
		auto& dFrameNo = saveFrameInfo.performanceFrameNo;
		auto dFrameTime = saveFrameInfo.frameTime;

#ifdef INSPECT_SAVE_FRAME
		s_system->savingState = savingStateBase + 1;
#endif
		if (saveFrameIdx < 0)
		{
			for_count(int, i, MAX_PREV_FRAMES)
			{
				dFrameTime.include(s_system->displayFrameInfo[i].frameTime);
			}
		}
		String fileName;
		String dateAndTime;
		{
			time_t currentTime = time(0);
			tm currentTimeInfo;
			tm* pcurrentTimeInfo = &currentTimeInfo;
#ifdef AN_CLANG
			pcurrentTimeInfo = localtime(&currentTime);
#else
			localtime_s(&currentTimeInfo, &currentTime);
#endif
			String suffix;
			if (saveFrameIdx < 0)
			{
				suffix = TXT(" combined");
			}
			else
			{
				int actFrameIdx = saveFrameIdx + s_system->savingExtraData.currFrameOffset;
				if (actFrameIdx == 0)
				{
					if (!s_system->savingExtraData.suffix.is_empty())
					{
						suffix = String::space() + s_system->savingExtraData.suffix;
					}
				}
				else if (actFrameIdx > 0)
				{
					suffix = String::printf(TXT(" prev%i"), actFrameIdx);
				}
				else
				{
					suffix = String::printf(TXT(" next%i"), -actFrameIdx);
				}
			}
			dateAndTime = String::printf(TXT("%04i-%02i-%02i %02i-%02i-%02i"),
				pcurrentTimeInfo->tm_year + 1900, pcurrentTimeInfo->tm_mon + 1, pcurrentTimeInfo->tm_mday,
				pcurrentTimeInfo->tm_hour, pcurrentTimeInfo->tm_min, pcurrentTimeInfo->tm_sec);
			fileName = String::printf(TXT("_logs%SperformanceFrame %S pf%06i%S.fpixml"),
				IO::get_directory_separator(),
				dateAndTime.to_char(),
				saveFrameIdx < 0? dFrameNo - s_system->savingExtraData.currFrameOffset : dFrameNo, // for combined store actual frame for which we want combined
				suffix.to_char());
		}
#ifdef INSPECT_SAVE_FRAME
		s_system->savingState = savingStateBase + 2;
#endif

		// now cross-fingers we won't quit before the performance system disappears but if it disappears, it means we should die anyway
		IO::XML::Document doc;
		if (auto* n_performanceFrame = doc.get_root()->add_node(TXT("performanceFrame")))
		{
#ifdef INSPECT_SAVE_FRAME
			s_system->savingState = savingStateBase + 3;
#endif
			n_performanceFrame->set_int_attribute(TXT("frameNo"), dFrameNo);
			n_performanceFrame->set_attribute(TXT("dateAndTime"), dateAndTime);
			n_performanceFrame->set_attribute(TXT("frameTime"), String::printf(TXT("%.12f"), dFrameTime.get_time_ms()));

			if (auto* n_cs = n_performanceFrame->add_node(TXT("concurrencyStats")))
			{
				n_cs->set_int_attribute(TXT("waitedInSpinLock"), cstats.waitedInSpinLock);
				n_cs->set_int_attribute(TXT("wasInSpinLock"), cstats.wasInSpinLock);
				n_cs->set_int_attribute(TXT("waitedInCounter"), cstats.waitedInCounter);
				n_cs->set_int_attribute(TXT("wasInCounter"), cstats.wasInCounter);
				n_cs->set_int_attribute(TXT("waitedInSemaphore"), cstats.waitedInSemaphore);
				n_cs->set_int_attribute(TXT("wasInSemaphore"), cstats.wasInSemaphore);
				n_cs->set_int_attribute(TXT("waitedInMRSWLockRead"), cstats.waitedInMRSWLockRead);
				n_cs->set_int_attribute(TXT("wasInMRSWLockRead"), cstats.wasInMRSWLockRead);
				n_cs->set_int_attribute(TXT("waitedInMRSWLockWrite"), cstats.waitedInMRSWLockWrite);
				n_cs->set_int_attribute(TXT("wasInMRSWLockWrite"), cstats.wasInMRSWLockWrite);
			}

#ifdef INSPECT_SAVE_FRAME
			s_system->savingState = savingStateBase + 4;
#endif
			if (auto* n_ais = n_performanceFrame->add_node(TXT("additionalInfos")))
			{
				Concurrency::ScopedSpinLock lock(s_system->displayAdditionalInfosLock);
				for_every(ai, saveFrameInfo.displayAdditionalInfos)
				{
					if (auto* n_ai = n_ais->add_node(TXT("additionalInfo")))
					{
						n_ai->add_text(ai->info);
					}
				}
				if (saveFrameIdx < 0)
				{
					for_count(int, i, MAX_PREV_FRAMES)
					{
						if (auto* n_ai = n_ais->add_node(TXT("additionalInfo")))
						{
							n_ai->add_text(String::printf(TXT("frame time (%c%i) : %.3fms"), i == 0? '=' : '-', i, s_system->displayFrameInfo[i].frameTime.get_time_ms()));
						}
					}
				}
			}

#ifdef INSPECT_SAVE_FRAME
			s_system->savingState = savingStateBase + 5;
#endif
			int32 threadCount = Concurrency::ThreadManager::get()->get_thread_count();
			for (int32 threadIdx = 0; threadIdx < threadCount; ++threadIdx)
			{
#ifdef INSPECT_SAVE_FRAME
				s_system->savingState = savingStateBase + 5 + savingStatePerThread * threadIdx;
#endif
				ThreadInfo const* thread = s_system->threads[threadIdx];

				if (auto* n_thread = n_performanceFrame->add_node(TXT("thread")))
				{
#ifdef PROFILE_PERFORMANCE
					n_thread->set_int_attribute(TXT("cpu"), Concurrency::ThreadManager::get()->get_thread_system_cpu(threadIdx));
#endif
					n_thread->set_int_attribute(TXT("id"), Concurrency::ThreadManager::get()->get_thread_system_id(threadIdx));
					{
						int threadPolicy = Concurrency::ThreadManager::get()->get_thread_policy(threadIdx);
						n_thread->set_int_attribute(TXT("policy"), threadPolicy);
						n_thread->set_attribute(TXT("policyHex"), String::printf(TXT("%X"), threadPolicy));
					}
					n_thread->set_int_attribute(TXT("priority"), Concurrency::ThreadManager::get()->get_thread_priority(threadIdx));
					n_thread->set_int_attribute(TXT("priorityNice"), Concurrency::ThreadManager::get()->get_thread_priority_nice(threadIdx));
					for (int iframe = 0; iframe < (saveFrameIdx < 0? MAX_PREV_FRAMES : 1); ++ iframe)
					{
#ifdef INSPECT_SAVE_FRAME
						s_system->savingState = savingStateBase + 5 + savingStatePerThread * threadIdx + iframe * 2 + 0;
#endif
						int useFrameIdx = saveFrameIdx < 0? iframe : saveFrameIdx;
#ifdef INSPECT_SAVE_FRAME
						s_system->savingStateCount = thread->displayBlocks[useFrameIdx]->get_size();
#endif
						for_every(block, *thread->displayBlocks[useFrameIdx])
						{
#ifdef INSPECT_SAVE_FRAME
							s_system->savingStateI = for_everys_index(block);
							s_system->savingStateISub = 0;
#endif
							if (auto* n_block = n_thread->add_node(TXT("block")))
							{
#ifdef INSPECT_SAVE_FRAME
								s_system->savingStateISub = 1;
#endif
								Range blockRelRange = block->get_relative_range(dFrameTime);
								bool blockHasFinished = block->has_finished();
								tchar const * blockName = block->get_tag().nameAsTCharHardCopy;
								String blockContextInfo(block->get_tag().contextInfo);
								bool blockIsLock = block->get_tag().type == BlockType::Lock;
								Colour blockColour = block->get_tag().colour;

#ifdef INSPECT_SAVE_FRAME
								s_system->savingStateISub = 2;
#endif
								if (blockName)
								{
									n_block->set_attribute(TXT("name"), blockName);
								}
#ifdef INSPECT_SAVE_FRAME
								s_system->savingStateISub = 3;
#endif
								if (!blockContextInfo.is_empty())
								{
									n_block->set_attribute(TXT("info"), blockContextInfo);
								}
								n_block->set_attribute(TXT("relTime"), String::printf(TXT("%.12f to %.12f"), blockRelRange.min, blockRelRange.max));
#ifdef INSPECT_SAVE_FRAME
								s_system->savingStateISub = 4;
#endif
								blockColour.save_to_xml_attribute(n_block, TXT("colour"));
								if (!blockHasFinished)
								{
									n_block->set_bool_attribute(TXT("finished"), false);
								}
								if (blockIsLock)
								{
									n_block->set_bool_attribute(TXT("lock"), true);
								}
#ifdef INSPECT_SAVE_FRAME
								s_system->savingStateISub = 5;
#endif
							}
						}
#ifdef INSPECT_SAVE_FRAME
						s_system->savingState = savingStateBase + 5 + savingStatePerThread * threadIdx + iframe * 2 + 1;
						s_system->savingStateCount = thread->displayMarkers[useFrameIdx]->get_size();
#endif
						for_every(marker, *thread->displayMarkers[useFrameIdx])
						{
#ifdef INSPECT_SAVE_FRAME
							s_system->savingStateI = for_everys_index(marker);
							s_system->savingStateISub = 0;
#endif
							if (auto* n_marker = n_thread->add_node(TXT("marker")))
							{
#ifdef INSPECT_SAVE_FRAME
								s_system->savingStateISub = 1;
#endif
								Range markerRelRange = marker->get_relative_range(dFrameTime);
#ifdef INSPECT_SAVE_FRAME
								s_system->savingStateISub = 2;
#endif

								n_marker->set_attribute(TXT("relTime"), String::printf(TXT("%.12f"), markerRelRange.min));
								Colour c = marker->get_colour();
								n_marker->set_attribute(TXT("r"), String::printf(TXT("%.3f"), c.r));
								n_marker->set_attribute(TXT("g"), String::printf(TXT("%.3f"), c.g));
								n_marker->set_attribute(TXT("b"), String::printf(TXT("%.3f"), c.b));
								n_marker->set_attribute(TXT("a"), String::printf(TXT("%.3f"), c.a));
#ifdef INSPECT_SAVE_FRAME
								s_system->savingStateISub = 3;
#endif
							}
						}
					}
				}
			}
		}
#ifdef INSPECT_SAVE_FRAME
		s_system->savingState = savingStateBase + savingStatePerFrame - 3;
#endif
		{
			IO::File file;
			if (file.create(IO::get_user_game_data_directory() + fileName))
			{
#ifdef INSPECT_SAVE_FRAME
				s_system->savingState = savingStateBase + savingStatePerFrame - 2;
#endif
				file.set_type(IO::InterfaceType::Text);
				doc.save_xml(&file);
			}
		}
#ifdef INSPECT_SAVE_FRAME
		s_system->savingState = savingStateBase + savingStatePerFrame - 1;
#endif
		break; // store only combined, storing others just disallows storing more often
	}

#ifdef INSPECT_SAVE_FRAME
	s_system->savingState = 1000;
#endif

	if (s_system->state == Saving)
	{
#ifdef INSPECT_SAVE_FRAME
		s_system->savingState = 1001;
#endif
		s_system->lastSaveTS.reset();
		s_system->state = Saved;
	}
}

void Performance::System::load_frame()
{
	if (s_system->state != Normal &&
		s_system->state != ShowLoaded)
	{
		return;
	}

	if (auto* threadManager = Concurrency::ThreadManager::get())
	{
		// info: you may want to load it here, instead of a separate thread to avoid issues with Name etc
		//threadManager->create_thread(load_frame_thread_func, nullptr, ThreadPriority::Lower, true);
		load_frame_thread_func(nullptr);
	}
}

void Performance::System::load_frame_thread_func(void* _data)
{
	if (!s_system)
	{
		return;
	}
	if (s_system->state != Normal &&
		s_system->state != ShowLoaded)
	{
		return;
	}

	String filePath;
#ifdef AN_WINDOWS
	::System::FaultHandler::set_ignore_faults(true); // otherwise exceptions will stop us when trying to open a file
	{
		HRESULT hr = CoInitializeEx(NULL, COINIT_APARTMENTTHREADED |
			COINIT_DISABLE_OLE1DDE);
		if (SUCCEEDED(hr))
		{
			IFileOpenDialog* pFileOpen;

			// Create the FileOpenDialog object.
			hr = CoCreateInstance(CLSID_FileOpenDialog, NULL, CLSCTX_ALL,
				IID_IFileOpenDialog, reinterpret_cast<void**>(&pFileOpen));

			HWND hwnd;
			HDC hdc;
			::System::Video3D::get()->get_hwnd_and_hdc(hwnd, hdc);

			if (SUCCEEDED(hr))
			{
				COMDLG_FILTERSPEC rgSpec[] =
				{
					{ TXT("frame performance info files (*.fpixml)"), TXT("*.fpixml") },
				};
				pFileOpen->SetFileTypes(1, rgSpec);
				{
					WCHAR cPath[4096] = L"";
					GetCurrentDirectoryW(sizeof(cPath) / sizeof(cPath[0]), cPath);
					IShellItem* currFolder;
					HRESULT hr = SHCreateItemFromParsingName(cPath, NULL, IID_PPV_ARGS(&currFolder));
					if (SUCCEEDED(hr))
					{
						pFileOpen->SetDefaultFolder(currFolder);
					}
				}
				// Show the Open dialog box.
				hr = pFileOpen->Show(hwnd);

				// Get the file name from the dialog box.
				if (SUCCEEDED(hr))
				{
					IShellItem* pItem;
					hr = pFileOpen->GetResult(&pItem);
					if (SUCCEEDED(hr))
					{
						PWSTR pszFilePath;
						hr = pItem->GetDisplayName(SIGDN_FILESYSPATH, &pszFilePath);

						// Display the file name to the user.
						if (SUCCEEDED(hr))
						{
							filePath = pszFilePath;
							CoTaskMemFree(pszFilePath);
						}
						pItem->Release();
					}
				}
				pFileOpen->Release();
			}
			CoUninitialize();
		}
	}
	::System::FaultHandler::set_ignore_faults(false);
#endif
	if (s_system->state != Normal &&
		s_system->state != ShowLoaded)
	{
		return;
	}
	if (filePath.is_empty())
	{
		return;
	}
	IO::File file;
	if (! file.open(filePath))
	{
		return;
	}
	file.set_type(IO::InterfaceType::Text);
	IO::XML::Document doc;
	if (!doc.load_xml(&file))
	{
		return;
	}
	s_system->state = Loading;
	if (auto* n_performanceFrame = doc.get_root()->first_child_named(TXT("performanceFrame")))
	{
		{
			Concurrency::ScopedSpinLock lock(s_system->displayAdditionalInfosLock);
			for_count(int, i, MAX_PREV_FRAMES)
			{
				s_system->displayFrameInfo[i] = FrameInfo();
			}
		}
		auto& displayFrameInfo = s_system->displayFrameInfo[CURR_FRAME_INFO_IDX];
		displayFrameInfo.performanceFrameNo = n_performanceFrame->get_int_attribute(TXT("frameNo"), displayFrameInfo.performanceFrameNo);
		s_system->showLoadedInfo = n_performanceFrame->get_string_attribute(TXT("dateAndTime"));
		{
			float dftLength = n_performanceFrame->get_float_attribute(TXT("frameTime"));
			displayFrameInfo.frameTime.set_based_zero(dftLength);
		}

		displayFrameInfo.concurrencyStats = Concurrency::Stats(); // clear
		if (auto* n_cs = n_performanceFrame->first_child_named(TXT("concurrencyStats")))
		{
			displayFrameInfo.concurrencyStats.waitedInSpinLock = n_cs->get_int_attribute(TXT("waitedInSpinLock"));
			displayFrameInfo.concurrencyStats.wasInSpinLock = n_cs->get_int_attribute(TXT("wasInSpinLock"));
			displayFrameInfo.concurrencyStats.waitedInCounter = n_cs->get_int_attribute(TXT("waitedInCounter"));
			displayFrameInfo.concurrencyStats.wasInCounter = n_cs->get_int_attribute(TXT("wasInCounter"));
			displayFrameInfo.concurrencyStats.waitedInSemaphore = n_cs->get_int_attribute(TXT("waitedInSemaphore"));
			displayFrameInfo.concurrencyStats.wasInSemaphore = n_cs->get_int_attribute(TXT("wasInSemaphore"));
			displayFrameInfo.concurrencyStats.waitedInMRSWLockRead = n_cs->get_int_attribute(TXT("waitedInMRSWLockRead"));
			displayFrameInfo.concurrencyStats.wasInMRSWLockRead = n_cs->get_int_attribute(TXT("wasInMRSWLockRead"));
			displayFrameInfo.concurrencyStats.waitedInMRSWLockWrite = n_cs->get_int_attribute(TXT("waitedInMRSWLockWrite"));
			displayFrameInfo.concurrencyStats.wasInMRSWLockWrite = n_cs->get_int_attribute(TXT("wasInMRSWLockWrite"));
		}

		{
			Concurrency::ScopedSpinLock lock(s_system->displayAdditionalInfosLock);
			displayFrameInfo.displayAdditionalInfos.clear();
			if (auto* n_ais = n_performanceFrame->first_child_named(TXT("additionalInfos")))
			{
				for_every(n_ai, n_ais->children_named(TXT("additionalInfo")))
				{
					AdditionalInfo ai;
					ai.info = n_ai->get_text();
					displayFrameInfo.displayAdditionalInfos.push_back(ai);
				}
			}
		}

		// clear all threads first
		for_every_ptr(thread, s_system->threads)
		{
			for_count(int, i, MAX_PREV_FRAMES)
			{
				thread->displayBlocks[i]->clear();
				thread->displayMarkers[i]->clear();
			}
		}

		s_system->loadedThreadCount = 0;
		int32 threadIdx = 0;
		for_every(n_thread, n_performanceFrame->children_named(TXT("thread")))
		{
			ThreadInfo * thread = s_system->threads[threadIdx];
			auto& threadDisplayBlocks = thread->displayBlocks[CURR_FRAME_INFO_IDX];
			auto& threadDisplayMarkers = thread->displayMarkers[CURR_FRAME_INFO_IDX];
			threadDisplayBlocks->clear();
			thread->read_cpu = n_thread->get_int_attribute(TXT("cpu"), NONE);
			thread->read_id = n_thread->get_int_attribute(TXT("id"), NONE);
			thread->read_policy = n_thread->get_int_attribute(TXT("policy"), NONE);
			thread->read_priority = n_thread->get_int_attribute(TXT("priority"), NONE);
			thread->read_priorityNice = n_thread->get_int_attribute(TXT("priorityNice"), NONE);
			for_every(n_block, n_thread->children_named(TXT("block")))
			{
				Range blockRelRange = Range::zero;
				blockRelRange.load_from_string(n_block->get_string_attribute(TXT("relTime")));
				bool blockHasFinished = n_block->get_bool_attribute(TXT("finished"), true);
				Name blockName = n_block->get_name_attribute(TXT("name"));
				String blockContextInfo = n_block->get_string_attribute(TXT("info"));
				Colour blockColour = Colour::grey;
				blockColour.load_from_xml_child_or_attr(n_block, TXT("colour"));
				bool blockIsLock = n_block->get_bool_attribute(TXT("lock"), false);

				if (!blockContextInfo.is_empty())
				{
					threadDisplayBlocks->push_back(Block(BlockTag(blockContextInfo, blockIsLock ? BlockType::Lock : BlockType::Normal, blockColour)));
				}
				else
				{
					threadDisplayBlocks->push_back(Block(BlockTag(blockName, blockIsLock ? BlockType::Lock : BlockType::Normal, blockColour)));
				}
				Timer blockTimer;
				blockTimer.set_from_relative(displayFrameInfo.frameTime, blockRelRange, blockHasFinished);
				auto& block = threadDisplayBlocks->get_last();
				block.update(blockTimer);
			}

			for_every(n_marker, n_thread->children_named(TXT("marker")))
			{
				float markerRel = n_marker->get_float_attribute(TXT("relTime"));

				Timer markerTimer;
				markerTimer.set_from_relative(displayFrameInfo.frameTime, Range(markerRel));

				Colour c = markerTimer.get_colour();
				c.load_from_xml(n_marker);
				markerTimer.set_colour(c);

				threadDisplayMarkers->push_back(markerTimer);
			}

			++threadIdx;
			++s_system->loadedThreadCount;
		}
	}

	if (s_system->state == Loading)
	{
		s_system->state = ShowLoaded;
	}
}

bool Performance::System::is_saving()
{
	if (s_system)
	{
		if (s_system->state == Saving)
		{
			return true;
		}
	}
	return false;
}

bool Performance::System::is_saving_or_just_saved()
{
	if (s_system)
	{
		if (s_system->state == Saving ||
			s_system->state == Saved ||
			s_system->state == FailedToSave ||
			s_system->lastSaveTS.get_time_since() < 0.5f)
		{
			return true;
		}
	}
	return false;
}
