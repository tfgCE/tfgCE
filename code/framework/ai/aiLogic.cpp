#include "aiLogic.h"

#include "aiLogicData.h"

#include "aiMindInstance.h"

#include "..\module\moduleAI.h"
#include "..\module\moduleAppearance.h"
#include "..\module\modulePresence.h"

#include "..\world\room.h"

#include "..\..\core\debug\debugRenderer.h"
#include "..\..\core\other\parserUtils.h"

using namespace Framework;
using namespace AI;

REGISTER_FOR_FAST_CAST(Logic);

Logic::Logic(MindInstance* _mind, LogicData const * _logicData)
: mind(_mind)
, logicData(_logicData)
#ifdef AN_USE_AI_LOG
, log(true)
#endif
{
#ifdef AN_USE_AI_LOG_TO_FILE
	String dirName = String(get_log_file_name()).replace(String(TXT(".log")), String(TXT("")));
	String fileName = ParserUtils::void_to_hex((_mind->get_owner_as_modules_owner())) + String::space() + _mind->get_owner_as_modules_owner()->ai_get_name();
	String fileNameCorrected;
	for_every(ch, fileName.get_data())
	{
		if (*ch != 0)
		{
			if (*ch == ' ' ||
				(*ch >= '0' && *ch <= '9') ||
				(*ch >= 'a' && *ch <= 'z') ||
				(*ch >= 'A' && *ch <= 'Z') ||
				*ch == '-')
			{
				fileNameCorrected += *ch;
			}
			else
			{
				fileNameCorrected += ' ';
			}
		}
	}
	log.set_output_to_file(dirName + IO::get_directory_separator() + fileNameCorrected.compress().trim() + TXT(".aiLog"));
#endif

	if (logicData)
	{
		autoRareAdvanceIfNotVisible = logicData->autoRareAdvanceIfNotVisible;
	}
}
	 
Logic::~Logic()
{
#ifdef AN_DEVELOPMENT
	an_assert(ended);
#endif
}

void Logic::learn_from(SimpleVariableStorage & _parameters)
{
}

void Logic::ready_to_activate()
{
#ifdef AN_DEVELOPMENT
	readiedToActivate = true;
#endif
}

void Logic::update_rare_logic_advance(float _deltaTime)
{
	if (autoRareAdvanceIfNotVisible.is_set())
	{	// if not visible, advance rarely
		bool rareAdvance = false;
		if (auto* imo = get_imo())
		{
			if (imo->get_room_distance_to_recently_seen_by_player() > 1)
			{
				rareAdvance = true;
			}
		}
		if (rareAdvance)
		{
			set_advance_logic_rarely(autoRareAdvanceIfNotVisible);
		}
		else
		{
			set_advance_logic_rarely(NP);
		}
	}
}

void Logic::advance(float _deltaTime)
{
#ifdef AN_DEVELOPMENT
	an_assert(readiedToActivate || !requiresReadyToActivate);
#endif
}

void Logic::end()
{
#ifdef AN_DEVELOPMENT
	ended = true;
#endif
}

#ifdef AN_USE_AI_LOG
void Logic::set_state_info(int _idx, tchar const * const _format, ...)
{
	stateInfo.set_size(max(stateInfo.get_size(), _idx + 1));
	tchar buf[4096];
	va_list list;
	va_start(list, _format);
#ifndef AN_CLANG
	int textsize = (int)tstrlen(_format) + 1;
	int memsize = textsize * sizeof(tchar);
	allocate_stack_var(tchar, format, textsize);
	memory_copy(format, _format, memsize);
	sanitise_string_format(format);
	tvsprintf(buf, 4096-1, format, list);
#else
	tvsprintf(buf, 4096 - 1, _format, list);
#endif
	stateInfo[_idx] = buf;
}

void Logic::clear_state_info(int _idx)
{
	if (stateInfo.is_index_valid(_idx))
	{
		stateInfo[_idx] = String::empty();
	}
}
#endif

void Logic::debug_draw_state() const
{
	IModulesOwner* imo = mind ? mind->get_owner_as_modules_owner() : nullptr;
	if (!imo || !imo->get_presence())
	{
		return;
	}
#ifdef AN_USE_AI_LOG
	debug_context(imo->get_presence()->get_in_room());
	Vector3 at = imo->get_presence()->get_centre_of_presence_WS();
	Vector3 moveDown = -Vector3::zAxis * 0.05f;
	debug_draw_text(true, imo->is_advancement_suspended()? Colour::blue : Colour::green, at, Vector2::half, true, 0.5f, NP, imo->ai_get_name().to_char());
	for_every(s, stateInfo)
	{
		at += moveDown;
		if (!s->is_empty())
		{
			debug_draw_text(true, Colour::greyLight, at, Vector2::half, true, 0.5f, NP, s->to_char());
		}
	}
	debug_no_context();
#endif
}

void Logic::log_logic_info(LogInfoContext& _log) const
{
	IModulesOwner* imo = mind ? mind->get_owner_as_modules_owner() : nullptr;
	if (imo)
	{
		bool anythingLogged = false;
		if (auto* ai = imo->get_ai())
		{
			auto& ra = ai->access_rare_logic_advance();
			Range interval = ra.get_used_interval();
			if (interval.is_empty())
			{
				_log.log(TXT("no rare advance logic"));
			}
			else
			{
				_log.log(TXT("%c rare advance logic %S %.3f : %.3f->%.3f"), ra.should_advance() ? TXT("+") : TXT("-"), ra.get_used_interval().to_string().to_char(), ra.get_advance_delta_time(), ra.get_accumulated_delta_time(), ra.get_current_interval());
			}
			anythingLogged = true;
		}
		{
			auto& ra = imo->get_ra_pose();
			Range interval = ra.get_used_interval();
			if (interval.is_empty())
			{
				_log.log(TXT("no rare advance pose"));
			}
			else
			{
				_log.log(TXT("%c rare advance pose %S %.3f : %.3f->%.3f"), ra.should_advance()? TXT("+") : TXT("-"), ra.get_used_interval().to_string().to_char(), ra.get_advance_delta_time(), ra.get_accumulated_delta_time(), ra.get_current_interval());
			}
			anythingLogged = true;
		}
		if (anythingLogged)
		{
			_log.log(TXT(""));
		}
	}
}

void Logic::set_advance_logic_rarely(Optional<Range> const& _rare)
{
	if (advanceLogicRarely == _rare)
	{
		return;
	}
	advanceLogicRarely = _rare;
	if (auto* mind = get_mind())
	{
		if (auto* imo = mind->get_owner_as_modules_owner())
		{
			if (auto* ai = imo->get_ai())
			{
				ai->access_rare_logic_advance().override_interval(advanceLogicRarely);
			}
		}
	}
}

void Logic::set_advance_appearance_rarely(Optional<Range> const& _rare)
{
	if (advanceAppearanceRarely == _rare)
	{
		return;
	}
	advanceAppearanceRarely = _rare;
	if (auto* mind = get_mind())
	{
		if (auto* imo = mind->get_owner_as_modules_owner())
		{
			imo->set_rare_pose_advance_override(advanceAppearanceRarely);
		}
	}
}

Framework::IModulesOwner* Logic::get_imo() const
{
	if (auto* m = get_mind())
	{
		return m->get_owner_as_modules_owner();
	}

	return nullptr;
}

float Logic::adjust_time_depending_on_distance(Range const& _time, RemapValue<float, float> const& _distToMulTime)
{
	float time = Random::get(_time);
	
	if (auto * imo = get_mind()->get_owner_as_modules_owner())
	{
		if (auto * p = imo->get_presence())
		{
			if (auto * r = p->get_in_room())
			{
				// we're good with using recently seen by player placement which is in space of our room, ignoring doors
				// because if we were seen, it means that most likely we have a straight line to the player
				// if we were not seen recently, used the largest/last distToMulTime
				auto & recSeenBy = r->get_recently_seen_by_player_placement();
				if (recSeenBy.is_set())
				{
					float distance = (recSeenBy.get().get_translation() - p->get_centre_of_presence_WS()).length();

					time *= _distToMulTime.remap(distance);
				}
				else
				{
					time *= _distToMulTime.get_last_to();
				}
			}
		}
	}

	return max(0.001f, time);
}
