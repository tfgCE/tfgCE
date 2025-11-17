#pragma once

#include "..\..\library\customLibraryDatas.h"

namespace Framework
{
	namespace MeshGeneration
	{
		namespace CustomData
		{
			template <typename Point> class Spline;

			template <typename Point>
			struct SplineRef
			{
			public:
				void be_copy_of(SplineRef<Point> const& _splineRef);
				Spline<Point>* get() const { return spline.get(); }
				bool load_from_xml(IO::XML::Node const * _node, tchar const * _useSplineAttr, tchar const * _ownSplineChild /* may be nullptr */, LibraryLoadingContext & _lc);
				bool load_from_xml_with_child_provided(IO::XML::Node const * _node, tchar const * _useSplineAttr, IO::XML::Node const * _ownSplineChild, LibraryLoadingContext & _lc);
				bool prepare_for_game(Library* _library, LibraryPrepareContext& _pfgContext);

			private:
				RefCountObjectPtr<Spline<Point>> spline = RefCountObjectPtr<Spline<Point>>(nullptr); // if name is null, this is own spline
				LibraryName useSpline;
			};
		};
	};
};
