#pragma once

#include "globalSettings.h"

#ifdef AN_WINDOWS
#include <xkeycheck.h>
#endif

#include <stdint.h>

#ifdef WIN64
typedef long long memoryint;
#else
typedef long memoryint;
#endif

#ifdef AN_ANDROID
typedef short char16;
#else
typedef wchar_t char16;
#endif
typedef char char8;

#ifdef AN_WIDE_CHAR
	// based on: http://stackoverflow.com/questions/17700797/printf-wprintf-s-s-ls-char-and-wchar-errors-not-announced-by-a-compil
	//      and: http://stackoverflow.com/questions/10000723/visual-studio-swprintf-is-making-all-my-s-formatters-want-wchar-t-instead-of
	typedef wchar_t tchar;
	#define tprintf wprintf
	#define tvprintf vwprintf
	#define tsprintf swprintf
	#define tsprintf_s swprintf_s
	#define tvsprintf vswprintf
	#define tstrlen wcslen
	#ifdef AN_CLANG
		#define tstrcmp wcscmp
		#define tstrcmpi(a,b) wcscasecmp(a,b)
	#else
		#define tstrcmp(a,b) wcsncmp(a,b,99999)
		#define tstrcmpi(a,b) _wcsnicmp(a,b,99999)
	#endif
	#define tscanf_s swscanf_s
	#define tsopen_s _wsopen_s
	#ifdef AN_CLANG
		#define ttof(a) wcstof(a, nullptr)
		#define ttoi(a) wcstol(a, nullptr, 10)
	#else
		#define ttof _wtof
		#define ttoi _wtoi
	#endif
	#define OutputDebugString OutputDebugStringW
	#define MessageBox MessageBoxW
	#define TXT_(_a) L##_a
	#define TXT(_text) TXT_(_text)
	#define STRING_CONTENT(_text) #_text
	#define STRINGIZE_(_text) STRING_CONTENT(_text)
	#define STRINGIZE(_text) TXT(STRINGIZE_(_text))
#else
	typedef char8 tchar;
	#define tprintf printf
	#define tvprintf vprintf
	#define tsprintf sprintf
	#define tsprintf_s sprintf_s
	#define tvsprintf vsprintf
	#define tstrlen strlen
	#define tstrcmp strcmp
	#define tstrcmpi _stricmp
	#define tscanf_s _sscanf_s
	#define tsopen_s _sopen_s
	#define ttof atof
	#define ttoi atoi
	#define OutputDebugString OutputDebugStringA
	#define MessageBox MessageBoxA
	#define TXT(_text) _text
	#define STRING_CONTENT(_text) #_text
	#define STRINGIZE_(_text) STRING_CONTENT(_text)
	#define STRINGIZE(_text) STRINGIZE_(_text)
#endif

// join two strings together, use with STRINGIZE, ie STRINGIZE(PPCAT(bat, tery)) gives "battery"
#define PPCAT_JOIN_AGAIN(A, B) A ## B
#define PPCAT_JOIN(A, B) PPCAT_JOIN_AGAIN(A, B)
#define PPCAT(A, B) PPCAT_JOIN(A, B)

typedef int64_t int64;
typedef int32_t int32;
typedef int16_t int16;
typedef char int8;

typedef uint64_t uint64;
typedef uint32_t uint32;
typedef uint16_t uint16;
typedef unsigned char uint8;

typedef unsigned char byte;

typedef unsigned int uint;

#ifdef AN_CLANG
typedef long int pointerint;
#else
#ifdef AN_64
typedef long long pointerint;
#else
typedef int pointerint;
#endif
#endif

#define bit(_bit) (int)(1<<(_bit))
#define is_flag_set(_where, _flag) (((_where) & (_flag)) != 0)
#define is_whole_flag_set(_where, _flag) (((_flag) & (~((_where) & (_flag)))) == 0)
#define is_any_flag_set(_where) ((_where) != 0)
#define without_flag(_variable, _flag) (_variable & (~(_flag)))
#define clear_flag(_variable, _flag) _variable = without_flag(_variable, _flag)
#define set_flag(_variable, _flag) _variable |= (_flag)

#define is_bit_set(_where, _bit) is_flag_set((_where), (bit(_bit)))
#define without_bit(_variable, _bit) without_flag((_where), (bit(_bit)))
#define clear_bit(_where, _bit) clear_flag((_where), (bit(_bit)))
#define set_bit(_where, _bit) set_flag((_where), (bit(_bit)))

#define magic_number
#define hardcoded

#define interface_struct struct
#define interface_class class

#ifndef override_
	#define override_ virtual
#endif
#define implement_ virtual

#define NONE -1

#define INF_INT 2147483646
#define INF_FLOAT 100000000000000000.0f
// not infinite but will have to be enough

#define EPSILON 0.00001f
#define MARGIN 0.001f
#define SMALL_MARGIN 0.000001f

#define SMALL_STEP 0.001f
#define ACCURACY 0.001f

#define MAX_THREAD_COUNT 32

// defines that will help to define usage

#define OUT_
#define REF_
#define CACHED_
#define WORKER_
#define RUNTIME_
#define AUTO_
#define OPTIONAL_
#define PLAIN_ /* plain data */
#define GS_ /* game saved */
#define TODO_
#define OWNER_ /* owner of object when it comes to main memory management */
#define FROM_DATA_
#define GENERATED_ /* generated during runtime */

#define REMOVE_AS_SOON_AS_POSSIBLE_
#define REMOVE_OR_CHANGE_BEFORE_COMMIT_
#define COMMENT_IF_NO_LONGER_REQUIRED_

// put AVOID_CALLING_ if a function/method should not be used (check notes there), put ACK if it is used knowing the limitations/other use
#define AVOID_CALLING_
#define AVOID_CALLING_ACK_

#define SIZE_
#define LOCATION_
#define POINT_
#define NORMAL_
#define SCALE_

#define RAD_
#define DEG_

#define A_BEFORE_B -1
#define B_BEFORE_A  1
#define A_AS_B      0

#define LIKELY
#define UNLIKELY

#ifdef AN_CLANG
	#define AN_CLANG_TYPENAME typename
	#define AN_NOT_CLANG_TYPENAME
	#define AN_CLANG_BASE base::
	#define AN_NOT_CLANG_INLINE
	#define AN_CLANG_TEMPLATE_SPECIALIZATION template <>
	#define AN_CLANG_TEMPLATE template
	#define AN_CLANG_NO_EXCEPT _NOEXCEPT
#else
	#define AN_CLANG_TYPENAME 
	#define AN_NOT_CLANG_TYPENAME typename
	#define AN_CLANG_BASE
	#define AN_NOT_CLANG_INLINE inline
	#define AN_CLANG_TEMPLATE_SPECIALIZATION
	#define AN_CLANG_TEMPLATE
	#define AN_CLANG_NO_EXCEPT
#endif

#ifdef AN_DEVELOPMENT_OR_PROFILER
	#define AN_DEVELOPMENT_OR_PROFILER_MUTABLE mutable
#else
	#define AN_DEVELOPMENT_OR_PROFILER_MUTABLE
#endif

enum ShouldAllowToFail
{
	DisallowToFail,
	AllowToFail,
};

#include "debugSettings.h"

// to make it easier to find pure gl calls
#ifdef AN_SYSTEM_INFO_DIRECT_GL_CALLS
	void store_draw_info(tchar const* _info, ...);
	#define DRAW_INFO(...) store_draw_info(__VA_ARGS__);
	void store_draw_object_name(tchar const * _whatsBeingDrawn);
	#define DRAW_OBJECT_NAME(_NAME) store_draw_object_name(_NAME);
	#define NO_DRAW_OBJECT_NAME() store_draw_object_name(nullptr);
	void store_direct_gl_call();
	#define DIRECT_GL_CALL_ store_direct_gl_call();
	void store_draw_triangles(int _count);
	// call post draw_call
	#define DRAW_TRIANGLES(_COUNT) store_draw_triangles(_COUNT);
	void store_draw_call();
	#define DRAW_CALL_ store_draw_call();
	void store_draw_call_render_target(bool _otherDraw);
	#define DRAW_CALL_RENDER_TARGET_ store_draw_call_render_target(false);
	#define DRAW_CALL_RENDER_TARGET_DRAW_ store_draw_call_render_target(true);
	#define DRAW_CALL_RENDER_TARGET_RELATED_
	void suspend_store_direct_gl_call(bool);
	#define SUSPEND_STORE_DRAW_CALL() suspend_store_draw_call(true);
	#define RESTORE_STORE_DRAW_CALL() suspend_store_draw_call(false);
	void suspend_store_draw_call(bool);
	#define DIRECT_GL_CALL_ store_direct_gl_call();
	#define SUSPEND_STORE_DIRECT_GL_CALL() suspend_store_direct_gl_call(true);
	#define RESTORE_STORE_DIRECT_GL_CALL() suspend_store_direct_gl_call(false);
#else
	#define DRAW_INFO(...)
	#define DRAW_OBJECT_NAME(_NAME)
	#define NO_DRAW_OBJECT_NAME()
	#define DIRECT_GL_CALL_ 
	#define DRAW_TRIANGLES(_COUNT)
	#define DRAW_CALL_ 
	#define DRAW_CALL_RENDER_TARGET_
	#define DRAW_CALL_RENDER_TARGET_DRAW_
	#define DRAW_CALL_RENDER_TARGET_RELATED_
	#define SUSPEND_STORE_DRAW_CALL()
	#define RESTORE_STORE_DRAW_CALL()
	#define SUSPEND_STORE_DIRECT_GL_CALL() 
	#define RESTORE_STORE_DIRECT_GL_CALL() 
#endif

//

//

#include "globalInclude.h"
