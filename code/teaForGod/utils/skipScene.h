#pragma once

#include "..\..\core\memory\refCountObjectPtr.h"

//

namespace TeaForGodEmperor
{
	namespace OverlayInfo
	{
		struct Element;
	};

	struct SkipScene
	{
	public:
		void reset();

		void set_overlay_distance(float _overlayDistance) { overlayDistance = _overlayDistance; }
		void set_overlay_scale(float _scale) { overlayScale = _scale; }
		void set_overlay_pitch(float _pitch) { overlayPitch = _pitch; }

		void prompt_skip_scene();
		bool is_prompting_skip_scene() const { return promptSkipScene > 0.0f; }

		void allow_skip_scene(bool _allowed = true) { allowed = _allowed; }
		bool should_skip_scene() const { return skipScene; }

		void update(float _deltaTime);

	private:
		bool allowed = false;

		bool skipScene = false;

		RefCountObjectPtr<TeaForGodEmperor::OverlayInfo::Element> maySkipPromptOIE;
		RefCountObjectPtr<TeaForGodEmperor::OverlayInfo::Element> maySkipProgressOIE;
		float promptSkipScene = 0.0f;
		float skipSceneProgress = 0.0f;

		// setup
		float overlayDistance = 1.0f;
		float overlayScale = 1.0f;
		float overlayPitch = 15.0f;
	};
};
