#pragma once

#include "..\globalDefinitions.h"
#include "..\globalSettings.h"
#include "..\globalInclude.h"

#include "..\math\math.h"
#include "..\types\name.h"
#include "..\types\colour.h"
#include "..\types\optional.h"
#include "..\memory\pooledObject.h"
#include "performanceTimer.h"

struct Range;

namespace Performance
{
	typedef int BlockID;

	namespace BlockType
	{
		enum Type
		{
			Normal,
			Lock
		};
	};

	struct BlockTag
	{
		static int const MAX_LENGTH = 63;
		static int get_max_length() { return MAX_LENGTH; }
		Name name;
		tchar const* nameAsTCharHardCopy = nullptr;
		tchar contextInfo[MAX_LENGTH + 1];
		BlockType::Type type;
		Colour colour;

		BlockTag(Name const & _name, BlockType::Type _type, Colour const & _colour)
		: name(_name)
		, type(_type)
		, colour(_colour)
		{
			contextInfo[0] = 0;
		}

		BlockTag(tchar const * _info, BlockType::Type _type, Colour const & _colour)
		: name(Name::invalid())
		, type(_type)
		, colour(_colour)
		{
			int length = min(get_max_length(), (int)tstrlen(_info));
			memory_copy(contextInfo, _info, sizeof(tchar) * length);
			contextInfo[length] = 0;
		}

		BlockTag(String const & _contextInfo, BlockType::Type _type, Colour const & _colour)
		: name(Name::invalid())
		, type(_type)
		, colour(_colour)
		{
			int length = min(get_max_length(), _contextInfo.get_length());
			memory_copy(contextInfo, _contextInfo.get_data().get_data(), sizeof(tchar) * length);
			contextInfo[length] = 0;
		}
	};

	class Block
	{
	public:
		Block(BlockTag const & _tag);
		~Block();

		void turn_into_copy(); // increase id so it is treated as copy

		void update(Timer const & _timer) { timer = _timer; }

		bool has_finished() const { return timer.has_finished(); }
		Range get_relative_range(Timer const & _timer) const { return timer.get_relative_range(_timer); }
		float get_time_ms() const { return timer.get_time_ms(); }

		void prepare_hard_copy() { tag.nameAsTCharHardCopy = tag.name.is_valid() ? tag.name.to_char() : nullptr; }

		BlockTag const & get_tag() const { return tag; }
		BlockID get_id() const { return id; }

		Optional<int> & access_level() { return level; }

	private:
		static BlockID s_id;
		static Concurrency::SpinLock s_idLock;

		Optional<int> level; // to speed up rendering

		BlockTag tag;
		BlockID id; // it will reside in memory due to being in a huge array, that's why id is cleared when it is unused

		Timer timer;

		static int get_next_id();
	};

};