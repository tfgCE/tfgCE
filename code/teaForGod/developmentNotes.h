#pragma once

#include "..\core\buildVer.h"
#include "..\core\types\string.h"

#define NOTE(_note) notes.push_back(String(TXT(_note)));
// use NOTE("put your note here")

inline String development_notes()
{
	List<String> notes;
	
	NOTE("this is a preview build");
	NOTE("");
	NOTE("above there is a debug menu");
	NOTE("");
	NOTE("please report any issues with send note/log");
	NOTE("dead end issues are easiest to report with");
	NOTE("     \"quick send info about wall/loading time issue\"");
	NOTE("");
	NOTE("when sending a log, a screenshot is attached");
	NOTE("of what you've seen before entering in-game menu");
	NOTE("");

	// list of things to check should be placed here
#ifndef BUILD_PUBLIC_RELEASE
#ifndef BUILD_PREVIEW_RELEASE
#endif
#endif

	return String::combine_lines(notes, String(TXT("~")));
}
