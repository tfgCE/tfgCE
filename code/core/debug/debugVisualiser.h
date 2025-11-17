#pragma once

struct Colour;
struct Vector3;

#include "..\concurrency\scopedSpinLock.h"
#include "..\concurrency\spinLock.h"
#include "..\containers\array.h"
#include "..\memory\refCountObject.h"
#include "..\types\name.h"
#include "..\types\colour.h"
#include "..\types\optional.h"
#include "..\system\input.h"

Colour get_debug_highlight_colour();
Colour get_debug_highlight_colour(float _pulseLength);

namespace System
{
	class Font;
};

struct DebugVisualiserLine
{
	Vector2 a;
	Vector2 b;
	Colour colour;

	DebugVisualiserLine() {}
	DebugVisualiserLine(Vector2 const & _a, Vector2 const & _b, Colour const & _colour) : a(_a), b(_b), colour(_colour) {}
};

struct DebugVisualiserLine3D
{
	Vector3 a;
	Vector3 b;
	Colour colour;

	DebugVisualiserLine3D() {}
	DebugVisualiserLine3D(Vector3 const & _a, Vector3 const & _b, Colour const & _colour) : a(_a), b(_b), colour(_colour) {}
};

struct DebugVisualiserCircle
{
	Vector2 at;
	float radius;
	Colour colour;

	DebugVisualiserCircle() {}
	DebugVisualiserCircle(Vector2 const & _at, float _radius, Colour const & _colour) : at(_at), radius(_radius), colour(_colour) {}
};

struct DebugVisualiserText
{
	Vector2 a;
	String text;
	Colour colour;
	float scale;
	Vector2 pt;
	VectorInt2 charOffset;

	DebugVisualiserText() {}
	DebugVisualiserText(Vector2 const & _a, String const & _text, Colour const & _colour, float _scale, Vector2 const & _pt, VectorInt2 const & _charOffset) : a(_a), text(_text), colour(_colour), scale(_scale), pt(_pt), charOffset(_charOffset) {}
};

struct DebugVisualiserText3D
{
	Vector3 a;
	String text;
	Colour colour;

	DebugVisualiserText3D() {}
	DebugVisualiserText3D(Vector3 const& _a, String const & _text, Colour const& _colour) : a(_a), text(_text), colour(_colour) {}
};

struct DebugVisualiserLog
{
	Optional<Colour> colour;
	String text;

	DebugVisualiserLog() {}
	DebugVisualiserLog(Optional<Colour> const& _colour, String const& _text) : colour(_colour), text(_text) {}
};
class DebugVisualiser;

typedef RefCountObjectPtr<DebugVisualiser> DebugVisualiserPtr;

class DebugVisualiser
: public RefCountObject
{
public:
	static DebugVisualiser* get_active() { Concurrency::ScopedSpinLock lock(listLock); /* avoid accessing list if it is being modified */ return s_active; }
	static void set_default_font(::System::Font* _font) { s_defaultFont = _font; }

	static bool is_blocked() { return s_block; }
	static void block(bool _block) { s_block = _block; }

public:
	DebugVisualiser(String const & _title = String::empty());
	virtual ~DebugVisualiser();

	void be_3d(bool _in3D = true) { in3D = _in3D; }

	bool is_active() const { return s_active == this; }
	void activate();
	void deactivate();

	virtual void use_font(::System::Font* _font, float _fontHeight = 0.0f);
	::System::Font* get_font() const { return font; }

	void set_background_colour(Colour const & _colour) { backgroundColour = _colour; }
	void set_text_scale(float _textScale) { textScale = _textScale;	}

public:
	void start_gathering_data();
	void clear();
	void add_log(tchar const* _text) { add_log(Optional<Colour>(), String(_text)); }
	void add_log(Optional<Colour> const& _colour, tchar const* _text) { add_log(_colour, String(_text)); }
	void add_log(Optional<Colour> const & _colour, String const & _text);
	void add_text_3d(Vector3 const& _a, String const& _text, Colour const& _colour);
	void add_line_3d(Vector3 const & _a, Vector3 const & _b, Colour const & _colour);
	void add_line(Vector2 const & _a, Vector2 const & _b, Colour const & _colour);
	void add_arrow(Vector2 const & _a, Vector2 const & _b, Colour const & _colour, float _arrowPt = 0.05f);
	void add_circle(Vector2 const & _a, float _radius, Colour const & _colour);
	void add_range2(Range2 const & _r, Colour const& _colour);
	void add_text(Vector2 const & _a, String const & _text, Colour const & _colour, Optional<float> const & _scale = NP, Optional<Vector2> const & _pt = NP, Optional<VectorInt2> const& _charOffset = NP);
	void end_gathering_data();

public:
	void fit_camera();

	bool advance_and_render_if_on_render_thread(); // if we've got debug visualiser running on render thread, returns true if did something

#ifdef AN_STANDARD_INPUT
	void show_and_wait_for_key(::System::Key::Type _key = ::System::Key::Space, ::System::Key::Type _breakKey = ::System::Key::B);
#else
	void show_and_wait_for_key(::System::Key::Type _key = ::System::Key::None, ::System::Key::Type _breakKey = ::System::Key::None);
#endif

public:
	virtual void on_activate();
	virtual void on_deactivate();
	virtual void advance();
	virtual void render();

	void render_and_display();

protected:
	Vector2 const & get_use_view_size() const { return useViewSize; }
	Vector2 get_mouse_at() const;

protected:
	static bool s_block;
	static Concurrency::SpinLock listLock;
	static DebugVisualiser* s_first;
	static DebugVisualiser* s_active;
	static ::System::Font* s_defaultFont;
	DebugVisualiser* prev = nullptr;
	DebugVisualiser* next = nullptr;
	DebugVisualiser* prevActiveStack = nullptr;
	DebugVisualiser* nextActiveStack = nullptr;

	String title;

	bool in3D = false;

	::System::Key::Type requestedKeyInput = ::System::Key::None;
	bool requestedKeyPressed = false;
	float requestedKeyBlink = 0.0f;
	::System::Key::Type breakKeyInput = ::System::Key::None;
	bool breakKeyPressed = false;
	float timeActive = 0.0f;

	::System::Font* font = nullptr;
	float fontHeight = 1.0f;
	float textScale = 1.0f;

	Concurrency::SpinLock dataLock = Concurrency::SpinLock(SYSTEM_SPIN_LOCK); // lock during gathering or displaying
	Array<DebugVisualiserText3D> texts3D;
	Array<DebugVisualiserLine3D> lines3D;
	Array<DebugVisualiserLine> lines;
	Array<DebugVisualiserCircle> circles;
	Array<DebugVisualiserText> texts;
	Array<DebugVisualiserLog> logs;

	Transform transform3D = Transform::identity;
	Colour backgroundColour = Colour::greyLight;
	Vector2 viewSize = Vector2(10.0f, 10.0f);
	Vector2 useViewSize = viewSize;
	static Vector2 camera;
	static float zoom;
	static float wholeScale;

	bool inputWasGrabbed = false;
};