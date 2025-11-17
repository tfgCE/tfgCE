#pragma once

#include "..\..\..\framework\module\moduleCustom.h"
#include "..\..\..\framework\module\moduleData.h"
#include "..\..\..\core\types\colour.h"
#include "..\..\..\core\system\timeStamp.h"

namespace TeaForGodEmperor
{
	namespace CustomModules
	{
		class MusicAccidentalData;

		struct MusicAccidentalPlay
		{
			// setup
			Name id;
			Name sound; // from sound module
			Name emissive; // from emissive module

			Range cooldownCheck = Range(1.0f, 4.0f); // just for this one - so we won't be checking every few seconds
			Range cooldownIndividual = Range(10.0f, 60.0f); // just for this one
			Range cooldown = Range(30.0f); // in general for id

			bool requiresVisualContact = false;

			Optional<float> maxDistance;
			Optional<bool> movingCloser;

			// running
			::System::TimeStamp cooldownCheckTS;
			::System::TimeStamp cooldownIndividualTS;

			bool load_from_xml(IO::XML::Node const* _node, Framework::LibraryLoadingContext& _lc);

			void reset_cooldown_check_ts();
			void reset_cooldown_individual_ts();
		};

		class MusicAccidental
		: public Framework::ModuleCustom
		{
			FAST_CAST_DECLARE(MusicAccidental);
			FAST_CAST_BASE(Framework::ModuleCustom);
			FAST_CAST_END();

			typedef Framework::ModuleCustom base;
		public:
			static Framework::RegisteredModule<Framework::ModuleCustom> & register_itself();

			MusicAccidental(Framework::IModulesOwner* _owner);
			virtual ~MusicAccidental();

			Array<MusicAccidentalPlay> & access_music() { return music; }
			Array<MusicAccidentalPlay> const& get_music() const { return music; }

		public:	// Module
			override_ void setup_with(::Framework::ModuleData const * _moduleData);

		private:
			MusicAccidentalData const * musicAccidentalData = nullptr;

			Array<MusicAccidentalPlay> music;
		};

		class MusicAccidentalData
		: public Framework::ModuleData
		{
			FAST_CAST_DECLARE(MusicAccidentalData);
			FAST_CAST_BASE(Framework::ModuleData);
			FAST_CAST_END();

			typedef Framework::ModuleData base;
		public:
			MusicAccidentalData(Framework::LibraryStored* _inLibraryStored);
			virtual ~MusicAccidentalData();

		protected: // Framework::ModuleData
			override_ bool read_parameter_from(IO::XML::Node const * _node, Framework::LibraryLoadingContext & _lc);

		private:
			Array<MusicAccidentalPlay> play;

			friend class MusicAccidental;
		};
	};
};

