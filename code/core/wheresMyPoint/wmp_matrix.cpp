#include "wmp_matrix.h"

#include "iWMPOwner.h"
#include "wmp_context.h"

#include "..\io\xml.h"
#include "..\math\math.h"
#include "..\other\parserUtils.h"

//

#ifdef AN_ALLOW_OPTIMIZE_OFF
#pragma optimize("", off)
#endif

//

using namespace WheresMyPoint;

//

bool WheresMyPoint::RawMatrix44::load_from_xml(IO::XML::Node const * _node)
{
	bool result = base::load_from_xml(_node);

	bool transpose = _node->get_bool_attribute_or_from_child_presence(TXT("transpose"));

	String order; // XYZ means x+y+z+ Xzy means x+z-y- this is in which order we read
	order = _node->get_string_attribute(TXT("order"));

	::Matrix44 m44 = ::Matrix44::zero;

	String rawData = _node->get_text();
	rawData = rawData.trim().compress();
	List<String> rawElements;
	rawData.split(String::space(), rawElements);
	if (rawElements.get_size() == 4 * 4)
	{
		for_count(int, y, 4)
		{
			for_count(int, x, 4)
			{
				int idx = y * 4 + x;
				float value = 0.0f;
				if (rawElements[idx].does_contain(TXT("e-")))
				{
					// just assume it is very close to 0
					value = 0.0f;
				}
				else
				{
					value = ParserUtils::parse_float(rawElements[idx], 0.0f);
				}
				m44.m[y][x] = value;
			}
		}

		if (transpose)
		{
			m44 = m44.transposed();
		}

		if (!order.is_empty())
		{
			if (order.get_length() == 3)
			{
				::Matrix44 om44 = m44;
				for_count(int, row, 3)
				{
					if (order[row] == 'X' ||
						order[row] == 'x')
					{
						float s = order[row] == 'X' ? 1.0f : -1.0f;
						for_count(int, column, 4)
						{
							m44.m[0][column] = om44.m[row][column] * s;
						}
					}
					if (order[row] == 'Y' ||
						order[row] == 'y')
					{
						float s = order[row] == 'Y' ? 1.0f : -1.0f;
						for_count(int, column, 4)
						{
							m44.m[1][column] = om44.m[row][column] * s;
						}
					}
					if (order[row] == 'Z' ||
						order[row] == 'z')
					{
						float s = order[row] == 'Z' ? 1.0f : -1.0f;
						for_count(int, column, 4)
						{
							m44.m[2][column] = om44.m[row][column] * s;
						}
					}
				}
				om44 = m44;
				for_count(int, column, 3)
				{
					if (order[column] == 'X' ||
						order[column] == 'x')
					{
						float s = order[column] == 'X' ? 1.0f : -1.0f;
						for_count(int, row, 4)
						{
							m44.m[row][0] = om44.m[row][column] * s;
						}
					}
					if (order[column] == 'Y' ||
						order[column] == 'y')
					{
						float s = order[column] == 'Y' ? 1.0f : -1.0f;
						for_count(int, row, 4)
						{
							m44.m[row][1] = om44.m[row][column] * s;
						}
					}
					if (order[column] == 'Z' ||
						order[column] == 'z')
					{
						float s = order[column] == 'Z' ? 1.0f : -1.0f;
						for_count(int, row, 4)
						{
							m44.m[row][2] = om44.m[row][column] * s;
						}
					}
				}
			}
			else
			{
				error_loading_xml(_node, TXT("invalid order, expected of length 3"));
				result = false;
			}
		}

		matrix = m44;
	}
	else
	{
		error_loading_xml(_node, TXT("expected 16 (4x4) elements, got %i"), rawElements.get_size());
		result = false;
	}

	return result;
}

bool WheresMyPoint::RawMatrix44::update(REF_ Value & _value, Context & _context) const
{
	bool result = true;

	_value.set(matrix);

	return result;
}
