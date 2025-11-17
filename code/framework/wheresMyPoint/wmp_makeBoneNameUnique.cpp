#include "wmp_makeBoneNameUnique.h"

#include "..\meshGen\meshGenGenerationContext.h"
#include "..\meshGen\meshGenElement.h"

#include "..\..\core\wheresMyPoint\iWMPOwner.h"
#include "..\..\core\wheresMyPoint\wmp_context.h"

#include "..\..\core\io\xml.h"
#include "..\..\core\math\math.h"

using namespace Framework;

bool MakeBoneNameUnique::update(REF_ WheresMyPoint::Value & _value, WheresMyPoint::Context & _context) const
{
	bool result = true;

	if (_value.get_type() == type_id<Name>())
	{
		WheresMyPoint::IOwner* wmpOwner = _context.get_owner();
		while (wmpOwner)
		{
			if (auto * context = fast_cast<MeshGeneration::GenerationContext>(wmpOwner))
			{
				Name boneName = _value.get_as<Name>();
				String orgBoneName = boneName.to_string();
				int tryIdx = 1;
				bool isUnique = false;
				while (!isUnique)
				{
					isUnique = true;
					for_every(bone, context->get_generated_bones())
					{
						if (boneName == bone->boneName)
						{
							isUnique = false;
						}
					}
					if (!isUnique)
					{
						boneName = Name(String::printf(TXT("%S_%i"), orgBoneName.to_char(), tryIdx));
						++tryIdx;
					}
				}
				_value.set(boneName);
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
