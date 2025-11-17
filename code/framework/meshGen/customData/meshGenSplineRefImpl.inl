template <typename Point>
bool Framework::MeshGeneration::CustomData::SplineRef<Point>::load_from_xml(IO::XML::Node const * _node, tchar const * _useSplineAttr, tchar const * _ownSplineChild, LibraryLoadingContext & _lc)
{
	return load_from_xml_with_child_provided(_node, _useSplineAttr, (_ownSplineChild == nullptr ? _node : _node->first_child_named(_ownSplineChild)), _lc);
}

template <typename Point>
bool Framework::MeshGeneration::CustomData::SplineRef<Point>::load_from_xml_with_child_provided(IO::XML::Node const * _node, tchar const * _useSplineAttr, IO::XML::Node const * _ownSplineChild, LibraryLoadingContext & _lc)
{
	bool result = true;

	useSpline.load_from_xml(_node, _useSplineAttr, _lc);

	if (!useSpline.is_valid())
	{
		spline = new Spline<Point>();
		if (IO::XML::Node const * node = _ownSplineChild)
		{
			result &= spline->load_from_xml(node, _lc);
		}
		else
		{
			error_loading_xml(_node, TXT("no spline defined (should have own spline or use exiting)"));
			result = false;
		}
	}
	else
	{
		// pointer will be set in prepare
		spline = nullptr;
	}

	return result;
}

template <typename Point>
bool Framework::MeshGeneration::CustomData::SplineRef<Point>::prepare_for_game(Library* _library, LibraryPrepareContext& _pfgContext)
{
	bool result = true;

	if (useSpline.is_valid())
	{
		IF_PREPARE_LEVEL(_pfgContext, LibraryPrepareLevel::Resolve)
		{
			an_assert(!spline.is_set(), TXT("should be deleted by now!"));
			if (CustomLibraryStoredData * found = _library->get_custom_datas().find(useSpline))
			{
				spline = fast_cast<Spline<Point>>(found->access_data());
			}
			if (! spline.is_set())
			{
				error(TXT("couldn't find cross section \"%S\""), useSpline.to_string().to_char());
				result = false;
			}
		}
	}
	else
	{
		an_assert(spline.is_set());
		result &= spline->prepare_for_game(_library, _pfgContext);
	}

	return result;
}

template <typename Point>
void Framework::MeshGeneration::CustomData::SplineRef<Point>::be_copy_of(SplineRef<Point> const& _splineRef)
{
	useSpline = _splineRef.useSpline;
	if (auto* s = _splineRef.spline.get())
	{
		auto* ns = new Spline<Point>();
		*ns = *s;
		spline = ns;
	}
	else
	{
		spline.clear();
	}
}