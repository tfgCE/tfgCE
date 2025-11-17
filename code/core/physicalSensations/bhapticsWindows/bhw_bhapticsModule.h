#pragma once

#include "bhw_bhapticsDevice.h"

#include "..\bhaptics\bh_availableDevices.h"

#include "..\..\containers\arrayStatic.h"
#include "..\..\types\optional.h"

namespace bhapticsWindows
{
	class BhapticsModule
	{
	public:
		static BhapticsModule* get() { an_assert(s_module); return s_module; }

		BhapticsModule();
		~BhapticsModule();

		bool is_ok() const { return isOk; }

		bool is_player_available() const { return dllHandle != nullptr; }
		
		bool done_initialisation() const { return doneInitialisation; }

	public: // commands
		void register_sensation(Name const& _id, String const& _sensationJson);

		void submit_sensation(Name const& _id);
		void submit_sensation_to_vest(Name const& _id, float _offsetYaw = 0.0f, float _offsetZ = 0.0f, Optional<float> const& _scaleIntensity = NP, Optional<float> const& _scaleDuration = NP);

		bool is_sensation_active(Name const& _id) const;

	public: // access to things
		Concurrency::SpinLock& access_devices_lock() const { return devicesLock; }
		Array<BhapticsDevice> const& get_devices(bool _update = false); // lock required

		void update_pending_devices();
		bool update_devices_with_pending_devices(); // returns true if devices updated (no lock required) (use to check if something has changed)

		an_bhaptics::AvailableDevices const& get_available_devices() const { return availableDevices; }
		
		void update_available_devices();

		void update_active_sensations();

	private:
		static BhapticsModule* s_module;

		void* dllHandle = nullptr;

		bool isOk = false;
		bool doneInitialisation = false;

		an_bhaptics::AvailableDevices availableDevices;

		mutable Concurrency::SpinLock callLock = Concurrency::SpinLock(TXT("bhapticsWindows.BhapticsModule.callLock"));

		mutable Concurrency::SpinLock registeredSensationsLock = Concurrency::SpinLock(TXT("bhapticsWindows.BhapticsModule.registeredSensationsLock"));
		Array<Name> registeredSensations;

		mutable Concurrency::SpinLock activeSensationsLock = Concurrency::SpinLock(TXT("bhapticsWindows.BhapticsModule.activeSensationsLock"));
		struct ActiveSensation
		{
			Name id;
			ArrayStatic<char8, 256> id8;

			ActiveSensation() { SET_EXTRA_DEBUG_INFO(id8, TXT("BhapticsModule::ActiveSensation.id8")); }
			explicit ActiveSensation(Name const& _id);
		};
		ArrayStatic<ActiveSensation, 32> activeSensations;

		mutable Concurrency::SpinLock devicesLock = Concurrency::SpinLock(TXT("bhapticsWindows.BhapticsModule.devicesLock"));
		Array<BhapticsDevice> devices;

		mutable Concurrency::SpinLock pendingDevicesLock = Concurrency::SpinLock(TXT("bhapticsWindows.BhapticsModule.pendingDevicesLock"));
		Array<BhapticsDevice> pendingDevices;

		void get_devices(OUT_ Array<BhapticsDevice>& _devices);
	};
};


