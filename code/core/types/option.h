#pragma once

#include "..\globalDefinitions.h"

/* Generic option to be used almost anywhere */
namespace Option
{
	enum Type
	{
		Off			= bit(0),
		On			= bit(1),
		Allow		= bit(2),
		Disallow	= bit(3),
		Block		= bit(4),	
		Available	= bit(5),
		True		= bit(6),
		False		= bit(7),
		Auto		= bit(8),
		Ask			= bit(9),
		Unknown		= bit(10),
		Stop		= bit(11),
		Run			= bit(12),
	};

	Type parse(tchar const * _value, Type _default = Off, int _allowedFlags = 0xffffffff, OPTIONAL_ OUT_ bool * result = nullptr);
	tchar const * to_char(Type _option);
};
