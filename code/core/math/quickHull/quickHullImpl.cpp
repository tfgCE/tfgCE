#include "quickHullImpl.h"

#include "quickHullQuickHull.h"

void QuickHullImpl::perform(float const* _inXYZVertices, int _vertexCount, std::function<void(float _x, float _y, float _z)> _add_point, std::function<void(int _a, int _b, int _c)> _add_triangle, float _eps)
{
	if (_eps < 0.0f)
	{
		_eps = quickhull::defaultEps<float>();
	}
	
	quickhull::QuickHull<float> qhAlgo;
	auto cHull = qhAlgo.getConvexHull(_inXYZVertices, _vertexCount, false, false, _eps);

	for (int i = 0; i < (int)cHull.getVertexBuffer().size(); ++i)
	{
		auto const& vertex = cHull.getVertexBuffer()[i];
		_add_point(vertex.x, vertex.y, vertex.z);
	}

	for (int i = 0; i < (int)cHull.getIndexBuffer().size(); i += 3)
	{
		int idx0 = (int)cHull.getIndexBuffer()[i];
		int idx1 = (int)cHull.getIndexBuffer()[i + 1];
		int idx2 = (int)cHull.getIndexBuffer()[i + 2];
		_add_triangle(idx0, idx1, idx2);
	}
}
