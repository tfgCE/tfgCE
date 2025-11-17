#pragma once

#include "uiCanvasSetup.h"
#include "uiCanvasElements.h"

#include "..\library\usedLibraryStored.h"

#include "..\..\core\math\math.h"
#include "..\..\core\memory\softPooledObject.h"

//

namespace System
{
	class RenderTarget;
	class Video3D;
};

namespace Framework
{
	class Font;

	namespace UI
	{
		interface_class ICanvasElement;
		struct CanvasInputContext;
		struct CanvasRenderContext;

		struct CanvasFont
		{
		public:
			UsedLibraryStored<Font> font;
			Vector2 size;
		};

		class Canvas
		: public SafeObject<Canvas>
		{
			typedef SafeObject<Canvas> base;
		public:
			Concurrency::SpinLock& access_lock() { return lock; }

		public:
			Canvas();
			~Canvas();

			void clear();
			void render(CanvasRenderContext & _context) const;
			void render_on(::System::Video3D* _v3d, ::System::RenderTarget* _rt = nullptr, Optional<Colour> const & _backgroundColour = NP);
			void render_on_output(bool _doesUseVR, ::System::Video3D* _v3d, ::System::RenderTarget* _rt = nullptr, Optional<Colour> const& _backgroundColour = NP);

			void update(CanvasInputContext const& _cic);

		public:
			ICanvasElement* find_by_id(Name const& _id, Optional<int> const & _additionalId = NP) const;

		public:
			void set_logical_size(Vector2 const & _logicalSize) { logicalSize = _logicalSize; update_convertions(); }
			Vector2 const & get_logical_size() const { return logicalSize; }

			void set_default_spacing(Vector2 const& _defaultSpacing) { defaultSpacing = _defaultSpacing; }
			Vector2 const& get_default_spacing() const { return defaultSpacing; }

			void set_real(Vector2 const & _realAnchor, Vector2 const & _realSize) { realAnchor = _realAnchor; realSize = _realSize; update_convertions(); }
			void set_real_size(Vector2 const & _realSize) { realSize = _realSize; update_convertions(); }
			void set_real_anchor(Vector2 const & _realAnchor) { realAnchor = _realAnchor; }
			Vector2 const & get_real_size() { return realSize; }

			Vector2 logical_to_real_location(Vector2 const & _location) const;
			Vector2 logical_to_real_vector(Vector2 const & _vector) const;
			Vector2 logical_to_real_size(Vector2 const & _size) const;
			float logical_to_real_height(float _height) const;

			Vector2 real_to_logical_location(Vector2 const & _location) const;
			Vector2 real_to_logical_vector(Vector2 const & _vector) const;
			Vector2 real_to_logical_size(Vector2 const & _size) const;
			float real_to_logical_height(float _height) const;

		public:
			CanvasSetup const & get_setup() const { return setup; }

			CanvasFont const * get_font_for_logical_size(float _size) const;
			CanvasFont const * get_font_for_real_size(float _size) const;

		public:
			CanvasElements& access_elements() { return elements; }

			void add(ICanvasElement* _element);
			
			bool does_hover_on(ICanvasElement const* _element) const;
			
			bool does_hover_on_or_inside(ICanvasElement const* _element) const;
			
			bool is_cursor_available_outside_canvas(int _cursorIdx) const;
			bool is_modal_window_active() const { return modalWindowActive; }

		private:
			mutable Concurrency::SpinLock lock = Concurrency::SpinLock(TXT("UI.Canvas.lock"));

			Vector2 logicalSize;
			Vector2 realSize;
			Vector2 realAnchor; // 0,0
			Vector2 logicalToRealSize;
			Vector2 logicalToRealLocation;
			
			Vector2 defaultSpacing = Vector2(10.0f, 5.0f);

			CanvasElements elements;
			bool modalWindowActive = false;

			CanvasSetup setup;

			Array<CanvasFont> fonts;

			struct Cursor
			{
				Optional<Vector2> at;
				Optional<Vector2> prevAt;
				::SafePtr<ICanvasElement> hoversOn;
				::SafePtr<ICanvasElement> holding;
				int holdingCustomId = 0;

				struct Button
				{
					// current or last
					bool isDown = false;
					bool wasDown = false;
					float downTime = 0.0f;
					float upTime = 0.0f;
					bool startedHoldOutsideCanvas = false;
				};
				ArrayStatic<Button, 3> buttons;
			};

			ArrayStatic<Cursor, 8> cursors;

			void update_convertions();
		};
	};
};

TYPE_AS_CHAR(Framework::UI::Canvas);