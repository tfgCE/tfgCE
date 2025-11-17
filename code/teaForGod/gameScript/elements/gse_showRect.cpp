#include "gse_showRect.h"

#include "..\..\overlayInfo\overlayInfoSystem.h"
#include "..\..\overlayInfo\elements\oie_customDraw.h"
#include "..\..\tutorials\tutorialSystem.h"

#include "..\..\..\core\system\video\video3dPrimitives.h"

//

using namespace TeaForGodEmperor;
using namespace GameScript;
using namespace Elements;

//

bool ShowRect::load_from_xml(IO::XML::Node const* _node, Framework::LibraryLoadingContext& _lc)
{
	bool result = base::load_from_xml(_node, _lc);

	id = _node->get_name_attribute(TXT("id"), id);
	colour.load_from_xml_child_or_attr(_node, TXT("colour"));
	alignPt.load_from_xml(_node, TXT("alignYaw"), TXT("alignPitch"));
	alignPt.load_from_xml_child_node(_node, TXT("align"));
	alignPt.load_from_xml_child_node(_node, TXT("align"), TXT("yaw"), TXT("pitch"));

	size.load_from_xml(_node, TXT("width"), TXT("height"));
	size.load_from_xml_child_node(_node, TXT("size"));
	size.load_from_xml_child_node(_node, TXT("size"), TXT("width"), TXT("height"));

	offset.load_from_xml_child_node(_node, TXT("offset"));
	offset.load_from_xml(_node, TXT("offsetPitch"), TXT("offsetYaw"), TXT("offsetRoll"));

	pulse = _node->get_bool_attribute_or_from_child_presence(TXT("pulse"), pulse);

	return result;
}

bool ShowRect::prepare_for_game(Framework::Library* _library, Framework::LibraryPrepareContext& _pfgContext)
{
	bool result = base::prepare_for_game(_library, _pfgContext);

	return result;
}

Framework::GameScript::ScriptExecutionResult::Type ShowRect::execute(Framework::GameScript::ScriptExecution& _execution, Framework::GameScript::ScriptExecution::Flags _flags) const
{
	if (auto* ois = OverlayInfo::System::get())
	{
		auto* c = new OverlayInfo::Elements::CustomDraw();

		Colour colourCopy = colour;
		bool pulseCopy = pulse;

		Vector2 sizeCopy = size;
		Vector2 alignPtCopy = alignPt;

		c->with_id(id);
		//c->with_pulse();
		c->with_draw(
			[colourCopy, pulseCopy, sizeCopy, alignPtCopy](float _active, float _pulse, Colour const& _colour)
		{
			if (! pulseCopy)
			{
				_pulse = 1.0f;
			}

			::System::Video3DPrimitives::fill_rect_xz(colourCopy, Vector3::zero, sizeCopy * 0.25f, alignPtCopy); // this translates roughly to angles
		});

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

	return Framework::GameScript::ScriptExecutionResult::Continue;
}
