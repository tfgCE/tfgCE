Tag::Tag()
{
}

Tag::Tag(Name const & _tag, float _relevance)
: tag( _tag )
{
	setup(_relevance);
}

Tag::Tag(Name const & _tag, int _relevance)
: tag(_tag)
{
	
	setup(_relevance);
}

//

Tags::Tags()
: baseTags( nullptr )
{
	SET_EXTRA_DEBUG_INFO(tags, TXT("Tags.tags"));
}

void Tags::base_on(Tags const * _other)
{
	baseTags = _other;
}

Tags & Tags::remove_tags(Tags const & _tags)
{
#ifdef USE_MORE_TAGS
	if (_tags.useMoreTags)
	{
		for_every(tag, _tags.moreTags)
		{
			if (tag->get_relevance() != 0.0f)
			{
				remove_tag(tag->get_tag());
			}
		}
	}
	else
#endif
	{
		for_every(tag, _tags.tags)
		{
			if (tag->get_relevance() != 0.0f)
			{
				remove_tag(tag->get_tag());
			}
		}
	}
	return *this;
}

Tags & Tags::remove_tag(Name const & _tag)
{
#ifdef USE_MORE_TAGS
	if (useMoreTags)
	{
		for_every(tag, moreTags)
		{
			if (tag->get_tag() == _tag)
			{
				moreTags.remove_fast_at(for_everys_index(tag));
				break;
			}
		}
	}
	else
#endif
	{
		for_every(tag, tags)
		{
			if (tag->get_tag() == _tag)
			{
				tags.remove_fast_at(for_everys_index(tag));
				break;
			}
		}
	}
#ifdef AN_NAT_VIS
	update_for_nat_vis();
#endif
	return *this;
}

Tags & Tags::set_tag(Name const & _tag, float _relevance)
{
	bool set = false;
#ifdef USE_MORE_TAGS
	if (useMoreTags)
	{
		for_every(tag, moreTags)
		{
			if (tag->get_tag() == _tag)
			{
				tag->setup(_relevance);
				set = true;
				break;
			}
		}
	}
	else
#endif
	{
		for_every(tag, tags)
		{
			if (tag->get_tag() == _tag)
			{
				tag->setup(_relevance);
				set = true;
				break;
			}
		}
	}
	if (!set)
	{
#ifdef USE_MORE_TAGS
		if (useMoreTags)
		{
			moreTags.push_back(Tag(_tag, _relevance));
		}
		else
		{
			if (tags.has_place_left())
			{
#endif
				tags.push_back(Tag(_tag, _relevance));
#ifdef USE_MORE_TAGS
			}
			else
			{
				useMoreTags = true;
				moreTags.make_space_for(tags.get_size() + 1);
				for_every(tag, tags)
				{
					moreTags.push_back(*tag);
				}
				moreTags.push_back(Tag(_tag, _relevance));
			}
		}
#endif
	}
#ifdef AN_NAT_VIS
	update_for_nat_vis();
#endif
	return *this;
}

Tags & Tags::set_tag(Name const & _tag, int _relevance)
{
	bool set = false;
#ifdef USE_MORE_TAGS
	if (useMoreTags)
	{
		for_every(tag, moreTags)
		{
			if (tag->get_tag() == _tag)
			{
				tag->setup(_relevance);
				set = true;
				break;
			}
		}
	}
	else
#endif
	{
		for_every(tag, tags)
		{
			if (tag->get_tag() == _tag)
			{
				tag->setup(_relevance);
				set = true;
				break;
			}
		}
	}
	if (!set)
	{
#ifdef USE_MORE_TAGS
		if (useMoreTags)
		{
			moreTags.push_back(Tag(_tag, _relevance));
		}
		else
		{
			if (tags.has_place_left())
			{
#endif
				tags.push_back(Tag(_tag, _relevance));
#ifdef USE_MORE_TAGS
			}
			else
			{
				useMoreTags = true;
				moreTags.make_space_for(tags.get_size() + 1);
				for_every(tag, tags)
				{
					moreTags.push_back(*tag);
				}
				moreTags.push_back(Tag(_tag, _relevance));
			}
		}
#endif
	}
#ifdef AN_NAT_VIS
	update_for_nat_vis();
#endif
	return *this;
}

bool Tags::has_tag(Name const & _tag) const
{
#ifdef USE_MORE_TAGS
	if (useMoreTags)
	{
		for_every(tag, moreTags)
		{
			if (tag->get_tag() == _tag)
			{
				return true;
			}
		}
	}
	else
#endif
	{
		for_every(tag, tags)
		{
			if (tag->get_tag() == _tag)
			{
				return true;
			}
		}
	}
	return baseTags ? baseTags->has_tag(_tag) : false;
}

float Tags::get_tag(Name const & _tag, float _default) const
{
#ifdef USE_MORE_TAGS
	if (useMoreTags)
	{
		for_every(tag, moreTags)
		{
			if (tag->get_tag() == _tag)
			{
				return tag->get_relevance();
			}
		}
	}
	else
#endif
	{
		for_every(tag, tags)
		{
			if (tag->get_tag() == _tag)
			{
				return tag->get_relevance();
			}
		}
	}
	return baseTags? baseTags->get_tag(_tag, _default) : _default;
}

int Tags::get_tag_as_int(Name const & _tag) const
{
#ifdef USE_MORE_TAGS
	if (useMoreTags)
	{
		for_every(tag, moreTags)
		{
			if (tag->get_tag() == _tag)
			{
				return tag->get_relevance_as_int();
			}
		}
	}
	else
#endif
	{
		for_every(tag, tags)
		{
			if (tag->get_tag() == _tag)
			{
				return tag->get_relevance_as_int();
			}
		}
	}
	return baseTags ? baseTags->get_tag_as_int(_tag) : 0;
}

bool Tags::is_contained_within(Tags const & _other) const
{
#ifdef USE_MORE_TAGS
	if (useMoreTags)
	{
		for_every_const(tag, moreTags)
		{
			if (_other.get_tag(tag->get_tag()) == 0.0f) // TODO compare?
			{
				return false;
			}
		}
	}
	else
#endif
	{
		for_every_const(tag, tags)
		{
			if (_other.get_tag(tag->get_tag()) == 0.0f) // TODO compare?
			{
				return false;
			}
		}
	}
	if (baseTags)
	{
		return baseTags->is_contained_within(_other);
	}
	else
	{
		return true;
	}
}

bool Tags::does_match_any_from(Tags const & _other) const
{
#ifdef USE_MORE_TAGS
	if (useMoreTags)
	{
		for_every_const(tag, moreTags)
		{
			if (_other.get_tag(tag->get_tag()) != 0.0f) // TODO compare?
			{
				return true;
			}
		}
	}
	else
#endif
	{
		for_every_const(tag, tags)
		{
			if (_other.get_tag(tag->get_tag()) != 0.0f) // TODO compare?
			{
				return true;
			}
		}
	}
	if (baseTags)
	{
		return baseTags->does_match_any_from(_other);
	}
	else
	{
		return false;
	}
}

float Tags::calc_cross_relevance(Tags const & _other) const
{
	float sum = 0.0f;
#ifdef USE_MORE_TAGS
	if (useMoreTags)
	{
		for_every_const(tag, moreTags)
		{
			sum += _other.get_tag(tag->get_tag()) * tag->get_relevance();
		}
	}
	else
#endif
	{
		for_every_const(tag, tags)
		{
			sum += _other.get_tag(tag->get_tag()) * tag->get_relevance();
		}
	}
	return sum;
}

