#include "gse_waitForEnergyTransfer.h"

#include "..\..\overlayInfo\overlayInfoSystem.h"
#include "..\..\overlayInfo\elements\oie_customDraw.h"
#include "..\..\tutorials\tutorialSystem.h"

#include "..\..\..\core\system\video\video3dPrimitives.h"

//

#ifdef AN_ALLOW_OPTIMIZE_OFF
#pragma optimize("", off)
#endif

//

using namespace TeaForGodEmperor;
using namespace GameScript;
using namespace Elements;

//

// execution variables
DEFINE_STATIC_NAME(energyTransferEnergyPool);
DEFINE_STATIC_NAME(energyTransferEnergyPoolStartedAt);

// ids
DEFINE_STATIC_NAME(waitForEnergyTransfer);

//

bool WaitForEnergyTransfer::load_from_xml(IO::XML::Node const* _node, Framework::LibraryLoadingContext & _lc)
{
	bool result = true;

	energyPoolMin.load_from_xml(_node, TXT("energyPoolMin"));
	energyPoolMax.load_from_xml(_node, TXT("energyPoolMax"));

	return result;
}

float WaitForEnergyTransfer::calculate_energy_pool_pt(Framework::GameScript::ScriptExecution& _execution, Optional<Energy> const& _energyPoolMin, Optional<Energy> const& _energyPoolMax)
{
	auto ep = _execution.get_variables().get_value<Energy>(NAME(energyTransferEnergyPool), Energy::zero());
	auto started = _execution.get_variables().get_value<Energy>(NAME(energyTransferEnergyPoolStartedAt), Energy::zero());

	if (_energyPoolMin.is_set())
	{
		if (_energyPoolMin.get() > started)
		{
			return clamp((ep - started).as_float() / (_energyPoolMin.get() - started).as_float(), 0.0f, 1.0f);
		}
		else
		{
			return 1.0f;
		}
	}
	if (_energyPoolMax.is_set())
	{
		if (_energyPoolMax.get() < started)
		{
			return clamp((ep - started).as_float() / (_energyPoolMax.get() - started).as_float(), 0.0f, 1.0f);
		}
		else
		{
			return 1.0f;
		}
	}

	return 1.0f;
}

Framework::GameScript::ScriptExecutionResult::Type WaitForEnergyTransfer::execute(Framework::GameScript::ScriptExecution& _execution, Framework::GameScript::ScriptExecution::Flags _flags) const
{
	if (energyPoolMin.is_set() || energyPoolMax.is_set())
	{
		auto ep = _execution.get_variables().get_value<Energy>(NAME(energyTransferEnergyPool), Energy::zero());
		{
			if (is_flag_set(_flags, Framework::GameScript::ScriptExecution::Flag::Entered))
			{
				_execution.access_variables().access<Energy>(NAME(energyTransferEnergyPoolStartedAt)) = ep;
			}
			if (auto* ois = OverlayInfo::System::get())
			{
				{
					auto* c = new OverlayInfo::Elements::CustomDraw();

					c->with_id(NAME(waitForEnergyTransfer));
					c->set_cant_be_deactivated_by_system();

					auto energyPoolMinCopy = energyPoolMin;
					auto energyPoolMaxCopy = energyPoolMax;
					c->with_draw(
						[&_execution, energyPoolMinCopy, energyPoolMaxCopy](float _active, float _pulse, Colour const& _colour)
					{
						float atPt = calculate_energy_pool_pt(_execution, energyPoolMinCopy, energyPoolMaxCopy);

						Colour colour = _colour.mul_rgb(_pulse).with_alpha(_active);

						::System::Video3DPrimitives::ring_xz(colour, Vector3::zero, 0.7f, 1.0f, _active < 1.0f, 0.0f, atPt, 0.05f);
					});

					Rotator3 offset = Rotator3(-10.0f, 0.0f, 0.0f);
					if (TutorialSystem::check_active())
					{
						TutorialSystem::configure_oie_element(c, offset);
					}
					else
					{
						c->with_location(OverlayInfo::Element::Relativity::RelativeToAnchor);
						c->with_rotation(OverlayInfo::Element::Relativity::RelativeToAnchor, offset);
						c->with_distance(10.0f);
					}

					ois->add(c);
				}
			}
			if (energyPoolMin.is_set() && energyPoolMin.get() <= ep)
			{
				return Framework::GameScript::ScriptExecutionResult::Continue;
			}
			if (energyPoolMax.is_set() && energyPoolMax.get() >= ep)
			{
				return Framework::GameScript::ScriptExecutionResult::Continue;
			}
		}
		return Framework::GameScript::ScriptExecutionResult::Repeat;
	}
	return Framework::GameScript::ScriptExecutionResult::Continue;
}

void WaitForEnergyTransfer::interrupted(Framework::GameScript::ScriptExecution& _execution) const
{
	base::interrupted(_execution);
	if (auto* ois = OverlayInfo::System::get())
	{
		ois->deactivate(NAME(waitForEnergyTransfer), true);
	}
}

