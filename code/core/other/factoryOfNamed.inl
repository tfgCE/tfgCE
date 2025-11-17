
template<typename Class, typename Key>
FactoryOfNamed<Class, Key>* FactoryOfNamed<Class, Key>::s_factory = nullptr;

template<typename Class, typename Key>
void FactoryOfNamed<Class, Key>::initialise_static()
{
	an_assert(s_factory == nullptr);
	s_factory = new FactoryOfNamed();
}

template<typename Class, typename Key>
void FactoryOfNamed<Class, Key>::close_static()
{
	delete_and_clear(s_factory);
}

template<typename Class, typename Key>
Class * FactoryOfNamed<Class, Key>::create(Key const & _name)
{
	an_assert(s_factory);
	if (auto * existing = s_factory->registered.get_existing(_name))
	{
		return (*existing)();
	}
	return nullptr;
}

template<typename Class, typename Key>
void FactoryOfNamed<Class, Key>::add(Key const & _name, Create _create)
{
	an_assert(s_factory);
	s_factory->registered[_name] = _create;
}

template<typename Class, typename Key>
void FactoryOfNamed<Class, Key>::set_log_development_info(LogDevelopmentInfo _log_development_info)
{
	an_assert(s_factory);
	s_factory->log_development_info = _log_development_info;
}

template<typename Class, typename Key>
void FactoryOfNamed<Class, Key>::for_every_key(std::function<bool(Key const& _name)> _do)
{
	for_every(reg, s_factory->registered)
	{
		if (_do(for_everys_map_key(reg)))
		{
			break;
		}
	}
}

template<typename Class, typename Key>
void FactoryOfNamed<Class, Key>::development_output_all(LogInfoContext& _log)
{
	an_assert(s_factory);

	struct AlphabeticalOrder
	{
		Key key;

		AlphabeticalOrder() {}
		explicit AlphabeticalOrder(Key const& _key) : key(_key) {}

		static int compare(void const* _a, void const* _b)
		{
			AlphabeticalOrder a = *plain_cast<AlphabeticalOrder>(_a);
			AlphabeticalOrder b = *plain_cast<AlphabeticalOrder>(_b);
			return ::String::diff_icase(a.key.to_char(), b.key.to_char());
		}
	};
	Array<AlphabeticalOrder> alphabeticalOrder;

	for_every(reg, s_factory->registered)
	{
		alphabeticalOrder.push_back(AlphabeticalOrder(for_everys_map_key(reg)));
	}

	sort(alphabeticalOrder);

	_log.log(TXT("in alphabetical order"));
	LOG_INDENT(_log);
	for_every(ao, alphabeticalOrder)
	{
		_log.log(TXT("%S"), ao->key.to_char());
		if (s_factory->log_development_info)
		{
			LOG_INDENT(_log);
			s_factory->log_development_info(_log, ao->key);
		}
	}
}
