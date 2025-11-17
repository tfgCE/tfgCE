#include "aiLogic_displayMessage.h"

#include "..\..\..\core\system\video\video3dPrimitives.h"

#include "..\..\..\framework\ai\aiMindInstance.h"
#include "..\..\..\framework\display\display.h"
#include "..\..\..\framework\display\displayDrawCommands.h"
#include "..\..\..\framework\module\custom\mc_display.h"
#include "..\..\..\framework\modulesOwner\modulesOwnerImpl.inl"

//

#ifdef AN_ALLOW_OPTIMIZE_OFF
#pragma optimize("", off)
#endif

//

using namespace TeaForGodEmperor;
using namespace AI;
using namespace Logics;

//

REGISTER_FOR_FAST_CAST(DisplayMessage);

DisplayMessage::DisplayMessage(::Framework::AI::MindInstance* _mind, ::Framework::AI::LogicData const * _logicData)
: base(_mind, _logicData, execute_logic)
{
	displayMessageData = fast_cast<DisplayMessageData>(_logicData);
}

DisplayMessage::~DisplayMessage()
{
	if (display)
	{
		display->use_background(nullptr);
		display->set_on_update_display(this, nullptr);
	}
}

void DisplayMessage::advance(float _deltaTime)
{
	base::advance(_deltaTime);

	{
		int lcc = Framework::LocalisedStrings::get_langauge_change_count();
		if (languageChangeCount != lcc)
		{
			languageChangeCount = lcc;
			redrawNow = true;
		}
	}

	if (displayMessageData->variableControlled)
	{
		auto* imo = get_imo();
		int newMessageIdx = NONE;
		int defIdx = NONE;
		for_every(m, displayMessageData->messages)
		{
			if (!m->variableRequired.is_valid())
			{
				defIdx = for_everys_index(m);
			}
			else if (imo)
			{
				if (imo->get_value<bool>(m->variableRequired, false))
				{
					newMessageIdx = for_everys_index(m);
				}
			}
		}

		if (newMessageIdx == NONE)
		{
			newMessageIdx = defIdx;
		}

		if (messageIdx != newMessageIdx)
		{
			messageIdx = newMessageIdx;
			redrawNow = true;
		}
	}
	else
	{
		messageTimeLeft -= _deltaTime;
		if (messageTimeLeft <= 0.0f && !displayMessageData->messages.is_empty() &&
			(messageIdx == NONE || displayMessageData->messages.get_size() > 1))
		{
			messageIdx = mod(messageIdx + 1, displayMessageData->messages.get_size());
			messageTimeLeft = displayMessageData->messages[messageIdx].messageTime.get(displayMessageData->messageTime);
			redrawNow = true;
		}
	}

	if (! display)
	{
		if (auto* mind = get_mind())
		{
			if (auto* imo = mind->get_owner_as_modules_owner())
			{
				if (auto* displayModule = imo->get_custom<::Framework::CustomModules::Display>())
				{
					display = displayModule->get_display();

					if (display)
					{
						display->draw_all_commands_immediately();
						display->set_on_update_display(this,
						[this](Framework::Display* _display)
						{
							if (!redrawNow)
							{
								return;
							}
							redrawNow = false;

							_display->drop_all_draw_commands();
							_display->add((new Framework::DisplayDrawCommands::CLS(Colour::black))->immediate_draw());
							_display->add((new Framework::DisplayDrawCommands::Border(Colour::black))->immediate_draw());

							int topY = display->get_char_resolution().y - 1;
							if (displayMessageData->header.is_valid() &&
								! displayMessageData->noHeader)
							{
								_display->add((new Framework::DisplayDrawCommands::TextAt())
									->text(displayMessageData->header.get())
									->in_region(Framework::RangeCharCoord2(RangeInt(0, display->get_char_resolution().x - 1),
																		   RangeInt(topY)))
									->v_align_top()
									->h_align_left()
									->immediate_draw());
								--topY;
							}
							if (displayMessageData->messages.is_index_valid(messageIdx))
							{
								auto& m = displayMessageData->messages[messageIdx];
								_display->add((new Framework::DisplayDrawCommands::TextAtMultiline())
										->text(m.message.get())
										->in_region(Framework::RangeCharCoord2(RangeInt(0, display->get_char_resolution().x - 1),
																		   RangeInt(0, topY - displayMessageData->headerSize + 1)))
										->v_align(m.valign == 0 ? Framework::DisplayVAlignment::Centre : (m.valign < 0 ? Framework::DisplayVAlignment::Bottom : Framework::DisplayVAlignment::Top))
										->h_align(m.align == 0? Framework::DisplayHAlignment::Centre : (m.align < 0? Framework::DisplayHAlignment::Left : Framework::DisplayHAlignment::Right))
										->immediate_draw(m.immediateDraw));
							}
						});
					}
				}
			}
		}
	}
}

void DisplayMessage::learn_from(SimpleVariableStorage & _parameters)
{
	base::learn_from(_parameters);
}

LATENT_FUNCTION(DisplayMessage::execute_logic)
{
	PERFORMANCE_GUARD_LIMIT(0.005f, TXT("[ai displayMessage] execute logic"));

	LATENT_SCOPE();
	LATENT_SCOPE_INFO(TXT("execute logic"));

	LATENT_PARAM_NOT_USED(::Framework::AI::MindInstance*, mind); // auto
	LATENT_PARAM_NOT_USED(::Framework::AI::Logic*, logic); // auto
	LATENT_END_PARAMS();

	LATENT_VAR(bool, depleted);

	LATENT_VAR(bool, buttonPressed);

	LATENT_BEGIN_CODE();

	buttonPressed = false;
	depleted = false;

	//ai_log(self, TXT("displayMessage, hello!"));

	while (true)
	{
		LATENT_WAIT(Random::get_float(0.1f, 0.2f));
	}

	LATENT_ON_BREAK();
	//

	LATENT_ON_END();
	//

	LATENT_END_CODE();
	LATENT_RETURN();
}

//

REGISTER_FOR_FAST_CAST(DisplayMessageData);

bool DisplayMessageData::load_from_xml(IO::XML::Node const * _node, ::Framework::LibraryLoadingContext & _lc)
{
	bool result = base::load_from_xml(_node, _lc);

	noHeader = _node->get_bool_attribute_or_from_child_presence(TXT("noHeader"), noHeader);
	headerSize = _node->get_int_attribute_or_from_child(TXT("headerSize"), headerSize);
	for_every(node, _node->children_named(TXT("header")))
	{
		headerSize = node->get_int_attribute(TXT("size"), headerSize);
	}
	header.load_from_xml_child(_node, TXT("header"), _lc, TXT("header"));
	
	messageTime = _node->get_float_attribute_or_from_child(TXT("messageTime"), messageTime);
	variableControlled = _node->get_bool_attribute_or_from_child_presence(TXT("variableControlled"), variableControlled);
	bool defImmediateDraw = _node->get_bool_attribute_or_from_child_presence(TXT("immediateDraw"));

	for_every(node, _node->children_named(TXT("message")))
	{
		Message m;
		if (auto* attr = node->get_attribute(TXT("align")))
		{
			if (attr->get_as_string() == TXT("centre"))
			{
				m.align = 0;
			}
			else if (attr->get_as_string() == TXT("right"))
			{
				m.align = 1;
			}
			else if (attr->get_as_string() == TXT("left"))
			{
				m.align = -1;
			}
			else
			{
				error_loading_xml(node, TXT("align value not recognised left/centre/right allowed only"));
			}
		}
		if (auto* attr = node->get_attribute(TXT("valign")))
		{
			if (attr->get_as_string() == TXT("centre"))
			{
				m.valign = 0;
			}
			else if (attr->get_as_string() == TXT("top"))
			{
				m.valign = 1;
			}
			else if (attr->get_as_string() == TXT("bottom"))
			{
				m.valign = -1;
			}
			else
			{
				error_loading_xml(node, TXT("valign value not recognised bottom/centre/top allowed only"));
			}
		}
		String messageId = String::printf(TXT("%i"), messages.get_size());
		if (m.message.load_from_xml(node, _lc, messageId.to_char()))
		{
			m.messageTime.load_from_xml(node, TXT("messageTime"));
			m.immediateDraw = node->get_bool_attribute_or_from_child_presence(TXT("immediateDraw"), defImmediateDraw);
			m.variableRequired = node->get_name_attribute_or_from_child(TXT("variableRequired"), m.variableRequired);
			messages.push_back(m);
		}
		else
		{
			error_loading_xml(node, TXT("couldn't load message"));
			result = false;
		}
	}

	return result;
}

bool DisplayMessageData::prepare_for_game(::Framework::Library* _library, ::Framework::LibraryPrepareContext& _pfgContext)
{
	bool result = base::prepare_for_game(_library, _pfgContext);

	return result;
}
