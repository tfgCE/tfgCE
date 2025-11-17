#include "math.h"

#include "..\serialisation\serialiser.h"

//

BEGIN_SERIALISER_FOR_ENUM(MathCompare::Type, byte)
	ADD_SERIALISER_FOR_ENUM(0, MathCompare::Lesser)
	ADD_SERIALISER_FOR_ENUM(1, MathCompare::LesserOrEqual)
	ADD_SERIALISER_FOR_ENUM(2, MathCompare::Equal)
	ADD_SERIALISER_FOR_ENUM(3, MathCompare::GreaterOrEqual)
	ADD_SERIALISER_FOR_ENUM(4, MathCompare::Greater)
	ADD_SERIALISER_FOR_ENUM(5, MathCompare::NotEqual)
END_SERIALISER_FOR_ENUM(MathCompare::Type)

