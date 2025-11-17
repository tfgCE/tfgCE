#pragma once

#include "bhj_bhapticsDevice.h"

#include "..\bhaptics\bh_availableDevices.h"

#include "..\..\containers\arrayStatic.h"
#include "..\..\types\optional.h"

#ifdef AN_ANDROID
#include "..\..\system\android\androidApp.h"
#endif

namespace bhapticsJava
{
	struct BhapticsDevicesSetup
	{
		struct Device
		{
			String address;
			an_bhaptics::PositionType::Type position = an_bhaptics::PositionType::MAX;
		};
		Array<Device> devices;

		bool is_stored(String const& _address) const;
		void store(String const& _address, an_bhaptics::PositionType::Type _position);
		Optional<an_bhaptics::PositionType::Type> get_position(String const& _address) const;
	};

	class BhapticsModule
	{
	public:
		static BhapticsModule* get() { an_assert(s_module); return s_module; }

		BhapticsModule();
		~BhapticsModule();

	public: // commands
		void toggle_position(String const& _address, bool _justToggle = false);

		void ping(String const& _address);
		void ping_all();

		void register_sensation(Name const& _id, String const& _sensationJson);

		void submit_sensation(Name const& _id, float _offsetYaw = 0.0f, float _offsetZ = 0.0f, Optional<float> const& _scaleIntensity = NP, Optional<float> const& _scaleDuration = NP);

		bool is_sensation_active(Name const& _id) const;

		void write_setup_to_main_config();
		void read_setup_from_main_config();

	public: // access to things
		Concurrency::SpinLock& access_devices_lock() const { return devicesLock; }
		Array<BhapticsDevice> const& get_devices(bool _update = false); // lock required

		void update_pending_devices();
		bool update_devices_with_pending_devices(); // returns true if devices updated (no lock required) (use to check if something has changed)

		an_bhaptics::AvailableDevices const& get_available_devices() const { return availableDevices; }
		
		void update_available_devices();

	public: // for JNI integration
		Concurrency::SpinLock& access_pending_devices_lock() const { return pendingDevicesLock; }
		Array<BhapticsDevice>& access_pending_devices() { return pendingDevices; }

		Concurrency::SpinLock& access_registered_sensations_lock() const { return registeredSensationsLock; }
		Array<Name> const & get_registered_sensations() const { return registeredSensations; }

		Concurrency::SpinLock& access_active_sensations_lock() const { return activeSensationsLock; }
		ArrayStatic<Name, 32>& access_active_sensations() { return activeSensations; }

		void store_active_sensations_from(String const& _changeResponse);

	private:
		static BhapticsModule* s_module;

		an_bhaptics::AvailableDevices availableDevices;

		mutable Concurrency::SpinLock devicesSetupLock = Concurrency::SpinLock(TXT("bhapticsJava.BhapticsModule.devicesSetupLock"));
		BhapticsDevicesSetup devicesSetup;

		mutable Concurrency::SpinLock callLock = Concurrency::SpinLock(TXT("bhapticsJava.BhapticsModule.callLock"));

#ifdef AN_ANDROID
		jclass class_activity;
		jclass class_bhapticsModule;

		jobject object_bhapticsModule;

		jmethodID method_getDeviceList;
		jmethodID method_refreshPairing;
		jmethodID method_registerSensation;
		jmethodID method_submitRegistered;
		jmethodID method_isPlaying;
		jmethodID method_togglePosition;
		jmethodID method_ping;
		jmethodID method_pingAll;
#endif

		mutable Concurrency::SpinLock registeredSensationsLock = Concurrency::SpinLock(TXT("bhapticsJava.BhapticsModule.registeredSensationsLock"));
		Array<Name> registeredSensations;

		mutable Concurrency::SpinLock activeSensationsLock = Concurrency::SpinLock(TXT("bhapticsJava.BhapticsModule.activeSensationsLock"));
		ArrayStatic<Name, 32> activeSensations;

		mutable Concurrency::SpinLock devicesLock = Concurrency::SpinLock(TXT("bhapticsJava.BhapticsModule.devicesLock"));
		Array<BhapticsDevice> devices;

		mutable Concurrency::SpinLock pendingDevicesLock = Concurrency::SpinLock(TXT("bhapticsJava.BhapticsModule.pendingDevicesLock"));
		Array<BhapticsDevice> pendingDevices;

		void get_devices(OUT_ Array<BhapticsDevice>& _devices);

		void store_setup();
		void store_setup(REF_ Array<BhapticsDevice>& _devices);
		void update_setup_to_devices();
		void update_setup_to_devices(REF_ Array<BhapticsDevice>& _devices);
	};
};


