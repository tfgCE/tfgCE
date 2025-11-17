#pragma once

#include "..\..\framework\library\libraryStored.h"

namespace Framework
{
	class EnvironmentType;
};

namespace TeaForGodEmperor
{
	class PilgrimageEnvironmentCycle;

	struct PilgrimageEnvironment
	{
		Name name;
		Name parent;
		Framework::UsedLibraryStored<Framework::EnvironmentType> environment;

		bool load_from_xml(IO::XML::Node const * _node, Framework::LibraryLoadingContext & _lc);
		bool prepare_for_game(Framework::Library* _library, Framework::LibraryPrepareContext& _pfgContext);
	};

	class PilgrimageEnvironmentMix
	: public RefCountObject
	{
	public:
		struct Entry
		{
			Framework::UsedLibraryStored<Framework::EnvironmentType> environment;
			float mix = 1.0f;

			Entry() {}
			Entry(Framework::UsedLibraryStored<Framework::EnvironmentType> const & _environment, float _mix) : environment(_environment), mix(_mix) {}

			bool load_from_xml(IO::XML::Node const * _node, Framework::LibraryLoadingContext & _lc);
			bool prepare_for_game(Framework::Library* _library, Framework::LibraryPrepareContext& _pfgContext);
		};

		bool load_from_xml(IO::XML::Node const * _node, Framework::LibraryLoadingContext & _lc);
		bool prepare_for_game(Framework::Library* _library, Framework::LibraryPrepareContext& _pfgContext);

		Name const & get_name() const { return name; }
		Array<Entry> const & get_entries() const { return entries; }

	private:
		Name name;
		Array<Entry> entries;

		float calculate_float_param(Name const & _param) const;

		void add_mix(PilgrimageEnvironmentMix const * _mix, float _mixed);

		friend class PilgrimageEnvironmentCycle;
	};

	/*
	 *	Environment cycle mixes environments into one existing (!) environment - it has to be provided as a normal environment and values will just get copied into it
	 */
	class PilgrimageEnvironmentCycle
	: public RefCountObject
	{
	public:
		bool load_from_xml(IO::XML::Node const * _node, Framework::LibraryLoadingContext & _lc);
		bool prepare_for_game(Framework::Library* _library, Framework::LibraryPrepareContext& _pfgContext);

		Name const & get_name() const { return name; }
		Array<RefCountObjectPtr<PilgrimageEnvironmentMix>> const & get_environments() const { return environments; }

		Array<Name> const & get_keep_params() const { return keepParams; }

		Range const & get_transition_range() const { return transitionRange; }

		bool should_mix_using_var() const { return mixUsingVar; }

	private:
		Name name; // should match an existing environment in pilgrimage
		bool mixUsingVar = false; // will be mixed using var provided by external means to pilgrimage
		Name putEvenParam; // used to put environments evenly using specified param
		Range putEvenRange = Range(0.0f, 1.0f);
		int normaliseToCycleCount = 0; // if greater than 1, bases on loaded environments. it should be at least number of environments
		Array<RefCountObjectPtr<PilgrimageEnvironmentMix>> loadedEnvironments;
		Array<RefCountObjectPtr<PilgrimageEnvironmentMix>> environments; // they are placed evenly
		Array<Name> keepParams; // variables/params to not mix, this allows tu use one main environment and keep some values from it while others are adjusted
		Range transitionRange = Range(0.0f, 1.0f); // transition changes during this, if this is (0.4, 0.6) pt@0.4->0.0, pt@0.5->0.5, pt@0.6->1.0, useful for sharp transforms

	};
};
