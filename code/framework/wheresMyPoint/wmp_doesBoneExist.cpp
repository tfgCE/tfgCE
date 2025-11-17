#include "wmp_doesBoneExist.h"

#include "..\meshGen\meshGenGenerationContext.h"
#include "..\meshGen\meshGenElement.h"

#include "..\..\core\wheresMyPoint\iWMPOwner.h"
#include "..\..\core\wheresMyPoint\wmp_context.h"

using namespace Framework;

bool DoesBoneExist::update(REF_ WheresMyPoint::Value & _value, WheresMyPoint::Context & _context) const
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
				for_every(bone, context->get_generated_bones())
				{
					if (boneName == bone->boneName)
					{
						_value.set(true);
						return true;
					}
				}
				_value.set(false);
				return true;
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
