#include "storyExecution.h"

#include "storyChapter.h"
#include "storyScene.h"

#include "..\teaForGod.h"
#include "..\game\game.h"
#include "..\library\library.h"
#include "..\pilgrimage\pilgrimage.h"

#include "..\..\core\containers\arrayStack.h"

#include "..\..\framework\debug\testConfig.h"
#include "..\..\framework\gameScript\gameScript.h"
#include "..\..\framework\library\library.h"
#include "..\..\framework\library\usedLibraryStored.inl"

//

#ifdef AN_ALLOW_OPTIMIZE_OFF
#pragma optimize("", off)
#endif

//

using namespace TeaForGodEmperor;
using namespace Story;

//

Execution::Execution()
{
}

Execution::~Execution()
{
}

void Execution::reset()
{
	stop(true);
	facts.clear();
}

void Execution::stop(bool _justStop)
{
	SetParams params;
	params.just_stop(_justStop);
	set_current_scene(nullptr, params);
	set_current_chapter(nullptr, params);
	scriptExecution.stop_all();
}

void Execution::update(float _deltaTime)
{
	if (stopAllGameScripts)
	{
		scriptExecution.stop_all(); // stop any script being executed
		stopAllGameScripts = false;
	}

	if (!scriptExecution.get_script() || (forcePendingGameScript && pendingGameScript.is_set()))
	{
#ifdef AN_DEVELOPMENT_OR_PROFILER
		if (!pendingGameScript.is_set())
		{
			Framework::LibraryName testGameScript = Framework::TestConfig::get_params().get_value<Framework::LibraryName>(Name(TXT("testGameScript")), Framework::LibraryName::invalid());
			if (testGameScript.is_valid())
			{
				pendingGameScript = Library::get_current_as<Library>()->get_game_scripts().find_may_fail(testGameScript);
			}
		}
#endif

		if (pendingGameScript.is_set())
		{
			forcePendingGameScript = false;
#ifdef OUTPUT_STORY
			output(TXT("[story] start pending script \"%S\""), pendingGameScript->get_name().to_string().to_char());
#endif
			scriptExecution.start(pendingGameScript.get());
			pendingGameScript.clear();
		}
		else if (currentScene.is_set())
		{
			set_current_scene(nullptr);
		}
	}

	chooseSceneTimeLeft = max(0.0f, chooseSceneTimeLeft - _deltaTime);

	if (!currentScene.is_set() &&
		currentChapter.get())
	{
		if (chooseSceneTimeLeft <= 0.0f)
		{
			ARRAY_STACK(Scene*, scenes, currentChapter->get_scenes().get_size());
			for_every_ref(scene, currentChapter->get_scenes())
			{
				if (scene->get_requirements().check(*this))
				{
					scenes.push_back(scene);
					break; // for time being only the first one
				}
			}
			if (scenes.is_empty())
			{
				chooseSceneTimeLeft = Random::get_float(0.3f, 0.8f);
			}
			else
			{
#ifdef OUTPUT_STORY
				output(TXT("[story] found a new scene!"));
#endif
				set_current_scene(scenes[Random::get_int(scenes.get_size())]);
			}
		}
	}

	scriptExecution.execute();
}

void Execution::do_clean_start_for(Story::Chapter* _chapter)
{
	if (auto* cc = _chapter)
	{
#ifdef OUTPUT_STORY
		output(TXT("[story] clean start for chapter \"%S\""), cc->get_name().to_string().to_char());
#endif
		cc->get_at_clean_start().perform(*this);
	}
}

void Execution::set_chapter(Story::Chapter * _chapter, SetParams const& _params)
{
	forcePendingGameScript = true;
	stopAllGameScripts = true;
	set_current_scene(nullptr); // clean, no scene
	set_current_chapter(_chapter, _params);
}

void Execution::set_scene(Story::Scene * _scene)
{
	set_current_scene(_scene);
}

void Execution::set_current_chapter(Story::Chapter* _chapter, SetParams const& _params)
{
	an_assert(!_chapter || !_params.justStop.get(false), TXT("if just stopping, no chapter should be provided"));
	if (currentChapter.get() == _chapter)
	{
		return;
	}
	if (auto* cc = currentChapter.get())
	{
		if (! _params.justStop.get(false))
		{
#ifdef OUTPUT_STORY
			output(TXT("[story] ending chapter \"%S\""), cc->get_name().to_string().to_char());
#endif
			cc->get_at_end().perform(*this);
		}
	}
#ifdef OUTPUT_STORY
	if (currentChapter.get() != _chapter)
	{
		output(TXT("[story] change chapter \"%S\" -> \"%S\""),
			currentChapter.get() ? currentChapter->get_name().to_string().to_char() : TXT("--"),
			_chapter ? _chapter->get_name().to_string().to_char() : TXT("--"));
	}
#endif
	currentChapter = _chapter;
	if (auto* cc = currentChapter.get())
	{
#ifdef OUTPUT_STORY
		output(TXT("[story] starting chapter \"%S\""), cc->get_name().to_string().to_char());
#endif
		cc->get_at_start().perform(*this);
	}
	set_current_scene(nullptr);
}

void Execution::set_current_scene(Story::Scene* _scene, SetParams const& _params)
{
	an_assert(!_scene || !_params.justStop.get(false), TXT("if just stopping, no scene should be provided"));
	if (currentScene.get() == _scene)
	{
		return;
	}
	if (auto* cs = currentScene.get())
	{
		if (!_params.justStop.get(false))
		{
#ifdef OUTPUT_STORY
			output(TXT("[story] ending scene \"%S\""), cs->get_name().to_string().to_char());
#endif
			cs->get_at_end().perform(*this);
		}
	}
#ifdef OUTPUT_STORY
	if (currentScene.get() != _scene)
	{
		output(TXT("[story] change scene \"%S\" -> \"%S\""),
			currentScene.get() ? currentScene->get_name().to_string().to_char() : TXT("--"),
			_scene ? _scene->get_name().to_string().to_char() : TXT("--"));
	}
#endif
	currentScene = _scene;
	pendingGameScript.clear();
	if (auto* cs = currentScene.get())
	{
#ifdef OUTPUT_STORY
		output(TXT("[story] starting scene \"%S\""), cs->get_name().to_string().to_char());
#endif
		pendingGameScript = cs->get_game_script();
		cs->get_at_start().perform(*this);
	}
}

void Execution::log(LogInfoContext& _log) const
{
	_log.log(TXT("chapter: %S"), currentChapter.get()? currentChapter->get_name().to_string().to_char() : TXT("--"));
	_log.log(TXT("scene: %S"), currentScene.get()? currentScene->get_name().to_string().to_char() : TXT("--"));
	_log.log(TXT("facts: %S"), facts.to_string().to_char());
	scriptExecution.log(_log);
}
