#pragma once

#include "..\navTask.h"

namespace Framework
{
	namespace Nav
	{
		class Mesh;

		namespace Tasks
		{
			class DestroyNavMesh
			: public Nav::Task
			{
				FAST_CAST_DECLARE(DestroyNavMesh);
				FAST_CAST_BASE(Nav::Task);
				FAST_CAST_END();

				typedef Nav::Task base;
			public:
				static Nav::Task* new_task(Nav::Mesh* _navMesh);

			protected:
				implement_ void perform();
				implement_ void cancel_if_related_to(Room* _room);
				implement_ void cancel_if_related_to(Nav::Mesh* _navMesh);
				override_ void cancel();

				implement_ tchar const* get_log_name() const { return TXT("destroy nav mesh"); }

			private:
				DestroyNavMesh(Nav::Mesh* _navMesh);

			private:
				Nav::MeshPtr navMesh;
			};
		};
	};
};
