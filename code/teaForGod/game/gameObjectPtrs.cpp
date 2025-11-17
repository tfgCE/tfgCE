#include "gameObjectPtrs.h"

using namespace TeaForGodEmperor;

#ifdef AN_DEVELOPMENT
template <typename Class>
int log_game_object_chain(tchar const * _className)
{
	int count = 0;
	if (Class* go = fast_cast<Class>(Class::get_first_game_object()))
	{
		output(_className);
		while (go)
		{
			output(go->get_game_object_info().to_char());
			go = fast_cast<Class>(go->get_next_game_object());
			++count;
		}
	}
	return count;
}

#define LOG_GAME_OBJECT_CHAIN(Class) log_game_object_chain<Class>(TXT(#Class))

int TeaForGodEmperor::log_game_objects()
{
	int count = 0;
	//count += LOG_GAME_OBJECT_CHAIN(BulletinBoard);
	return count;
}
#endif

bool TeaForGodEmperor::resolve_pending_game_objects()
{
	bool result = true;
	//result &= BulletinBoardPtr::resolve_pending();
	return result;
}
