template <typename Element>
ScopedDelete<Element>::ScopedDelete(Element * _obj)
: obj(_obj)
{
	an_assert(obj);
}

template <typename Element>
ScopedDelete<Element>::~ScopedDelete()
{
	delete obj;
}
