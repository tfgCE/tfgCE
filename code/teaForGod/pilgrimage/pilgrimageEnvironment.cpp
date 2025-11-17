#include "pilgrimageEnvironment.h"

#include "..\..\core\containers\arrayStack.h"
#include "..\..\framework\world\environmentType.h"

#ifdef AN_CLANG
#include "..\..\framework\library\usedLibraryStored.inl"
#endif

using namespace TeaForGodEmperor;

//

bool PilgrimageEnvironment::load_from_xml(IO::XML::Node const * _node, Framework::LibraryLoadingContext & _lc)
{
	bool result = true;

	name = _node->get_name_attribute_or_from_child(TXT("name"), name);
	parent = _node->get_name_attribute_or_from_child(TXT("parent"), parent);
	result &= environment.load_from_xml(_node, TXT("type"), _lc);

	return result;
}

bool PilgrimageEnvironment::prepare_for_game(Framework::Library* _library, Framework::LibraryPrepareContext& _pfgContext)
{
	bool result = true;

	result &= environment.prepare_for_game_find(_library, _pfgContext, Framework::LibraryPrepareLevel::Resolve);

	return result;
}

//

bool PilgrimageEnvironmentCycle::load_from_xml(IO::XML::Node const * _node, Framework::LibraryLoadingContext & _lc)
{
	bool result = true;

	name = _node->get_name_attribute_or_from_child(TXT("name"), name);
	error_loading_xml_on_assert(name.is_valid(), result, _node, TXT("no name provided"));

	mixUsingVar = _node->get_bool_attribute_or_from_child_presence(TXT("mixUsingVar"), mixUsingVar);

	transitionRange.load_from_xml(_node, TXT("transitionRange"));

	putEvenParam = _node->get_name_attribute_or_from_child(TXT("putEvenParam"), putEvenParam);
	putEvenRange.load_from_xml(_node, TXT("putEvenRange"));

	normaliseToCycleCount = _node->get_int_attribute_or_from_child(TXT("normaliseToCycleCount"), normaliseToCycleCount);

	for_every(node, _node->all_children())
	{
		if (node->get_name() == TXT("keepParam"))
		{
			keepParams.push_back(node->get_name(Name::invalid()));
		}
		if (node->get_name() == TXT("putEven"))
		{
			putEvenParam = node->get_name_attribute_or_from_child(TXT("param"), putEvenParam);
			putEvenRange.load_from_xml(node, TXT("range"));
		}
		if (node->get_name() == TXT("environment") ||
			node->get_name() == TXT("environmentMix"))
		{
			RefCountObjectPtr<PilgrimageEnvironmentMix> pem;
			pem = new PilgrimageEnvironmentMix();
			pem->name = name;
			if (pem->load_from_xml(node, _lc))
			{
				error_loading_xml_on_assert(pem->name == name, result, node, TXT("why is name different now?"));
				loadedEnvironments.push_back(pem);
			}
			else
			{
				error_loading_xml(node, TXT("could not load environment info"));
			}
		}
	}

	warn_loading_xml_on_assert(normaliseToCycleCount == 0 || normaliseToCycleCount >= loadedEnvironments.get_size(), _node, TXT("normaliseToCycleCount(%i) should be at least number of loaded environments(%i), otherwise we may miss bits"), normaliseToCycleCount, loadedEnvironments.get_size());

	return result;
}

struct PutEvenSrc
{
	int idx;
	float pt;
	PutEvenSrc() {}
	PutEvenSrc(int _idx, float _pt) : idx(_idx), pt(_pt) {}

	static int compare(void const * _a, void const * _b)
	{
		PutEvenSrc const * a = plain_cast<PutEvenSrc>(_a);
		PutEvenSrc const * b = plain_cast<PutEvenSrc>(_b);
		if (a->pt < b->pt)
		{
			return A_BEFORE_B;
		}
		if (a->pt > b->pt)
		{
			return B_BEFORE_A;
		}
		return A_AS_B;
	}
};

bool PilgrimageEnvironmentCycle::prepare_for_game(Framework::Library* _library, Framework::LibraryPrepareContext& _pfgContext)
{
	bool result = true;

	for_every_ref(e, loadedEnvironments)
	{
		result &= e->prepare_for_game(_library, _pfgContext);
	}

	if (_pfgContext.get_current_level() == Framework::LibraryPrepareLevel::Resolve)
	{
		if (putEvenParam.is_valid())
		{
			environments.clear();

			int eCount = loadedEnvironments.get_size();
			if (normaliseToCycleCount > 1)
			{
				eCount = normaliseToCycleCount;
			}

			ARRAY_STACK(PutEvenSrc, peSrc, loadedEnvironments.get_size());
			for_every_ref(srcE, loadedEnvironments)
			{
				peSrc.push_back(PutEvenSrc(for_everys_index(srcE), putEvenRange.mod_into(srcE->calculate_float_param(putEvenParam))));
			}
			sort(peSrc);

			for_count(int, i, eCount)
			{
				float shouldBe = putEvenRange.get_at((float)i / (float)eCount);
				// find between which environments shouldBe is to add it to the mix evenly
				// between last and first
				int prev = peSrc.get_size() - 1;
				int next = 0;
				for_count(int, n, peSrc.get_size() - 1)
				{
					if (shouldBe >= peSrc[n].pt && shouldBe < peSrc[n + 1].pt)
					{
						prev = n;
						next = n + 1;
					}
				}
				float prevPt = peSrc[prev].pt;
				float nextPt = peSrc[prev].pt + mod(peSrc[next].pt - peSrc[prev].pt, putEvenRange.length()); // next has to be greater than prev (to calculate mix properly)

				float useNext = (shouldBe - prevPt) / (nextPt - prevPt);

				// try to use single one
				float const threshold = 0.01f;
				if (useNext <= threshold)
				{
					useNext = 0.0f;
				}
				else if (useNext >= 1.0f - threshold)
				{
					useNext = 1.0f;
				}

				{
					RefCountObjectPtr<PilgrimageEnvironmentMix> newMix;
					newMix = new PilgrimageEnvironmentMix();
					newMix->name = name;
					newMix->add_mix(loadedEnvironments[prev].get(), 1.0f - useNext);
					newMix->add_mix(loadedEnvironments[next].get(), useNext);
					environments.push_back(newMix);
				}
			}
		}
		else
		{
			for_every_ref(srcE, loadedEnvironments)
			{
				RefCountObjectPtr<PilgrimageEnvironmentMix> newMix;
				newMix = new PilgrimageEnvironmentMix();
				newMix->name = name;
				newMix->add_mix(srcE, 1.0f);
				environments.push_back(newMix);
			}
		}
	}

	return result;
}

//

bool PilgrimageEnvironmentMix::load_from_xml(IO::XML::Node const * _node, Framework::LibraryLoadingContext & _lc)
{
	bool result = true;

	name = _node->get_name_attribute_or_from_child(TXT("name"), name);

	if (_node->first_child_named(TXT("environment")))
	{
		for_every(node, _node->children_named(TXT("environment")))
		{
			Entry entry;
			if (entry.load_from_xml(node, _lc))
			{
				entries.push_back(entry);
			}
			else
			{
				error_loading_xml(node, TXT("could not load pilgrimage part environment"));
				result = false;
			}
		}
	}
	else if (_node->has_attribute(TXT("type")))
	{
		Entry entry;
		entry.mix = 1.0f;
		if (entry.load_from_xml(_node, _lc))
		{
			entries.push_back(entry);
		}
		else
		{
			error_loading_xml(_node, TXT("could not load pilgrimage part environment"));
			result = false;
		}
	}
	else
	{
		error_loading_xml(_node, TXT("no environments defined"));
		result = false;
	}

	return result;
}

bool PilgrimageEnvironmentMix::prepare_for_game(Framework::Library* _library, Framework::LibraryPrepareContext& _pfgContext)
{
	bool result = true;

	for_every(entry, entries)
	{
		result &= entry->prepare_for_game(_library, _pfgContext);
	}

	return result;
}

float PilgrimageEnvironmentMix::calculate_float_param(Name const & _param) const
{
	float result = 0.0f;
	float mixSum = 0.0f;

	for_every(entry, entries)
	{
		if (auto * param = entry->environment->get_info().get_params().get_param(_param))
		{
			mixSum += entry->mix;
			result += param->as<float>() * entry->mix;
		}
	}

	if (mixSum != 0.0f)
	{
		result /= mixSum;
	}

	return result;
}

void PilgrimageEnvironmentMix::add_mix(PilgrimageEnvironmentMix const * _mix, float _mixed)
{
	_mixed = clamp(_mixed, 0.0f, 1.0f);
	if (_mixed <= 0.0f)
	{
		// why should we mix?
		return;
	}
	for_every(srcE, _mix->entries)
	{
		bool found = false;
		for_every(entry, entries)
		{
			if (entry->environment == srcE->environment)
			{
				entry->mix += srcE->mix * _mixed;
				found = true;
				break;
			}
		}
		if (!found)
		{
			entries.push_back(Entry(srcE->environment, srcE->mix * _mixed));
		}
	}
}

//

bool PilgrimageEnvironmentMix::Entry::load_from_xml(IO::XML::Node const * _node, Framework::LibraryLoadingContext & _lc)
{
	bool result = true;

	result &= environment.load_from_xml(_node, TXT("type"), _lc);
	mix = _node->get_float_attribute_or_from_child(TXT("mix"), mix);

	return result;
}

bool PilgrimageEnvironmentMix::Entry::prepare_for_game(Framework::Library* _library, Framework::LibraryPrepareContext& _pfgContext)
{
	bool result = true;

	result &= environment.prepare_for_game_find(_library, _pfgContext, Framework::LibraryPrepareLevel::Resolve);

	return result;
}
