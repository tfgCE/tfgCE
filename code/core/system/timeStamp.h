#pragma once

#include "..\globalDefinitions.h"
#include "..\utils.h"

#include "timePoint.h"

namespace System
{
	struct TimeStamp
	{
	public:
		TimeStamp();

		void reset(float _offset = 0.0f);

		float get_time_since() const;
		float get_time_to_zero() const;

	private:
		TimePoint timePoint;
		float offset = 0.0f;
	};

	struct ScopedTimeStamp
	{
	public:
		ScopedTimeStamp(float& _storeIn) : storeIn(_storeIn){}
		~ScopedTimeStamp() { storeIn = ts.get_time_since(); }
	private:
		TimeStamp ts;
		float& storeIn;
	};

	struct ScopedTimeStampLimitGuard
	{
	public:
		ScopedTimeStampLimitGuard(float _limit, tchar const * _info, bool _breakOnLimit = false);
		ScopedTimeStampLimitGuard(float _limit, tchar const * _info, tchar const * _secondInfo, bool _breakOnLimit = false);
		~ScopedTimeStampLimitGuard();
	private:
		static int const MAX_LENGTH = 255;
		static int get_MAX_LENGTH() { return MAX_LENGTH; }
		TimeStamp ts;
		float limit;
		bool breakOnLimit = false;
		tchar info[MAX_LENGTH + 1];
		tchar secondInfo[MAX_LENGTH + 1];
	};

	struct ScopedTimeStampOutput
	{
	public:
		ScopedTimeStampOutput(tchar const * _info);
		~ScopedTimeStampOutput();
	private:
		static int const MAX_LENGTH = 255;
		static int get_MAX_LENGTH() { return MAX_LENGTH; }
		TimeStamp ts;
		tchar info[MAX_LENGTH + 1];
	};

};
