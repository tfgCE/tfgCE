#pragma once

#include "..\..\framework\gameScript\gameScriptExecution.h"

#include "..\..\core\tags\tag.h"
#include "..\..\core\functionParamsStruct.h"

#include "..\..\framework\library\usedLibraryStored.h"

namespace TeaForGodEmperor
{
	namespace Story
	{
		class Chapter;
		class Scene;

		class Execution
		{
		public:
			Execution();
			~Execution();

			Concurrency::MRSWLock& access_facts_lock() const { return factsLock; }
			Tags& access_facts() { an_assert(factsLock.is_acquired_write_on_this_thread()); return facts; }
			Tags const & get_facts() const { an_assert(factsLock.is_acquired() || factsLock.is_acquired_to_read()); return facts; }

			Framework::GameScript::ScriptExecution& access_script_execution() { return scriptExecution; }

			void log(LogInfoContext& _log) const;

		public:
			struct SetParams
			{
				ADD_FUNCTION_PARAM_DEF(SetParams, bool, justStop, just_stop, true);
			};

			void reset();
			void stop(bool _justStop = false); // if no _justStop, will also run "on end"
			void update(float _deltaTime);
			void set_chapter(Story::Chapter * _chapter, SetParams const & _params = SetParams());
			void set_scene(Story::Scene * _scene);

			void do_clean_start_for(Story::Chapter* _chapter);

		private:
			Framework::GameScript::ScriptExecution scriptExecution;

			mutable Concurrency::MRSWLock factsLock;
			Tags facts; // storyFacts
			Framework::UsedLibraryStored<Story::Chapter> currentChapter;
			Framework::UsedLibraryStored<Story::Scene> currentScene;
			float chooseSceneTimeLeft = 0.0f;

			Framework::UsedLibraryStored<Framework::GameScript::Script> pendingGameScript;
			bool forcePendingGameScript = false;
			bool stopAllGameScripts = false;

			void set_current_chapter(Story::Chapter * _chapter, SetParams const& _params = SetParams());
			void set_current_scene(Story::Scene * _scene, SetParams const & _params = SetParams());
		};
	};
};
