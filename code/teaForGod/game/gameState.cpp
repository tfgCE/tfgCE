#include "gameState.h"

#include "..\pilgrimage\pilgrimage.h"

#include "..\..\framework\library\library.h"
#include "..\..\framework\library\usedLibraryStored.inl"

//

#ifdef AN_ALLOW_OPTIMIZE_OFF
#pragma optimize("", off)
#endif

//

using namespace TeaForGodEmperor;

//

GameState::GameState()
{
	gear = new PilgrimGear();
}

GameState::~GameState()
{
}

bool GameState::load_from_xml(IO::XML::Node const* _node)
{
	bool result = true;

	gameStats.clear();
	gameModifiers.clear();
	pilgrimageState.clear();
	pilgrimageVariables.clear();
	storyFacts.clear();
	gear = new PilgrimGear();

	//

	result &= when.load_from_xml(_node->first_child_named(TXT("when")));

	result &= gameStats.load_from_xml_child_node(_node, TXT("stats"));
	
	result &= gameDefinition.load_from_xml_child_node(_node, TXT("gameDefinition"), TXT("use"));
	gameSubDefinitions.clear();
	for_every(n, _node->children_named(TXT("gameSubDefinition")))
	{
		Framework::UsedLibraryStored<GameSubDefinition> gsd;
		gsd.load_from_xml(n, TXT("use"));
		gameSubDefinitions.push_back(gsd);
	}
	result &= pilgrimage.load_from_xml_child_node(_node, TXT("pilgrimage"), TXT("use"));

	result &= gameModifiers.load_from_xml_attribute_or_child_node(_node, TXT("gameModifiers"));

	result &= storyFacts.load_from_xml_attribute_or_child_node(_node, TXT("storyFacts"));

	//

	result &= pilgrimageState.load_from_xml_child_node(_node, TXT("pilgrimageState"));
	result &= pilgrimageVariables.load_from_xml_child_node(_node, TXT("pilgrimageVariables"));

	openWorld = OpenWorld();
	for_every(node, _node->children_named(TXT("openWorld")))
	{
		for_every(ncont, node->children_named(TXT("unobfuscatedPilgrimageDevices")))
		{
			for_every(n, ncont->children_named(TXT("unobfuscated")))
			{
				Framework::UsedLibraryStored<PilgrimageDevice> opd;
				if (opd.load_from_xml(n, TXT("pilgrimageDevice")))
				{
					openWorld.unobfuscatedPilgrimageDevices.push_back(opd);
				}
				else
				{
					error_loading_xml(n, TXT("can't load unobfuscated pilgrimage device"));
					result = false;
				}
			}
		}

		for_every(ncont, node->children_named(TXT("cells")))
		{
			for_every(n, ncont->children_named(TXT("cell")))
			{
				OpenWorld::Cell c;
				c.at.load_from_xml(n);
				for_every(kn, n->children_named(TXT("knownPilgrimageDevices")))
				{
					PilgrimageInstanceOpenWorld::KnownPilgrimageDevice kpd;
					kpd.depleted = kn->get_bool_attribute_or_from_child_presence(TXT("depleted"), kpd.depleted);
					kpd.visited = kn->get_bool_attribute_or_from_child_presence(TXT("visited"), kpd.visited);
					kpd.device.load_from_xml(kn, TXT("device"));
					kpd.stateVariables.load_from_xml_child_node(kn, TXT("state"));
					c.knownPilgrimageDevices.push_back(kpd);
				}
				openWorld.cells.push_back(c);
			}
		}

		struct LoadCellInfos
		{
			int temp;
			static bool load(IO::XML::Node const* node, tchar const * _what, Array<VectorInt2>& _where)
			{
				bool result = true;
				for_every(ncont, node->children_named(_what))
				{
					for_every(n, ncont->children_named(TXT("cell")))
					{
						VectorInt2 at = VectorInt2::zero;
						at.load_from_xml(n);
						_where.push_back(at);
					}
				}
				return result;
			}
		};
		LoadCellInfos::load(node, TXT("knownExitsCells"), openWorld.knownExitsCells);
		LoadCellInfos::load(node, TXT("knownDevicesCells"), openWorld.knownDevicesCells);
		LoadCellInfos::load(node, TXT("knownSpecialDevicesCells"), openWorld.knownSpecialDevicesCells);
		LoadCellInfos::load(node, TXT("knownCells"), openWorld.knownCells);
		LoadCellInfos::load(node, TXT("visitedCells"), openWorld.visitedCells);

		for_every(n, node->children_named(TXT("longDistanceDetection")))
		{
			Concurrency::ScopedSpinLock lock(openWorld.longDistanceDetection.lock);
			openWorld.longDistanceDetection.clear();
			for_every(nd, n->children_named(TXT("at")))
			{
				if (openWorld.longDistanceDetection.detections.has_place_left())
				{
					openWorld.longDistanceDetection.detections.push_back();
					auto& d = openWorld.longDistanceDetection.detections.get_last();
					d.at.load_from_xml(nd);
					for_every(ne, nd->children_named(TXT("detected")))
					{
						if (d.elements.has_place_left())
						{
							d.elements.push_back();
							auto& e = d.elements.get_last();
							e.radius = ne->get_float_attribute(TXT("radius"), -1.0f);
							e.directionTowards.load_from_xml(ne, TXT("directionTowards"));
							e.deviceAt.load_from_xml(ne);
						}
					}
				}
			}
		}
	}

	for_every(node, _node->children_named(TXT("killedInfos")))
	{
		for_every(n, node->children_named(TXT("killedInfo")))
		{
			PilgrimageKilledInfo pki;
			pki.worldAddress.parse(n->get_string_attribute(TXT("address")));
			pki.poiIdx = n->get_int_attribute(TXT("poiIdx"), pki.poiIdx);
			killedInfos.push_back(pki);
		}
	}

	//

	for_every(node, _node->children_named(TXT("gear")))
	{
		result &= gear->load_from_xml(node);
	}

	an_assert(result);

	return result;
}

bool GameState::save_to_xml(IO::XML::Node * _node) const
{
	bool result = true;

	if (!gameDefinition.get() ||
		!pilgrimage.get())
	{
		return false;
	}
	result &= when.save_to_xml(_node->add_node(TXT("when")));
	result &= gameStats.save_to_xml(_node->add_node(TXT("stats")));
	if (auto* n = _node->add_node(TXT("gameDefinition")))
	{
		n->set_attribute(TXT("use"), gameDefinition->get_name().to_string().to_char());
	}
	for_every_ref(gsd, gameSubDefinitions)
	{
		if (auto* n = _node->add_node(TXT("gameSubDefinition")))
		{
			n->set_attribute(TXT("use"), gsd->get_name().to_string().to_char());
		}
	}
	if (auto* n = _node->add_node(TXT("pilgrimage")))
	{
		n->set_attribute(TXT("use"), pilgrimage->get_name().to_string().to_char());
	}
	if (auto* n = _node->add_node(TXT("gameModifiers")))
	{
		n->add_text(gameModifiers.to_string());
	}
	if (auto* n = _node->add_node(TXT("storyFacts")))
	{
		n->add_text(storyFacts.to_string());
	}

	//

	pilgrimageState.save_to_xml_child_node(_node, TXT("pilgrimageState"));
	pilgrimageVariables.save_to_xml_child_node(_node, TXT("pilgrimageVariables"));

	if (auto* own = _node->add_node(TXT("openWorld")))
	{
		if (auto* csn = own->add_node(TXT("unobfuscatedPilgrimageDevices")))
		{
			for_every(opd, openWorld.unobfuscatedPilgrimageDevices)
			{
				if (auto* n = csn->add_node(TXT("unobfuscated")))
				{
					n->set_attribute(TXT("pilgrimageDevice"), opd->get_name().to_string().to_char());
				}
			}
		}
		if (auto* csn = own->add_node(TXT("cells")))
		{
			for_every(c, openWorld.cells)
			{
				if (auto* cn = csn->add_node(TXT("cell")))
				{
					c->at.save_to_xml(cn);
					for_every(kpd, c->knownPilgrimageDevices)
					{
						if (auto* kpdn = cn->add_node(TXT("knownPilgrimageDevices")))
						{
							if (kpd->depleted)
							{
								kpdn->add_node(TXT("depleted"));
							}
							if (kpd->visited)
							{
								kpdn->add_node(TXT("visited"));
							}
							if (!kpd->device.get() && !kpd->device.get_name().is_valid())
							{
								error(TXT("missing device type"));
								return false;
							}
							kpdn->set_attribute(TXT("device"), kpd->device.get_name().to_string().to_char());
							kpd->stateVariables.save_to_xml_child_node(kpdn, TXT("state"));
						}
					}
				}
			}
		}

		struct SaveCellInfos
		{
			int temp;
			static bool save(IO::XML::Node * node, tchar const* _what, Array<VectorInt2> const & _where)
			{
				bool result = true;
				if (auto* n = node->add_node(_what))
				{
					for_every(w, _where)
					{
						if (auto* cn = n->add_node(TXT("cell")))
						{
							w->save_to_xml(cn);
						}
					}
				}
				return result;
			}
		};
		SaveCellInfos::save(own, TXT("knownExitsCells"), openWorld.knownExitsCells);
		SaveCellInfos::save(own, TXT("knownDevicesCells"), openWorld.knownDevicesCells);
		SaveCellInfos::save(own, TXT("knownSpecialDevicesCells"), openWorld.knownSpecialDevicesCells);
		SaveCellInfos::save(own, TXT("knownCells"), openWorld.knownCells);
		SaveCellInfos::save(own, TXT("visitedCells"), openWorld.visitedCells);

		if (auto* n = own->add_node(TXT("longDistanceDetection")))
		{
			Concurrency::ScopedSpinLock lock(openWorld.longDistanceDetection.lock);
			for_every(d, openWorld.longDistanceDetection.detections)
			{
				if (auto* nd = n->add_node(TXT("at")))
				{
					d->at.save_to_xml(nd);

					for_every(e, d->elements)
					{
						if (auto* ne = nd->add_node(TXT("detected")))
						{
							ne->set_float_attribute(TXT("radius"), e->radius);
							if (e->directionTowards.is_set())
							{
								e->directionTowards.get().save_to_xml_child_node(ne, TXT("directionTowards"));
							}
							e->deviceAt.save_to_xml(ne);
						}
					}
				}
			}
		}
	}

	if (auto* kis = _node->add_node(TXT("killedInfos")))
	{
		for_every(ki, killedInfos)
		{
			if (auto* n = kis->add_node(TXT("killedInfo")))
			{
				n->set_attribute(TXT("address"), ki->worldAddress.to_string());
				n->set_int_attribute(TXT("poiIdx"), ki->poiIdx);
			}
		}
	}

	//

	if (auto* node = _node->add_node(TXT("gear")))
	{
		result &= gear->save_to_xml(node);
	}

	an_assert(result);

	return result;
}

bool GameState::resolve_library_links()
{
	bool result = true;

	// allow some things to fail as objects or some unlocks could have changed
	// only the definition and pilgrimage is important as they have to be there

	/* result &= */gear->resolve_library_links();

	if (auto* lib = Framework::Library::get_current())
	{
		for_every(c, openWorld.cells)
		{
			for_every(kpd, c->knownPilgrimageDevices)
			{
				/* result &= */kpd->device.find_may_fail(lib);
			}
		}
		for_every(opd, openWorld.unobfuscatedPilgrimageDevices)
		{
			/* result &= */opd->find_may_fail(lib);
		}
		result &= gameDefinition.find_may_fail(lib);
		for_every(gsd, gameSubDefinitions)
		{
			result &= gsd->find_may_fail(lib);
		}
		result &= pilgrimage.find_may_fail(lib);
	};

#ifdef AN_DEVELOPMENT_OR_PROFILER
	if (Framework::Library::may_ignore_missing_references())
	{
		result = true;
	}
#endif

#ifdef AN_DEVELOPMENT_OR_PROFILER
	if (!Framework::Library::is_skip_loading() && ! result)
	{
		error_dev_investigate(TXT("couldn't resolve links for game state"));
	}
#else
	{
		warn_dev_ignore(TXT("couldn't resolve links for game state"));
	}
#endif

	return result;
}

void GameState::debug_log(tchar const* _nameInfo)
{
	IO::XML::Document doc;
	IO::XML::Node* node = doc.get_root()->add_node(_nameInfo);
	save_to_xml(node);
	String asString = node->to_string_slow();
	List<String> lines;
	asString.split(String(TXT("\n")), lines);
	LogInfoContext log;
	for_every(line, lines)
	{
		log.log(line->to_char());
	}
	log.output_to_output();
}
