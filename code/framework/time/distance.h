#pragma once

#include "..\..\core\fastCast.h"
#include "..\..\core\types\string.h"
#include "..\..\core\math\math.h"
#include "..\text\stringFormatter.h"

namespace IO
{
	namespace XML
	{
		class Node;
	};
};

namespace Framework
{
	struct LibraryLoadingContext;

	struct Distance
	{
	public:
		static Distance const & zero() { return s_zero; }
		static Distance parse_from(String const & _string, Distance const & _defValue) { Distance result = _defValue; result.parse_from(_string); return result; }

		Distance() {}
		explicit Distance(String const & _text) { parse_from(_text); }
		explicit Distance(int _meters) : meters(_meters) {}
		static Distance from_meters(int _distance);
		static Distance from_meters(float _distance);

		// use MeasurementSystem to provide value with as_meters
		String get_for_parse() const;

		bool parse_from(String const & _text);

		bool is_ok() const { return meters >= 0 && subMeters >= 0.0f; }
		bool is_zero() const { return meters == 0 && subMeters == 0.0f; }

		float as_meters() const { return (float)meters + subMeters; }
		int get_full_meters() const { return meters; }
		float get_sub_meters() const { return subMeters; }

		void add_meters(float _meters);
		void add_meters(int _meters);

		bool load_from_xml(IO::XML::Node const* _node, Framework::LibraryLoadingContext& _lc);
		bool load_from_xml(IO::XML::Node const * _node);
		bool load_from_xml(IO::XML::Node const* _node, tchar const* _attrName);
		bool load_from_xml(IO::XML::Attribute const* _attr);

	private:
		static const Distance s_zero;

		int meters = 0;
		float subMeters = 0.0f;
	};

};
