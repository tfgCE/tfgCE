Transform ModuleAppearance::get_ms_to_ws_transform() const
{
	return get_os_to_ws_transform().to_world(get_ms_to_os_transform());
}

Transform ModuleAppearance::get_os_to_ws_transform() const
{
	an_assert(get_owner() && get_owner()->get_presence());
	return get_owner()->get_presence()->get_os_to_ws_transform();
}

Transform ModuleAppearance::get_ms_to_os_transform() const
{
	return meshInstance.get_placement();
}
