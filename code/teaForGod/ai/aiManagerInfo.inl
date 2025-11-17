template <typename Class>
Class const * ManagerInfo::get_data(Name const & _name) const
{
	for_every_ref(md, datas)
	{
		if (md->get_name() == _name)
		{
			if (Class const * data = fast_cast<Class>(md))
			{
				return data;
			}
		}
	}
	return nullptr;
}
