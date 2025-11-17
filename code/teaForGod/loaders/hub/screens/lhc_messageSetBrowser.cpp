#include "lhc_messageSetBrowser.h"

#include "..\loaderHub.h"
#include "..\widgets\lhw_button.h"
#include "..\widgets\lhw_text.h"

#ifdef AN_CLANG
#include "..\..\..\..\framework\library\usedLibraryStored.inl"
#endif

//

#ifdef AN_ALLOW_OPTIMIZE_OFF
#pragma optimize("", off)
#endif

//

// localised strings
DEFINE_STATIC_NAME_STR(lsClose, TXT("hub; message set browser; close"));
DEFINE_STATIC_NAME_STR(lsLoading, TXT("hub; message set browser; loading"));

// widget ids
DEFINE_STATIC_NAME(idClose);

// input
DEFINE_STATIC_NAME(tabLeft);
DEFINE_STATIC_NAME(tabRight);

//

using namespace Loader;
using namespace HubScreens;

//

REGISTER_FOR_FAST_CAST(MessageSetBrowser);

MessageSetBrowser* MessageSetBrowser::browse(TeaForGodEmperor::MessageSet* _messageSet, BrowseParams const& _params, Hub* _hub, Name const & _id, bool _beModal, Optional<Vector2> const & _size, Optional<Rotator3> const & _at, Optional<float> const & _radius, Optional<bool> const & _beVertical, Optional<Rotator3> const & _verticalOffset, Optional<Vector2> const & _pixelsPerAngle, Optional<float> const& _scaleAutoButtons, Optional<Vector2> const& _textAlignPt)
{
	if (!_messageSet)
	{
		return nullptr;
	}

	auto* browser = browse(_params, _hub, _id, _beModal, _size, _at, _radius, _beVertical, _verticalOffset, _pixelsPerAngle, _scaleAutoButtons, _textAlignPt);
	browser->messageSet = _messageSet;

	browser->prepare_messages();

	if (_params.showMessage.is_set())
	{
		browser->show_message(_params.showMessage.get());
	}
	else if (_params.startWithRandomMessage.get(false))
	{
		browser->show_random_message();
	}
	else
	{
		browser->show_message(0);
	}

	return browser;
}

MessageSetBrowser* MessageSetBrowser::browse(Array<TeaForGodEmperor::MessageSet*> const & _messageSets, BrowseParams const& _params, Hub* _hub, Name const& _id, bool _beModal, Optional<Vector2> const& _size, Optional<Rotator3> const& _at, Optional<float> const& _radius, Optional<bool> const& _beVertical, Optional<Rotator3> const& _verticalOffset, Optional<Vector2> const& _pixelsPerAngle, Optional<float> const& _scaleAutoButtons, Optional<Vector2> const& _textAlignPt)
{
	auto* browser = browse(_params, _hub, _id, _beModal, _size, _at, _radius, _beVertical, _verticalOffset, _pixelsPerAngle, _scaleAutoButtons, _textAlignPt);
	for_every_ptr(ms, _messageSets)
	{
		for_every(m, ms->get_all())
		{
			browser->messages.push_back(m);
		}
	}

	browser->prepare_messages();

	if (_params.showMessage.is_set())
	{
		browser->show_message(_params.showMessage.get());
	}
	else if (_params.startWithRandomMessage.get(false))
	{
		browser->show_random_message();
	}
	else
	{
		browser->show_message(0);
	}

	return browser;
}

MessageSetBrowser* MessageSetBrowser::browse(BrowseParams const& _params, Hub* _hub, Name const & _id, bool _beModal, Optional<Vector2> const & _size, Optional<Rotator3> const & _at, Optional<float> const & _radius, Optional<bool> const & _beVertical, Optional<Rotator3> const & _verticalOffset, Optional<Vector2> const & _pixelsPerAngle, Optional<float> const& _scaleAutoButtons, Optional<Vector2> const& _textAlignPt)
{
	Vector2 ppa = _pixelsPerAngle.get(Vector2(32.0f, 32.0f));
	MessageSetBrowser* browser = new MessageSetBrowser(_hub, _id, _size.get(Vector2(45.0f, 40.0f)), _at.get(_hub->get_current_forward() * Rotator3(0.0f, 1.0f, 0.0f)), _radius.get(_hub->get_radius() * 0.6f), _beVertical, _verticalOffset, ppa, _scaleAutoButtons, _textAlignPt);

	if (_beModal)
	{
		browser->be_modal();
	}
	browser->randomOrder = _params.randomOrder.get(false);
	browser->forLoading = _params.forLoading.get(false);
	browser->mayClose = !browser->forLoading;
	browser->cantBeClosed = _params.cantBeClosed.get(false);
	browser->title = _params.title.get(String::empty());
	browser->activate();
	_hub->add_screen(browser);

	browser->followYawDeadZoneByBorder = HubScreen::DEFAULT_YAW_DEAD_ZONE_BY_BORDER;

	return browser;
}

MessageSetBrowser::MessageSetBrowser(Hub* _hub, Name const & _id, Vector2 const & _size, Rotator3 const & _at, float _radius, Optional<bool> const & _beVertical, Optional<Rotator3> const & _verticalOffset, Optional<Vector2> const & _pixelsPerAngle, Optional<float> const& _scaleAutoButtons, Optional<Vector2> const& _textAlignPt)
: base(_hub, _id, _size, _at, _radius, _beVertical, _verticalOffset, _pixelsPerAngle)
, scaleAutoButtons(_scaleAutoButtons.get(2.0f))
, textAlignPt(_textAlignPt)
{
}

void MessageSetBrowser::may_close()
{
	mayClose = true;
	if (!requiresExplicitClose || cantBeClosed)
	{
		deactivate();
	}
	else
	{
		if (auto* w = fast_cast<HubWidgets::Button>(get_widget(NAME(idClose))))
		{
			w->set(NAME(lsClose));
			w->enable(true);
		}
	}
}

void MessageSetBrowser::prepare_messages()
{
	if (!randomOrder)
	{
		return;
	}

	if (messageSet)
	{
		for_every(msg, messageSet->get_all())
		{
			messages.push_back(msg);

		}
		messageSet = nullptr;
	}

	sort(messages, TeaForGodEmperor::MessageSet::Message::compare_ptr);
}

void MessageSetBrowser::show_random_message()
{
	if (messageSet)
	{
		show_message(Random::get_int(messageSet->get_all().get_size()));
		return;
	}
	else if (!messages.is_empty())
	{
		show_message(Random::get_int(messages.get_size()));
		return;
	}

	show_message(0);
}

void MessageSetBrowser::show_message(Name const & _id)
{
	if (messageSet)
	{
		for_every(msg, messageSet->get_all())
		{
			if (msg->id == _id)
			{
				show_message(for_everys_index(msg));
				return;
			}
		}
	}
	else if (!messages.is_empty())
	{
		for_every_ptr(msg, messages)
		{
			if (msg->id == _id)
			{
				show_message(for_everys_index(msg));
				return;
			}
		}
	}

	error(TXT("could not find a message with an id \"%S\""), _id.to_char());
	show_message(0);
}

void MessageSetBrowser::show_message(tchar const * _id)
{
	if (messageSet)
	{
		for_every(msg, messageSet->get_all())
		{
			if (msg->id == _id)
			{
				show_message(for_everys_index(msg));
				return;
			}
		}
	}
	else if (!messages.is_empty())
	{
		for_every_ptr(msg, messages)
		{
			if (msg->id == _id)
			{
				show_message(for_everys_index(msg));
				return;
			}
		}
	}

	error(TXT("could not find a message with an id \"%S\""), _id);
	show_message(0);
}

void MessageSetBrowser::show_message(int _idx)
{
	messageIdx = _idx;

	clear();
	hub->clear_special_highlights();

	String message;
	String useTitle = title;

	int msgsCount = 0;
	{
		TeaForGodEmperor::MessageSet::Message const* msg = nullptr;
		if (messageSet)
		{
			auto& msgs = messageSet->get_all();
			msgsCount = msgs.get_size();
			messageIdx = clamp(messageIdx, 0, msgsCount - 1);
			if (msgs.is_index_valid(messageIdx))
			{
				msg = &msgs[messageIdx];
			}
		}
		else if (!messages.is_empty())
		{
			msgsCount = messages.get_size();
			messageIdx = clamp(messageIdx, 0, msgsCount - 1);
			if (messages.is_index_valid(messageIdx))
			{
				msg = messages[messageIdx];
			}
		}
		
		if (msg)
		{
			if (msg->title.is_valid())
			{
				String msgTitle = msg->title.get();
				if (!msgTitle.is_empty())
				{
					useTitle = msg->title.get();
				}
			}
			if (msg->message.is_valid())
			{
				message = msg->message.get();
			}
			for_every(hw, msg->highlightWidgets)
			{
				hub->special_highlight(hw->screen, hw->widget);
			}
		}
		else
		{
			messageIdx = 0;
		}
	}

	float spacing = max(HubScreen::s_fontSizeInPixels.x, HubScreen::s_fontSizeInPixels.y);

	float lowSpaceRequired = HubScreen::s_fontSizeInPixels.y * 5.0f;
	{
		Range2 at = mainResolutionInPixels;

		float top = mainResolutionInPixels.y.max - spacing;
		at.x.min += spacing;
		at.x.max -= spacing;

		if (!useTitle.is_empty())
		{
			at.y.max = top;
			at.y.min = top - spacing;

			Range2 useAt = at;
			useAt.expand_by(Vector2(-2.0f, -2.0f));

			auto* w = new HubWidgets::Text(Name::invalid(),
				at,
				useTitle);
			add_widget(w);

			top = at.y.min - spacing;
		}

		if (!message.is_empty())
		{
			at.y.max = top;
			at.y.min = mainResolutionInPixels.y.min + lowSpaceRequired;

			Range2 useAt = at;
			useAt.expand_by(Vector2(-2.0f, -2.0f));

			auto* w = new HubWidgets::Text(Name::invalid(),
				at,
				message);
			w->scroll.scrollType = HubWidgets::InnerScroll::VerticalScroll;
			if (!useTitle.is_empty())
			{
				w->alignPt.y = 1.0f;
			}
			w->alignPt.x = 0.0f;
			if (textAlignPt.is_set())
			{
				w->alignPt = textAlignPt.get();
			}
			add_widget(w);
		}
	}

	{
		Range2 at = mainResolutionInPixels;

		at.y.max = at.y.min + lowSpaceRequired - HubScreen::s_fontSizeInPixels.y;
		at.y.min += HubScreen::s_fontSizeInPixels.y;

		if (! cantBeClosed)
		{
			String text = LOC_STR(NAME(lsClose));
			String textLoading = LOC_STR(NAME(lsLoading));
			at.x.min = mainResolutionInPixels.x.centre() - (float)(max(text.get_length(), textLoading.get_length()) + 2) * HubScreen::s_fontSizeInPixels.x * 0.5f * scaleAutoButtons;
			at.x.max = mainResolutionInPixels.x.centre() + (float)(max(text.get_length(), textLoading.get_length()) + 2) * HubScreen::s_fontSizeInPixels.x * 0.5f * scaleAutoButtons;

			auto* w = new HubWidgets::Button(NAME(idClose), at, ! mayClose && forLoading? textLoading : text);
			w->scale = Vector2(scaleAutoButtons, scaleAutoButtons);
			w->activateOnTriggerHold = true;
			w->on_click = [this](HubWidget* _widget, Vector2 const & _at, HubInputFlags::Flags _flags)
			{
				deactivate();
			};
			add_widget(w);
			w->enable(mayClose);
		}

		if (messageIdx > 0 || randomOrder)
		{
			at.x.min = mainResolutionInPixels.x.min + spacing;
			at.x.max = mainResolutionInPixels.x.min + spacing + HubScreen::s_fontSizeInPixels.x * (3.0f * scaleAutoButtons);

			auto* w = new HubWidgets::Button(Name::invalid(), at, String(TXT("<")));
			w->scale = Vector2(scaleAutoButtons, scaleAutoButtons);
			w->on_click = [this, msgsCount](HubWidget* _widget, Vector2 const & _at, HubInputFlags::Flags _flags)
			{
				show_message(mod(messageIdx - 1, msgsCount));
				requiresExplicitClose = true;
			};
			add_widget(w);
		}

		if (messageIdx < msgsCount - 1 || randomOrder)
		{
			at.x.min = mainResolutionInPixels.x.max - spacing - HubScreen::s_fontSizeInPixels.x * (3.0f * scaleAutoButtons);
			at.x.max = mainResolutionInPixels.x.max - spacing;

			auto* w = new HubWidgets::Button(Name::invalid(), at, String(TXT(">")));
			w->scale = Vector2(scaleAutoButtons, scaleAutoButtons);
			w->on_click = [this, msgsCount](HubWidget* _widget, Vector2 const & _at, HubInputFlags::Flags _flags)
			{
				show_message(mod(messageIdx + 1, msgsCount));
				requiresExplicitClose = true;
			};
			add_widget(w);
		}
	}
}

void MessageSetBrowser::process_input(int _handIdx, Framework::GameInput const & _input, float _deltaTime)
{
	base::process_input(_handIdx, _input, _deltaTime);

	if (_input.has_button_been_pressed(NAME(tabLeft)))
	{
		show_message(messageIdx - 1);
		requiresExplicitClose = true;
	}
	if (_input.has_button_been_pressed(NAME(tabRight)))
	{
		show_message(messageIdx + 1);
		requiresExplicitClose = true;
	}
}

void MessageSetBrowser::on_deactivate()
{
	hub->clear_special_highlights();

	base::on_deactivate();
}
