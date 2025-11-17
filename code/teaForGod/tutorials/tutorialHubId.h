#pragma once

#include "..\..\core\containers\arrayStatic.h"

struct String;
struct Name;

namespace TeaForGodEmperor
{
	struct TutorialHubId
	{
		static int const MAX_LENGTH = 64;
		ArrayStatic<tchar, MAX_LENGTH> id;

		TutorialHubId() {}
		explicit TutorialHubId(String const& _id) { set(_id); }

		bool operator ==(TutorialHubId const& _other) const { return id == _other.id; }
		bool operator ==(Name const& _other) const;

		bool is_set() const { return ! id.is_empty(); }

		void set(String const& _id);
	};
};
