bool TagCondition::check(Tags const & _tags) const
{
	return element == nullptr || element->check(_tags) != 0.0f;
}

float TagCondition::Element::check(Tags const & _tags) const
{
	if (type == Type::Tag)
	{
		return _tags.get_tag(tag);
	}
	else if (type == Type::Value)
	{
		return value;
	}
	else if (type == Type::Add)
	{
		return check_for_arg(argumentA, _tags) + check_for_arg(argumentB, _tags);
	}
	else if (type == Type::And)
	{
		return check_for_arg(argumentA, _tags) != 0.0f && check_for_arg(argumentB, _tags) != 0.0f ? 1.0f : 0.0f;
	}
	else if (type == Type::Not)
	{
		return check_for_arg(argumentA, _tags) != 0.0f ? 0.0f : 1.0f;
	}
	else if (type == Type::Or)
	{
		return check_for_arg(argumentA, _tags) != 0.0f || check_for_arg(argumentB, _tags) != 0.0f ? 1.0f : 0.0f;
	}
	else if (type == Type::Equal)
	{
		return check_for_arg(argumentA, _tags) == check_for_arg(argumentB, _tags);
	}
	else if (type == Type::LessThan)
	{
		return check_for_arg(argumentA, _tags) < check_for_arg(argumentB, _tags);
	}
	else if (type == Type::LessOrEqual)
	{
		return check_for_arg(argumentA, _tags) <= check_for_arg(argumentB, _tags);
	}
	else if (type == Type::GreaterThan)
	{
		return check_for_arg(argumentA, _tags) > check_for_arg(argumentB, _tags);
	}
	else if (type == Type::GreaterOrEqual)
	{
		return check_for_arg(argumentA, _tags) >= check_for_arg(argumentB, _tags);
	}
	else
	{
		return 0.0f;
	}
}

float TagCondition::Element::check_for_arg(Element* _argument, Tags const & _tags) const
{
	return _argument != nullptr ? _argument->check(_tags) : 0.0f;
}

