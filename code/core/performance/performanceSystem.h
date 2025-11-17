#pragma once

#include "..\globalDefinitions.h"
#include "..\globalSettings.h"
#include "..\globalInclude.h"

#include "..\concurrency\mrswLock.h"

#include "performanceBlock.h"

#include "..\containers\list.h"
#include "..\system\timeStamp.h"

namespace System
{
	class Font;
};

#define MAX_PREV_FRAMES 3
#define CURR_FRAME_INFO_IDX 0

//#define INSPECT_SAVE_FRAME

namespace Performance
{
	struct BlockToken;

	struct ThreadInfo
	{
		Concurrency::SpinLock gatherBlockLock = Concurrency::SpinLock(SYSTEM_SPIN_LOCK);
		Concurrency::SpinLock gatherMarkerLock = Concurrency::SpinLock(SYSTEM_SPIN_LOCK);

		static int const MAX_SIZE = 10000; // if exceeded, increase here, we may have pointers to blocks hanging there, we don't want to write over some innocent memory
		static int const MAX_SIZE_MARKERS = 100; // if exceeded, increase here, we may have pointers to blocks hanging there, we don't want to write over some innocent memory

		// for markers, only start stamp of the timer is used

		// pointers to working blocks - in one information is gathered, from other is displayed
		Array<Block>* displayBlocks[MAX_PREV_FRAMES];
		Array<Block>* gatherBlocks;
		Array<Timer>* displayMarkers[MAX_PREV_FRAMES];
		Array<Timer>* gatherMarkers;
		// working blocks:
		Array<Block> blocks[MAX_PREV_FRAMES + 1];
		Array<Timer> markers[MAX_PREV_FRAMES + 1];

		int read_cpu = NONE;
		int read_id = NONE;
		int read_policy = NONE;
		int read_priority = NONE;
		int read_priorityNice = NONE;

		ThreadInfo();
		Block* add_block(BlockTag const & _tag);
		Block* find_block(BlockToken const & _token);

		void add_marker(Timer const& _timer);

		void swap_blocks();
		void clear_gatherer();
	};

	struct BlockToken
	{
		Block * block;
		BlockID id;

		BlockToken()
		: block(nullptr)
		, id(0)
		{}

		bool is_valid() const { return block != nullptr;  }
	};
	
	/* maybe not very long, but those are distinctive jobs that are happening on the way */
	struct LongJob
	{
		String name;
		Range timeRange = Range::empty;
		float avg = 0.0f;
		float sum = 0.0f;
		int count = 0;
		void add_time(float _t)
		{
			timeRange.include(_t);
			++ count;
			sum += _t;
			avg = sum / (float)count;
		}
	};

	struct SystemCustomDisplay
	{
		Optional<VectorInt2> viewportSize;
		Optional<Vector2> mouseAt;

		Optional<float> zoom;
		Optional<float> pan;

		Optional<bool> inputActivate;
	};

	class System
	{
	public:
		static int get_max_prev_frames() { return MAX_PREV_FRAMES; }

		static System* get() { return s_system; }
		static void initialise_static();
		static void close_static();

		static int get_performance_frame_no();

		static void add_block(BlockTag const & _tag, Timer const & _timer, OUT_ BlockToken & _token);
		static void update_block(BlockTag const & _tag, Timer const & _timer, BlockToken & _token);

		static void add_marker(Timer const& _timer);

		static void start_frame();
		static void end_frame(bool _keepExistingResults = false);

		// display and stay inside until we close it
		static void display(::System::Font const * _font, bool _render, bool _outputLog, LogInfoContext* _logInto = nullptr, SystemCustomDisplay const * _customDisplay = nullptr);
		
		static void add_long_job(tchar const * _name, Timer const & _time);
		static void output_long_jobs();

		static void set_additional_info(tchar const* _id, String const& _info);

		static bool can_gather_new_data();

		static void save_frame(Optional<bool> const & _force = NP/*false*/, Optional<String> const& _suffix = NP, int _currFrameOffset = 0);
		static void load_frame();
		
		static bool is_saving();
		static bool is_saving_or_just_saved();

	private:
		static System* s_system;

		enum State
		{
			Normal,			// gathers data every frame - gathering data is possible only here
			Saving,			// saving to a file
			Saved,			// saved, when a frame is ended will switch to normal
			FailedToSave,	// failed to save
			Loading,		// loads from a file, when loaded, will switch to showing loaded
			ShowLoaded		// won't be gathering new frames, will always keep existing
		};
		volatile State state = Normal;
#ifdef INSPECT_SAVE_FRAME
		volatile int savingState = 0;
		volatile int savingStateI = 0;
		volatile int savingStateISub = 0;
		volatile int savingStateCount = 0;
#endif

		struct SavingExtraData
		{
			String suffix;
			int currFrameOffset; // 0 this one, 1 previous, 2 previous
		} savingExtraData;
		::System::TimeStamp lastSaveTS;
		int savedCount = 0;

		// replace process lock with counters indicating whether we're gathering or processing frame
		Concurrency::MRSWLock processLock; // processing info, although we reverse roles as we can have multiple reads due to having separate threads

		struct AdditionalInfo
		{
			tchar const* id;
			String info;

			static int compare(void const* _a, void const* _b)
			{
				AdditionalInfo const* a = plain_cast<AdditionalInfo>(_a);
				AdditionalInfo const* b = plain_cast<AdditionalInfo>(_b);
				return String::diff_icase(a->id, b->id);
			}
		};

		struct FrameInfo
		{
			Concurrency::Stats concurrencyStats;
			Timer frameTime;
			int performanceFrameNo = NONE;
			Array<AdditionalInfo> displayAdditionalInfos;
		};
		Array<ThreadInfo*> threads;
		int loadedThreadCount = 0;
		String showLoadedInfo;
		Timer frameTime;
		float centre = 0.5f;
		float zoom = 1.0f;
		int performanceFrameNo = NONE;
		FrameInfo displayFrameInfo[MAX_PREV_FRAMES];
		Optional<int> savedFrameNo;

		Concurrency::SpinLock longJobsLock = Concurrency::SpinLock(SYSTEM_SPIN_LOCK);
		List<LongJob> longJobs;

		Concurrency::SpinLock additionalInfosLock = Concurrency::SpinLock(SYSTEM_SPIN_LOCK);
		Array<AdditionalInfo> additionalInfos;
		Concurrency::SpinLock displayAdditionalInfosLock = Concurrency::SpinLock(SYSTEM_SPIN_LOCK); // locks displayFrameInfos

		System();
		~System();

		ThreadInfo* get_thread(int32 _threadIdx);

		static void prepare_to_save_frame();
		static void save_frame_thread_func(void* _data);
		static void load_frame_thread_func(void* _data);
	};

};
