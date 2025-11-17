#include "bhj_bhapticsModule.h"

#include "..\bhaptics\bh_library.h"

#include "..\..\mainConfig.h"
#include "..\..\concurrency\scopedSpinLock.h"
#include "..\..\io\json.h"
#include "..\..\other\parserUtils.h"

#ifdef AN_ANDROID
#include "..\..\system\javaEnv.h"
#endif

//

#ifdef AN_ALLOW_OPTIMIZE_OFF
#pragma optimize("", off)
#endif

#ifdef AN_ALLOW_EXTENSIVE_LOGS
	#ifdef LOG_BHAPTICS
		#define VERBOSE_BHAPTICS
	#endif
#endif
//

using namespace bhapticsJava;

//

BhapticsModule* BhapticsModule::s_module = nullptr;

BhapticsModule::BhapticsModule()
{
	SET_EXTRA_DEBUG_INFO(activeSensations, TXT("BHJ::BhapticsModule.activeSensations"));

	an_assert(!s_module);
	s_module = this;

#ifdef AN_ANDROID
	JNIEnv* env = System::JavaEnv::get_env();
	class_activity = System::JavaEnv::find_class(env, "com/voidroom/TeaForGodActivity");
#ifdef VERBOSE_BHAPTICS
	output(TXT("[bhaptics] class_activity %p"), class_activity);
#endif
	System::JavaEnv::make_global_ref(env, class_activity);
	class_bhapticsModule = System::JavaEnv::find_class(env, "com/voidroom/BhapticsModule");
#ifdef VERBOSE_BHAPTICS
	output(TXT("[bhaptics] class_bhapticsModule %p"), class_bhapticsModule);
#endif
	System::JavaEnv::make_global_ref(env, class_bhapticsModule);

	if (class_bhapticsModule)
	{
		an_assert(System::Android::App::get().get_activity());

		jobject object_activity = System::Android::App::get().get_activity()->clazz;

		jmethodID method_getBhapticsModule = env->GetMethodID(class_activity, "getBhapticsModule", "()Lcom/voidroom/BhapticsModule;");
#ifdef VERBOSE_BHAPTICS
		output(TXT("[bhaptics] method_getBhapticsModule %p"), method_getBhapticsModule);
#endif
		object_bhapticsModule = env->CallObjectMethod(object_activity, method_getBhapticsModule); // this will initialise bhaptics module
#ifdef VERBOSE_BHAPTICS
		output(TXT("[bhaptics] called method_getBhapticsModule %p"), method_getBhapticsModule);
		output(TXT("[bhaptics] object_bhapticsModule %p"), object_bhapticsModule);
#endif
		System::JavaEnv::make_global_ref(env, object_bhapticsModule);

		method_getDeviceList = env->GetMethodID(class_bhapticsModule, "getDeviceList", "()Ljava/lang/String;");
#ifdef VERBOSE_BHAPTICS
		output(TXT("[bhaptics] method_getDeviceList %p"), method_getDeviceList);
#endif
		method_refreshPairing = env->GetMethodID(class_bhapticsModule, "refreshPairing", "()V");
#ifdef VERBOSE_BHAPTICS
		output(TXT("[bhaptics] method_refreshPairing %p"), method_refreshPairing);
#endif
		method_registerSensation = env->GetMethodID(class_bhapticsModule, "registerSensation", "(Ljava/lang/String;Ljava/lang/String;)V");
#ifdef VERBOSE_BHAPTICS
		output(TXT("[bhaptics] method_registerSensation %p"), method_registerSensation);
#endif
		method_submitRegistered = env->GetMethodID(class_bhapticsModule, "submitRegistered", "(Ljava/lang/String;Ljava/lang/String;FFFF)V");
#ifdef VERBOSE_BHAPTICS
		output(TXT("[bhaptics] method_submitRegistered %p"), method_submitRegistered);
#endif
		method_isPlaying = env->GetMethodID(class_bhapticsModule, "isPlaying", "(Ljava/lang/String;)Z");
#ifdef VERBOSE_BHAPTICS
		output(TXT("[bhaptics] method_isPlaying %p"), method_isPlaying);
#endif
		method_togglePosition = env->GetMethodID(class_bhapticsModule, "togglePosition", "(Ljava/lang/String;)V");
#ifdef VERBOSE_BHAPTICS
		output(TXT("[bhaptics] method_togglePosition %p"), method_togglePosition);
#endif
		method_ping = env->GetMethodID(class_bhapticsModule, "ping", "(Ljava/lang/String;)V");
#ifdef VERBOSE_BHAPTICS
		output(TXT("[bhaptics] method_ping %p"), method_ping);
#endif
		method_pingAll = env->GetMethodID(class_bhapticsModule, "pingAll", "()V");
#ifdef VERBOSE_BHAPTICS
		output(TXT("[bhaptics] method_pingAll %p"), method_pingAll);
#endif
	}
#endif

#ifdef VERBOSE_BHAPTICS
	output(TXT("[bhaptics] init, register"));
#endif

	// register sensations through the library
	an_bhaptics::Library::register_for(this);

#ifdef VERBOSE_BHAPTICS
	output(TXT("[bhaptics] registered, get devices"));
#endif

	// read the current setup
	read_setup_from_main_config();

	// get devices as they are
	{
		Concurrency::ScopedSpinLock lock(access_devices_lock(), true);
		get_devices(true);
	}
}

BhapticsModule::~BhapticsModule()
{
#ifdef AN_ANDROID
	JNIEnv* env = System::JavaEnv::get_env();
	env->DeleteGlobalRef(object_bhapticsModule);
	env->DeleteGlobalRef(class_bhapticsModule);
	env->DeleteGlobalRef(class_activity);
#endif

	an_assert(s_module == this);
	s_module = nullptr;
}

void BhapticsModule::toggle_position(String const& _address, bool _justToggle)
{
#ifdef LOG_BHAPTICS
	output(TXT("[bhaptics] toggle position %S"), _address.to_char());
#endif
#ifdef AN_ANDROID
	auto* env = System::JavaEnv::get_env();
	jstring javaDeviceAddress = env->NewStringUTF(_address.to_char8_utf_array().get_data());
	env->CallVoidMethod(object_bhapticsModule, method_togglePosition, javaDeviceAddress);
	env->DeleteLocalRef(javaDeviceAddress);
#endif
	if (!_justToggle)
	{
		store_setup();
	}
}

void BhapticsModule::ping(String const& _address)
{
#ifdef LOG_BHAPTICS
	output(TXT("[bhaptics] unpair all %S"), _address.to_char());
#endif
#ifdef AN_ANDROID
	auto* env = System::JavaEnv::get_env();
	jstring javaDeviceAddress = env->NewStringUTF(_address.to_char8_utf_array().get_data());
	env->CallVoidMethod(object_bhapticsModule, method_ping, javaDeviceAddress);
	env->DeleteLocalRef(javaDeviceAddress);
#endif
}

void BhapticsModule::ping_all()
{
#ifdef LOG_BHAPTICS
	output(TXT("[bhaptics] ping all"));
#endif
#ifdef AN_ANDROID
	auto* env = System::JavaEnv::get_env();
	env->CallVoidMethod(object_bhapticsModule, method_pingAll);
#endif
}

void BhapticsModule::register_sensation(Name const& _id, String const& _sensationJson)
{
	{
#ifdef LOG_BHAPTICS
		output(TXT("[bhaptics] register sensation %S"), _id.to_char());
#endif
#ifdef AN_ANDROID
		auto* env = System::JavaEnv::get_env();
		jstring javaId = env->NewStringUTF(_id.to_string().to_char8_utf_array().get_data());
		jstring javaSensationJson = env->NewStringUTF(_sensationJson.to_char8_utf_array().get_data());
		env->CallVoidMethod(object_bhapticsModule, method_registerSensation, javaId, javaSensationJson);
		env->DeleteLocalRef(javaSensationJson);
		env->DeleteLocalRef(javaId);
#endif
#ifdef LOG_BHAPTICS
		output(TXT("[bhaptics] done"));
#endif
	}
	{
		Concurrency::ScopedSpinLock lock(registeredSensationsLock, true);
		registeredSensations.push_back(_id);
	}
}

void BhapticsModule::submit_sensation(Name const& _id, float _offsetYaw, float _offsetZ, Optional<float> const & _scaleIntensity, Optional<float> const& _scaleDuration)
{
	Concurrency::ScopedSpinLock lock(callLock, true);
#ifdef LOG_BHAPTICS
	output(TXT("[bhaptics] submit registered %S  %.1f':z%.3f    Ix%.1f  Dx%.1f"), _id.to_char(), _offsetYaw, _offsetZ, _scaleIntensity.get(1.0f), _scaleDuration.get(1.0f));
#endif
#ifdef AN_ANDROID
	auto* env = System::JavaEnv::get_env();
	jstring javaId = env->NewStringUTF(_id.to_string().to_char8_utf_array().get_data());
	jstring javaAltId = env->NewStringUTF(_id.to_string().to_char8_utf_array().get_data());
	env->CallVoidMethod(object_bhapticsModule, method_submitRegistered, javaId, javaAltId, _scaleIntensity.get(1.0f), _scaleDuration.get(1.0f), _offsetYaw, _offsetZ);
	env->DeleteLocalRef(javaId);
	env->DeleteLocalRef(javaAltId);
#endif
#ifdef LOG_BHAPTICS
	output(TXT("[bhaptics] done"));
#endif
}

bool BhapticsModule::is_sensation_active(Name const& _id) const
{
	bool result = false;
	Concurrency::ScopedSpinLock lock(callLock, true);
#ifdef AN_ANDROID
	auto* env = System::JavaEnv::get_env();
	jstring javaId = env->NewStringUTF(_id.to_string().to_char8_utf_array().get_data());
	result = env->CallBooleanMethod(object_bhapticsModule, method_isPlaying, javaId);
	env->DeleteLocalRef(javaId);
#endif
	return result;
}

void BhapticsModule::get_devices(OUT_ Array<BhapticsDevice>& _devices)
{
	// no lock here
#ifdef LOG_BHAPTICS
	output(TXT("[bhaptics] get devices"));
#endif
#ifdef AN_ANDROID
	auto* env = System::JavaEnv::get_env();
	jstring javaDevicesString = (jstring)env->CallObjectMethod(object_bhapticsModule, method_getDeviceList);
	const char* nativeDevicesString = env->GetStringUTFChars(javaDevicesString, 0);
	String devicesListString = String::from_char8_utf(nativeDevicesString);
	env->ReleaseStringUTFChars(javaDevicesString, nativeDevicesString);
#else
	String devicesListString;
#ifdef AN_DEVELOPMENT
	devicesListString = TXT("[{\"DeviceName\":\"Tactosy2_V0.6\", \"Address\" : \"F9:5A:BE:9D:A3:83\", \"Battery\" : -1, \"Position\" : \"ForearmL\", \"IsConnected\" : false, \"IsPaired\" : true}, { \"DeviceName\":\"Tactot3_V1.1\",\"Address\" : \"DE:31:8B:1D:C3:22\",\"Battery\" : 100,\"Position\" : \"Vest\",\"IsConnected\" : true,\"IsPaired\" : true }]");
#endif
#endif
#ifdef LOG_BHAPTICS
	output(TXT("[bhaptics] done"));
#endif
	BhapticsDevice::parse(devicesListString, OUT_ _devices);
}

Array<BhapticsDevice> const& BhapticsModule::get_devices(bool _update)
{
	an_assert(devicesLock.is_locked_on_this_thread());

	if (_update)
	{
		get_devices(devices);
		update_setup_to_devices(devices);
		store_setup(devices); // to add any changes
		update_available_devices();
		Concurrency::ScopedSpinLock lock(pendingDevicesLock, true);
		pendingDevices.clear();
	}

	return devices;
}

void BhapticsModule::store_setup()
{
#ifdef VERBOSE_BHAPTICS
	output(TXT("[bhaptics] store_setup"));
#endif

	{
		// get the most recent list
		an_assert(devicesLock.is_locked_on_this_thread());
		get_devices(devices);
	}
	store_setup(devices);

#ifdef VERBOSE_BHAPTICS
	output(TXT("[bhaptics] store_setup [done]"));
#endif
}

void BhapticsModule::store_setup(REF_ Array<BhapticsDevice>& _devices)
{
#ifdef VERBOSE_BHAPTICS
	output(TXT("[bhaptics] store_setup (+dev list)"));
#endif

	Concurrency::ScopedSpinLock lock(devicesSetupLock, true);
	
	while (_devices.get_size() > 20)
	{
		_devices.pop_front();
	}

	for_every(device, _devices)
	{
#ifdef VERBOSE_BHAPTICS
		output(TXT("[bhaptics]   \"%S\" = \"%S\""), device->address.to_char(), an_bhaptics::PositionType::to_char(device->position));
#endif
		devicesSetup.store(device->address, device->position);
	}

#ifdef VERBOSE_BHAPTICS
	output(TXT("[bhaptics] store_setup (+dev list) [done]"));
#endif
}

void BhapticsModule::update_setup_to_devices()
{
#ifdef VERBOSE_BHAPTICS
	output(TXT("[bhaptics] update_setup_to_devices"));
#endif
	{
		// get the most recent list
		an_assert(devicesLock.is_locked_on_this_thread());
		get_devices(devices);
	}
	update_setup_to_devices(devices);
#ifdef VERBOSE_BHAPTICS
	output(TXT("[bhaptics] update_setup_to_devices [done]"));
#endif
}

void BhapticsModule::update_setup_to_devices(REF_ Array<BhapticsDevice>& _devices)
{
#ifdef VERBOSE_BHAPTICS
	output(TXT("[bhaptics] update_setup_to_devices (+dev list)"));
#endif
	{
		Concurrency::ScopedSpinLock lock(devicesSetupLock, true);

		for_every(device, _devices)
		{
			auto storedPos = devicesSetup.get_position(device->address);
			if (storedPos.is_set() &&
				storedPos.get() != device->position)
			{
#ifdef VERBOSE_BHAPTICS
				output(TXT("[bhaptics] toggle position due to setup \"%S\" : \"%S\" current \"%S\""), device->address.to_char(),
					an_bhaptics::PositionType::to_char(storedPos.get()),
					an_bhaptics::PositionType::to_char(device->position));
#endif
				toggle_position(device->address, true);
			}
		}
	}

#ifdef VERBOSE_BHAPTICS
	output(TXT("[bhaptics] update_setup_to_devices (+dev list) [done]"));
#endif
}

void BhapticsModule::write_setup_to_main_config()
{
	Concurrency::ScopedSpinLock lock(devicesSetupLock, true);

	RefCountObjectPtr<IO::JSON::Document> doc;
	doc = new IO::JSON::Document();
	for_every(device, devicesSetup.devices)
	{
		auto* dNode = doc->add_array_element(TXT("device"));
		dNode->set(TXT("address"), device->address);
		dNode->set(TXT("position"), an_bhaptics::PositionType::to_char(device->position));
	}

	String docAsString = doc->to_string();
	MainConfig::access_global().set_bhaptics_setup(docAsString);
}

void BhapticsModule::read_setup_from_main_config()
{
#ifdef VERBOSE_BHAPTICS
	output(TXT("[bhaptics] read_setup_from_main_config"));
#endif

	Concurrency::ScopedSpinLock lock(devicesSetupLock, true);
	devicesSetup.devices.clear();
		
	String docAsString = MainConfig::global().get_bhaptics_setup();
#ifdef VERBOSE_BHAPTICS
	output(TXT("[bhaptics] docAsString = \"%S\""), docAsString.to_char());
#endif
	RefCountObjectPtr<IO::JSON::Document> doc;
	doc = new IO::JSON::Document();
	if (doc->parse(docAsString))
	{
#ifdef VERBOSE_BHAPTICS
		output(TXT("[bhaptics] parsed"));
#endif
		if (auto* deviceList = doc->get_array_elements(TXT("device")))
		{
#ifdef VERBOSE_BHAPTICS
			output(TXT("[bhaptics] got list"));
#endif
			for_every(d, *deviceList)
			{
				String address = d->get(TXT("address"));
				String posString = d->get(TXT("position"));
#ifdef VERBOSE_BHAPTICS
				output(TXT("[bhaptics] add \"%S\" : \"%S\""), address.to_char(), posString.to_char());
#endif
				if (!address.is_empty() &&
					!posString.is_empty())
				{
					auto position = an_bhaptics::PositionType::parse(posString, an_bhaptics::PositionType::MAX);
					if (position != an_bhaptics::PositionType::MAX)
					{
						devicesSetup.store(address, position);
					}
				}
			}
		}
	}
#ifdef VERBOSE_BHAPTICS
	output(TXT("[bhaptics] read_setup_from_main_config [done]"));
#endif
}

void BhapticsModule::update_pending_devices()
{
	Concurrency::ScopedSpinLock lock(pendingDevicesLock, true);
	get_devices(pendingDevices);
}

bool BhapticsModule::update_devices_with_pending_devices()
{
	if (pendingDevices.get_size() == 0)
	{
		return false;
	}

	{
		Concurrency::ScopedSpinLock lock(pendingDevicesLock, true);
		{
			Concurrency::ScopedSpinLock lock(devicesLock, true);
			devices = pendingDevices;
			update_setup_to_devices(devices); 
			store_setup(devices); // to add any changes
			update_available_devices();
		}
		pendingDevices.clear();
	}

	return true;
}

void BhapticsModule::update_available_devices()
{
	Concurrency::ScopedSpinLock lock(devicesLock, true);
	availableDevices.clear();
	for_every(d, devices)
	{
		if (d->isConnected)
		{
			availableDevices.set(d->position);
		}
	}
#ifdef LOG_BHAPTICS
	output(TXT("[bhaptics] updated available devices %S"), availableDevices.to_string().to_char());
#endif
}

void BhapticsModule::store_active_sensations_from(String const& changeResponse)
{
	ArrayStatic<Name, 32> activeSensations; SET_EXTRA_DEBUG_INFO(activeSensations, TXT("BHJ::BhapticsModule::store_active_sensations_from.activeSensations"));
	int activeKeysAt = changeResponse.find_first_of(String(TXT("ActiveKeys")));
	if (activeKeysAt >= 0)
	{
		int openBracketAt = changeResponse.find_first_of('[', activeKeysAt);
		int closeBracketAt = changeResponse.find_first_of(']', openBracketAt);
		++openBracketAt;
		--closeBracketAt;
		if (closeBracketAt > openBracketAt)
		{
			Concurrency::ScopedSpinLock lock(access_registered_sensations_lock(), true);
			tchar const* at = &changeResponse[openBracketAt];
			tchar const* endAt = &changeResponse[closeBracketAt];
			while (at < endAt)
			{
				if (*at == '"')
				{
					++at;
					tchar activeKey[256];
					tchar* s = activeKey;
					while (*at != '"')
					{
						*s = *at;
						++s;
						++at;
					}
					*s = 0;
					for_every(rs, get_registered_sensations())
					{
						if (*rs == activeKey)
						{
							activeSensations.push_back(*rs);
							break;
						}
					}
				}
				++at; // to move to next
			}
		}
	}
	{
		Concurrency::ScopedSpinLock lock(access_active_sensations_lock(), true);
#ifdef LOG_BHAPTICS
		if (access_active_sensations() != activeSensations)
		{
			String asString;
			for_every(as, activeSensations)
			{
				asString += as->to_char();
				asString += TXT(" ");
			}
			output(TXT("[bhaptics] active sensations: %S"), asString.to_char());
		}
#endif
		access_active_sensations().clear();
		access_active_sensations().add_from(activeSensations);
	}
}

//

bool BhapticsDevicesSetup::is_stored(String const& _address) const
{
	for_every(device, devices)
	{
		if (device->address == _address)
		{
			return true;
		}
	}

	return false;
}

void BhapticsDevicesSetup::store(String const& _address, an_bhaptics::PositionType::Type _position)
{
	for_every(device, devices)
	{
		if (device->address == _address)
		{
			device->position = _position;
			return;
		}
	}

	Device d;
	d.address = _address;
	d.position = _position;
	devices.push_back(d);
}

Optional<an_bhaptics::PositionType::Type> BhapticsDevicesSetup::get_position(String const& _address) const
{
	for_every(device, devices)
	{
		if (device->address == _address)
		{
			return device->position;
		}
	}

	return NP;
}
