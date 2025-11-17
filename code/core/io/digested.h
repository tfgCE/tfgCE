#pragma once

#include "..\globalDefinitions.h"

class Serialiser;

struct String;

namespace IO
{
	namespace XML
	{
		class Node;
	};
	class Interface;

	struct Digested
	{
	public:
		Digested();
		Digested(Interface* _toDigest);

		void digest(IO::XML::Node const * _node);
		void digest(String const & _toDigest);
		void digest(Interface* _toDigest);

		String to_string() const;

		bool is_valid() const;
		bool operator==(Digested const & _digested) const;

		bool serialise(Serialiser & _serialiser);

		void add(Digested const& _digested);

	private:
		static int const c_length = 16;
		static int const c_intLength = 4;
		union
		{
			uint8 value[c_length];
			uint32 intValue[c_intLength];
		};
	};
};
