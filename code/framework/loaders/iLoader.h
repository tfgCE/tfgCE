#pragma once

#include "..\..\core\globalDefinitions.h"

struct Vector2;
struct VectorInt2;
struct Colour;

namespace System
{
	class Video3D;
};

namespace Loader
{
	interface_class ILoader
	{
	public:
		ILoader() : isActive(false) {}
		virtual ~ILoader() {}
		virtual void update(float _deltaTime) = 0;
		virtual void display(::System::Video3D * _v3d, bool _vr) = 0;
		virtual bool activate() { bool wasActive = isActive; isActive = true; return ! wasActive; } // return true if was activated
		virtual void deactivate() { isActive = false; } // derived classes may want to override_ this method and call ILoader::deactivate to actually deactivate
		void force_deactivate() { isActive = false; } // this should be used only in rare cases when we want to stop loader immediately

		bool is_active() const { return isActive; }

	private:
		bool isActive;
	};
};
