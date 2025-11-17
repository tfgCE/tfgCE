#include "wmp_makeSocketNameUnique.h"

#include "..\meshGen\meshGenGenerationContext.h"
#include "..\meshGen\meshGenElement.h"

#include "..\..\core\wheresMyPoint\iWMPOwner.h"
#include "..\..\core\wheresMyPoint\wmp_context.h"

#include "..\..\core\io\xml.h"
#include "..\..\core\math\math.h"

using namespace Framework;

bool MakeSocketNameUnique::update(REF_ WheresMyPoint::Value & _value, WheresMyPoint::Context & _context) const
{
	bool result = true;

	if (_value.get_type() == type_id<Name>())
	{
		WheresMyPoint::IOwner* wmpOwner = _context.get_owner();
		while (wmpOwner)
		{
			if (auto * context = fast_cast<MeshGeneration::GenerationContext>(wmpOwner))
			{
				Name socketName = _value.get_as<Name>();
				String orgSocketName = socketName.to_string();
				int tryIdx = 1;
				bool isUnique = false;
				while (!isUnique)
				{
					isUnique = context->find_socket(socketName) == NONE;
					if (!isUnique)
					{
						socketName = Name(String::printf(TXT("%S_%i"), orgSocketName.to_char(), tryIdx));
						++tryIdx;
					}
				}
				_value.set(socketName);
				break;
			}
			wmpOwner = wmpOwner->get_wmp_owner();
		}
		if (!wmpOwner)
		{
			error_processing_wmp(TXT("mesh gen only!"));
			return false;
		}
	}
	else
	{
		error_processing_wmp(TXT("current value's type \"%S\" not supported"), RegisteredType::get_name_of(_value.get_type()));
		result = false;
	}

	return result;
}
