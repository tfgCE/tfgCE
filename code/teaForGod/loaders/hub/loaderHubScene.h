#pragma once

#include "..\..\..\core\fastCast.h"
#include "..\..\..\core\memory\refCountObject.h"
#include "..\..\..\core\types\name.h"

namespace Framework
{
	class GameInput;

	namespace Render
	{
		class Scene;
		class Context;
	};
};

namespace Loader
{
	class Hub;

	class HubScene
	: public RefCountObject
	{
		FAST_CAST_DECLARE(HubScene);
		FAST_CAST_END();

	public:
		HubScene() {}
		HubScene(Hub* _hub) : hub(_hub) {}
		virtual ~HubScene() {}

		void set_hub(Hub* _hub) { an_assert(!hub || hub == _hub); hub = _hub; }
		Hub* get_hub() const { return hub; }

		bool is_active() const { return isActive; }

		virtual void on_activate(HubScene* _prev) { isActive = true; } // if null, whole hub activated
		virtual void on_update(float _deltaTime) {}
		virtual void on_deactivate(HubScene* _next) { isActive = false; } // if null, no next scene, whole hub deactivated
		virtual void on_loader_deactivate() {} // if loader was told to deactivate
		virtual void on_post_render(Framework::Render::Scene* _scene, Framework::Render::Context & _context) {} // through render scene
		virtual void on_post_render_render_scene(int _eyeIdx) {} // when all rendering is done (render target unbound!)
		virtual void process_input(int _handIdx, Framework::GameInput const& _input, float _deltaTime) {}

		virtual bool does_want_to_end() { return false; }

		void allow_auto_tips(bool _autoTips = true) { autoTips = _autoTips; }
		bool does_allow_auto_tips() const { return autoTips; }

		void disallow_auto_tip(Name const& _id) { disallowAutoTips.push_back_unique(_id); }
		bool does_allow_auto_tip(Name const& _id) const { return ! disallowAutoTips.does_contain(_id); }

	private:
		Hub* hub = nullptr;

		bool isActive = false;

		bool autoTips = false;
		Array<Name> disallowAutoTips;
	};

};
