#include "splashInfo.h"

using namespace Splash;

Info* Info::s_info = nullptr;

void Info::initialise_static()
{
	if (s_info)
	{
		return;
	}

	s_info = new Info();
}

void Info::close_static()
{
	delete_and_clear(s_info);
}

void Info::add_info(String const & _string, int r, int g, int b, int i)
{
	if (!s_info)
	{
		return;
	}
	
	s_info->infos.push_back(Element(_string, r, g, b, i));
}

void Info::send_to_output()
{
	if (!s_info)
	{
		return;
	}

	for_every(info, s_info->infos)
	{
		output_colour(info->r, info->g, info->b, info->i);
		output(info->info.to_char());
	}
	output_colour();
}

List<String> Info::get_all_info()
{
	List<String> result;
	if (s_info)
	{
		for_every(info, s_info->infos)
		{
			result.push_back(info->info);
		}
	}
	return result;
}
