#pragma once

#include "editorBase.h"

#include "..\..\render\renderCamera.h"

//

namespace Framework
{
	namespace Editor
	{
		class Base2D;

		struct Gizmo2D
		: public RefCountObject
		, public SafeObject<Gizmo2D>
		, public SoftPooledObject<Gizmo2D>
		{
			bool grabbable = true; // if not grabbable, still could be used to display something
			Vector2 atWorld = Vector2::zero;
			Optional<Vector2> fromWorld;
			Optional<Vector2> fromWorld2;
			Optional<Vector2> axisWorld;
			Optional<float> value;
			Optional<float> valueScale;
			Colour colour = Colour::cyan;
			float radius2d = 0.04f;
			float radiusWorld = 0.0f;

			void reset()
			{
				grabbable = true;
				atWorld = Vector2::zero;
				fromWorld.clear();
				fromWorld2.clear();
				axisWorld.clear();
				value.clear();
				valueScale.clear();
				colour = Colour::cyan;
				radius2d = 0.04f;
				radiusWorld = 0.0f;
			}

		protected: // RefCountObject
			override_ void destroy_ref_count_object() { SoftPooledObject<Gizmo2D>::release(); }

		protected: // SoftPooledObject
			override_ void on_get() { reset(); make_safe_object_available(this); }
			override_ void on_release() { reset(); make_safe_object_unavailable(); }

		protected: friend class SoftPooledObject<Gizmo2D>;
				   friend class SoftPool<Gizmo2D>;
			Gizmo2D(): SafeObject<Gizmo2D>(nullptr) {} // will explicitly make it available in "on_get"
			~Gizmo2D() {}
		};

		// main gizmo and two axes
		struct Gizmo2DTwoAxes
		{
		public:
			Vector2 atWorld = Vector2::zero;
			Optional<Vector2> fromWorld;
			Optional<Vector2> fromWorld2;
			Colour colour = Colour::cyan;
			float axesDistance2d = 0.06f;

			bool is_in_use() const { return gizmo.get() != nullptr; }
			void clear();
			void update(Base2D* _base2D);

		protected:
			float gizmoAxesDistance = 0.0f;
			RefCountObjectPtr<Gizmo2D> gizmo;
			RefCountObjectPtr<Gizmo2D> gizmoXY;
			RefCountObjectPtr<Gizmo2D> gizmoX;
			RefCountObjectPtr<Gizmo2D> gizmoY;
		};

		struct Gizmo2DTwoAxesAndRotation
		: public Gizmo2DTwoAxes
		{
		private:
			typedef Gizmo2DTwoAxes base;

		public:
			Rotator3 rotation = Rotator3::zero;

			void clear();
			void update(Base2D* _base2D);

		protected:
			RefCountObjectPtr<Gizmo2D> gizmoYaw;
		};

		class Base2D
		: public Base
		{
			FAST_CAST_DECLARE(Base2D);
			FAST_CAST_BASE(Base);
			FAST_CAST_END();

			typedef Base base;

		public:
			Base2D();
			virtual ~Base2D();

		public:
			override_ void restore_for_use(SimpleVariableStorage const& _setup);
			override_ void store_for_later(SimpleVariableStorage & _setup) const;

		public:
			override_ void process_controls();

			override_ void pre_render(CustomRenderContext* _customRC); // setup camera etc
			override_ void render_main(CustomRenderContext* _customRC); // doesn't do anything right now
			override_ void render_debug(CustomRenderContext* _customRC); // renders actual debug renderer, provide data for it in derived classes

			override_ void setup_ui(REF_ SetupUIContext& _setupUIContext); // create all ui elements (for current mode etc)
			override_ void update_ui_highlight();

		protected:
			virtual Range2 get_grid_range() const;
			virtual Vector2 get_grid_step() const;
			virtual Vector2 get_grid_zero_at() const;
			virtual float get_grid_alpha(float _v, float _step) const;
				
			// ui
		protected:
			virtual void setup_ui_add_to_bottom_left_panel(UI::Canvas* c, UI::CanvasWindow* panel, float scale = 1.0f);

		protected:
			virtual void on_hover(int _cursorIdx, Vector2 const& _rayAt) {}
			virtual void on_press(int _cursorIdx, int _buttonIdx, Vector2 const& _rayAt) {}
			virtual void on_click(int _cursorIdx, int _buttonIdx, Vector2 const& _rayAt) {}
			virtual bool on_hold_early(int _cursorIdx, int _buttonIdx, Vector2 const& _rayAt) { return on_hold(_cursorIdx, _buttonIdx, _rayAt); }
			virtual bool allow_click_after_hold_early(int _cursorIdx, int _buttonIdx) { return true; }
			virtual bool on_hold(int _cursorIdx, int _buttonIdx, Vector2 const& _rayAt); // true if starts holding
			virtual bool on_held(int _cursorIdx, int _buttonIdx, Vector2 const& _rayAt, Vector2 const& _movedBy, Vector2 const& _cursorMovedBy2DNormalised); // true if keeps holding
			virtual void on_release(int _cursorIdx, int _buttonIdx, Vector2 const& _rayAt) {}

		protected:
			virtual void render_edited_asset();

		protected:
			Vector2 cursor_ray_start(Optional<Vector2> const& _cursorAt) const;
			Vector2 cursor_moved_by_2d_normalised_into_3d(Optional<Vector2> const& _cursorMovedBy, Optional<Vector2> const & _inWorld = NP) const; // if _inWorld not specified, will asume: for perspective this is at distance 1, for othogonal doesn't make difference
			Vector2 cursor_moved_by_2d_normalised(Optional<Vector2> const& _cursorMovedBy) const; // normalised to y

		public:
			float calculate_radius_for(Vector2 const& _inWorld, float _2dNormalisedRadius) const;

		private:
			Vector2 reverse_projection_cursor(Optional<Vector2> const& _cursorAt, Optional<float> const& _depth = NP) const; // into camera space
			Vector2 reverse_projection_cursor_2d_normalised(Optional<Vector2> const& _cursorAt, Optional<float> const & _depth = NP) const; // into camera space, cursorAt already in -1,1 range

		protected:
			struct Gizmos
			{
				bool on = true;
				Array<SafePtr<Gizmo2D>> gizmos;
			} gizmos;

		public:
			void add_gizmo(Gizmo2D* g);
			void remove_gizmo(Gizmo2D* g); // but because of use of ref count object, it might be not required
			bool is_gizmo_grabbed(Gizmo2D const * g) const; // by any cursor

		protected:
			Name storeRestoreNames_cameraPivot;
			Name storeRestoreNames_cameraScale;

		protected:
			struct Camera
			{
				Vector2 pivot = Vector2::zero;
				float scale = 0.1f;

				RUNTIME_ Transform at = Transform::identity; // this is for calculations and use, we consider Y being up
				RUNTIME_ Transform renderAt = Transform::identity; // this is for rendering, Z is up
				RUNTIME_ Range zRange = Range(0.01f, 1000.0f);
				RUNTIME_ Matrix44 projectionMatrix = Matrix44::identity;
				RUNTIME_ Vector2 rtSize = Vector2(1000.0f, 1000.0f);

				void zoom(float _by);
				void update_at();
			} camera;

			struct Grid
			{
				bool on = true;
				float size = 10.0f;
				Vector2 start = Vector2::zero;
				Vector2 x = Vector2::xAxis;
				Vector2 y = Vector2::yAxis;
			} grid;

			struct Axes
			{
				bool on = true;
			} axes;

			struct Controls
			{
				struct Cursor
				{
					struct Button
					{
						bool wasDown = false;
						bool isDown = false;

						// current or last (if isDown, timeUp is last time up)
						float timeDown = 0.0f;
						float timeUp = 0.0f;

						bool held = false;
					};
					ArrayStatic<Button, 3> buttons;

					SafePtr<Gizmo2D> hoveringOverGizmo;
					SafePtr<Gizmo2D> grabbedGizmo;
				};
				Array<Cursor> cursors;
			} controls;
			bool hoveringOverAnyGizmo = false;
			bool grabbedAnyGizmo = false;

		};
	}
}