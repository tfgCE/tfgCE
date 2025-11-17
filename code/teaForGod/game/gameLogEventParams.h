#pragma once

/**
 *	List of events and params
 *
 *	  +-------------+
 *	  |	PARAM TYPES |
 *	  +---+---------+
 *	  | b | bool    |
 *	  | E | Energy  |
 *	  | f | float   |
 *	  | i | int     |
 *	  | N | Name    |
 *	  +---+---------+
 *
 *		GAME LOG EVENT NAME			DESCRIPTION																			T PARAM 0									T PARAM 1									T PARAM 2									T PARAM 3
 *	------------------------------+------------------------------------------------------------------------------------+-+-----------------------------------------+-+-----------------------------------------+-+-----------------------------------------+-+-----------------------------------------+
 *		died						player has died																		N instigator's library name's name			N kill source's library name's name			b controlled by (local) player
 *		grabbedEnergyQuantum   		player grabbed energy quantum                                                       i energy quantum type (enum)                f max distance                              i number of pulls
 *		healthRegenOnDeathStatus	player has died - this is info about player's health regen at the moment of death	E energy in the backup
 *		madeEnergyQuantumRelease    player shot someone which resulted in an energy orb release
 */

// died
#define DIED__INSTIGATOR          0
#define DIED__SOURCE              1
#define DIED__BY_LOCAL_PLAYER     2

// grabbedEnergyQuantum
#define GRABBED_ENERGY_QUANTUM__TYPE  0
#define GRABBED_ENERGY_QUANTUM__MAX_DISTANCE  1
#define GRABBED_ENERGY_QUANTUM__NUMBER_OF_PULLS  2

// healthRegenOnDeathStatus
#define HEALTH_REGEN_ON_DEATH_STATUS__ENERGY_IN_BACKUP  0
