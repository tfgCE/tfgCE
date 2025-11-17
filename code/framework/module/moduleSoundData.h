#pragma once

#include "moduleData.h"

#include "..\ai\aiMessageTemplate.h"
#include "..\sound\soundPlayParams.h"

#include "..\..\core\containers\map.h"
#include "..\..\core\other\remapValue.h"

namespace Framework
{
	class Sample;

	struct ModuleSoundSample
	{
		Name id;
		Name as; // samples and play params from other one
		Name paramsAs; // copy params from other one
		bool mayNotExist = false;
		Array<Framework::UsedLibraryStored<Framework::Sample>> samples; // multiple samples
		Name socket;
		Name onStartPlay;
		Name onStopPlay; // if stopped by user, useful for loops
		Transform placement = Transform::identity;
		bool playDetached = false;
		SoundPlayParams playParams;
		bool autoPlay = false;
		Range cooldown = Range::empty;
		Optional<float> chance;
		Tags tags;

		bool remapPitchContinuous = false;
		RemapValue<float, float> remapPitch;
		Name remapPitchUsingVar;

		bool remapVolumeContinuous = false;
		RemapValue<float, float> remapVolume;
		Name remapVolumeUsingVar;

		Array<AI::MessageTemplate> aiMessages; // hearable only by actors!

		bool load_from_xml(IO::XML::Node const * _node, LibraryLoadingContext & _lc);
		bool prepare_for_game(Map<Name, ModuleSoundSample> const & _samples, Library* _library, LibraryPrepareContext& _pfgContext);
	};

	class ModuleSoundData
	: public ModuleData
	{
		FAST_CAST_DECLARE(ModuleSoundData);
		FAST_CAST_BASE(ModuleData);
		FAST_CAST_END();

		typedef ModuleData base;
	public:
		ModuleSoundData(LibraryStored* _inLibraryStored);

	public: // ModuleData
		override_ bool load_from_xml(IO::XML::Node const * _node, LibraryLoadingContext & _lc);
		override_ bool prepare_for_game(Library* _library, LibraryPrepareContext& _pfgContext);

	protected: // ModuleData
		override_ bool read_parameter_from(IO::XML::Node const * _node, LibraryLoadingContext & _lc);

	protected:	friend class ModuleSound;
		Map<Name, ModuleSoundSample> const & get_samples() const { return samples; }

	protected:
		Map<Name, ModuleSoundSample> samples;
	};
};


