#pragma once

#include "..\uiCanvasElement.h"

namespace Framework
{
	namespace UI
	{
		interface_class ICanvasElement;
		struct CanvasWindow;

		namespace Utils
		{
			void place_below(Canvas* _c, CanvasWindow* _e, REF_ ICanvasElement*& _below, float _horizontalAlignmentPt, Optional<float> const & _spacing = NP);
			void place_above(Canvas* _c, CanvasWindow* _e, REF_ ICanvasElement*& _above, float _horizontalAlignmentPt, Optional<float> const& _spacing = NP);
			void place_on_left(Canvas* _c, CanvasWindow* _e, REF_ ICanvasElement*& _of, float _verticalAlignmentPt, Optional<float> const& _spacing = NP);
			void place_on_right(Canvas* _c, CanvasWindow* _e, REF_ ICanvasElement*& _of, float _verticalAlignmentPt, Optional<float> const& _spacing = NP);
			void place_below(Canvas* _c, CanvasWindow* _e, Name const & _belowId, float _horizontalAlignmentPt, Optional<float> const & _spacing = NP);
			void place_above(Canvas* _c, CanvasWindow* _e, Name const& _aboveId, float _horizontalAlignmentPt, Optional<float> const& _spacing = NP);
			void place_on_left(Canvas* _c, CanvasWindow* _e, Name const& _ofId, float _verticalAlignmentPt, Optional<float> const& _spacing = NP);
			void place_on_right(Canvas* _c, CanvasWindow* _e, Name const& _ofId, float _verticalAlignmentPt, Optional<float> const& _spacing = NP);
		};
	};
};
