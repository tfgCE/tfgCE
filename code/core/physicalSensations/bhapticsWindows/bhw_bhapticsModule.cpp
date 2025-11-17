#include "bhw_bhapticsModule.h"

#include "bhw_hapticLibrary.h"

#include "..\bhaptics\bh_library.h"

#include "..\..\concurrency\scopedSpinLock.h"
#include "..\..\io\file.h"
#include "..\..\other\parserUtils.h"
#include "..\..\system\core.h"

#ifdef AN_WINDOWS
#include "tlhelp32.h"
#endif

//

#ifdef AN_ALLOW_OPTIMIZE_OFF
#pragma optimize("", off)
#endif

//

using namespace bhapticsWindows;

//

BhapticsModule* BhapticsModule::s_module = nullptr;

BhapticsModule::BhapticsModule()
{
	SET_EXTRA_DEBUG_INFO(activeSensations, TXT("BHW::BhapticsModule.activeSensations"));

	an_assert(!s_module);
	s_module = this;

#ifdef PHYSICAL_SENSATIONS_BHAPTICS_WINDOWS
	{
		String loadLibrary(TXT("haptic_library.dll"));

		dllHandle = LoadLibrary(loadLibrary.to_char());
	}

	{
		char path[1000];
		int size = 0;
		bool result = bhapticsLibrary::TryGetExePath(path, 1000, size);
		if (result)
		{
			String exePath = String::from_char8(path);
			if (!exePath.is_empty() && IO::File::does_exist(exePath))
			{
				bool isPlayerRunning = false;
				{
					String exeName;
					int lastDirChar = exePath.find_last_of('\\');
					lastDirChar = max(lastDirChar, exePath.find_last_of('/'));
					if (lastDirChar != NONE && lastDirChar > 0)
					{
						exeName = exePath.get_sub(lastDirChar + 1);
					}
					else
					{
						exeName = exePath;
					}

					if (exeName.to_lower().get_right(4) != TXT(".exe"))
					{
						exeName += TXT(".exe");
					}

					HANDLE snapShot = ::CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);

					if (snapShot != INVALID_HANDLE_VALUE)
					{
						PROCESSENTRY32 Entry;
						Entry.dwSize = sizeof(PROCESSENTRY32);

						if (::Process32First(snapShot, &Entry))
						{
							do
							{
								if (String::compare_icase(exeName, Entry.szExeFile))
								{
									isPlayerRunning = true;
									break;
								}
							}
							while (::Process32Next(snapShot, &Entry));
						}
					}

					::CloseHandle(snapShot);
				}

				if (!isPlayerRunning)
				{
					//playerHandle = CreateProc(*exePath, nullptr, true, true, false, nullptr, 0, nullptr, nullptr);

					//TerminateProc(playerHandle);
					//CloseProc(playerHandle);
				}
				
				if (isPlayerRunning)
				{
					// player running
					isOk = true;
				}
			}
			else
			{
				// not installed
			}
		}
		else
		{
			// not registered? not installed?
		}
	}

	if (isOk)
	{
		Array<char8> appName = String(::System::Core::get_app_name()).to_char8_utf_array();
		bhapticsLibrary::InitialiseSync(appName.get_data(), appName.get_data());
	}
#endif

	if (isOk)
	{
		// register sensations through the library
		an_bhaptics::Library::register_for(this);

		// get devices as they are
		{
			Concurrency::ScopedSpinLock lock(access_devices_lock(), true);
			get_devices(true);
		}
	}

	doneInitialisation = true;
}

BhapticsModule::~BhapticsModule()
{
#ifdef PHYSICAL_SENSATIONS_BHAPTICS_WINDOWS
	if (isOk)
	{
		bhapticsLibrary::Destroy();
	}

	if (dllHandle)
	{
		::FreeLibrary((HMODULE)dllHandle);
	}
#endif

	an_assert(s_module == this);
	s_module = nullptr;
}

void BhapticsModule::register_sensation(Name const& _id, String const& _sensationJson)
{
	{
#ifdef LOG_BHAPTICS
		output(TXT("[bhaptics] register sensation %S"), _id.to_char());
#endif
#ifdef PHYSICAL_SENSATIONS_BHAPTICS_WINDOWS
		Array<char8> key = _id.to_string().to_char8_utf_array();
		Array<char8> content = _sensationJson.to_char8_utf_array();
		bhapticsLibrary::RegisterFeedback(key.get_data(), content.get_data());
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

void BhapticsModule::submit_sensation(Name const& _id)
{
	Concurrency::ScopedSpinLock lock(callLock, true);
#ifdef LOG_BHAPTICS
	output(TXT("[bhaptics] submit registered %S"), _id.to_char());
#endif
#ifdef PHYSICAL_SENSATIONS_BHAPTICS_WINDOWS
	Array<char8> key = _id.to_string().to_char8_utf_array();
	bhapticsLibrary::SubmitRegistered(key.get_data());
#endif
#ifdef LOG_BHAPTICS
	output(TXT("[bhaptics] done"));
#endif
	{
		Concurrency::ScopedSpinLock lock(activeSensationsLock, true);
		activeSensations.push_back_or_replace(ActiveSensation(_id));
	}
}

void BhapticsModule::submit_sensation_to_vest(Name const& _id, float _offsetYaw, float _offsetZ, Optional<float> const & _scaleIntensity, Optional<float> const& _scaleDuration)
{
	Concurrency::ScopedSpinLock lock(callLock, true);
#ifdef LOG_BHAPTICS
	output(TXT("[bhaptics] submit registered to vest %S  %.1f':%.3f    x%.1f  x%.1f"), _id.to_char(), _offsetYaw, _offsetZ, _scaleIntensity.get(1.0f), _scaleDuration.get(1.0f));
#endif
#ifdef PHYSICAL_SENSATIONS_BHAPTICS_WINDOWS
	Array<char8> key = _id.to_string().to_char8_utf_array();
	bhaptics::ScaleOption so;
	so.Intensity = _scaleIntensity.get(1.0f);
	so.Duration = _scaleDuration.get(1.0f);
	bhaptics::RotationOption ro;
	ro.OffsetAngleX = _offsetYaw;
	ro.OffsetY = _offsetZ;
	bhapticsLibrary::SubmitRegisteredAlt(key.get_data(), key.get_data(), so, ro);
#endif
#ifdef LOG_BHAPTICS
	output(TXT("[bhaptics] done"));
#endif
	{
		Concurrency::ScopedSpinLock lock(activeSensationsLock, true);
		activeSensations.push_back_or_replace(ActiveSensation(_id));
	}
}

bool BhapticsModule::is_sensation_active(Name const& _id) const
{
	Concurrency::ScopedSpinLock lock(activeSensationsLock, true);
	for_every(as, activeSensations)
	{
		if (as->id == _id)
		{
			return true;
		}
	}
	return false;
}

void BhapticsModule::get_devices(OUT_ Array<BhapticsDevice>& _devices)
{
	// no lock here
#ifdef LOG_BHAPTICS
	output(TXT("[bhaptics] get devices"));
#endif
#ifdef PHYSICAL_SENSATIONS_BHAPTICS_WINDOWS
	// only those we support
	if (bhapticsLibrary::IsDevicePlaying(bhaptics::PositionType::Vest)) _devices.push_back(BhapticsDevice(an_bhaptics::PositionType::Vest));
	if (bhapticsLibrary::IsDevicePlaying(bhaptics::PositionType::HandL)) _devices.push_back(BhapticsDevice(an_bhaptics::PositionType::HandL));
	if (bhapticsLibrary::IsDevicePlaying(bhaptics::PositionType::HandR)) _devices.push_back(BhapticsDevice(an_bhaptics::PositionType::HandR));
	if (bhapticsLibrary::IsDevicePlaying(bhaptics::PositionType::ForearmL)) _devices.push_back(BhapticsDevice(an_bhaptics::PositionType::ForearmL));
	if (bhapticsLibrary::IsDevicePlaying(bhaptics::PositionType::ForearmR)) _devices.push_back(BhapticsDevice(an_bhaptics::PositionType::ForearmR));
	if (bhapticsLibrary::IsDevicePlaying(bhaptics::PositionType::Head)) _devices.push_back(BhapticsDevice(an_bhaptics::PositionType::Head));
#else
	String devicesListString;
#ifdef AN_DEVELOPMENT
	devicesListString = TXT("[{\"DeviceName\":\"Tactosy2_V0.6\", \"Address\" : \"F9:5A:BE:9D:A3:83\", \"Battery\" : -1, \"Position\" : \"ForearmL\", \"IsConnected\" : false, \"IsPaired\" : true}, { \"DeviceName\":\"Tactot3_V1.1\",\"Address\" : \"DE:31:8B:1D:C3:22\",\"Battery\" : 100,\"Position\" : \"Vest\",\"IsConnected\" : true,\"IsPaired\" : true }]");
#endif
#endif
#ifdef LOG_BHAPTICS
	output(TXT("[bhaptics] done"));
#endif
}

Array<BhapticsDevice> const& BhapticsModule::get_devices(bool _update)
{
	an_assert(devicesLock.is_locked_on_this_thread());

	if (_update)
	{
		get_devices(devices);
		update_available_devices();
		Concurrency::ScopedSpinLock lock(pendingDevicesLock, true);
		pendingDevices.clear();
	}

	return devices;
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
		availableDevices.set(d->position);
	}
#ifdef LOG_BHAPTICS
	output(TXT("[bhaptics] updated available devices %S"), availableDevices.to_string().to_char());
#endif
}

void BhapticsModule::update_active_sensations()
{
	Concurrency::ScopedSpinLock lock(activeSensationsLock, true);

	for (int i = 0; i < activeSensations.get_size(); ++i)
	{
#ifdef PHYSICAL_SENSATIONS_BHAPTICS_WINDOWS
		auto& as = activeSensations[i];
		if (!bhapticsLibrary::IsPlayingKey(as.id8.get_data()))
#endif
		{
			activeSensations.remove_fast_at(i);
			--i;
		}
	}

}

//
BhapticsModule::ActiveSensation::ActiveSensation(Name const& _id)
{
	id = _id;
	id8.clear();
	id8.add_from(id.to_string().to_char8_utf_array());
}
