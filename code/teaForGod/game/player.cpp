#include "player.h"

//

using namespace TeaForGodEmperor;

//

REGISTER_FOR_FAST_CAST(Player);

Player::Player()
{
}

Player::~Player()
{
}

void Player::on_actor_change()
{
	base::on_actor_change();

	allowControllingBoth = false;
}
