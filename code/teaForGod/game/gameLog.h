#pragma once

#include "gameLogEventParams.h"

#include "..\..\core\concurrency\mrswLock.h"
#include "..\..\core\containers\map.h"
#include "..\..\core\types\name.h"

#include "..\..\framework\library\usedLibraryStored.h"

namespace Framework
{
	interface_class IModulesOwner;
};

namespace TeaForGodEmperor
{
	class Tutorial;
	struct Energy;

	class GameLog
	{
	public:
		enum Counter
		{
			InstalledActiveEXM,
			InstalledAutoEnergyTransferEXM,
			UsedActiveEXM,
			AmmoDepleted,
			EnergyUnbalanced,
			TransferKioskDeposit,
			TransferKioskWithdraw,
			EXMsEnergyTransfer,

			MAX
		};
	public:
		struct Entry
		{
			Entry(Name const& _name = Name::invalid());

			Entry & set_as_int(int _param, int _value);
			int get_as_int(int _param) const;

			Entry & set_as_bool(int _param, bool _value);
			bool get_as_bool(int _param) const;

			Entry& set_as_float(int _param, float _value);
			float get_as_float(int _param) const;

			Entry& set_as_name(int _param, Name const & _value);
			Name get_as_name(int _param) const;

			Entry& set_as_energy(int _param, Energy const & _value);
			Energy get_as_energy(int _param) const;

			int get_frame() const { return frame; }
			Name const & get_name() const { return name; }

		private:
			int frame;
			Name name;
			int params[4];

		};

		struct SuggestedTutorial
		{
			int idx = 0;
			bool done = false;
			Tutorial* tutorial = nullptr;
			ArrayStatic<Name, 4> lsReasons;
			int minutesSinceLastPlayed = NONE;
			int howManyTimes = 0;

			static int compare(void const* _a, void const* _b);
		};

		static GameLog& get() { an_assert(s_log); return *s_log; }

		GameLog();
		~GameLog();

		void lookup_tutorials();

		Concurrency::MRSWLock& access_lock() const { return entriesLock; }

	public:
		void clear_and_open();
		void close();

		List<Entry> const& get_entries() const { an_assert(entriesLock.is_acquired()); return entries; }
		Entry& add_entry(Entry const& _entry);
		void increase_counter(GameLog::Counter _counter);

		void check_for_energy_balance(Framework::IModulesOwner* imo);

	public:
		void choose_suggested_tutorials();

		Array<SuggestedTutorial> const& get_suggested_tutorials() const { return suggestedTutorials; }

	private:
		static GameLog* s_log;

		mutable Concurrency::MRSWLock entriesLock;

		bool isOpen = false;
		List<Entry> entries; // list may fragment the memory but should be easier to grow
		Entry emptyEntry;

		Array<SuggestedTutorial> suggestedTutorials;

		Array<int> counters;

		struct EnergyLevelsIssues
		{
			bool runOutOfEnergyHealth = false;
			bool runOutOfEnergyWeapons = false;
			bool energyUnbalanced = false;

			bool did_run_out_of_energy() const { return runOutOfEnergyHealth || runOutOfEnergyWeapons; }
			bool is_it_worth_mentioning() const { return did_run_out_of_energy() && energyUnbalanced; }

			void suggest_tutorial(GameLog* owner, Name const& _name); // will add reasons on its own
		};
		friend struct EnergyLevelsIssues;

		Map<Name, Framework::UsedLibraryStored<TeaForGodEmperor::Tutorial>> tutorials;

		void lookup_tutorial(Name const & _name);

		void suggest_tutorial(Name const& _name, Name const & _lsReason = Name::invalid());

	private:
		struct NamedEntries
		{
			struct Iterator
			{
				Iterator(NamedEntries const* _owner, List<Entry>::ConstIterator const & _iterator) : owner(_owner), iEntry(_iterator) {}

				Iterator& operator ++ ();
				// post
				Iterator operator ++ (int);
				// comparison
				bool operator == (Iterator const& _other) const { return owner == _other.owner && iEntry == _other.iEntry; }
				bool operator != (Iterator const& _other) const { return owner != _other.owner || iEntry != _other.iEntry; }

				Entry const& operator * ()  const { assert_slow(iEntry); return *iEntry; }
				Entry const& operator -> () const { assert_slow(iEntry); return *iEntry; }

			private:
				NamedEntries const* owner;
				List<Entry>::ConstIterator iEntry;
			};

			NamedEntries(List<Entry> const * _entries, Name const& _entryName): entries(_entries), entryName(_entryName) {}

			Iterator const begin() const { return Iterator(this, entries->begin()); }
			Iterator const end() const { return Iterator(this, entries->end()); }

		private:
			List<Entry> const* entries;
			Name entryName;

			friend struct Iterator;
		};

		NamedEntries const all_entries_named(Name const& _name) const { return NamedEntries(&entries, _name); }
		Entry const* find_last(Name const& _name) const;
	};

};
