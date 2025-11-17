#include "bhj_bhapticsDevice.h"

#include "..\..\io\json.h"

//

using namespace bhapticsJava;

//

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
		device.address = node->get(TXT("Address"));
		device.battery = node->get_as_int(TXT("Battery"));
		device.position = an_bhaptics::PositionType::parse(node->get(TXT("Position")));
		device.isConnected = node->get_as_bool(TXT("IsConnected"));
		device.isPaired = node->get_as_bool(TXT("IsPaired"));
#ifdef LOG_BHAPTICS
		output(TXT("[bhaptics]  device %i %S"), _devices.get_size(), device.isPaired? TXT("PAIRED") : TXT(""));
		output(TXT("[bhaptics]    name      : %S"), device.deviceName.to_char());
		output(TXT("[bhaptics]    address   : %S"), device.address.to_char());
		output(TXT("[bhaptics]    battery   : %i"), device.battery);
		output(TXT("[bhaptics]    position  : %S"), an_bhaptics::PositionType::to_char(device.position));
		output(TXT("[bhaptics]    connected : %S"), device.isConnected? TXT("CONNECTED") : TXT("not connected"));
		output(TXT("[bhaptics]    paired    : %S"), device.isPaired? TXT("PAIRED") : TXT("not paired"));
#endif
		_devices.push_back(device);
	}
}
