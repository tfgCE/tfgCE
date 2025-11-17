#pragma once

#include "..\..\..\core\fastCast.h"
#include "..\..\..\core\types\string.h"

#include "..\..\ui\uiCanvas.h"

//

class SimpleVariableStorage;

//

namespace Framework
{
	class EditorWorkspace;
	class LibraryStored;
	struct CustomRenderContext;

	struct EditorContext;

	namespace UI
	{
		struct CanvasWindow;
		namespace Utils
		{
			struct Grid2Menu;
		}
	};

	namespace Editor
	{
		struct SetupUIContext
		{
			// valid for creation
			UI::ICanvasElement* topLeft = nullptr;
			UI::ICanvasElement* topLeft_right = nullptr;
			UI::ICanvasElement* topLeft_down = nullptr;
			UI::ICanvasElement* topRight = nullptr;
			UI::ICanvasElement* topRight_left = nullptr;
			UI::ICanvasElement* topRight_down = nullptr;
			UI::ICanvasElement* bottomLeft = nullptr;
			UI::ICanvasElement* bottomLeft_right = nullptr;
			UI::ICanvasElement* bottomLeft_up = nullptr;
			UI::ICanvasElement* bottomRight = nullptr;
			UI::ICanvasElement* bottomRight_left = nullptr;
			UI::ICanvasElement* bottomRight_up = nullptr;
		};

		class Base
		: public RefCountObject
		{
			FAST_CAST_DECLARE(Base);
			FAST_CAST_END();

		public:
			Base();
			virtual ~Base();

			Optional<bool> is_input_grabbed() const { return inputGrabbed; }

		public:
			void set_editor_context(EditorContext* _editorContext) { editorContext = _editorContext; }
			EditorContext* get_editor_context() const { return editorContext; }

			UI::Canvas* get_canvas() const;

		public:
			Vector2 get_name_font_scale();

		public:
			virtual void edit(LibraryStored* _asset, String const& _filePathToSave = String::empty());
			LibraryStored* get_edited_asset() const { return editedAsset.get(); }

			// these methods are used when switching between editors + stored for workspace
			virtual void restore_for_use(SimpleVariableStorage const& _setup);
			virtual void store_for_later(SimpleVariableStorage & _setup) const;

		public:
			template<typename Class>
			static void read(SimpleVariableStorage const& _setup, OUT_ Class& _value, Name const& _name);
			template<typename Class>
			static void write(SimpleVariableStorage & _setup, Class const & _value, Name const& _name);

		public:
			virtual void process_controls();

			virtual void pre_render(CustomRenderContext* _customRC); // setup camera etc
			virtual void render_main(CustomRenderContext* _customRC);
			virtual void render_debug(CustomRenderContext* _customRC); // will render debug rendered, if you have anything to add, do it before calling base's render_debug
			virtual void render_ui(CustomRenderContext* _customRC);

			virtual void setup_ui(REF_ SetupUIContext& _setupUIContext); // create all ui elements (for current mode etc)
			virtual void update_ui_highlight();

			virtual void show_asset_props() {}

		protected:
			virtual void setup_ui_add_to_top_name_panel(UI::Canvas* c, UI::CanvasWindow* panel, float scale = 1.0f);

		protected:
			EditorContext* editorContext = nullptr;

			UsedLibraryStored<LibraryStored> editedAsset;
			String filePathToSave;

			Colour backgroundColour = Colour::blue.mul_rgb(0.2f);

			//

			Optional<bool> inputGrabbed;

		protected:
			void add_common_props_to_asset_props(UI::Utils::Grid2Menu& menu);
			void edit_custom_attributes();
		};
	}
}