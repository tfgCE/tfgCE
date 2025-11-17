#pragma once

#include "..\..\core\globalDefinitions.h"

/*
 *	usage:
 *		navTask = mind->access_navigation().find_path_to(found);
 *		LATENT_NAV_TASK(navTask);
 *		if (navTask.is_set() && navTask->has_succeed())
 *		{
 *			... on success
 *		}
 */
#define LATENT_NAV_TASK(navTaskPtr) \
	while (true) \
	{ \
		{ \
			scoped_call_stack_info(TXT("wait for task to end")); \
			if (! navTaskPtr.is_set() || navTaskPtr->is_done()) \
			{ \
				break; \
			} \
		} \
		LATENT_YIELD(); \
	}

 /*
  *	usage:
  *		navTask = mind->access_navigation().find_path_to(found);
  *		LATENT_NAV_TASK_WHILE(navTask)
  *		{
  *			... do while doing nav task
  *		}
  */
#define LATENT_NAV_TASK_WHILE(navTaskPtr) \
	while (navTaskPtr.is_set() && ! navTaskPtr->is_done())

 /*
  *	usage:
  *		navTask = mind->access_navigation().find_path_to(found);
  *		LATENT_NAV_TASK_IF_ALREADY_SUCCEED(navTask);
  *		{
  *			... on success (but finished)
  *		}
  *		else
  *		{
  *			... on failure (but finished)
  *		}
  */
#define LATENT_NAV_TASK_IF_ALREADY_SUCCEED(navTaskPtr) \
		latentScope.doOnceHack = 1; \
		while (latentScope.doOnceHack -- && \
			navTaskPtr.is_set() && navTaskPtr->is_done()) \
			if (navTaskPtr->has_succeed())

 /*
  *	usage:
  *		navTask = mind->access_navigation().find_path_to(found);
  *		LATENT_NAV_TASK_IF_STILL_ACTIVE(navTask);
  *		{
  *			... still active
  *		}
  */
#define LATENT_NAV_TASK_IF_STILL_ACTIVE(navTaskPtr) \
	if (navTaskPtr.is_set() && ! navTaskPtr->is_done())

 /*
  *	usage:
  *		navTask = mind->access_navigation().find_path_to(found);
  *		LATENT_NAV_TASK_IF_DONE(navTask);
  *		{
  *			... finished
  *		}
  */
#define LATENT_NAV_TASK_IF_DONE(navTaskPtr) \
	if (navTaskPtr.is_set() && navTaskPtr->is_done())

 /*
  *	usage:
  *		navTask = mind->access_navigation().find_path_to(found);
  *		LATENT_NAV_TASK_WAIT_IF_SUCCEED(navTask);
  *		{
  *			... on success
  *		}
  *		else
  *		{
  *			... on failure
  *		}
  */
#ifdef AN_LOG_NAV_TASKS
	// we skip LATENT_NAV_TASK_IF_ALREADY_SUCCEED to allow "else" for "succeed"
	#define LATENT_NAV_TASK_WAIT_IF_SUCCEED(navTaskPtr) \
		LATENT_NAV_TASK(navTaskPtr); \
		latentScope.doOnceHack = 1; \
		while (latentScope.doOnceHack -- && \
			navTaskPtr.is_set() && navTaskPtr->is_done() && \
			navTaskPtr->log_to_output(TXT("nav task wait done"))) \
			if (navTaskPtr->has_succeed())
#else
	#define LATENT_NAV_TASK_WAIT_IF_SUCCEED(navTaskPtr) \
		LATENT_NAV_TASK(navTaskPtr); \
		LATENT_NAV_TASK_IF_ALREADY_SUCCEED(navTaskPtr)
#endif
