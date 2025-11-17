#pragma once

#include "..\..\core\memory\refCountObjectPtr.h"

namespace Framework
{
	namespace Nav
	{
		class Node;
		typedef RefCountObjectPtr<Node> NodePtr;

		class Mesh;
		typedef RefCountObjectPtr<Mesh> MeshPtr;

		class Task;
		typedef RefCountObjectPtr<Task> TaskPtr;

		class Path;
		typedef RefCountObjectPtr<Path> PathPtr;
	}
}