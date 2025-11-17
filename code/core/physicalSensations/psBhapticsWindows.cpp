#include "psBhapticsWindows.h"

#include "bhaptics\bh_library.h"

#include "..\mainConfig.h"

#include "..\splash\splashLogos.h"

//

#ifdef AN_ALLOW_OPTIMIZE_OFF
#pragma optimize("", off)
#endif

//

using namespace PhysicalSensations;

//

// physical sensation system id
DEFINE_STATIC_NAME(bhaptics);

// sensation ids (as registered)
DEFINE_STATIC_NAME(RecoilWeak);
DEFINE_STATIC_NAME(RecoilMedium);
DEFINE_STATIC_NAME(RecoilStrong);
DEFINE_STATIC_NAME(RecoilVeryStrong);

//

REGISTER_FOR_FAST_CAST(BhapticsWindows);

void BhapticsWindows::splash_logo()
{
	Splash::Logos::add(TXT("bhaptics"), SPLASH_LOGO_DEVICE);
}

Name const& BhapticsWindows::system_id()
{
	return NAME(bhaptics);
}

void BhapticsWindows::start()
{
	terminate();

	new BhapticsWindows();
}

BhapticsWindows::BhapticsWindows()
{
}

BhapticsWindows::~BhapticsWindows()
{
	delete_and_clear(bhapticsModule);
}

void BhapticsWindows::async_init()
{
	if (initialising.acquire_if_not_locked())
	{
		bhapticsModule = new bhapticsWindows::BhapticsModule();

		isOk = bhapticsModule->is_ok();
	}
}

void BhapticsWindows::internal_start_sensation(REF_ SingleSensation & _sensation)
{
#ifdef LOG_BHAPTICS
	output(TXT("[bhaptics] start sensation %i"), _sensation.type);
	output(TXT("[bhaptics] available devices %S"), bhapticsModule->get_available_devices().to_string().to_char());
#endif

	ArrayStatic<an_bhaptics::SensationToTrigger, 4> sensations; SET_EXTRA_DEBUG_INFO(sensations, TXT("BhapticsWindows::internal_start_sensation(single).sensations"));
	an_bhaptics::Library::get_sensation_ids_for(_sensation, bhapticsModule->get_available_devices(), sensations);
	for_every(s, sensations)
	{
#ifdef LOG_BHAPTICS
		output(TXT("[bhaptics] -> single sensationId:%S"), s->sensationId.to_char());
#endif
		if (s->sensationId.is_valid() &&
			bhapticsModule->get_available_devices().has(s->position))
		{
			if (s->position == an_bhaptics::PositionType::Vest)
			{
				float offsetYaw;
				float offsetZ;
				an_bhaptics::Library::get_offsets_for_vest(_sensation, s->sensationId, OUT_ offsetYaw, OUT_ offsetZ);
				bhapticsModule->submit_sensation_to_vest(s->sensationId, offsetYaw, offsetZ);
			}
			else
			{
				bhapticsModule->submit_sensation(s->sensationId);
			}
		}
	}
}

void BhapticsWindows::internal_start_sensation(REF_ OngoingSensation & _sensation)
{
#ifdef LOG_BHAPTICS
	output(TXT("[bhaptics] start ongoing sensation %i"), _sensation.type);
	output(TXT("[bhaptics] available devices %S"), bhapticsModule->get_available_devices().to_string().to_char());
#endif

	ArrayStatic<an_bhaptics::SensationToTrigger, 4> sensations; SET_EXTRA_DEBUG_INFO(sensations, TXT("BhapticsWindows::internal_start_sensation(ongoing).sensations"));
	an_bhaptics::Library::get_sensation_ids_for(_sensation, bhapticsModule->get_available_devices(), sensations);
	for_every(s, sensations)
	{
#ifdef LOG_BHAPTICS
		output(TXT("[bhaptics] -> ongoing sensationId:%S"), s->sensationId.to_char());
#endif
		if (s->sensationId.is_valid() &&
			bhapticsModule->get_available_devices().has(s->position))
		{
			OngoingSensationInfo os;
			os.id = _sensation.id;
			os.sensationId = s->sensationId;
			if (s->position == an_bhaptics::PositionType::Vest)
			{
				float offsetYaw;
				float offsetZ;
				an_bhaptics::Library::get_offsets_for_vest(_sensation, s->sensationId, OUT_ offsetYaw, OUT_ offsetZ);
				bhapticsModule->submit_sensation_to_vest(s->sensationId, offsetYaw, offsetZ);
				os.vest = true;
				os.offsetYaw = offsetYaw;
				os.offsetZ = offsetZ;
			}
			else
			{
				bhapticsModule->submit_sensation(s->sensationId);
			}
			ongoingSensations.push_back(os);
		}
	}
}

void BhapticsWindows::internal_sustain_sensation(Sensation::ID _id)
{
	for_every(os, ongoingSensations)
	{
		if (os->id == _id)
		{
			if (!bhapticsModule->is_sensation_active(os->sensationId))
			{
				// restart
				if (os->vest)
				{
					bhapticsModule->submit_sensation_to_vest(os->sensationId, os->offsetYaw, os->offsetZ);
				}
				else
				{
					bhapticsModule->submit_sensation(os->sensationId);
				}
			}
		}
	}
}

void BhapticsWindows::internal_stop_sensation(Sensation::ID _id)
{
	for (int i = 0; i < ongoingSensations.get_size(); ++i)
	{
		auto& os = ongoingSensations[i];
		if (os.id == _id)
		{
			ongoingSensations.remove_fast_at(i);
			--i;
		}
	}
}

bool BhapticsWindows::internal_stop_all_sensations()
{
	ongoingSensations.clear();
	return true;
}

void BhapticsWindows::internal_advance(float _deltaTime)
{
	bhapticsModule->update_active_sensations();
}
