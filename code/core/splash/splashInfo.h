#pragma once

#include "..\types\string.h"
#include "..\containers\list.h"

namespace Splash
{
	struct Info
	{
	public:
		static void initialise_static();
		static void close_static();

		static void add_info(String const & _string, int r = 1, int g = 1, int b = 1, int i = 0);

		static void send_to_output(); // simple sending to output

		static List<String> get_all_info();

	private:
		static Info* s_info;

		struct Element
		{
			String info;
			int r, g, b, i;

			Element(String const & _info, int _r, int _g, int _b, int _i) : info(_info), r(_r), g(_g), b(_b), i(_i) {}
		};
		List<Element> infos;
	};
};
