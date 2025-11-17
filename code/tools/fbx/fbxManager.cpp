#include "fbxManager.h"

#include "..\..\core\memory\memory.h"

#ifdef USE_FBX
using namespace FBX;

Manager* Manager::s_manager = nullptr;

Manager::Manager()
: sdkManager(nullptr)
, isValid(false)
{
	an_assert(s_manager == nullptr);
	s_manager = this;
	initialise();
}

Manager::~Manager()
{
	an_assert(s_manager == this);
	s_manager = nullptr;
	close();
}

bool Manager::initialise_static()
{
	new Manager();
	return s_manager->isValid;
}

void Manager::close_static()
{
	delete s_manager;
}

bool Manager::initialise()
{
	isValid = false;

	sdkManager = FbxManager::Create();
	if (!sdkManager)
	{
		error(TXT("can't create fbx manager"));
		return false;
	}

	FbxIOSettings* ios = FbxIOSettings::Create(sdkManager, IOSROOT);
	sdkManager->SetIOSettings(ios);

	isValid = true;

	return true;
}

void Manager::close()
{
	if (sdkManager)
	{
		sdkManager->Destroy();
	}
}

#endif
