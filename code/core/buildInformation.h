#pragma once

#include "globalDefinitions.h"

struct String;

tchar const * get_build_information();
tchar const * get_build_version();
tchar const * get_build_version_short();
tchar const * get_build_date();
String get_build_version_short_custom(int _maj, int _min, int _subMin, int _buildNo);
tchar const * get_platform();
int get_build_version_major();
int get_build_version_minor();
int get_build_version_sub_minor();
int get_build_version_build_no();
