#pragma once

#include <functional>

#include "..\utils.h"

#include "..\containers\map.h"

#include "..\debug\logInfoContext.h"

template<typename Class, typename Key>
class FactoryOfNamed
{
public:
	typedef std::function< Class* () > Create;
	typedef std::function< void (LogInfoContext & _log, Key const & _for) > LogDevelopmentInfo;

	static void initialise_static();
	static void close_static();

	static Class * create(Key const & _name);
	static void for_every_key(std::function<bool(Key const & _name)> _do); // return true if done

	static void add(Key const & _name, Create _create);

	static void set_log_development_info(LogDevelopmentInfo);
	static void development_output_all(LogInfoContext& _log);

private:
	static FactoryOfNamed* s_factory;

	Map<Key, Create> registered;
	LogDevelopmentInfo log_development_info = nullptr;
};

#include "factoryOfNamed.inl"
