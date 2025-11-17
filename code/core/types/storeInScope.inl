template <typename Element>
StoreInScope<Element>::StoreInScope(Element & _source)
: source(_source)
, copy(_source)
{
}

template <typename Element>
StoreInScope<Element>::~StoreInScope()
{
	// restore source
	source = copy;
}
