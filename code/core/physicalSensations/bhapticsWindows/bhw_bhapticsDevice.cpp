#include "bhw_bhapticsDevice.h"

#include "..\..\io\json.h"

//

using namespace bhapticsWindows;

//

BhapticsDevice::BhapticsDevice(an_bhaptics::PositionType::Type _position)
: position(_position)
{
	deviceName = an_bhaptics::PositionType::to_char(_position);
}

void BhapticsDevice::parse(String const& _devicesJSON, OUT_ Array<BhapticsDevice>& _devices)
{
#ifdef LOG_BHAPTICS
	output(TXT("[bhaptics] parse devices: [string] %S"), _devicesJSON.to_char());
#endif
	RefCountObjectPtr<IO::JSON::Document> json;
	json = new IO::JSON::Document();
	json->parse(_devicesJSON);
	parse(json.get(), OUT_ _devices);
}

void BhapticsDevice::parse(IO::JSON::Node const* _json, OUT_ Array<BhapticsDevice>& _devices)
{
#ifdef LOG_BHAPTICS
	output(TXT("[bhaptics] parse devices: [json] %S"), _json->to_string().to_char());
#endif
	_devices.clear();
	for_every(node, _json->get_elements())
	{
		BhapticsDevice device;
		device.deviceName = node->get(TXT("DeviceName"));
		device.position = an_bhaptics::PositionType::parse(node->get(TXT("Position")));
#ifdef LOG_BHAPTICS
		output(TXT("[bhaptics]  device %i"), _devices.get_size());
		output(TXT("[bhaptics]    name     : %S"), device.deviceName.to_char());
		output(TXT("[bhaptics]    position : %S"), an_bhaptics::PositionType::to_char(device.position));
#endif
		_devices.push_back(device);
	}
}
