#include "editorBasicInfo.h"

#include "..\editorContext.h"

#include "..\..\ui\uiCanvasButton.h"
#include "..\..\ui\uiCanvasWindow.h"

#include "..\..\ui\utils\uiGrid2Menu.h"

#include "..\..\..\core\other\simpleVariableStorage.h"

//

#include "editorBaseImpl.inl"

//

#ifdef AN_ALLOW_OPTIMIZE_OFF
#pragma optimize("", off)
#endif

//

using namespace Framework;

//

DEFINE_STATIC_NAME(editorBasicInfo_props);

//

REGISTER_FOR_FAST_CAST(Editor::BasicInfo);

Editor::BasicInfo::BasicInfo()
{
}

Editor::BasicInfo::~BasicInfo()
{
}

void Editor::BasicInfo::show_asset_props()
{
	auto* c = get_canvas();

	if (auto* e = c->find_by_id(NAME(editorBasicInfo_props)))
	{
		e->remove();
		return;
	}

	if (auto* ls = get_edited_asset())
	{
		UI::Utils::Grid2Menu menu;

		auto* window = menu.step_1_create_window(c, 1.0f)->set_title(TXT("basic props"));
		window->set_id(NAME(editorBasicInfo_props));
		window->set_modal(false)->set_closable(true);
		add_common_props_to_asset_props(menu);
		menu.step_3_add_no_button();
		menu.step_3_add_button(TXT("close"))
#ifdef AN_STANDARD_INPUT
			->set_shortcut(System::Key::Esc)
#endif
			->set_on_press([window](Framework::UI::ActContext const& _context)
				{
					window->remove();
				});

		menu.step_4_finalise(c->get_logical_size().x * 0.5f);

		update_ui_highlight();
	}
}
