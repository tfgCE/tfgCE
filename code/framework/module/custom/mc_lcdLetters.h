#pragma once

#include "..\moduleCustom.h"
#include "..\..\video\material.h"

namespace Framework
{
	namespace LCDLetter
	{
		enum Type
		{
			Segment7,
			Directional, // each letter is a direction, clockwise starting with up/forward
		};

		Type parse(String const & _value);
	};

	class LCDLetters
	{
	public:
		struct Segment7Letter
		{
			ArrayStatic<bool, 7> on;

			Segment7Letter() { SET_EXTRA_DEBUG_INFO(on, TXT("LCDLetters::Segment7Letter.on")); clear(); }

			void clear() { on.set_size(7); memory_zero(on.begin(), on.get_data_size()); }
		};

		static void initialise_static();
		static void close_static();

		static LCDLetters* get() { return s_lcdLetters; }

		static bool load_from_xml(IO::XML::Node const * _node);

		static Segment7Letter const & get_segment_7(tchar const & _ch);

	private:
		static LCDLetters* s_lcdLetters;
		static Segment7Letter s_segment7Empty;

		Map<tchar, Segment7Letter> segment7Letters;

		static Segment7Letter parse(String const & _text);
	};

	namespace CustomModules
	{
		class LCDLetters
		: public ModuleCustom
		{
			FAST_CAST_DECLARE(LCDLetters);
			FAST_CAST_BASE(ModuleCustom);
			FAST_CAST_END();

			typedef Framework::ModuleCustom base;
		public:
			static Framework::RegisteredModule<Framework::ModuleCustom> & register_itself();

			LCDLetters(Framework::IModulesOwner* _owner);
			virtual ~LCDLetters();

		public:
			void set_content(tchar const * _content, bool _update = true);
			void set_colour(Colour const & _colour, bool _update = true);
			void set_colour(int _letterIndex, Optional<Colour> const & _colour = NP, bool _update = true);
			void clear_individual_colours(bool _update = true);

			void update();

		public:	// Module
			override_ void reset();
			override_ void activate();
			override_ void setup_with(::Framework::ModuleData const * _moduleData);

			override_ void initialise();

		public: // ModuleCustom
			override_ void advance_post(float _deltaTime);

		private:
			LCDLetter::Type type = LCDLetter::Segment7; // hardcoded

			String defaultContent;
			int firstLetterParamIndex;
			Name letterColourParamName; // lcdColours
			Name letterPowerParamName; // lcdPower

			String content;

			float blendTime = 0.0f;

			Colour letterColour = Colour::white;
			Array<Optional<Colour>> letterColours;

			Optional<int> materialIndex; // if not set, to all
			int letterCount = 0;

			Array<float> powerParams;
			Array<Colour> colourParams;
			Array<float> targetPowerParams;
			Array<Colour> targetColourParams;
			Concurrency::SpinLock paramsLock = Concurrency::SpinLock(TXT("Framework.CustomModules.LCDLetters.paramsLock"));

			void update_blend(float _deltaTime);
		};
	};
};

