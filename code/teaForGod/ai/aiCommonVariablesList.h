/**
 *	Common variables
 *
 *	These are all variables that AI share/use in similar manner, all of them are explained below, their purpose and functionality.
 *	Variables are contained within ModulesOwner, so they are accessible to anyone.
 *	Although simple variable storage info, because it is contained in same place in memory, it is exposed as a pointer.
 *	To use with reference, use LATENT_COMMON_VAR macros
 *
 *	Because AIs have similar variables used by various functions, I decided to keep a list here with explanation (and make it easier to get those variables)
 *
 *	Note that some of the variables might be set via spawning mechanism (mobilityType, perceptionStartOff, etc)
 * 
 *	Accessible through var name, ie:
 *
 *	SafePtr<Framework::ModulesOwner>* enemyPtrVar;
 *	...
 *	AU::GetCommonVariable::enemyPtr(mind, enemyPtrVar);
 *	...
 *	(*enemyPtrVar) = newEnemy;
 *
 *
 *	Although with latent functions, it is better to use macros:
 *
 *	LATENT_COMMON_VAR_BEGIN()
 *	LATENT_COMMON_VAR(SafePtr<Framework::IModulesOwner>, enemy);
 *	LATENT_COMMON_VAR(float, aggressiveness);
 *	LATENT_COMMON_VAR_END()
 *
 *
 *	This file is an actual list of common variables.
 *	It uses macro VAR to keep one list and be used with two different macros.
 */

VAR(Name,										scriptedBehaviour);				/** ai logic should be off */
VAR(bool, 										unableToAttack);				/** can no longer attack */
VAR(bool, 										unableToShoot);					/** can no longer shoot, can still attack though */
VAR(bool, 										unableToScan);					/** can no longer scan (this doesn't have to go with sight impaired) */
VAR(bool, 										ignoreViolenceDisallowed);		/** in certain cases we allow to ignore disallowing violence (like when we're attacked) */
VAR(bool, 										confused);						/** enemy is confused and may stop attacking, etc */
VAR(bool, 										perceptionStartOff);			/** won't look for an enemy, will behave a bit differently, only if somehow noticed, will switch this off
																				 *	perception switches this off
																				 *  enemy may walk in front of them and they won't notice! */
VAR(bool, 										perceptionSightImpaired);		/** won't be able to see - due to damage */
VAR(bool, 										perceptionNoSight);				/** won't be able to see - in general */
VAR(bool, 										perceptionSightMovementBased);	/** able to see but only a moving target */
VAR(bool, 										perceptionNoHearing);			/** won't be able to hear - in general */
VAR(PerceptionPause,							perceptionPaused);				/** to allow pausing perception */
VAR(PerceptionLock,								perceptionEnemyLock);			/** to allow changing enemy or keeping it */
VAR(SafePtr<Framework::IModulesOwner>,			enemy);							/** just a safe pointer to enemy, as a reference */
VAR(Framework::RelativeToPresencePlacement,		enemyPlacement);				/** assumed enemy location, AI should use this to think where enemy is, pereception task should update this var,
																				 *	if it points at modules owner, it is real, followed location,
																				 *	if not (ie, it's plain transform) it is assumed location,
																				 *	if empty, we have no idea where our enemy is */
VAR(Vector3,									defaultTargetingOffsetOS);		/** Offset used for targeting - if no target available, used to investigate, etc. assumes pilgrim, so the offset is always the same */
VAR(Vector3,									enemyTargetingOffsetOS);		/** Offset used for targeting - it is updated as long as enemyPlacement points at actual object, if it's not, this variable
																				 *	stores last seen state */
VAR(Vector3,									enemyCentreOffsetOS);			/** Centre of presence offset - it is updated as long as enemyPlacement points at actual object, if it's not, this variable
																				 *	stores last seen state */
VAR(Framework::PresencePath,					enemyPath);						/** this is for internal use by perception although it is exposed here as we may sometimes want to cheat and know enemy location */
VAR(float,										aggressiveness);				/** tells how aggressive we are at the moment */
VAR(bool,										berserkerMode);					/** top aggressiveness, attacks not looking at anything else */
VAR(Framework::InRoomPlacement,					investigate);					/** point somewhere to investigate */
VAR(int,										investigateIdx);				/** to know if we're investigating something else */
VAR(bool,										investigateDir);				/** just a direction to investigate */
VAR(bool,										investigateGlance);				/** instead of moving straight to the thing to investigate, glance at that point, perception will turn it off */
VAR(bool,										encouragedInvestigate);			/** investigation is highly encouraged but not forced */
VAR(bool,										forceInvestigate);				/** is forced to investigate */
VAR(Name,										forceInvestigateReason);		/** reason to do force investigate */
VAR(Name,										mobilityType);					/** defined through POI or spawn manager, can be one of following:
																				 *	sedentary	stays in the room
																				 *	ambushed	doesn't move until enemy comes, has to see enemy
																				 *	lurking		moves but doesn't leave room, has to see enemy
																				 *	curious		leaves room if there is something to investigate
																				 *	hunter		travels around, looks for enemy */
VAR(bool,										remainInRoom);					/** remain in room no matter what */

VAR(Range,										stayCloseGoFurtherDist);		/** how far it should go (if at all) */
VAR(Range,										stayCloseGoFurtherTime);		/** how long (if at all) */
VAR(Range,										stayCloseGoFurtherInterval);	/** interval, optional */
VAR(Name,										stayCloseGait);					/** gait, optional */
VAR(float,										stayCloseGaitDist);				/** when gait is triggered, optional */

VAR(SafePtr<Framework::IModulesOwner>,			scriptedLookAt);				/** if we want to force to look at something/someone */
