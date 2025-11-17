#include "cooldowns.h"

#include "..\..\core\io\xml.h"

//

using namespace Framework;

//

template<>
Cooldowns<8> Cooldowns<8>::s_empty = Cooldowns<8>();
