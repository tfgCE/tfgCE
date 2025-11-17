#pragma once

#include "fbxLib.h"

#include "..\..\core\debug\debug.h"

#ifdef USE_FBX
namespace FBX
{
	class Manager
	{
	public:
		static bool initialise_static();
		static void close_static();

		static bool is_valid() { return s_manager && s_manager->isValid; }

		static FbxManager* get_sdk_manager() { an_assert(s_manager) return s_manager->sdkManager; }

	protected:
		Manager();
		~Manager();

		bool initialise();
		void close();

	private:
		static Manager* s_manager;

		FbxManager* sdkManager;

		bool isValid;
	};

};
#endif
