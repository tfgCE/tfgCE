#pragma once

#include "bh_enums.h"

#include "..\sensations.h"

#include "..\..\containers\map.h"

#include "..\..\types\name.h"
#include "..\..\types\hand.h"
#include "..\..\types\optional.h"
#include "..\..\types\string.h"

namespace bhapticsJava
{
	class BhapticsModule;
};

namespace bhapticsWindows
{
	class BhapticsModule;
};

namespace an_bhaptics
{
	struct AvailableDevices;

	struct SensationToTrigger
	{
		Name sensationId;
		PositionType::Type position = PositionType::Vest;

		SensationToTrigger() {}
		SensationToTrigger(Name const& _sensationId, PositionType::Type _position) : sensationId(_sensationId), position(_position) {}
	};

	class Library
	{
	public:
		static void initialise_static();
		static void close_static();

		static void register_for(bhapticsJava::BhapticsModule*);
		static void register_for(bhapticsWindows::BhapticsModule*);
		static void set(Name const& _id, String const& _sensationJson);
		static String get(Name const& _id);

		static bool load_from_xml(IO::XML::Node const* _node);

		static void get_sensation_ids_for(PhysicalSensations::SingleSensation const& _sensation, AvailableDevices const& _availableDevices, OUT_ ArrayStatic<SensationToTrigger, 4> & _sensations);
		static void get_sensation_ids_for(PhysicalSensations::OngoingSensation const& _sensation, AvailableDevices const& _availableDevices, OUT_ ArrayStatic<SensationToTrigger, 4>& _sensations);
		static an_bhaptics::PositionType::Type get_position_type_for(PhysicalSensations::Sensation const& _sensation, AvailableDevices const& _availableDevices);
		static void get_offsets_for_vest(PhysicalSensations::Sensation const& _sensation, Name const& _sensationId, OUT_ float & _offsetYaw, OUT_ float & _offsetZ);
		static void update_sensation_id_or_position(REF_ Name& _sensationId, REF_ PositionType::Type& _position);

	private:
		static Library* s_library;

		Concurrency::SpinLock sensationsLock = Concurrency::SpinLock(TXT("an_bhaptics.Library.sensationsLock"));

		struct Sensation
		{
			Name id;
			String sensationJson;
		};
		Array<Sensation> sensations;
	};
}