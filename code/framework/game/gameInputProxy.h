#pragma once

#include "gameInput.h"

namespace Framework
{
	class GameInputProxy
	: public IGameInput
	{
	public:
		GameInputProxy();

		void advance(); // clear for next frame

		void clear();
		void remove_stick(Name const & _name);
		void remove_mouse(Name const & _name);
		void remove_button(Name const & _name);

		void set_stick(Name const & _name, Vector2 const & _v);
		void set_mouse(Name const & _name, Vector2 const & _v);

		void set_button_state(Name const & _name, float const & _v);
		void set_button_state(Name const & _name, bool _v) { set_button_state(_name, _v ? 1.0f : 0.0f); }

	public: // IGameInput
		implement_ bool has_stick(Name const & _name) const;
		implement_ Vector2 get_stick(Name const & _name) const;

		implement_ bool has_mouse(Name const & _name) const;
		implement_ Vector2 get_mouse(Name const & _name) const;

		implement_ float get_button_state(Name const & _name) const;
		implement_ bool has_button(Name const & _name) const;
		implement_ bool is_button_pressed(Name const & _name) const;
		implement_ bool has_button_been_pressed(Name const & _name) const;
		implement_ bool has_button_been_released(Name const & _name) const;

	private:
		struct Stick
		{
			Name name;
			Vector2 value = Vector2::zero;
			Vector2 prevValue = Vector2::zero;
		};
		Array<Stick> sticks;

		struct Mouse
		{
			Name name;
			Vector2 movement = Vector2::zero;
		};
		Array<Mouse> mouses;

		struct Button
		{
			Name name;
			float value = 0.0f;
			float prevValue = 0.0f;
		};
		Array<Button> buttons;
	};

	struct GameInputProxyDefinition
	{
		struct Control
		{
			Name from;
			Name to;
			bool load_from_xml(IO::XML::Node const * _node);
		};

		struct Mouse
		: public Control
		{
			typedef Control base;
			float mul = 1.0f;
			bool load_from_xml(IO::XML::Node const * _node);
		};

		struct Stick
		: public Control
		{
			typedef Control base;
			enum Mode
			{
				Mode_AsIs,
				Mode_MapToRange
			};
			Mode mode = Mode_AsIs;
			Range range = Range(0.0f, 1.0f);
			Name ifButton; // if set and button is not active, stick won't be provided at all
			bool load_from_xml(IO::XML::Node const * _node);
		};

		Array<Stick> sticks;
		Array<Mouse> mouses;
		Array<Control> buttons;

		bool load_from_xml(IO::XML::Node const * _node);
		void process(REF_ GameInputProxy & _proxy, IGameInput const & _from) const;
	};
};
