#include "tutorialHubId.h"

#include "..\..\core\types\name.h"
#include "..\..\core\types\string.h"

//

using namespace TeaForGodEmperor;

//

void TutorialHubId::set(String const& _id)
{
	if (_id.get_length() > MAX_LENGTH)
	{
		warn(TXT("not enough space to copy id"));
	}
	id.set_size(min(MAX_LENGTH, _id.get_data().get_size()));
	memory_copy(id.begin(), _id.get_data().begin(), id.get_data_size());
}

bool TutorialHubId::operator ==(Name const& _other) const
{
	return String::compare_icase(id.get_data(), _other.to_char());
}
