#pragma once

#include "loaderHubScene.h"

#include "..\..\..\core\types\optional.h"

#include "..\..\..\framework\loaders\iLoader.h"

namespace Loader
{
	class Hub;
	class HubScene;

	class HubInSequence
	: public ILoader
	{
		typedef ILoader base;

	public:
		HubInSequence(Hub* _hub, HubScene* _scene, Optional<bool> const & _allowFadeOut = NP);
		virtual ~HubInSequence();

	public: // ILoader
		implement_ void update(float _deltaTime);
		implement_ void display(::System::Video3D* _v3d, bool _vr);
		implement_ bool activate();
		implement_ void deactivate();

	private:
		Hub* hub;
		RefCountObjectPtr<HubScene> scene;
		bool allowFadeOut = true;
	};
};
