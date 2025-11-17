#pragma once

#include "bh_enums.h"

namespace an_bhaptics
{
	struct AvailableDevices
	{
		void clear() { availableDevices = 0; }
		void set(PositionType::Type _type, bool _setTo = true);
		void clear(PositionType::Type _type) { set(_type, false); }
		bool has(PositionType::Type _type) const;

		String to_string() const;

	private:
		int availableDevices = 0;

	};
};
