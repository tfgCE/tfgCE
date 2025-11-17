#pragma once

#include "video3d.h"

class Serialiser;

namespace System
{
	namespace IndexElementSize
	{
		enum Type
		{
			None = 0,
			OneByte = GL_UNSIGNED_BYTE,
			TwoBytes = GL_UNSIGNED_SHORT,
			FourBytes = GL_UNSIGNED_INT,
		};

		inline DataFormatValueType::Type to_data_format_value_type(IndexElementSize::Type _type)
		{
			if (_type == OneByte) { return DataFormatValueType::Uint8; } else
			if (_type == TwoBytes) { return DataFormatValueType::Uint16; } else
			if (_type == FourBytes) { return DataFormatValueType::Uint32; }
			an_assert(false);
			return DataFormatValueType::Uint8;
		}

		inline int to_memory_size(IndexElementSize::Type _type)
		{
			if (_type == OneByte) { return 1; } else
			if (_type == TwoBytes) { return 2; } else
			if (_type == FourBytes) { return 4; }
			an_assert(false);
			return 1;
		}

		inline Type parse(String const & _value)
		{
			if (_value == TXT("oneByte") || _value == TXT("1") || _value == TXT("uint8")) return OneByte;
			if (_value == TXT("twoBytes") || _value == TXT("2") || _value == TXT("uint16")) return TwoBytes;
			if (_value == TXT("fourBytes") || _value == TXT("4") || _value == TXT("uint32")) return FourBytes;
			if (_value == TXT("none")) return None;
			error(TXT("value \"%S\" not recognised"), _value.to_char());
			return None;
		}
	};

	/**
	 *	Describes index (element) buffer (size of single index data)
	 */
	struct IndexFormat
	{
	public:
		IndexFormat()
		: okToUse(false)
		, elementSize(IndexElementSize::None)
		{}

		inline IndexFormat & with_element_size(IndexElementSize::Type _elementSize) { elementSize = _elementSize; okToUse = false; return *this; }
		inline IndexFormat & not_used() { elementSize = IndexElementSize::None; okToUse = false; return *this; }

		inline bool is_ok_to_be_used() const { return okToUse; }

		void calculate_stride();

		IndexElementSize::Type get_element_size() const { return elementSize; }

		uint32 get_stride() const { return stride; }

		bool serialise(Serialiser & _serialiser);

		bool load_from_xml(IO::XML::Node const * _node); // can adjust existing format, will calculate stride

		bool operator==(IndexFormat const & _other) const;

	private:
		bool okToUse;

		IndexElementSize::Type elementSize;

		uint32 stride; // in bytes
	};
	inline IndexFormat index_format() { return IndexFormat(); }

};

bool serialise(Serialiser& _serialiser, ::System::IndexElementSize::Type & _data);
