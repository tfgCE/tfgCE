#pragma once

#include "energy.h"

#include "..\..\framework\time\dateTime.h"

namespace TeaForGodEmperor
{
	namespace PostGameSummary
	{
		enum Type
		{
			None,
			Died,
			Interrupted,
			ReachedDemoEnd, // if tutorial, finished tutorial
			ReachedEnd, // if tutorial, finished tutorial
		};
	};

	struct GameStats
	{
		static GameStats & get() { return *s_stats; }
		static void use_tutorial() { s_stats = &s_statsTutorial; }
		static void use_pilgrimage() { s_stats = &s_statsPilgrimage; }

		Concurrency::SpinLock& access_lock() const { return statsLock; }

		// use it in single thread or via methods below
		Framework::DateTime when;
		String size;
		String seed;

		float distance = 0.0f;
		int timeInSeconds = 0;
		int deaths = 0;
		int finished = 0;
		int kills = 0;
		int shotsFired = 0;
		int itemsPickedUp = 0;
		Energy experience = Energy::zero();
		Energy experienceFromMission = Energy::zero(); // this is separate (as: this is from the actual objectives)
		int meritPoints = 0; // these can be gained only by doing mission, so no need to differentiate

		GameStats();

		void reset();
		void clear();

		void add_experience(Energy const& _experience);
		void add_experience_from_mission(Energy const& _experienceFromMission);
		void add_merit_points(int _points);
		void killed();
		void shot_fired();
		void picked_up_item(void* _item);

		GameStats& operator=(GameStats const& _src);

		//

		bool load_from_xml_child_node(IO::XML::Node const* _node, tchar const* _childName);
		bool save_to_xml(IO::XML::Node* _node) const;

	private:
		static GameStats s_statsTutorial;
		static GameStats s_statsPilgrimage;
		static GameStats* s_stats;

		mutable Concurrency::SpinLock statsLock = Concurrency::SpinLock(TXT("TeaForGodEmperor.GameStats.statsLock"));

		Array<void*> itemsPickedUpArray;
	};
};
