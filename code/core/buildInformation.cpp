#include "buildInformation.h"

#include "other\parserUtils.h"
#include "types\string.h"

#include <time.h>

#include "buildVer.h"

bool buildInfoCreated = false;
bool verInfoCreated = false;
bool verInfoShortCreated = false;
tchar buildInfo[1024];
tchar verInfo[1024];
tchar verInfoShort[1024];

tchar const * get_build_information()
{
	if (!buildInfoCreated)
	{
		String buildInformation;
		
		buildInformation += get_build_version();

		buildInformation += String::printf(TXT(" built on: %S, %S"), TXT(__DATE__), TXT(__TIME__));

		/*
		// this can be used to create define in some external tool
		time_t currentTime = time(0);
		tm currentTimeInfo;
		tm* pcurrentTimeInfo = &currentTimeInfo;
#ifdef AN_CLANG
		pcurrentTimeInfo = localtime(&currentTime);
#else
		localtime_s(&currentTimeInfo, &currentTime);
#endif
		buildInformation += String::printf("  when: %04i-%02i-%02i", pcurrentTimeInfo->tm_year + 1900, pcurrentTimeInfo->tm_mon + 1, pcurrentTimeInfo->tm_mday);
		buildInformation += String::printf(" %02i:%02i:%02i", pcurrentTimeInfo->tm_hour, pcurrentTimeInfo->tm_min, pcurrentTimeInfo->tm_sec);
		*/

		memory_copy(buildInfo, &buildInformation[0], (buildInformation.get_length() + 1) * sizeof(tchar));

		buildInfoCreated = true;
	}
	return buildInfo;
}

tchar const * get_build_version()
{
	if (!verInfoCreated)
	{
		String buildInformation;

		buildInformation += String::printf(TXT("v%i.%i.%i"), BUILD_VER_MAJOR, BUILD_VER_MINOR, BUILD_VER_SUBMINOR);

		buildInformation += String::printf(TXT(" (#%i)"), BUILD_NO);

		if (tstrlen(TXT(BUILD_VER_SUFFIX)) > 0)
		{
			buildInformation += String::printf(TXT(" [%S]"), TXT(BUILD_VER_SUFFIX));
		}

#ifdef AN_DEBUG
		buildInformation += TXT(" [debug]");
#else
#ifdef AN_DEV
		buildInformation += TXT(" [dev]");
#else
#ifdef AN_RELEASE
		//buildInformation += TXT("[release]"); // no such info required
#else
#ifdef AN_PROFILER
		buildInformation += TXT(" [profiler]");
#else
		buildInformation += TXT(" [undefined]");
#endif
#endif
#endif
#endif
		memory_copy(verInfo, &buildInformation[0], (buildInformation.get_length() + 1) * sizeof(tchar));

		verInfoCreated = true;
	}
	return verInfo;
}

tchar const* get_build_version_short()
{
	if (!verInfoShortCreated)
	{
		String buildInformation;

		buildInformation += String::printf(TXT("v%i.%i.%i"), BUILD_VER_MAJOR, BUILD_VER_MINOR, BUILD_VER_SUBMINOR);

		buildInformation += String::printf(TXT(" (#%i)"), BUILD_NO);

		memory_copy(verInfoShort, &buildInformation[0], (buildInformation.get_length() + 1) * sizeof(tchar));

		verInfoShortCreated = true;
	}
	return verInfoShort;
}

tchar const* get_build_date()
{
	return TXT(BUILD_DATE);
}

String get_build_version_short_custom(int _maj, int _min, int _subMin, int _buildNo)
{
	return String::printf(TXT("v%i.%i.%i (#%i)"), _maj, _min, _subMin, _buildNo);
}

tchar const* get_platform()
{
#ifdef AN_WINDOWS
	return TXT("windows");
#endif
#ifdef AN_QUEST
	return TXT("quest");
#endif
#ifdef AN_VIVE
	return TXT("vive");
#endif
#ifdef AN_PICO
	return TXT("pico");
#endif
#ifdef AN_ANDROID
	return TXT("android");
#endif
	return TXT("??");
}

int get_build_version_major()
{
	return BUILD_VER_MAJOR;
}

int get_build_version_minor()
{
	return BUILD_VER_MINOR;
}

int get_build_version_sub_minor()
{
	return BUILD_VER_SUBMINOR;
}

int get_build_version_build_no()
{
	return BUILD_NO;
}
