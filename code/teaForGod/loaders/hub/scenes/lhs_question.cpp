#include "lhs_question.h"

#include "..\loaderHub.h"
#include "..\loaderHubScreen.h"

#include "..\screens\lhc_question.h"

//

DEFINE_STATIC_NAME(question);

//

using namespace Loader;
using namespace HubScenes;

//

REGISTER_FOR_FAST_CAST(Question);

Question::Question(String const& _question, std::function<void()> _yes, std::function<void()> _no)
: question(_question)
, on_yes(_yes)
, on_no(_no)
{
}

Question::Question(Name const& _locStrId, std::function<void()> _yes, std::function<void()> _no)
: question(LOC_STR(_locStrId))
, on_yes(_yes)
, on_no(_no)
{
}

void Question::on_activate(HubScene* _prev)
{
	base::on_activate(_prev);

	questionDone = false;

	get_hub()->allow_to_deactivate_with_loader_immediately(false);

	get_hub()->deactivate_all_screens();

	show();
}

void Question::show()
{
	HubScreens::Question::ask(get_hub(), question,
		[this]()
		{
			if (on_yes)
			{
				on_yes();
			}
			questionDone = true;
		},
		[this]()
		{
			if (on_no)
			{
				on_no();
			}
			questionDone = true;
		});
}

void Question::on_update(float _deltaTime)
{
	base::on_update(_deltaTime);
}

void Question::on_deactivate(HubScene* _next)
{
	base::on_deactivate(_next);
	questionDone = true;
}
