#include "mne_cart.h"

#include "..\..\ai\logics\aiLogic_elevatorCart.h"

#include "..\..\..\framework\ai\aiMindInstance.h"
#include "..\..\..\framework\game\game.h"
#include "..\..\..\framework\module\modules.h"
#include "..\..\..\framework\nav\navMesh.h"
#include "..\..\..\framework\nav\navSystem.h"
#include "..\..\..\framework\nav\nodes\navNode_Point.h"
#include "..\..\..\framework\object\scenery.h"
#include "..\..\..\framework\world\doorInRoom.h"

#include "..\..\..\core\concurrency\scopedMRSWLock.h"

//

#ifdef AN_ALLOW_OPTIMIZE_OFF
#pragma optimize("", off)
#endif

//

using namespace TeaForGodEmperor;
using namespace ModuleNavElements;

//

DEFINE_STATIC_NAME(cartLength);
DEFINE_STATIC_NAME(cartWidth);

DEFINE_STATIC_NAME_STR(cartPointA, TXT("cart point A"));
DEFINE_STATIC_NAME_STR(cartPointB, TXT("cart point B"));
DEFINE_STATIC_NAME_STR(cartNodeA, TXT("cart node A"));
DEFINE_STATIC_NAME_STR(cartNodeB, TXT("cart node B"));

//

REGISTER_FOR_FAST_CAST(Cart);

static Framework::ModuleNavElement* create_module(Framework::IModulesOwner* _owner)
{
	return new Cart(_owner);
}

Framework::RegisteredModule<Framework::ModuleNavElement> & Cart::register_itself()
{
	return Framework::Modules::navElement.register_module(String(TXT("cart")), create_module);
}

Cart::Cart(Framework::IModulesOwner* _owner)
: base(_owner)
{
}

Cart::~Cart()
{
}

void Cart::clear()
{
	Concurrency::ScopedMRSWLockWrite lock(navLock);

	if (auto * node = cartNodeA.get()) { node->outdate(); }
	if (auto * node = cartNodeB.get()) { node->outdate(); }
	if (auto * node = cartPointANode.get()) { node->outdate(); }
	if (auto * node = cartPointBNode.get()) { node->outdate(); }

	nodes.clear();
	cartNodeA.clear();
	cartNodeB.clear();
	cartPointANode.clear();
	cartPointBNode.clear();
	cartPointAConnection.clear();
	cartPointBConnection.clear();
}

void Cart::add_to(Framework::Nav::Mesh* _navMesh)
{
	an_assert(Framework::Game::get()->get_nav_system()->is_in_writing_mode());

	clear();

	Concurrency::ScopedMRSWLockWrite lock(navLock);

	if (auto * cart = get_owner())
	{
		if (auto const * ai = cart->get_ai())
		{
			if (auto const * mind = ai->get_mind())
			{
				if (auto * logic = fast_cast<AI::Logics::ElevatorCart>(mind->get_logic()))
				{
					auto * cartPointA = logic->get_cart_point_a();
					auto * cartPointB = logic->get_cart_point_b();
					auto * elevatorStopA = logic->get_elevator_stop_a();
					auto * elevatorStopB = logic->get_elevator_stop_b();
					if (cartPointA && cartPointB &&
						elevatorStopA && elevatorStopB)
					{
						Transform cartNodeAPlacementLS = Transform::identity;
						Transform cartNodeBPlacementLS = Transform::identity;

						float cartWidth = cart->access_variables().get_value<float>(NAME(cartWidth), 0.0f);

						{
							float maxOffset = max(0.0f, cartWidth * 0.5f - 0.2f magic_number); // to leave margin
							
							for (int p = 0; p < 2; ++p)
							{
								if (auto* dir = (p == 0 ? logic->get_elevator_stop_a_door() : logic->get_elevator_stop_b_door()))
								{
									Vector3 dirLocLS = (p == 0 ? cartPointA : cartPointB)->get_presence()->get_placement().to_local(dir->get_placement()).get_translation();
									if (cartWidth > 0.0f)
									{
										dirLocLS.x = clamp(dirLocLS.x, -maxOffset, maxOffset);
									}
									dirLocLS.y = 0.0f;
									dirLocLS.z = 0.0f;
									auto & cartNodePlacementLS = (p == 0 ? cartNodeAPlacementLS : cartNodeBPlacementLS);
									cartNodePlacementLS.set_translation(dirLocLS);
								}
							}
						}

						cartNodeA = Framework::Nav::Nodes::Point::get_one();
						cartNodeA->place_LS(cartNodeAPlacementLS, cart);
						cartNodeA->be_blocked_temporarily_stop();
						_navMesh->add(cartNodeA.get());
						cartNodeA->set_flags_from_nav_mesh();

						if (cartNodeBPlacementLS.get_translation() != cartNodeAPlacementLS.get_translation())
						{
							cartNodeB = Framework::Nav::Nodes::Point::get_one();
							cartNodeB->place_LS(cartNodeBPlacementLS, cart);
							cartNodeB->be_blocked_temporarily_stop();
							_navMesh->add(cartNodeB.get());
							cartNodeB->set_flags_from_nav_mesh();
						}

						float cartLength = cart->access_variables().get_value<float>(NAME(cartLength), 1.0f);

						float cartPointOpenLength = cartLength * 0.5f + 0.5f;

						cartPointANode = Framework::Nav::Nodes::Point::get_one();
						cartPointANode->be_open_node(true, Vector3::yAxis * cartPointOpenLength);
						cartPointANode->place_WS(cartPointA->get_presence()->get_placement().to_world(cartNodeAPlacementLS), elevatorStopA);
						cartPointANode->set_name(NAME(cartPointA));
						_navMesh->add(cartPointANode.get());
						cartPointANode->set_flags_from_nav_mesh();

						cartPointBNode = Framework::Nav::Nodes::Point::get_one();
						cartPointBNode->be_open_node(true, Vector3::yAxis * cartPointOpenLength);
						cartPointBNode->place_WS(cartPointB->get_presence()->get_placement().to_world(cartNodeBPlacementLS), elevatorStopB);
						cartPointBNode->set_name(NAME(cartPointB));
						_navMesh->add(cartPointBNode.get());
						cartPointBNode->set_flags_from_nav_mesh();

						cartPointAConnection = cartNodeA->connect(cartPointANode.get());
						cartNodeA->set_name(NAME(cartNodeA));
						if (cartNodeB.is_set())
						{
							cartNodeB->set_name(NAME(cartNodeB));
							cartNodeA->connect(cartNodeB.get());
							cartPointBConnection = cartNodeB->connect(cartPointBNode.get());
						}
						else
						{
							cartPointBConnection = cartNodeA->connect(cartPointBNode.get());
						}

						cartPointAConnection->set_blocked_temporarily();
						cartPointBConnection->set_blocked_temporarily();

						nodes.push_back(cartNodeA);
						if (cartNodeB.is_set())
						{
							nodes.push_back(cartNodeB);
						}
						nodes.push_back(cartPointANode);
						nodes.push_back(cartPointBNode);
					}
				}
			}
		}
	}

	if (auto* node = cartNodeA.get()) { node->be_important_path_node(); }
	if (auto* node = cartNodeB.get()) { node->be_important_path_node(); }
	if (auto* node = cartPointANode.get()) { node->be_important_path_node(); }
	if (auto* node = cartPointBNode.get()) { node->be_important_path_node(); }
}

void Cart::for_nodes(std::function<void(Framework::Nav::Node* _node)> _do) const
{
	Concurrency::ScopedMRSWLockRead lock(navLock);

	for_every_ref(node, nodes)
	{
		_do(node);
	}
}
