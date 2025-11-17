#pragma once

#include <functional>

namespace QuickHullImpl
{
	void perform(float const* _inXYZVertices, int _vertexCount, std::function<void(float _x, float _y, float _z)> _add_point, std::function<void(int _a, int _b, int _c)> _add_triangle, float _eps = -1.0f);
};
