#pragma once

#include "editorBase.h"

#include "..\..\render\renderCamera.h"

//

namespace Framework
{
	namespace Editor
	{
		class Base3D;

		struct Gizmo3D
		: public RefCountObject
		, public SafeObject<Gizmo3D>
		, public SoftPooledObject<Gizmo3D>
		{
			bool grabbable = true; // if not grabbable, still could be used to display something
			Vector3 atWorld = Vector3::zero;
			Optional<Vector3> fromWorld;
			Optional<Vector3> fromWorld2;
			Optional<Vector3> axisWorld;
			Optional<Vector3> planeNormalWorld;
			Optional<float> value;
			Optional<float> valueScale;
			Colour colour = Colour::cyan;
			float radius2d = 0.04f;
			float radiusWorld = 0.0f;

			void reset()
			{
				grabbable = true;
				atWorld = Vector3::zero;
				fromWorld.clear();
				fromWorld2.clear();
				axisWorld.clear();
				planeNormalWorld.clear();
				value.clear();
				valueScale.clear();
				colour = Colour::cyan;
				radius2d = 0.04f;
				radiusWorld = 0.0f;
			}

		protected: // RefCountObject
			override_ void destroy_ref_count_object() { SoftPooledObject<Gizmo3D>::release(); }

		protected: // SoftPooledObject
			override_ void on_get() { reset(); make_safe_object_available(this); }
			override_ void on_release() { reset(); make_safe_object_unavailable(); }

		protected: friend class SoftPooledObject<Gizmo3D>;
				   friend class SoftPool<Gizmo3D>;
			Gizmo3D(): SafeObject<Gizmo3D>(nullptr) {} // will explicitly make it available in "on_get"
			~Gizmo3D() {}
		};

		// main gizmo and three axes
		struct Gizmo3DThreeAxes
		{
		public:
			Vector3 atWorld = Vector3::zero;
			Optional<Vector3> fromWorld;
			Optional<Vector3> fromWorld2;
			Colour colour = Colour::cyan;
			float axesDistance2d = 0.06f;

			bool is_in_use() const { return gizmo.get() != nullptr; }
			void clear();
			void update(Base3D* _base3D);

		protected:
			float gizmoAxesDistance = 0.0f;
			RefCountObjectPtr<Gizmo3D> gizmo;
			RefCountObjectPtr<Gizmo3D> gizmoX;
			RefCountObjectPtr<Gizmo3D> gizmoY;
			RefCountObjectPtr<Gizmo3D> gizmoZ;
		};

		struct Gizmo3DThreeAxesAndRotation
		: public Gizmo3DThreeAxes
		{
		private:
			typedef Gizmo3DThreeAxes base;

		public:
			Rotator3 rotation = Rotator3::zero;

			void clear();
			void update(Base3D* _base3D);

		protected:
			RefCountObjectPtr<Gizmo3D> gizmoPitch;
			RefCountObjectPtr<Gizmo3D> gizmoYaw;
			RefCountObjectPtr<Gizmo3D> gizmoRoll;
		};

		class Base3D
		: public Base
		{
			FAST_CAST_DECLARE(Base3D);
			FAST_CAST_BASE(Base);
			FAST_CAST_END();

			typedef Base base;

		public:
			Base3D();
			virtual ~Base3D();

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

			// ui
		protected:
			virtual void setup_ui_add_to_bottom_left_panel(UI::Canvas* c, UI::CanvasWindow* panel, float scale = 1.0f);

		protected:
			virtual void on_hover(int _cursorIdx, Vector3 const& _rayStart, Vector3 const& _rayDir) {}
			virtual void on_press(int _cursorIdx, int _buttonIdx, Vector3 const& _rayStart, Vector3 const& _rayDir) {}
			virtual void on_click(int _cursorIdx, int _buttonIdx, Vector3 const& _rayStart, Vector3 const& _rayDir) {}
			virtual bool on_hold_early(int _cursorIdx, int _buttonIdx, Vector3 const& _rayStart, Vector3 const& _rayDir) { return on_hold(_cursorIdx, _buttonIdx, _rayStart, _rayDir); }
			virtual bool allow_click_after_hold_early(int _cursorIdx, int _buttonIdx) { return true; }
			virtual bool on_hold(int _cursorIdx, int _buttonIdx, Vector3 const& _rayStart, Vector3 const& _rayDir);
			virtual bool on_held(int _cursorIdx, int _buttonIdx, Vector3 const& _rayStart, Vector3 const& _rayDir, Vector3 const& _movedBy, Vector2 const& _cursorMovedBy2DNormalised);
			virtual void on_release(int _cursorIdx, int _buttonIdx, Vector3 const& _rayStart, Vector3 const& _rayDir) {}

		protected:
			virtual void render_edited_asset();

		protected:
			Vector3 cursor_ray_start(Optional<Vector2> const& _cursorAt) const;
			Vector3 cursor_ray_dir(Optional<Vector2> const& _cursorAt) const;
			Vector3 cursor_moved_by_2d_normalised_into_3d(Optional<Vector2> const& _cursorMovedBy, Optional<Vector3> const & _inWorld = NP) const; // if _inWorld not specified, will asume: for perspective this is at distance 1, for othogonal doesn't make difference
			Vector2 cursor_moved_by_2d_normalised(Optional<Vector2> const& _cursorMovedBy) const; // normalised to y

		public:
			float calculate_radius_for(Vector3 const& _inWorld, float _2dNormalisedRadius) const;

		private:
			Vector3 reverse_projection_cursor(Optional<Vector2> const& _cursorAt, Optional<float> const& _depth = NP) const; // into camera space
			Vector3 reverse_projection_cursor_2d_normalised(Optional<Vector2> const& _cursorAt, Optional<float> const & _depth = NP) const; // into camera space, cursorAt already in -1,1 range

		protected:
			struct Gizmos
			{
				bool on = true;
				Array<SafePtr<Gizmo3D>> gizmos;
			} gizmos;

		public:
			void add_gizmo(Gizmo3D* g);
			void remove_gizmo(Gizmo3D* g); // but because of use of ref count object, it might be not required
			bool is_gizmo_grabbed(Gizmo3D const* g) const; // by any cursor

		protected:
			struct Camera
			{
				Vector3 pivot = Vector3::zero;
				Vector3 perspectiveUp = Vector3::zAxis;
				Quat perspectiveRotation = Quat::identity;
				float perspectiveDistance = 2.0f;
				float fov = 90.0f;
				bool orthogonal = false;
				Rotator3 orthogonalRotation = Rotator3::zero;
				bool lastOrthogonalTop = true;
				bool lastOrthogonalBack = true;
				bool lastOrthogonalLeft = true;
				int orthogonalAxis = 0;
				float orthogonalScale = 1.0f;

				RUNTIME_ Transform at = Transform::identity;
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
				Vector3 start = Vector3::zero;
				Vector3 x = Vector3::xAxis;
				Vector3 y = Vector3::yAxis;
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

					SafePtr<Gizmo3D> hoveringOverGizmo;
					SafePtr<Gizmo3D> grabbedGizmo;
				};
				Array<Cursor> cursors;
			} controls;
			bool hoveringOverAnyGizmo = false;
			bool grabbedAnyGizmo = false;

		protected:

			void switch_camera_to_perspective(bool _fromOrthogonal);
			void switch_camera_to_orthogonal(Rotator3 const& _rotator, Optional<Rotator3> const& _altRotator = NP);

		};
	}
}