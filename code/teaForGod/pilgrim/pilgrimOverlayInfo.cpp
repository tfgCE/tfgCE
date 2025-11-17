#include "pilgrimOverlayInfo.h"

#include "..\artDir.h"
#include "..\teaForGod.h"
#include "..\ai\aiRayCasts.h"
#include "..\game\game.h"
#include "..\game\gameDirector.h"
#include "..\library\lineModel.h"
#include "..\library\missionDefinition.h"
#include "..\modules\moduleAI.h"
#include "..\modules\custom\mc_pickup.h"
#include "..\modules\custom\health\mc_health.h"
#include "..\modules\gameplay\modulePilgrim.h"
#include "..\modules\gameplay\equipment\me_gun.h"
#include "..\overlayInfo\overlayInfoSystem.h"
#include "..\overlayInfo\elements\oie_altTexturePart.h"
#include "..\overlayInfo\elements\oie_customDraw.h"
#include "..\overlayInfo\elements\oie_inputPrompt.h"
#include "..\overlayInfo\elements\oie_mesh.h"
#include "..\overlayInfo\elements\oie_text.h"
#include "..\overlayInfo\elements\oie_texturePart.h"
#include "..\pilgrimage\pilgrimage.h"
#include "..\pilgrimage\pilgrimageInstanceOpenWorld.h"
#include "..\sound\subtitleSystem.h"
#include "..\sound\voiceoverSystem.h"
#include "..\utils\reward.h"

#include "..\..\core\recordVideoSettings.h"
#include "..\..\core\concurrency\scopedSpinLock.h"
#include "..\..\core\system\video\video3dPrimitives.h"

#include "..\..\framework\ai\aiMindInstance.h"
#include "..\..\framework\ai\aiSocial.h"
#include "..\..\framework\gameScript\gameScriptExecution.h"
#include "..\..\framework\library\library.h"
#include "..\..\framework\library\usedLibraryStored.inl"
#include "..\..\framework\module\moduleAppearance.h"
#include "..\..\framework\module\moduleCollision.h"
#include "..\..\framework\module\modulePresence.h"
#include "..\..\framework\module\moduleSound.h"
#include "..\..\framework\modulesOwner\modulesOwnerImpl.inl"
#include "..\..\framework\object\actor.h"

#include <time.h>

//

#ifdef AN_ALLOW_OPTIMIZE_OFF
#pragma optimize("", off)
#endif

#ifdef AN_ALLOW_EXTENSIVE_LOGS
	#define OUTPUT_OVERLAY_INFO
	#define OUTPUT_OVERLAY_INFO_HIDE
	#ifdef AN_DEVELOPMENT
		//#define OUTPUT_OVERLAY_INFO_IN_HAND
		#define OUTPUT_OVERLAY_INFO_REFERENCE
		#define OUTPUT_OVERLAY_INFO_INFO
	#endif
#endif

#define EXM_OFFSET 0.05f

#define PILGRIM_LOG_AUTO_AT_LINE (PlayerPreferences::should_hud_follow_pitch()? 0 : 5)

#define SHOW_INVESTIGATE_LOCKS_TARGET
//#define WITH_DETAILED_HEALTH_RULES_INFO

#define DESCRIPTOR_ELEMENT_ADDITIONAL_SCALE 0.6f

#define WEAPON_PARTS_IN_EACH_COLUMN

//

using namespace TeaForGodEmperor;

//

// room tags
DEFINE_STATIC_NAME(openWorldExterior);

// no door marker tags
DEFINE_STATIC_NAME(noDoorMarkerPending);

// overlay info system ids
DEFINE_STATIC_NAME(map);
DEFINE_STATIC_NAME(pilgrimStat);
DEFINE_STATIC_NAME(pilgrimEquipmentStat);
DEFINE_STATIC_NAME(pilgrimEquipmentInHandLeftStat);
DEFINE_STATIC_NAME(pilgrimEquipmentInHandRightStat);
DEFINE_STATIC_NAME(leftHand);
DEFINE_STATIC_NAME(rightHand);
DEFINE_STATIC_NAME(infoAboutCell);
DEFINE_STATIC_NAME(exm);
DEFINE_STATIC_NAME(permanentEXM);
DEFINE_STATIC_NAME(inputPrompt);
DEFINE_STATIC_NAME(tip);
DEFINE_STATIC_NAME(mainLog);
DEFINE_STATIC_NAME(mainLogLineIndicator);
DEFINE_STATIC_NAME(investigateLog);
DEFINE_STATIC_NAME(subtitleLog);
DEFINE_STATIC_NAME(symbolReplacement);
DEFINE_STATIC_NAME(turnCounterInfo);
DEFINE_STATIC_NAME(objectDescriptor);

// input (game input definition)
DEFINE_STATIC_NAME(game);

// input (game input definition option)
DEFINE_STATIC_NAME(cs_blockOverlayInfoOnHeadSideTouch);

// input (controls)
DEFINE_STATIC_NAME(showOverlayInfo);
DEFINE_STATIC_NAME(useEXM);
DEFINE_STATIC_NAME(deployMainEquipment);
DEFINE_STATIC_NAME(grabEquipment);

// shader program params
DEFINE_STATIC_NAME(overlayColour);
DEFINE_STATIC_NAME(overlayOverrideColour);
DEFINE_STATIC_NAME(cooldownColour);
DEFINE_STATIC_NAME(cooldownY);
DEFINE_STATIC_NAME(useAlpha);

// sounds
DEFINE_STATIC_NAME(changedOpenWorldCell);

// global references
DEFINE_STATIC_NAME_STR(grOverlayInfoShader, TXT("pi.ov.in; shader"));
//
DEFINE_STATIC_NAME_STR(grOverlayInfoFont, TXT("pi.ov.in; font"));
DEFINE_STATIC_NAME_STR(grOverlayInfoFontStats, TXT("pi.ov.in; font stats"));
//
DEFINE_STATIC_NAME_STR(grOverlayInfoPilgrimLeft, TXT("pi.ov.in; pilgrim; left"));
DEFINE_STATIC_NAME_STR(grOverlayInfoPilgrimRight, TXT("pi.ov.in; pilgrim; right"));
DEFINE_STATIC_NAME_STR(grOverlayInfoPilgrimHealth, TXT("pi.ov.in; pilgrim; health"));
DEFINE_STATIC_NAME_STR(grOverlayInfoPilgrimAmmo, TXT("pi.ov.in; pilgrim; ammo"));
DEFINE_STATIC_NAME_STR(grOverlayInfoPilgrimStorage, TXT("pi.ov.in; pilgrim; storage"));
#ifdef WITH_CRYSTALITE
DEFINE_STATIC_NAME_STR(grOverlayInfoPilgrimCrystalite, TXT("pi.ov.in; pilgrim; crystalite"));
#endif
DEFINE_STATIC_NAME_STR(grOverlayInfoPilgrimExperience, TXT("pi.ov.in; pilgrim; experience"));
DEFINE_STATIC_NAME_STR(grOverlayInfoPilgrimMeritPoints, TXT("pi.ov.in; pilgrim; merit points"));
DEFINE_STATIC_NAME_STR(grOverlayInfoPilgrimTime, TXT("pi.ov.in; pilgrim; time"));
DEFINE_STATIC_NAME_STR(grOverlayInfoPilgrimDistance, TXT("pi.ov.in; pilgrim; distance"));
//
DEFINE_STATIC_NAME_STR(grOverlayInfoEquipmentHealth, TXT("pi.ov.in; equipment; health"));
//
DEFINE_STATIC_NAME_STR(grOverlayInfoGunShotCost, TXT("pi.ov.in; gun; shot cost"));
DEFINE_STATIC_NAME_STR(grOverlayInfoGunDamage, TXT("pi.ov.in; gun; damage"));
DEFINE_STATIC_NAME_STR(grOverlayInfoGunArmourPiercing, TXT("pi.ov.in; gun; armour piercing"));
DEFINE_STATIC_NAME_STR(grOverlayInfoGunAntiDeflection, TXT("pi.ov.in; gun; anti deflection"));
DEFINE_STATIC_NAME_STR(grOverlayInfoGunMagazine, TXT("pi.ov.in; gun; magazine"));
DEFINE_STATIC_NAME_STR(grOverlayInfoGunSpread, TXT("pi.ov.in; gun; spread"));
//
DEFINE_STATIC_NAME_STR(grOverlayInfoKill, TXT("pi.ov.in; kill"));
DEFINE_STATIC_NAME_STR(grOverlayInfoMelee, TXT("pi.ov.in; melee"));
//
DEFINE_STATIC_NAME_STR(grOverlayInfoPermanentEXM, TXT("pi.ov.in; permanent exm line model"));
DEFINE_STATIC_NAME_STR(grOverlayInfoPermanentEXMp1, TXT("pi.ov.in; permanent exm +1 line model"));
DEFINE_STATIC_NAME_STR(grOverlayInfoPassiveEXM, TXT("pi.ov.in; passive exm line model"));
DEFINE_STATIC_NAME_STR(grOverlayInfoActiveEXM, TXT("pi.ov.in; active exm line model"));
DEFINE_STATIC_NAME_STR(grOverlayInfoIntegralEXM, TXT("pi.ov.in; integral exm line model"));
DEFINE_STATIC_NAME_STR(grOverlayInfoInstallEXM, TXT("pi.ov.in; install exm line model"));
DEFINE_STATIC_NAME_STR(grOverlayInfoMoveEXMThroughStack, TXT("pi.ov.in; move exm through stack"));
DEFINE_STATIC_NAME_STR(grOverlayInfoEXMToReplace, TXT("pi.ov.in; exm to replace"));

DEFINE_STATIC_NAME_STR(grOverlayInfoSpeakNewMapAvailable, TXT("pi.ov.in; speak; new map available"));
DEFINE_STATIC_NAME_STR(grOverlayInfoSpeakSymbolUnobfuscated, TXT("pi.ov.in; speak; symbol unobfuscated"));
DEFINE_STATIC_NAME_STR(grOverlayInfoSpeakUpgradeInstalled, TXT("pi.ov.in; speak; upgrade installed"));
DEFINE_STATIC_NAME_STR(grOverlayInfoSpeakHealthCritical, TXT("pi.ov.in; speak; health critical"));
DEFINE_STATIC_NAME_STR(grOverlayInfoSpeakLowHealth, TXT("pi.ov.in; speak; low health"));
DEFINE_STATIC_NAME_STR(grOverlayInfoSpeakHalfHealth, TXT("pi.ov.in; speak; half health"));
DEFINE_STATIC_NAME_STR(grOverlayInfoSpeakFullHealth, TXT("pi.ov.in; speak; full health"));
DEFINE_STATIC_NAME_STR(grOverlayInfoSpeakAmmoCritical, TXT("pi.ov.in; speak; ammo critical"));
DEFINE_STATIC_NAME_STR(grOverlayInfoSpeakAmmoCriticalLeft, TXT("pi.ov.in; speak; ammo left critical"));
DEFINE_STATIC_NAME_STR(grOverlayInfoSpeakAmmoCriticalRight, TXT("pi.ov.in; speak; ammo right critical"));
DEFINE_STATIC_NAME_STR(grOverlayInfoSpeakHalfAmmo, TXT("pi.ov.in; speak; half ammo"));
DEFINE_STATIC_NAME_STR(grOverlayInfoSpeakHalfAmmoLeft, TXT("pi.ov.in; speak; half ammo left"));
DEFINE_STATIC_NAME_STR(grOverlayInfoSpeakHalfAmmoRight, TXT("pi.ov.in; speak; half ammo right"));
DEFINE_STATIC_NAME_STR(grOverlayInfoSpeakLowAmmo, TXT("pi.ov.in; speak; low ammo"));
DEFINE_STATIC_NAME_STR(grOverlayInfoSpeakLowAmmoLeft, TXT("pi.ov.in; speak; low ammo left"));
DEFINE_STATIC_NAME_STR(grOverlayInfoSpeakLowAmmoRight, TXT("pi.ov.in; speak; low ammo right"));
DEFINE_STATIC_NAME_STR(grOverlayInfoSpeakFullAmmo, TXT("pi.ov.in; speak; full ammo"));
DEFINE_STATIC_NAME_STR(grOverlayInfoSpeakFullAmmoLeft, TXT("pi.ov.in; speak; full ammo left"));
DEFINE_STATIC_NAME_STR(grOverlayInfoSpeakFullAmmoRight, TXT("pi.ov.in; speak; full ammo right"));
DEFINE_STATIC_NAME_STR(grOverlayInfoSpeakFollowGuidance, TXT("pi.ov.in; speak; follow guidance"));
DEFINE_STATIC_NAME_STR(grOverlayInfoSpeakFollowGuidanceAmmo, TXT("pi.ov.in; speak; follow guidance ammo"));
DEFINE_STATIC_NAME_STR(grOverlayInfoSpeakFollowGuidanceHealth, TXT("pi.ov.in; speak; follow guidance health"));
DEFINE_STATIC_NAME_STR(grOverlayInfoSpeakFindEnergySource, TXT("pi.ov.in; speak; find energy source"));
DEFINE_STATIC_NAME_STR(grOverlayInfoSpeakSuccess, TXT("pi.ov.in; speak; success"));
DEFINE_STATIC_NAME_STR(grOverlayInfoSpeakReceivingData, TXT("pi.ov.in; speak; receiving data"));
DEFINE_STATIC_NAME_STR(grOverlayInfoSpeakTransferComplete, TXT("pi.ov.in; speak; transfer complete"));
DEFINE_STATIC_NAME_STR(grOverlayInfoSpeakInterfaceBoxFound, TXT("pi.ov.in; speak; interface box found"));
DEFINE_STATIC_NAME_STR(grOverlayInfoSpeakInfestationEnded, TXT("pi.ov.in; speak; infestation ended"));
DEFINE_STATIC_NAME_STR(grOverlayInfoSpeakSystemStabilised, TXT("pi.ov.in; speak; system stabilised"));

// door variables
DEFINE_STATIC_NAME(aCell);
DEFINE_STATIC_NAME(bCell);

// localised strings
DEFINE_STATIC_NAME_STR(lsCharsFastMelee, TXT("chars; fast-melee"));
DEFINE_STATIC_NAME_STR(lsCharsPilgrimOverlayLockedToHead, TXT("chars; pilgrim overlay locked to head"));
DEFINE_STATIC_NAME_STR(lsCharsExperience, TXT("chars; experience"));
DEFINE_STATIC_NAME_STR(lsCharsMeritPoints, TXT("chars; merit points"));
//
DEFINE_STATIC_NAME_STR(lsEquipmentMagazineShort, TXT("pi.ov.in; gun; magazine; short"));
DEFINE_STATIC_NAME_STR(lsEquipmentArmourPiercingShort, TXT("pi.ov.in; gun; armour piercing; short"));
//
DEFINE_STATIC_NAME_STR(lsTipInputOpen, TXT("pi.ov.in; tip; input open"));
DEFINE_STATIC_NAME_STR(lsTipInputLock, TXT("pi.ov.in; tip; input lock"));
DEFINE_STATIC_NAME_STR(lsTipInputActivateEXM, TXT("pi.ov.in; tip; input activate EXM"));
DEFINE_STATIC_NAME_STR(lsTipInputDeployWeapon, TXT("pi.ov.in; tip; input deploy weapon"));
DEFINE_STATIC_NAME_STR(lsTipInputHoldWeapon, TXT("pi.ov.in; tip; input hold weapon"));
DEFINE_STATIC_NAME_STR(lsTipInputRemoveDamagedWeaponLeft, TXT("pi.ov.in; tip; input remove damaged weapon; left"));
DEFINE_STATIC_NAME_STR(lsTipInputRemoveDamagedWeaponRight, TXT("pi.ov.in; tip; input remove damaged weapon; right"));
DEFINE_STATIC_NAME_STR(lsTipInputUseEnergyCoil, TXT("pi.ov.in; tip; input use energy coil"));
DEFINE_STATIC_NAME_STR(lsTipInputHowToUseEnergyCoil, TXT("pi.ov.in; tip; input how to use energy coil"));
DEFINE_STATIC_NAME_STR(lsTipInputRealMovement, TXT("pi.ov.in; tip; input real movement"));
DEFINE_STATIC_NAME_STR(lsTipGuidanceFollowDot, TXT("pi.ov.in; tip; guidance; follow dot"));
DEFINE_STATIC_NAME_STR(lsTipUpgradeSelection, TXT("pi.ov.in; tip; upgrade selection"));
//
DEFINE_STATIC_NAME_STR(lsTurnCounterInfoTurnLeft, TXT("pi.ov.in; turn counter; turn left"));
DEFINE_STATIC_NAME_STR(lsTurnCounterInfoTurnRight, TXT("pi.ov.in; turn counter; turn right"));
//
DEFINE_STATIC_NAME_STR(lsEquipmentDamaged, TXT("pi.ov.in; gun; damaged"));
//
DEFINE_STATIC_NAME_STR(lsObjectDescriptorPickUp, TXT("pi.ov.in; object descriptor; pick up"));
// investigate
DEFINE_STATIC_NAME(value); // common param for string formatter
DEFINE_STATIC_NAME(limit); // common param for string formatter
DEFINE_STATIC_NAME_STR(lsInvestigateNoObject, TXT("pi.ov.in; inv; no object"));
DEFINE_STATIC_NAME_STR(lsInvestigateHealthIcon, TXT("pi.ov.in; inv; health; icon"));
DEFINE_STATIC_NAME_STR(lsInvestigateHealthInfo, TXT("pi.ov.in; inv; health; info"));
DEFINE_STATIC_NAME_STR(lsInvestigateHealthRuleIcon, TXT("pi.ov.in; inv; health rule; icon"));
DEFINE_STATIC_NAME_STR(lsInvestigateHealthRuleInfo, TXT("pi.ov.in; inv; health rule; info"));
DEFINE_STATIC_NAME_STR(lsInvestigateProneToExplosionsInfo, TXT("pi.ov.in; inv; prone to explo; info"));
DEFINE_STATIC_NAME_STR(lsInvestigateNoSightIcon, TXT("pi.ov.in; inv; no sight; icon"));
DEFINE_STATIC_NAME_STR(lsInvestigateNoSightInfo, TXT("pi.ov.in; inv; no sight; info"));
DEFINE_STATIC_NAME_STR(lsInvestigateSightMovementBasedIcon, TXT("pi.ov.in; inv; sight movement based; icon"));
DEFINE_STATIC_NAME_STR(lsInvestigateSightMovementBasedInfo, TXT("pi.ov.in; inv; sight movement based; info"));
DEFINE_STATIC_NAME_STR(lsInvestigateNoHearingIcon, TXT("pi.ov.in; inv; no hearing; icon"));
DEFINE_STATIC_NAME_STR(lsInvestigateNoHearingInfo, TXT("pi.ov.in; inv; no hearing; info"));
DEFINE_STATIC_NAME_STR(lsInvestigateAllyIcon, TXT("pi.ov.in; inv; ally; icon"));
DEFINE_STATIC_NAME_STR(lsInvestigateAllyInfo, TXT("pi.ov.in; inv; ally; info"));
DEFINE_STATIC_NAME_STR(lsInvestigateEnemyIcon, TXT("pi.ov.in; inv; enemy; icon"));
DEFINE_STATIC_NAME_STR(lsInvestigateEnemyInfo, TXT("pi.ov.in; inv; enemy; info"));
DEFINE_STATIC_NAME_STR(lsInvestigateShieldIcon, TXT("pi.ov.in; inv; shield; icon"));
DEFINE_STATIC_NAME_STR(lsInvestigateShieldInfo, TXT("pi.ov.in; inv; shield; info"));
DEFINE_STATIC_NAME_STR(lsInvestigateConfusedIcon, TXT("pi.ov.in; inv; confused; icon"));
DEFINE_STATIC_NAME_STR(lsInvestigateConfusedInfo, TXT("pi.ov.in; inv; confused; info"));
DEFINE_STATIC_NAME_STR(lsInvestigateArmourInfo, TXT("pi.ov.in; inv; armour; info"));
DEFINE_STATIC_NAME_STR(lsInvestigateArmour100Icon, TXT("pi.ov.in; inv; armour 100; icon"));
DEFINE_STATIC_NAME_STR(lsInvestigateArmour75Icon, TXT("pi.ov.in; inv; armour 75; icon"));
DEFINE_STATIC_NAME_STR(lsInvestigateArmour50Icon, TXT("pi.ov.in; inv; armour 50; icon"));
DEFINE_STATIC_NAME_STR(lsInvestigateArmour25Icon, TXT("pi.ov.in; inv; armour 25; icon"));
DEFINE_STATIC_NAME_STR(lsInvestigateRicochetInfo, TXT("pi.ov.in; inv; ricochet; info"));
DEFINE_STATIC_NAME_STR(lsInvestigateRicochetIcon, TXT("pi.ov.in; inv; ricochet; icon"));

DEFINE_STATIC_NAME_STR(lsPleaseWait, TXT("hub; common; please wait"));

DEFINE_STATIC_NAME_STR(lsMissionObjectiveVisitedCellReward, TXT("missions; poi; visited cell; reward"));
DEFINE_STATIC_NAME_STR(lsMissionObjectiveVisitedCellResult, TXT("missions; poi; visited cell; result"));
DEFINE_STATIC_NAME_STR(lsMissionObjectiveVisitedInterfaceBoxReward, TXT("missions; poi; visited interface box; reward"));
DEFINE_STATIC_NAME_STR(lsMissionObjectiveVisitedInterfaceBoxResult, TXT("missions; poi; visited interface box; result"));
DEFINE_STATIC_NAME_STR(lsMissionObjectiveBringItemsReward, TXT("missions; poi; bring items; reward"));
DEFINE_STATIC_NAME_STR(lsMissionObjectiveBringItemsResult, TXT("missions; poi; bring items; result"));
DEFINE_STATIC_NAME_STR(lsMissionObjectiveHackBoxesRequirement, TXT("missions; poi; hack boxes; requirement"));
DEFINE_STATIC_NAME_STR(lsMissionObjectiveHackBoxesReward, TXT("missions; poi; hack boxes; reward"));
DEFINE_STATIC_NAME_STR(lsMissionObjectiveHackBoxesResult, TXT("missions; poi; hack boxes; result"));
DEFINE_STATIC_NAME_STR(lsMissionObjectiveInfestationsRequirement, TXT("missions; poi; infestations; requirement"));
DEFINE_STATIC_NAME_STR(lsMissionObjectiveInfestationsReward, TXT("missions; poi; infestations; reward"));
DEFINE_STATIC_NAME_STR(lsMissionObjectiveInfestationsResult, TXT("missions; poi; infestations; result"));
DEFINE_STATIC_NAME_STR(lsMissionObjectiveRefillsRequirement, TXT("missions; poi; refills; requirement"));
DEFINE_STATIC_NAME_STR(lsMissionObjectiveRefillsReward, TXT("missions; poi; refills; reward"));
DEFINE_STATIC_NAME_STR(lsMissionObjectiveRefillsResult, TXT("missions; poi; refills; result"));

// sockets
DEFINE_STATIC_NAME(overlayEXMPassive);
DEFINE_STATIC_NAME(overlayEXMActive);
DEFINE_STATIC_NAME(overlayEquipment);

// overlay reasons
DEFINE_STATIC_NAME(pilgrimOverlayInfoMain);
DEFINE_STATIC_NAME(showAllEquipmentStats);

// show in hand stats reasons
DEFINE_STATIC_NAME(gunDamagedShortInfo);

// exms
DEFINE_STATIC_NAME_STR(exmMeleeKillAmmo, TXT("_mka"));
DEFINE_STATIC_NAME_STR(exmMeleeKillHealth, TXT("_mkh"));
DEFINE_STATIC_NAME_STR(exmMeleeDamage, TXT("_md"));

// voiceover actor
DEFINE_STATIC_NAME(pilgrimOverlayInfo);

// collision flags
DEFINE_STATIC_NAME(pilgrimOverlayInfoTrace);

// colours
DEFINE_STATIC_NAME(pilgrim_overlay_weaponPart_damaged)

// game definition tags
DEFINE_STATIC_NAME(noExplicitTutorials);

// object tags
DEFINE_STATIC_NAME(investigateTarget);

// object variables
DEFINE_STATIC_NAME(armourMaterialInfo);
DEFINE_STATIC_NAME(ricochetMaterialInfo);
DEFINE_STATIC_NAME(perceptionSightImpaired);
DEFINE_STATIC_NAME(perceptionNoSight);
DEFINE_STATIC_NAME(perceptionSightMovementBased);
DEFINE_STATIC_NAME(perceptionNoHearing);
DEFINE_STATIC_NAME(spawnedShield);
DEFINE_STATIC_NAME(confused);

//

int cqr_index(VectorInt2 const& _at, RangeInt2 const& _mapRange)
{
	if (!_mapRange.x.does_contain(_at.x) ||
		!_mapRange.y.does_contain(_at.y))
	{
		return NONE;
	}

	return (_at.y - _mapRange.y.min) * _mapRange.x.length() + _at.x - _mapRange.x.min;
}

struct CachedCellQueryResult
: public PilgrimageInstanceOpenWorld::CellQueryResult
{
	int priority = 0;
	Colour colour = Colour::white;
	Vector2 relCentre = Vector2::zero;

	Vector2 c; // centre
	Vector2 cl, cu, cr, cd; // at centre
	Vector2 bl, bu, br, bd; // border
	Vector2 el, eu, er, ed; // exits

	float calculate_scale_off()
	{
		return max(0.15f, (cr.x - cl.x) * 0.5f);
	}

	void fill(VectorInt2 const & showCentreAt, bool _doSideExits = true)
	{
		Vector2 relAt = (at - showCentreAt).to_vector2();
		Random::Generator rg = randomSeed;
		
		//

		c = relAt;
		relCentre = calculate_rel_centre();

		//

		cl = relAt + relCentre;
		cu = cl;
		cr = cl;
		cd = cl;
		float radius = rg.get_float(0.1f, 0.2f);
		radius = min(radius, min(0.5f - abs(relCentre.x), 0.5f - abs(relCentre.y)) - 0.1f);
		radius = max(radius, 0.05f);
		cl.x -= radius;
		cu.y += radius;
		cr.x += radius;
		cd.y -= radius;
		eu = cu;
		el = cl;
		ed = cd;
		er = cr;
		if (_doSideExits)
		{
			bool sideSensitiveExits[4];
			Range sideExitsOffset;
			cellSet.get_local_side_exits_info_for_map(sideSensitiveExits, sideExitsOffset);
			// rotate to world
			for_count(int, rot, dir)
			{
				bool temp = sideSensitiveExits[DirFourClockwise::Down];
				sideSensitiveExits[DirFourClockwise::Down] = sideSensitiveExits[DirFourClockwise::Left];
				sideSensitiveExits[DirFourClockwise::Left] = sideSensitiveExits[DirFourClockwise::Up];
				sideSensitiveExits[DirFourClockwise::Up] = sideSensitiveExits[DirFourClockwise::Right];
				sideSensitiveExits[DirFourClockwise::Right] = temp;
			}

			if (sideSensitiveExits[DirFourClockwise::Up])
			{
				eu.x = relAt.x + rg.get(sideExitsOffset) * (float)(sideExits[DirFourClockwise::Up]);
				eu.y = sideExits[DirFourClockwise::Up] < 0? cl.y : cr.y;
			}
			if (sideSensitiveExits[DirFourClockwise::Down])
			{
				ed.x = relAt.x + rg.get(sideExitsOffset) * (float)(sideExits[DirFourClockwise::Down]);
				ed.y = sideExits[DirFourClockwise::Down] < 0? cl.y : cr.y;
			}
			if (sideSensitiveExits[DirFourClockwise::Left])
			{
				el.y = relAt.y + rg.get(sideExitsOffset) * (float)(sideExits[DirFourClockwise::Left]);
				el.x = sideExits[DirFourClockwise::Left] < 0? cd.x : cu.x;
			}
			if (sideSensitiveExits[DirFourClockwise::Right])
			{
				er.y = relAt.y + rg.get(sideExitsOffset) * (float)(sideExits[DirFourClockwise::Right]);
				er.x = sideExits[DirFourClockwise::Right] < 0 ? cd.x : cu.x;
			}
		}
		bl = el;
		bu = eu;
		br = er;
		bd = ed;
		bl.x = relAt.x - 0.5f;
		bu.y = relAt.y + 0.5f;
		br.x = relAt.x + 0.5f;
		bd.y = relAt.y - 0.5f;
		//
		priority = 0;
		if (visited) priority = 3; else
		if (knownDevices) priority = 2; else
		if (knownExits) priority = 1;
	}

	CachedCellQueryResult& operator=(PilgrimageInstanceOpenWorld::CellQueryResult const& _other)
	{
		PilgrimageInstanceOpenWorld::CellQueryResult::operator=(_other);
		return *this;
	}
};

static bool update_exm(Name const& _exm, REF_ EXMType const*& _exmType)
{
	if (!_exm.is_valid() && _exmType)
	{
		_exmType = nullptr;
		return true;
	}
	if (_exm.is_valid() && (!_exmType || _exmType->get_id() != _exm))
	{
		_exmType = EXMType::find(_exm);
		return true;
	}
	return false;
}

#ifdef OUTPUT_OVERLAY_INFO
void output_reasons(tchar const* _header, ArrayStatic<Name, 8> const& _reasons)
{
	output(TXT("%S"), _header);
	for_every(r, _reasons)
	{
		output(TXT("  %S"), r->to_char());
	}
}
#endif

Energy get_physical_violence_ref(ModulePilgrim* mp)
{
	return mp->get_physical_violence_damage(Hand::Left /* doesn't matter which one it is */, false) + mp->get_physical_violence_damage(Hand::Left /* doesn't matter which one it is */, true);
}

String get_physical_violence_str(ModulePilgrim* mp)
{
	Energy weak = mp->get_physical_violence_damage(Hand::Left /* doesn't matter which one it is */, false);
	Energy strong = mp->get_physical_violence_damage(Hand::Left /* doesn't matter which one it is */, true);
	return weak == strong? strong.as_string(0).pad_left(3) : weak.as_string(0) + LOC_STR(NAME(lsCharsFastMelee)) + strong.as_string(0);
}

//

struct PilgrimOverlayInfo::MissionUtils
{
	PilgrimOverlayInfo* self;
	OverlayInfo::System* overlayInfoSystem;
	Transform& placement;
	Transform offsetDown;
	float size;

	MissionUtils(PilgrimOverlayInfo* _self, OverlayInfo::System* _overlayInfoSystem, Transform& _placement, Transform const& _offsetDown, float _size) :self(_self), overlayInfoSystem(_overlayInfoSystem), placement(_placement), offsetDown(_offsetDown), size(_size) {}

	OverlayInfo::Element* add(String const & _text)
	{
		auto* e = new OverlayInfo::Elements::Text();
		e->with_id(NAME(pilgrimStat));
		e->with_usage(OverlayInfo::Usage::World);
		e->with_font(self->oiFontStats.get());
		e->with_smaller_decimals();

		self->setup_overlay_element_placement(e, placement);

		e->with_horizontal_align(1.0f);
		e->with_vertical_align(0.5f);

		e->with_letter_size(size);

		overlayInfoSystem->add(e);

		placement = placement.to_world(offsetDown);

		e->with_text(_text);

		return e;
	}
};

//

bool PilgrimOverlayInfo::s_collectHealthDamageForLocalPlayer = false;

REGISTER_FOR_FAST_CAST(PilgrimOverlayInfo);

PilgrimOverlayInfo::PilgrimOverlayInfo()
: SafeObject<PilgrimOverlayInfo>(this)
{
	if (auto* overlayShader = ::Framework::Library::get_current()->get_global_references().get<::Framework::ShaderProgram>(NAME(grOverlayInfoShader)))
	{
		overlayShaderInstance = new ::System::ShaderProgramInstance();
		overlayShaderInstance->set_shader_program(overlayShader->get_shader_program());
	}

	colourHere.parse_from_string(String(TXT("map_overlay_here")));
	colourHighlight.parse_from_string(String(TXT("map_overlay_highlight")));
	colourKnownExits.parse_from_string(String(TXT("map_overlay_knownExits")));
	colourKnownExitsAutoMapOnly.parse_from_string(String(TXT("map_overlay_knownExits_autoMapOnly")));
	colourKnownDevices.parse_from_string(String(TXT("map_overlay_knownDevices")));
	colourKnownDevicesAutoMapOnly.parse_from_string(String(TXT("map_overlay_knownDevices_autoMapOnly")));
	colourVisited.parse_from_string(String(TXT("map_overlay_visited")));
	colourLineModel.parse_from_string(String(TXT("map_overlay_lineModel")));
	colourLongDistanceDetection.parse_from_string(String(TXT("map_overlay_long_distance_detection")));
	colourLongDistanceDetectionByDirection.parse_from_string(String(TXT("map_overlay_long_distance_direction")));

	colourReplaceEXM.parse_from_string(String(TXT("pilgrim_overlay_replace_exm")));
	colourInstallEXM.parse_from_string(String(TXT("pilgrim_overlay_install_exm")));
	colourActiveEXM.parse_from_string(String(TXT("pilgrim_overlay_active_exm")));
	colourActiveEXMCooldown.parse_from_string(String(TXT("pilgrim_overlay_active_exm_cooldown")));
	
	colourCellInfoEnterFrom.parse_from_string(String(TXT("pilgrim_overlay_cell_info_enter_from")));

	colourInvestigateDamage.parse_from_string(String(TXT("pilgrim_overlay_investigate__damage")));
	
	colourMissionType.parse_from_string(String(TXT("pilgrim_overlay_mission_title")));
	
	if (auto* lib = Framework::Library::get_current())
	{
		oiFont = lib->get_global_references().get<Framework::Font>(NAME(grOverlayInfoFont));
		oiFontStats = lib->get_global_references().get<Framework::Font>(NAME(grOverlayInfoFontStats));

		oipLeft = lib->get_global_references().get<Framework::TexturePart>(NAME(grOverlayInfoPilgrimLeft));
		oipRight = lib->get_global_references().get<Framework::TexturePart>(NAME(grOverlayInfoPilgrimRight));
		oipHealth = lib->get_global_references().get<Framework::TexturePart>(NAME(grOverlayInfoPilgrimHealth));
		oipAmmo = lib->get_global_references().get<Framework::TexturePart>(NAME(grOverlayInfoPilgrimAmmo));
		oipStorage = lib->get_global_references().get<Framework::TexturePart>(NAME(grOverlayInfoPilgrimStorage));
#ifdef WITH_CRYSTALITE
		oipCrystalite = lib->get_global_references().get<Framework::TexturePart>(NAME(grOverlayInfoPilgrimCrystalite));
#endif
		oipExperience = lib->get_global_references().get<Framework::TexturePart>(NAME(grOverlayInfoPilgrimExperience));
		oipMeritPoints = lib->get_global_references().get<Framework::TexturePart>(NAME(grOverlayInfoPilgrimMeritPoints));
		oipTime = lib->get_global_references().get<Framework::TexturePart>(NAME(grOverlayInfoPilgrimTime));
		oipDistance = lib->get_global_references().get<Framework::TexturePart>(NAME(grOverlayInfoPilgrimDistance));

		oieHealth = lib->get_global_references().get<Framework::TexturePart>(NAME(grOverlayInfoEquipmentHealth));

		oigShotCost = lib->get_global_references().get<Framework::TexturePart>(NAME(grOverlayInfoGunShotCost));
		oigDamage = lib->get_global_references().get<Framework::TexturePart>(NAME(grOverlayInfoGunDamage));
		//oigArmourPiercing = lib->get_global_references().get<Framework::TexturePart>(NAME(grOverlayInfoGunArmourPiercing));
		//oigAntiDeflection = lib->get_global_references().get<Framework::TexturePart>(NAME(grOverlayInfoGunAntiDeflection));
		//oigMagazineSize = lib->get_global_references().get<Framework::TexturePart>(NAME(grOverlayInfoGunMagazine));
		//oigSpread = lib->get_global_references().get<Framework::TexturePart>(NAME(grOverlayInfoGunSpread));

		oiKill = lib->get_global_references().get<Framework::TexturePart>(NAME(grOverlayInfoKill));
		oiMelee = lib->get_global_references().get<Framework::TexturePart>(NAME(grOverlayInfoMelee));

		oiPermanentEXM = lib->get_global_references().get<LineModel>(NAME(grOverlayInfoPermanentEXM));
		oiPermanentEXMp1 = lib->get_global_references().get<LineModel>(NAME(grOverlayInfoPermanentEXMp1));
		oiPassiveEXM = lib->get_global_references().get<LineModel>(NAME(grOverlayInfoPassiveEXM));
		oiActiveEXM = lib->get_global_references().get<LineModel>(NAME(grOverlayInfoActiveEXM));
		oiIntegralEXM = lib->get_global_references().get<LineModel>(NAME(grOverlayInfoIntegralEXM));
		oiInstallEXM = lib->get_global_references().get<LineModel>(NAME(grOverlayInfoInstallEXM));
		oiMoveEXMThroughStack = lib->get_global_references().get<LineModel>(NAME(grOverlayInfoMoveEXMThroughStack));
		oiEXMToReplace = lib->get_global_references().get<LineModel>(NAME(grOverlayInfoEXMToReplace));
		
		speakNewMapAvailable = lib->get_global_references().get<Framework::Sample>(NAME(grOverlayInfoSpeakNewMapAvailable));
		speakSymbolUnobfuscated = lib->get_global_references().get<Framework::Sample>(NAME(grOverlayInfoSpeakSymbolUnobfuscated));
		speakUpgradeInstalled = lib->get_global_references().get<Framework::Sample>(NAME(grOverlayInfoSpeakUpgradeInstalled));
		speakHealthCritical = lib->get_global_references().get<Framework::Sample>(NAME(grOverlayInfoSpeakHealthCritical));
		speakLowHealth = lib->get_global_references().get<Framework::Sample>(NAME(grOverlayInfoSpeakLowHealth));
		speakHalfHealth = lib->get_global_references().get<Framework::Sample>(NAME(grOverlayInfoSpeakHalfHealth));
		speakFullHealth = lib->get_global_references().get<Framework::Sample>(NAME(grOverlayInfoSpeakFullHealth));
		speakLowAmmo = lib->get_global_references().get<Framework::Sample>(NAME(grOverlayInfoSpeakLowAmmo));
		speakHalfAmmo = lib->get_global_references().get<Framework::Sample>(NAME(grOverlayInfoSpeakHalfAmmo));
		speakHalfAmmoHand[Hand::Left] = lib->get_global_references().get<Framework::Sample>(NAME(grOverlayInfoSpeakHalfAmmoLeft));
		speakHalfAmmoHand[Hand::Right]  = lib->get_global_references().get<Framework::Sample>(NAME(grOverlayInfoSpeakHalfAmmoRight));
		speakLowAmmoHand[Hand::Left] = lib->get_global_references().get<Framework::Sample>(NAME(grOverlayInfoSpeakLowAmmoLeft));
		speakLowAmmoHand[Hand::Right]  = lib->get_global_references().get<Framework::Sample>(NAME(grOverlayInfoSpeakLowAmmoRight));
		speakAmmoCritical = lib->get_global_references().get<Framework::Sample>(NAME(grOverlayInfoSpeakAmmoCritical));
		speakAmmoCriticalHand[Hand::Left] = lib->get_global_references().get<Framework::Sample>(NAME(grOverlayInfoSpeakAmmoCriticalLeft));
		speakAmmoCriticalHand[Hand::Right]  = lib->get_global_references().get<Framework::Sample>(NAME(grOverlayInfoSpeakAmmoCriticalRight));
		speakFullAmmo = lib->get_global_references().get<Framework::Sample>(NAME(grOverlayInfoSpeakFullAmmo));
		speakFullAmmoHand[Hand::Left] = lib->get_global_references().get<Framework::Sample>(NAME(grOverlayInfoSpeakFullAmmoLeft));
		speakFullAmmoHand[Hand::Right] = lib->get_global_references().get<Framework::Sample>(NAME(grOverlayInfoSpeakFullAmmoRight));
		speakFollowGuidance = lib->get_global_references().get<Framework::Sample>(NAME(grOverlayInfoSpeakFollowGuidance));
		speakFollowGuidanceAmmo = lib->get_global_references().get<Framework::Sample>(NAME(grOverlayInfoSpeakFollowGuidanceAmmo));
		speakFollowGuidanceHealth = lib->get_global_references().get<Framework::Sample>(NAME(grOverlayInfoSpeakFollowGuidanceHealth));
		speakFindEnergySource = lib->get_global_references().get<Framework::Sample>(NAME(grOverlayInfoSpeakFindEnergySource));
		speakSuccess = lib->get_global_references().get<Framework::Sample>(NAME(grOverlayInfoSpeakSuccess));
		speakReceivingData = lib->get_global_references().get<Framework::Sample>(NAME(grOverlayInfoSpeakReceivingData));
		speakTransferComplete = lib->get_global_references().get<Framework::Sample>(NAME(grOverlayInfoSpeakTransferComplete));
		speakInterfaceBoxFound = lib->get_global_references().get<Framework::Sample>(NAME(grOverlayInfoSpeakInterfaceBoxFound));
		speakInfestationEnded = lib->get_global_references().get<Framework::Sample>(NAME(grOverlayInfoSpeakInfestationEnded));
		speakSystemStabilised = lib->get_global_references().get<Framework::Sample>(NAME(grOverlayInfoSpeakSystemStabilised));
	}

	speakWarningColour.parse_from_string(String(TXT("pilgrim_overlay_warning")));

	for_count(int, iHand, Hand::MAX)
	{
		for_every(exm, showEXMs.hands[iHand].passiveEXMs)
		{
			exm->rd.mark_new_data_required();
		}
		showEXMs.hands[iHand].activeEXM.rd.mark_new_data_required();
	}

	keepOverlayInfoElements.push_back(NAME(tip));
	keepOverlayInfoElements.push_back(NAME(map));
	keepOverlayInfoElements.push_back(NAME(inputPrompt));
	keepOverlayInfoElements.push_back_unique(NAME(pilgrimEquipmentInHandLeftStat));
	keepOverlayInfoElements.push_back_unique(NAME(pilgrimEquipmentInHandRightStat));
	keepOverlayInfoElements.push_back(NAME(mainLog));
	keepOverlayInfoElements.push_back(NAME(mainLogLineIndicator));
	keepOverlayInfoElements.push_back(NAME(investigateLog));
	keepOverlayInfoElements.push_back(NAME(objectDescriptor));

	pilgrimOverlayInfoTrace.clear();
	pilgrimOverlayInfoTrace = Collision::DefinedFlags::get_existing_or_basic(NAME(pilgrimOverlayInfoTrace));

	stringStatOf = String(TXT("/"));

	goingInWrongDirectionTS.reset(-1000.0f);

	{
		investigateInfo.health.setup(NAME(lsInvestigateHealthIcon), NAME(lsInvestigateHealthInfo));
		investigateInfo.healthRule.setup(NAME(lsInvestigateHealthRuleIcon), NAME(lsInvestigateHealthRuleInfo));
		investigateInfo.proneToExplosions.setup(Name::invalid(), NAME(lsInvestigateProneToExplosionsInfo));
		investigateInfo.noSight.setup(NAME(lsInvestigateNoSightIcon), NAME(lsInvestigateNoSightInfo));
		investigateInfo.sightMovementBased.setup(NAME(lsInvestigateSightMovementBasedIcon), NAME(lsInvestigateSightMovementBasedInfo));
		investigateInfo.noHearing.setup(NAME(lsInvestigateNoHearingIcon), NAME(lsInvestigateNoHearingInfo));
		investigateInfo.ally.setup(NAME(lsInvestigateAllyIcon), NAME(lsInvestigateAllyInfo));
		investigateInfo.enemy.setup(NAME(lsInvestigateEnemyIcon), NAME(lsInvestigateEnemyInfo));
		investigateInfo.shield.setup(NAME(lsInvestigateShieldIcon), NAME(lsInvestigateShieldInfo));
		investigateInfo.confused.setup(NAME(lsInvestigateConfusedIcon), NAME(lsInvestigateConfusedInfo));
		investigateInfo.armour100.setup(NAME(lsInvestigateArmour100Icon), NAME(lsInvestigateArmourInfo));
		investigateInfo.armour75.setup(NAME(lsInvestigateArmour75Icon), Name::invalid());
		investigateInfo.armour50.setup(NAME(lsInvestigateArmour50Icon), Name::invalid());
		investigateInfo.armour25.setup(NAME(lsInvestigateArmour25Icon), Name::invalid());
		investigateInfo.ricochet.setup(NAME(lsInvestigateRicochetIcon), NAME(lsInvestigateRicochetInfo));
	}
}

PilgrimOverlayInfo::~PilgrimOverlayInfo()
{
	if (observingPresence)
	{
		observingPresence->remove_presence_observer(this);
		observingPresence = nullptr;
	}
	if (auto* game = Game::get_as<Game>())
	{
		if (auto* overlayInfoSystem = game->access_overlay_info_system())
		{
			overlayInfoSystem->force_clear();
		}
	}
	make_safe_object_unavailable();
	delete_and_clear(overlayShaderInstance);
}

void PilgrimOverlayInfo::set_owner(Framework::IModulesOwner* _owner)
{
	if (observingPresence)
	{
		observingPresence->remove_presence_observer(this);
		observingPresence = nullptr;
	}
	owner = _owner;
	// filter due to reasons, be an observer only if we require it
	if (GameSettings::get().difficulty.showDirectionsOnlyToObjective)
	{
		if (_owner)
		{
			observingPresence = _owner->get_presence();
			an_assert(observingPresence);
			observingPresence->add_presence_observer(this);
		}
	}

	noExplicitTutorials = false;
	if (auto* gd = GameDefinition::get_chosen())
	{
		if (gd->get_tags().get_tag(NAME(noExplicitTutorials)))
		{
			noExplicitTutorials = true;
		}
	}
}

void PilgrimOverlayInfo::tried_to_shoot_empty(Hand::Type _hand)
{
#ifdef AN_DEVELOPMENT_OR_PROFILER
	if (auto* mp = owner->get_gameplay_as<ModulePilgrim>())
	{
		an_assert(mp->beingAdvanced.get() != 0);
	}
#endif
	pilgrimEnergyState.playNoAmmo[GameSettings::get().difficulty.commonHandEnergyStorage? 0 : _hand] = true;
}

void PilgrimOverlayInfo::on_presence_changed_room(Framework::ModulePresence* _presence, Framework::Room* _intoRoom, Transform const& _intoRoomTransform, Framework::DoorInRoom* _exitThrough, Framework::DoorInRoom* _enterThrough, Framework::DoorInRoomArrayPtr const* _throughDoors)
{
	if (_exitThrough &&
		(_exitThrough->get_door() && _exitThrough->get_door()->get_door_type() && _exitThrough->get_door()->get_door_type()->is_nav_connector()) &&
		GameSettings::get().difficulty.showDirectionsOnlyToObjective &&
		PlayerPreferences::should_allow_follow_guidance() &&
		! GameDirector::get()->is_follow_guidance_blocked() &&
		! PilgrimageInstance::get()->is_follow_guidance_blocked())
	{
		PilgrimageInstanceOpenWorld::DoorLeadsTowardParams params;
		params.to_objective((TeaForGodEmperor::GameSettings::get().difficulty.weaponInfinite || !pilgrimEnergyState.ammoLow) &&
							(TeaForGodEmperor::GameSettings::get().difficulty.playerImmortal || !pilgrimEnergyState.healthVeryLow)); // allow towards objective only if not critical
		if (GameSettings::get().difficulty.pathOnly)
		{
			params.to_objective(!pilgrimEnergyState.ammoLow && !pilgrimEnergyState.healthVeryLow); // allow towards objective only if not critical
			// if we're critical, do not tell about directions
		}
		else
		{
			if (!TeaForGodEmperor::GameSettings::get().difficulty.weaponInfinite) { params.to_ammo(!pilgrimEnergyState.healthVeryLow && pilgrimEnergyState.ammoCouldBeTopped); } // allow towards objective only if not critical and we miss a bit
			if (!TeaForGodEmperor::GameSettings::get().difficulty.playerImmortal) { params.to_health(pilgrimEnergyState.healthCouldBeTopped); }// only if we miss a bit
		}
		if (GameDirector::get()->are_ammo_storages_unavailable())
		{
			// force going to ammo or to objective to calibrate
			params.to_objective(true);
			params.to_ammo(true);
		}
		PilgrimageInstanceOpenWorld::keep_possible_door_leads(_intoRoom, REF_ params);
		if (params.toAmmo || params.toHealth || params.toObjective)
		{
			auto leads = PilgrimageInstanceOpenWorld::does_door_lead_towards(_exitThrough, params);
			if (leads.is_set())
			{
				if (leads.get())
				{
					goingInWrongDirectionCount = 0;
					hide_tip(PilgrimOverlayInfoTip::GuidanceFollowDot);
				}
				else
				{
					++goingInWrongDirectionCount;
					if (followGuidanceSpokenTS.get_time_since() >= 10.0f &&
						goingInWrongDirectionTS.get_time_since() >= (((params.toHealth && pilgrimEnergyState.healthVeryLow) || (params.toAmmo && pilgrimEnergyState.ammoLow)) ? 10.0f : 20.0f) &&
						goingInWrongDirectionCount >= (params.toObjective? 2 : 1))
					{
						if (params.toHealth)
						{
							speak(pilgrimEnergyState.healthCritical? speakHealthCritical.get() : speakLowHealth.get(), SpeakParams().with_colour(speakWarningColour).at_line(PILGRIM_LOG_AUTO_AT_LINE));
							speak(speakFindEnergySource.get(), SpeakParams().with_colour(speakWarningColour).at_line(PILGRIM_LOG_AUTO_AT_LINE));
							speak(speakFollowGuidanceHealth.get(), SpeakParams().with_colour(speakWarningColour).at_line(PILGRIM_LOG_AUTO_AT_LINE));
							followGuidanceSpokenTS.reset();
						}
						else if (params.toAmmo)
						{
							speak(pilgrimEnergyState.ammoCritical ? speakAmmoCritical.get() : speakLowAmmo.get(), SpeakParams().with_colour(speakWarningColour).at_line(PILGRIM_LOG_AUTO_AT_LINE));
							speak(speakFindEnergySource.get(), SpeakParams().with_colour(speakWarningColour).at_line(PILGRIM_LOG_AUTO_AT_LINE));
							speak(speakFollowGuidanceAmmo.get(), SpeakParams().with_colour(speakWarningColour).at_line(PILGRIM_LOG_AUTO_AT_LINE));
							followGuidanceSpokenTS.reset();
						}
						else if (params.toObjective)
						{
#ifndef AN_RECORD_VIDEO
							speak(speakFollowGuidance.get(), SpeakParams().at_line(PILGRIM_LOG_AUTO_AT_LINE));
							if (goingInWrongDirectionTipTS.get_time_since() > 20.0f) 
							{
								goingInWrongDirectionTipTS.reset();
								show_tip(PilgrimOverlayInfoTip::GuidanceFollowDot,
									PilgrimOverlayInfo::ShowTipParams().be_forced().follow_camera_pitch().with_time_to_deactivate_tip(5.0f));
							}
#endif
							followGuidanceSpokenTS.reset();
						}
						goingInWrongDirectionCount = 0;
						goingInWrongDirectionTS.reset();
					}
				}
			}
		}
		else
		{
			goingInWrongDirectionCount = 0;
		}
	}
}

void PilgrimOverlayInfo::on_presence_added_to_room(Framework::ModulePresence* _presence, Framework::Room* _room, Framework::DoorInRoom* _enteredThroughDoor)
{
	goingInWrongDirectionCount = 0;
}

void PilgrimOverlayInfo::on_presence_removed_from_room(Framework::ModulePresence* _presence, Framework::Room* _room)
{
	goingInWrongDirectionCount = 0;
}

void PilgrimOverlayInfo::on_presence_destroy(Framework::ModulePresence* _presence)
{
	if (observingPresence == _presence)
	{
		observingPresence->remove_presence_observer(this);
		observingPresence = nullptr;
	}
}

void PilgrimOverlayInfo::update_map_if_visible()
{
	if (showMap.visible)
	{
		showMap.rd.mark_new_data_required();
	}
}

void PilgrimOverlayInfo::new_map_available(bool _knownLocation, bool _inform, Optional<VectorInt2> const & _showKnownLocation)
{
	if (!PlayerPreferences::should_show_map() || GameDirector::get()->is_map_blocked())
	{
		return;
	}
#ifdef OUTPUT_OVERLAY_INFO
	output(TXT("[POvIn] new map available"));
#endif
	showMap.newAvailable = true;
	showMap.knownLocation = _knownLocation;
	showMap.showKnownLocation = _showKnownLocation;

	if (_inform)
	{
		bool speakNow = true;
		if (auto* piow = PilgrimageInstanceOpenWorld::get())
		{
			speakNow = piow->get_special_rules().pilgrimOverlayInformsAboutNewMap;
		}
		if (speakNow)
		{
			speak(speakNewMapAvailable.get());
		}
	}

	if (currentlyShowing == Main && 
		! showMap.visible)
	{
		if (auto* game = Game::get_as<Game>())
		{
			if (auto* overlayInfoSystem = game->access_overlay_info_system())
			{
				show_map(owner.get(), overlayInfoSystem);
			}
		}
	}
}

void PilgrimOverlayInfo::on_upgrade_installed()
{
	speak(speakUpgradeInstalled.get());
}

void PilgrimOverlayInfo::clear_map_highlights()
{
	Concurrency::ScopedSpinLock lock(showMap.highlightAtLock);

#ifdef OUTPUT_OVERLAY_INFO
	output(TXT("[POvIn] clear map highlighs"));
#endif
	showMap.highlightAt.clear();
}

void PilgrimOverlayInfo::highlight_map_at(VectorInt3 const& _at)
{
	Concurrency::ScopedSpinLock lock(showMap.highlightAtLock);

#ifdef OUTPUT_OVERLAY_INFO
	output(TXT("[POvIn] highligh map at %ix%i"), _at.x, _at.y);
#endif
	ShowMap::HighlightAt hAt;
	hAt.to = _at;
	showMap.highlightAt.push_back(hAt);
	showMap.highlightsLastVisibleAt.reset(); // so they won't disappear on show
	showMap.resetHighlightsLastVisibleAt = true; // if it won't open automatically
}

void PilgrimOverlayInfo::configure_tip_oie_element(OverlayInfo::Element* _element, Rotator3 const& _offset, Optional<float> const & _preferredDistance, Optional<bool> const& _followPitch) const
{
	_element->with_location(OverlayInfo::Element::Relativity::RelativeToAnchorPY/*RelativeToCameraFlat*/);
	_element->with_follow_camera_location_z(0.1f);
	if (_followPitch.get(false))
	{
		_element->with_rotation(OverlayInfo::Element::Relativity::RelativeToAnchorPY, _offset);
	}
	else
	{
		_element->with_rotation(OverlayInfo::Element::Relativity::RelativeToAnchor, _offset);
	}
	_element->with_distance(_preferredDistance.get(currentlyShowing != None || requestedToShow != None ? statsDist * 1.0f : 0.6f));
}

bool PilgrimOverlayInfo::is_showing_tip(PilgrimOverlayInfoTip::Type _tip) const
{
	return currentTip.is_set() && currentTip.get() == _tip;
}

void PilgrimOverlayInfo::hide_tips()
{
	if (auto* game = Game::get_as<Game>())
	{
		if (auto* overlayInfoSystem = game->access_overlay_info_system())
		{
			overlayInfoSystem->deactivate(NAME(tip));
		}
	}

	currentTip.clear();
	timeToDeactivateCurrentTip.clear();
}

void PilgrimOverlayInfo::done_tip(PilgrimOverlayInfoTip::Type _tip, Optional<Hand::Type> const& _forHand)
{
	if (currentTip.is_set() &&
		currentTip.get() == _tip &&
		(!_forHand.is_set() || !currentTipForHand.is_set() || currentTipForHand == _forHand))
	{
		tipsDone.tips[_tip].done = true;
		tipsDone.tips[_tip].when.reset();

		hide_tips();
	}
}

void PilgrimOverlayInfo::hide_tip(PilgrimOverlayInfoTip::Type _tip)
{
	if (currentTip.is_set() &&
		currentTip.get() == _tip)
	{
		hide_tips();
	}
}

void PilgrimOverlayInfo::show_tip(PilgrimOverlayInfoTip::Type _tip, ShowTipParams const& _params)
{
#ifdef AN_RECORD_VIDEO
	return;
#endif

	auto& tip = tipsDone.tips[_tip];
	if (tip.done &&
		tip.when.get_time_since() < tip.interval && !_params.force.get(false))
	{
		return;
	}

	hide_tips();

	if (!PlayerPreferences::should_show_tips_during_game())
	{
		done_tip(_tip);
		return;
	}

	if (auto* game = Game::get_as<Game>())
	{
		if (auto* overlayInfoSystem = game->access_overlay_info_system())
		{
			//if (_tip != PilgrimOverlayInfoTip::MAX) // add only exceptions here, there are none yet
			{
				OverlayInfo::Elements::InputPrompt* ip = nullptr;
				float ipPitch = _params.pitchOffset.get(7.0f);
				bool followCameraPitch = _params.followCameraPitch.get(PlayerPreferences::should_hud_follow_pitch());
				if (_tip == PilgrimOverlayInfoTip::InputOpen)
				{
					ip = new OverlayInfo::Elements::InputPrompt(NAME(game), NAME(showOverlayInfo));
					ip->with_additional_info(LOC_STR(NAME(lsTipInputOpen)));
					ip->with_usage(OverlayInfo::Usage::World);
				}
				else if (_tip == PilgrimOverlayInfoTip::InputLock)
				{
					if (PlayerPreferences::get_pilgrim_overlay_locked_to_head() == Option::Allow)
					{
						ip = new OverlayInfo::Elements::InputPrompt(NAME(game), NAME(showOverlayInfo));
						ip->with_additional_info(LOC_STR(NAME(lsTipInputLock)));
						ip->with_usage(OverlayInfo::Usage::World);
					}
				}
				else if (_tip == PilgrimOverlayInfoTip::InputActivateEXM)
				{
					ip = new OverlayInfo::Elements::InputPrompt(NAME(game), NAME(useEXM));
					if (_params.locStrId.is_set())
					{
						ip->with_additional_info(LOC_STR(_params.locStrId.get()));
					}
					else if (_params.text.is_set() && !_params.text.get().is_empty())
					{
						ip->with_additional_info(_params.text.get());
					}
					else
					{
						ip->with_additional_info(LOC_STR(NAME(lsTipInputActivateEXM)));
					}
					ip->with_usage(OverlayInfo::Usage::World);
				}
				else if (_tip == PilgrimOverlayInfoTip::InputDeployWeapon)
				{
					ip = new OverlayInfo::Elements::InputPrompt(NAME(game), NAME(deployMainEquipment));
					ip->with_additional_info(LOC_STR(NAME(lsTipInputDeployWeapon)));
					ip->with_usage(OverlayInfo::Usage::World);
				}
				else if (_tip == PilgrimOverlayInfoTip::InputHoldWeapon)
				{
					ip = new OverlayInfo::Elements::InputPrompt(NAME(game), NAME(grabEquipment));
					ip->with_additional_info(LOC_STR(NAME(lsTipInputHoldWeapon)));
					ip->with_usage(OverlayInfo::Usage::World);
				}
				else if (_tip == PilgrimOverlayInfoTip::InputRemoveDamagedWeaponRight)
				{
					ip = new OverlayInfo::Elements::InputPrompt();
					ip->with_additional_info(LOC_STR(NAME(lsTipInputRemoveDamagedWeaponRight)));
					ip->with_usage(OverlayInfo::Usage::World);
				}
				else if (_tip == PilgrimOverlayInfoTip::InputRemoveDamagedWeaponLeft)
				{
					ip = new OverlayInfo::Elements::InputPrompt();
					ip->with_additional_info(LOC_STR(NAME(lsTipInputRemoveDamagedWeaponLeft)));
					ip->with_usage(OverlayInfo::Usage::World);
				}
				else if (_tip == PilgrimOverlayInfoTip::InputUseEnergyCoil)
				{
					ip = new OverlayInfo::Elements::InputPrompt();
					ip->with_additional_info(LOC_STR(NAME(lsTipInputUseEnergyCoil)));
					ip->with_usage(OverlayInfo::Usage::World);
				}
				else if (_tip == PilgrimOverlayInfoTip::InputHowToUseEnergyCoil)
				{
					ip = new OverlayInfo::Elements::InputPrompt();
					ip->with_additional_info(LOC_STR(NAME(lsTipInputHowToUseEnergyCoil)));
					ip->with_usage(OverlayInfo::Usage::World);
				}
				else if (_tip == PilgrimOverlayInfoTip::InputRealMovement)
				{
					ip = new OverlayInfo::Elements::InputPrompt();
					ip->with_additional_info(LOC_STR(NAME(lsTipInputRealMovement)));
					ip->with_usage(OverlayInfo::Usage::World);
				}
				else if (_tip == PilgrimOverlayInfoTip::GuidanceFollowDot)
				{
					ip = new OverlayInfo::Elements::InputPrompt();
					ip->with_additional_info(LOC_STR(NAME(lsTipGuidanceFollowDot)));
					ip->with_usage(OverlayInfo::Usage::World);
				}
				else if (_tip == PilgrimOverlayInfoTip::UpgradeSelection)
				{
					ip = new OverlayInfo::Elements::InputPrompt();
					ip->with_additional_info(LOC_STR(NAME(lsTipUpgradeSelection)));
					ip->with_usage(OverlayInfo::Usage::World);
				}
				else if (_tip == PilgrimOverlayInfoTip::Custom)
				{
					if (_params.locStrId.is_set())
					{
						ip = new OverlayInfo::Elements::InputPrompt();
						ip->with_additional_info(LOC_STR(_params.locStrId.get()));
						ip->with_usage(OverlayInfo::Usage::World);
					}
					else if (_params.text.is_set() && !_params.text.get().is_empty())
					{
						ip = new OverlayInfo::Elements::InputPrompt();
						ip->with_additional_info(_params.text.get());
						ip->with_usage(OverlayInfo::Usage::World);
					}
					else
					{
						error(TXT("no locStrId provided for a custom tip"));
					}
				}
				else
				{
					todo_implement(TXT("add input prompt"));
				}
				if (ip)
				{
					ip->with_id(NAME(tip));

					ip->with_pulse();
					ip->with_scale(_params.scale.get(1.0f));

					ip->for_hand(_params.forHand);

					configure_tip_oie_element(ip, Rotator3(ipPitch, 0.0f, 0.0f), NP, followCameraPitch);

					overlayInfoSystem->add(ip);
				}
			}
		}
	}

	currentTip = _tip;
	currentTipForHand = _params.forHand;
	timeToDeactivateCurrentTip = _params.timeToDeactivateTip;

	tip.when.reset();
}

void PilgrimOverlayInfo::advance(float _deltaTime, bool _controlActive, Vector2 const & _controlStick)
{
	if (!owner.get())
	{
		return;
	}

	{
		float globalTintFadeTarget = currentlyShowing == Main? 0.5f : 0.0f;
		globalTintFade = blend_to_using_time(globalTintFade, globalTintFadeTarget, 0.2f, _deltaTime);
		ArtDir::set_global_tint_fade(globalTintFade);
	}

	bool clearEverything = false;
	bool forcedMainChange = false;
	bool forcedMainChangeDueToLock = false;

	if (auto* g = Game::get_as<Game>())
	{
		if (g->get_taking_controls_at() != 0.0f)
		{
			clearEverything = true;
		}
	}

	if (auto* h = owner->get_custom<CustomModules::Health>())
	{
		if (!h->is_alive())
		{
			clearEverything = true;
		}
	}

	if (GameSettings::get().difficulty.noOverlayInfo)
	{
		clearEverything = true;
		if (requestedToShow == Main)
		{
			requestedToShow = None;
		}
	}

	bool switchRequestedOnInput = false;
	{
		Option::Type shouldBeLocked = PlayerPreferences::get_pilgrim_overlay_locked_to_head();
		{
			if ((lockedToHead && shouldBeLocked == Option::Off) ||
				(!lockedToHead && shouldBeLocked == Option::On))
			{
				forcedMainChange = requestedToShow == Main;
				forcedMainChangeDueToLock = forcedMainChange;
				clearEverything = !forcedMainChange;
				if (shouldBeLocked == Option::On) lockedToHead = true;
				if (shouldBeLocked == Option::Off) lockedToHead = false;
			}
			lockedToHeadForced = shouldBeLocked == Option::On;
		}

		float prevControlActiveFor = controlActiveFor.get(0.0f);
		if (_controlActive)
		{
			controlActiveFor = controlActiveFor.get(0.001f) + _deltaTime;
		}
		else
		{
			controlActiveFor.clear();
		}

		if (shouldBeLocked != Option::Allow || requestedToShow != Main)
		{
			// if not in allow or not in main, always switch on press
			if (!controlConsumed)
			{
				switchRequestedOnInput = controlActiveFor.get(0.0f) > 0.0f && prevControlActiveFor <= 0.0f;
				if (switchRequestedOnInput)
				{
					controlConsumed = true;
				}
			}
		}
		else
		{
			float threshold = 1.0f;
			if (controlActiveFor.get(0.0f) > threshold && prevControlActiveFor <= threshold)
			{
				// held for a while, lock or unlock
				lockedToHead = ! lockedToHead;
				forcedMainChange = requestedToShow == Main;
				clearEverything = !forcedMainChange;
				controlConsumed = true;
				done_tip(PilgrimOverlayInfoTip::InputLock);
			}
			else
			{
				if (!controlConsumed)
				{
					if (lockedToHead)
					{
						// switch on press
						switchRequestedOnInput = controlActiveFor.get(0.0f) > 0.0f && prevControlActiveFor <= 0.0f;
						if (switchRequestedOnInput)
						{
							controlConsumed = true;
						}
					}
					else
					{
						if (!controlActiveFor.is_set() && prevControlActiveFor > 0.0f && prevControlActiveFor <= threshold)
						{
							// tapped (just released), switch requested
							switchRequestedOnInput = true;
							if (switchRequestedOnInput)
							{
								controlConsumed = true;
							}
						}
						else
						{
							// wait while holding controls
						}
					}
				}
			}
		}

		if (!controlActiveFor.is_set())
		{
			// clear consumed post processing as we might be using this info for release
			controlConsumed = false;
		}
	}

	if (clearEverything)
	{
		// do everything to clear
#ifdef OUTPUT_OVERLAY_INFO_HIDE
		if (requestedToShow != None)
		{
			output(TXT("[POvIn:hide] clear everything"));
		}
#endif
		requestedToShow = None;
		if (currentTip.is_set() &&
			timeToDeactivateCurrentTip.is_set())
		{
			timeToDeactivateCurrentTip = 0.0f;
		}
		clear_main_log();
	}

	// object descriptor only
	{
		if (clearEverything && ! mayRequestLookingForPickups)
		{
			clear_object_descriptions();
		}
		else
		{
			advance_object_descriptor(_deltaTime);
		}
	}

	if (currentTip.is_set() &&
		timeToDeactivateCurrentTip.is_set())
	{
		timeToDeactivateCurrentTip = timeToDeactivateCurrentTip.get() - _deltaTime;
		if (timeToDeactivateCurrentTip.get() < 0.0f)
		{
			hide_tips();
		}
	}

	if (currentlyShowing == Main)
	{
		pilgrimStats.timeSinceUpdateGameStats += _deltaTime;
		if (pilgrimStats.timeSinceUpdateGameStats >= 0.1f)
		{
			pilgrimStats.timeSinceUpdateGameStats = 0.0f;
			Game::get_as<Game>()->update_game_stats();
		}
	}

	{
		// we always want to update inOpenWorldCell as knownLocation uses it, even in autoMapOnly mode
		Framework::Room* currentlyInRoom = owner->get_presence()->get_in_room();
		auto* piow = PilgrimageInstanceOpenWorld::get();
		if (currentlyInRoom != inRoom ||
			inOpenWorldCellForPilgrimageRef != piow)
		{
			inRoom = currentlyInRoom;
			Optional<VectorInt2> inCell;
			Optional<Vector3> inCellVisible;
			if (inOpenWorldCellForPilgrimageRef != piow)
			{
				inOpenWorldCell.clear();
				inOpenWorldCellForPilgrimageRef = piow;
			}
			if (piow)
			{
				inCell = piow->find_cell_at(owner.get());
				inCellVisible = piow->find_cell_at_for_map(owner.get());
				if ((!inOpenWorldCell.is_set() && inCell.is_set()) ||
					(inOpenWorldCell.is_set() && inCell.is_set() && inOpenWorldCell.get() != inCell.get()))
				{
					auto wasInOpenWorldCell = inOpenWorldCell;
					inOpenWorldCell = inCell;
					if (!GameSettings::get().difficulty.autoMapOnly &&
						!GameDirector::get()->is_guidance_blocked())
					{
						piow->mark_cell_visited(inCell.get());
						if (!GameSettings::get().difficulty.showDirectionsOnlyToObjective)
						{
							auto r = piow->query_cell_texture_parts_at(inCell.get());
							inOpenWorldCellInfoTextures.clear();
							for_every(e, r.elements)
							{
								InOpenWorldCellInfoTexture cit;
								cit.offset = e->offset;
								cit.texturePartUse = e->texturePartUse;

								if (e->cellDir.is_set() &&
									e->texturePartUseEntrance.texturePart &&
									wasInOpenWorldCell.is_set() &&
									inOpenWorldCell.is_set() &&
									(wasInOpenWorldCell.get() - inOpenWorldCell.get()) == e->cellDir.get())
								{
									cit.altOffsetPt = cit.offset * -0.6f;
									cit.altTexturePartUse = e->texturePartUseEntrance;
									cit.altColourOverride = colourCellInfoEnterFrom;
									cit.periodTime = 1.0f;
									cit.altPt = 0.5f;
									cit.altFirst = true;
									cit.altOnTop = true;
								}

								inOpenWorldCellInfoTextures.push_back(cit);
							}
						}
						showOpenWorldCellInfo = wasInOpenWorldCell.is_set(); // otherwise we may get strange location of info
					}
					inOpenWorldCell = inCell; // always store
				}
				if ((!inOpenWorldCellVisible.is_set() && inCellVisible.is_set()) ||
					(inOpenWorldCellVisible.is_set() && inCellVisible.is_set() && inOpenWorldCellVisible.get() != inCellVisible.get()))
				{
					if (!GameSettings::get().difficulty.autoMapOnly)
					{
						if (showMap.visible)
						{
							// just update map
							showMap.rd.mark_new_data_required();
						}
					}
					inOpenWorldCellVisible = inCellVisible; // always store
				}
				if (!GameSettings::get().difficulty.autoMapOnly)
				{
					if (inCell.is_set())
					{
						int owDist = inRoom->get_tags().get_tag_as_int(NAME(openWorldExterior));
						{
							if (piow->mark_cell_known(inCell.get(), owDist))
							{
								if (showMap.visible)
								{
									showMap.rd.mark_new_data_required();
								}
							}
						}
					}
				}
			}
		}
	}

	if (showMap.newAvailable)
	{
#ifdef OUTPUT_OVERLAY_INFO
		output(TXT("[POvIn] new map available? consume"));
#endif
		set_reference_placement(); // force new placement, we want the map to appear in front of us

		showMap.newAvailable = false;
		if (currentlyShowing == Main || showMap.visible)
		{
#ifdef OUTPUT_OVERLAY_INFO
			output(TXT("[POvIn] in main, mark new data required"));
#endif

			// just update map
			showMap.rd.mark_new_data_required();
		}
		else if (currentlyShowing == None || currentlyShowing == ShowInfo || requestedToShow == None)
		{
#ifdef OUTPUT_OVERLAY_INFO
			output(TXT("[POvIn] show map (currently %S -> requested %S)"), to_char(currentlyShowing), to_char(requestedToShow));
#endif

			if (!GameSettings::get().difficulty.noOverlayInfo)
			{
				requestedToShow = Main;
				mapOffset = VectorInt2::zero;
			}
		}
	}

	if (currentlyShowing == Main)
	{
		update_pilgrim_stats(owner.get());
		update_equipment_stats(owner.get());
		update_subtitle_log();
	}

	if (currentlyShowing == Investigate)
	{
		update_investigate_info();
	}

	{
		if (showMainLog.clearMainLogLineIndicatorOnNoVoiceoverActor.is_valid())
		{
			if (auto* vos = VoiceoverSystem::get())
			{
				if (!vos->is_speaking(showMainLog.clearMainLogLineIndicatorOnNoVoiceoverActor))
				{
					showMainLog.clearMainLogLineIndicatorOnNoVoiceoverActor = Name::invalid();
					update_main_log(false, false);
				}
			}
		}
	}

	if (currentlyShowing == Main)
	{
		if (owner.get())
		{
			if (auto* mp = owner->get_gameplay_as<ModulePilgrim>())
			{
				// make them appear in main if they were not visible
				if (equipmentsStats.showInHandReasons[Hand::Left].is_empty() &&
					mp->get_hand_equipment(Hand::Left))
				{
					show_in_hand_equipment_stats(Hand::Left, NAME(showAllEquipmentStats));
				}
				if (equipmentsStats.showInHandReasons[Hand::Right].is_empty() &&
					mp->get_hand_equipment(Hand::Right))
				{
					show_in_hand_equipment_stats(Hand::Right, NAME(showAllEquipmentStats));
				}
			}
		}
	}

	if (!equipmentsStats.showInHandReasons[Hand::Left].is_empty() ||
		!equipmentsStats.showInHandReasons[Hand::Right].is_empty())
	{
		update_in_hand_equipment_stats(_deltaTime);
	}

	update_symbol_replacements(_deltaTime);

	update_turn_counter_info();

	update_exm_infos(owner.get());

	if (auto* game = Game::get_as<Game>())
	{
		if (auto* overlayInfoSystem = game->access_overlay_info_system())
		{
			if (!showInfos.is_empty())
			{
				Concurrency::ScopedSpinLock lock(showInfosLock);
				for_every(showInfo, showInfos)
				{
					if (showInfo->visible &&
						showInfo->hideWhenMovesTooFarFrom.is_set() && showInfo->hideWhenMovesTooFarFromDist > 0.0f)
					{
						float dist = (showInfo->hideWhenMovesTooFarFrom.get().get_translation() - overlayInfoSystem->get_camera_placement().get_translation()).length();
						if (dist > showInfo->hideWhenMovesTooFarFromDist)
						{
							showInfo->visible = false;
							if (auto* e = showInfo->element.get())
							{
								e->deactivate();
							}
							showInfo->element.clear();
						}
					}
				}
			}

			if (requestedToShow == Main)
			{
				forcedMainChange |= bootLevelRequested != bootLevel;
			}
			bootLevel = bootLevelRequested;
			{
				bool newShowHealthStatus = is_at_boot_level(4) && ! GameDirector::get()->is_health_status_blocked();
				bool newShowAmmoStatus = is_at_boot_level(4) && !GameDirector::get()->is_ammo_status_blocked();
				bool newShowMainHealthStatus = newShowHealthStatus;
				if (auto* imo = owner.get())
				{
					if (auto* h = imo->get_custom<CustomModules::Health>())
					{
						if (h->is_super_health_storage())
						{
							newShowMainHealthStatus = false;
						}
					}
				}
				forcedMainChange |= newShowHealthStatus != showHealthStatus;
				forcedMainChange |= newShowMainHealthStatus != showMainHealthStatus;
				forcedMainChange |= newShowAmmoStatus != showAmmoStatus;
				showHealthStatus = newShowHealthStatus;
				showMainHealthStatus = newShowMainHealthStatus;
				showAmmoStatus = newShowAmmoStatus;
				fullUpdateEquipmentStatsRequired |= !showAmmoStatus; // require it only if not visible
			}
			if (disableWhenMovesTooFarFrom.is_set() && disableWhenMovesTooFarFromDist > 0.0f)
			{
				float dist = (disableWhenMovesTooFarFrom.get().get_translation() - overlayInfoSystem->get_camera_placement().get_translation()).length();
				if (dist > disableWhenMovesTooFarFromDist)
				{
#ifdef OUTPUT_OVERLAY_INFO_HIDE
					if (requestedToShow != None)
					{
						output(TXT("[POvIn:hide] moved too far away"));
					}
#endif
					requestedToShow = None;
					disableWhenMovesTooFarFrom.clear();
				}
			}
			if (showMap.visible && disableMapWhenMovesTooFarFrom.is_set() && disableMapWhenMovesTooFarFromDist > 0.0f)
			{
				float dist = (disableMapWhenMovesTooFarFrom.get().get_translation() - overlayInfoSystem->get_camera_placement().get_translation()).length();
				if (dist > disableMapWhenMovesTooFarFromDist)
				{
#ifdef OUTPUT_OVERLAY_INFO_HIDE
					output(TXT("[POvIn:hide] hide map, moved too far away"));
#endif
					hide_map(overlayInfoSystem);
					disableMapWhenMovesTooFarFrom.clear();
				}
			}
			{
				bool shouldWideStats = should_use_wide_stats();
				if (wideStats != shouldWideStats)
				{
					wideStats = shouldWideStats;
					redo_main_overlay_elements();
				}
			}
			if (requestedToShow == Main)
			{
				VectorInt2 newMapStick = TypeConversions::Normal::f_i_closest(_controlStick);
				if (mapStick.is_zero())
				{
					mapStickSameTimeLimit = 0.4f;
				}
				if (mapStick == newMapStick)
				{
					if (!newMapStick.is_zero())
					{
						mapStickSameFor += _deltaTime;
						if (mapStickSameFor >= mapStickSameTimeLimit)
						{
							mapStickSameTimeLimit = max(0.1f, mapStickSameTimeLimit * 0.75f);
							mapStick = VectorInt2::zero; // will allow change
						}
					}
				}
				VectorInt2 changeMapOffsetBy = VectorInt2::zero;
				if (mapStick.x == 0 && newMapStick.x != 0)
				{
					changeMapOffsetBy.x = newMapStick.x;
				}
				if (mapStick.y == 0 && newMapStick.y != 0)
				{
					changeMapOffsetBy.y = newMapStick.y;
				}
				mapStick = newMapStick;
				if (!changeMapOffsetBy.is_zero())
				{
					mapStickSameFor = 0.0f;
					mapOffset += changeMapOffsetBy;
					showMap.rd.mark_new_data_required();
				}
			}
			if (GameDirector::get()->is_pilgrim_overlay_blocked())
			{
#ifdef OUTPUT_OVERLAY_INFO_HIDE
				if (requestedToShow != None)
				{
					output(TXT("[POvIn:hide] blocked"));
				}
#endif
				requestedToShow = None;
			}
			else
			{
				if (switchRequestedOnInput)
				{
					if (requestedToShow == Main ||
						GameSettings::get().difficulty.noOverlayInfo)
					{
#ifdef OUTPUT_OVERLAY_INFO_HIDE
						if (requestedToShow != None)
						{
							output(TXT("[POvIn:hide] control active"));
						}
#endif
						requestedToShow = None;
						done_tip(PilgrimOverlayInfoTip::InputOpen);
					}
					else
					{
						keepShowingFor = 0.0f;
						requestedToShow = Main;
						mapOffset = VectorInt2::zero;
						done_tip(PilgrimOverlayInfoTip::InputOpen);
					}
				}
			}
			if (keepShowingFor > 0.0f)
			{
				keepShowingFor = max(0.0f, keepShowingFor - _deltaTime);
				if (keepShowingFor == 0.0f)
				{
#ifdef OUTPUT_OVERLAY_INFO_HIDE
					if (requestedToShow != None)
					{
						output(TXT("[POvIn:hide] keep showing for reached 0"));
					}
#endif
					requestedToShow = None;
				}
			}
			if (showOpenWorldCellInfo)
			{
				showOpenWorldCellInfo = false;
				if (inOpenWorldCell.is_set())
				{
					add_info_about_cell(owner.get(), overlayInfoSystem, 3.0f);
				}
			}
			if (requestedToShow == None && !showInfos.is_empty())
			{
				requestedToShow = ShowInfo;
			}
			if (currentlyShowing != requestedToShow || (currentlyShowing == Main && forcedMainChange))
			{
#ifdef OUTPUT_OVERLAY_INFO
				output(TXT("[POvIn] change showing! %S -> %S"), to_char(currentlyShowing), to_char(requestedToShow));
#endif

				if (requestedToShow == None)
				{
					// reset locked to head status
					lockedToHead = lockedToHeadForced;
				}

				if (!forcedMainChange)
				{
					disableWhenMovesTooFarFrom.clear();
					disableMapWhenMovesTooFarFrom.clear();
				}
				if (currentlyShowing == Main)
				{
					internal_hide_exms(NAME(pilgrimOverlayInfoMain));
					internal_hide_permanent_exms(NAME(pilgrimOverlayInfoMain));
					if (!forcedMainChange)
					{
						hide_map(overlayInfoSystem);
					}

					hide_in_hand_equipment_stats(Hand::Left, NAME(showAllEquipmentStats));
					hide_in_hand_equipment_stats(Hand::Right, NAME(showAllEquipmentStats));

					showMap.knownLocation = false; // no longer known
					showMap.showKnownLocation.clear();
					if (showMap.lastValidLocation.is_set())
					{
						showMap.lastLocation = showMap.lastValidLocation;
					}
					else if (showMap.lastLocation.is_set())
					{
						showMap.lastLocation = showMap.lastLocation.get() + mapOffset;
					}
					mapOffset = VectorInt2::zero; //
				}
				if (currentlyShowing == ShowInfo && (requestedToShow != Main || !allowShowInfoInMain))
				{
					Concurrency::ScopedSpinLock lock(showInfosLock);
					for_every(showInfo, showInfos)
					{
						if (showInfo->allowAutoHide)
						{
							showInfo->visible = false;
							if (auto* e = showInfo->element.get())
							{
								e->deactivate();
							}
							showInfo->element.clear();
						}
					}
				}
				{
					Concurrency::ScopedSpinLock lock(keepOverlayInfoElementsLock);
					overlayInfoSystem->deactivate_but_keep(keepOverlayInfoElements, true);
				}
				if (!forcedMainChange)
				{
					clear_reference_placement_if_possible(); // if we're none right now
				}
				if (currentlyShowing == Main ||
					(requestedToShow == Main && currentlyShowing != requestedToShow))
				{
					// clear, we will be displaying main log attached to us or not displaying at all
					update_main_log(true);
				}
				if (currentlyShowing == Investigate)
				{
					overlayInfoSystem->mark_reset_anchor_placement();
				}
				//
				//
				//
				// actual change
				currentlyShowing = requestedToShow;
				//
				//
				//
				if (currentlyShowing == PilgrimOverlayInfo::Main)
				{
					Framework::GameScript::ScriptExecution::trigger_execution_trap(NAME(pilgrimOverlayInfoMain));
				}
				if (!forcedMainChange)
				{
					clear_reference_placement_if_possible();
				}
				if (currentlyShowing == None)
				{
#ifdef OUTPUT_OVERLAY_INFO
					output(TXT("[POvIn] showing nothing, clear ref placement"));
#endif
					clear_reference_placement_if_possible(); // if we switched to none
				}
				else if (currentlyShowing != ShowInfo)
				{
#ifdef OUTPUT_OVERLAY_INFO
					output(TXT("[POvIn] showing something else than show info, set ref placement"));
#endif
					set_reference_placement_if_not_set();
				}

				if (currentlyShowing == Main)
				{
					if (lockedToHead)
					{
						disableWhenMovesTooFarFrom.clear();
					}
					else
					{
						float dist = 0.2f;
						disableWhenMovesTooFarFrom = Transform(overlayInfoSystem->get_camera_placement().get_translation() + overlayInfoSystem->get_camera_placement().get_axis(Axis::Forward) * dist, overlayInfoSystem->get_camera_placement().get_orientation());
						disableWhenMovesTooFarFromDist = 0.45f;
					}
					if (!forcedMainChange)
					{
						show_map(owner.get(), overlayInfoSystem);
					}
					else
					{
						// we could switch between locked states
						update_disable_map_when_moves_too_far_from(overlayInfoSystem);
					}
					add_main(owner.get(), overlayInfoSystem);
					show_exms(NAME(pilgrimOverlayInfoMain));
					show_permanent_exms(NAME(pilgrimOverlayInfoMain));

					//show_tip(PilgrimOverlayInfoTip::InputOpen); handled by tutorial

					show_in_hand_equipment_stats(Hand::Left, NAME(showAllEquipmentStats));
					show_in_hand_equipment_stats(Hand::Right, NAME(showAllEquipmentStats));

					showMainLog.displayedLines = 0; // redraw all

					if (allowShowInfoInMain)
					{
						add_show_info(owner.get(), overlayInfoSystem);
					}
				}
				if (currentlyShowing == ShowInfo)
				{
					float dist = 0.2f;
					disableWhenMovesTooFarFrom = Transform(overlayInfoSystem->get_camera_placement().get_translation() + overlayInfoSystem->get_camera_placement().get_axis(Axis::Forward) * dist, overlayInfoSystem->get_camera_placement().get_orientation());
					disableWhenMovesTooFarFromDist = 0.45f;
					add_show_info(owner.get(), overlayInfoSystem);
				}
				if (currentlyShowing == Investigate)
				{
					disableWhenMovesTooFarFrom.clear();
				}
				if (currentlyShowing == Investigate)
				{
					update_investigate_log(false); // display
					update_main_log(true); // hide/clear
				}
				else
				{
					update_main_log(false); // redisplay main log
					update_investigate_log(true); // hide/clear
				}
			}
			else
			{
				if ((currentlyShowing == ShowInfo || (allowShowInfoInMain && currentlyShowing == Main)) &&
					updateShowInfos)
				{
					add_show_info(owner.get(), overlayInfoSystem);
				}
			}
		}
	}

	// do these post currentlyShowing change as we want to have reference placement updated properly

	if (!PlayerPreferences::should_show_map() || GameDirector::get()->is_map_blocked())
	{
		showMap.visible = false;
		showMap.rd.clear();
	}
	else if (showMap.rd.should_do_new_data_to_render())
	{
		showMap.rd.clear();

		if (auto* piow = PilgrimageInstanceOpenWorld::get())
		{
			if (piow->get_pilgrimage()->open_world__get_definition()->is_map_available())
			{
				Optional<VectorInt3> showMapAtCell;
				Vector3 showMapAtCellOffset = Vector3::zero;
				if (inOpenWorldCellForPilgrimageRef == piow)
				{
					if (inOpenWorldCellVisible.is_set())
					{
						showMapAtCellOffset = inOpenWorldCellVisible.get();
						showMapAtCell = TypeConversions::Normal::f_i_closest(inOpenWorldCellVisible.get());
						showMapAtCellOffset -= showMapAtCell.get().to_vector3();
					}
				}
				if (!showMapAtCell.is_set())
				{
					// starting room exists not in a cell
					if (owner.get() && owner->get_presence()->get_in_room() == piow->get_starting_room())
					{
						showMapAtCell = piow->get_start_at().to_vector_int3();
						showMapAtCellOffset = Vector3::zero;
					}
				}
				{
					if (!GameSettings::get().difficulty.autoMapOnly)
					{
						showMap.knownLocation = true;
						showMap.showKnownLocation.clear();
					}
					if (showMap.knownLocation)
					{
						if (showMap.showKnownLocation.is_set())
						{
							showMap.lastKnownLocation = showMap.showKnownLocation;
						}
						else
						{
							showMap.lastKnownLocation = showMapAtCell.is_set() ? Optional<VectorInt2>(showMapAtCell.get().to_vector_int2()) : NP;
						}
					}
					// override with last location, unless we know location
					if (GameSettings::get().difficulty.autoMapOnly && !showMap.knownLocation && showMap.lastLocation.is_set())
					{
						showMapAtCell = showMap.lastLocation.get().to_vector_int3();
						showMapAtCellOffset = Vector3::zero;
					}
					else
					{
						showMap.lastLocation = showMapAtCell.is_set() ? Optional<VectorInt2>(showMapAtCell.get().to_vector_int2()) : NP;
					}
				}
				if (showMapAtCell.is_set())
				{
					RangeInt2 mapRange = RangeInt2::empty;
					mapRange.include(showMapAtCell.get().to_vector_int2());
					VectorInt2 mapRangeOffset = mapOffset; // to fix situation when we start with an offset (when location is unknown and not fixed

					VectorInt2 showCentreAt = showMapAtCell.get().to_vector_int2();
				
					VectorInt3 markPresentAt = showMapAtCell.get();
					Vector3 markPresentAtCentre = showMapAtCellOffset;
					if (GameSettings::get().difficulty.autoMapOnly)
					{
						if (showMap.lastKnownLocation.is_set())
						{
							markPresentAt = showMap.lastKnownLocation.get().to_vector_int3();
						}
					}

					bool largerX = false;
					bool largerY = false;
					int mapMaxSize = hardcoded 12;
					bool allowShowingWholeMap = GameSettings::get().difficulty.showLineModelAlwaysOtherwiseOnMap;
					if (auto* owDef = piow->get_pilgrimage()->open_world__get_definition())
					{
						if (owDef->is_whole_map_known())
						{
							allowShowingWholeMap = true;
						}
						RangeInt2 mapSize = RangeInt2::empty;
						if (allowShowingWholeMap)
						{
							mapSize = owDef->get_size();
							if (!mapSize.x.is_empty() && mapSize.x.length() + 2 <= mapMaxSize)
							{
								mapRange.x = mapSize.x;
								mapRange.x.expand_by(1);
								{
									if (showMap.knownLocation)
									{
										showCentreAt.x = mapRange.x.centre();
									}
									mapRangeOffset.x = -(mapRange.x.centre() - showCentreAt.x);
								}
								mapOffset.x = 0;
								largerX = true;
							}
							if (!mapSize.y.is_empty() && mapSize.y.length() + 2 <= mapMaxSize)
							{
								mapRange.y = mapSize.y;
								mapRange.y.expand_by(1);
								{
									if (showMap.knownLocation)
									{
										showCentreAt.y = mapRange.y.centre();
									}
									mapRangeOffset.y = -(mapRange.y.centre() - showCentreAt.y);
								}
								mapOffset.y = 0;
								largerY = true;
							}
						}
						// now grow
						if (!largerX && !largerY)
						{
							int radius = mapMaxSize / 2;
							mapRange.x.expand_by(radius);
							mapRange.y.expand_by(radius);
						}
						else
						{
							if (mapSize.x.is_empty())
							{
								mapRange.x.expand_by(mapRange.y.length() / 2);
							}
							if (mapSize.y.is_empty())
							{
								mapRange.y.expand_by(mapRange.x.length() / 2);
							}
						}
					}

					// force include the piece where we are
					mapRange.include(showMapAtCell.get().to_vector_int2());

					// store where we are
					RangeInt2 mapRangeAtPilgrim = mapRange;

					mapRange.x.min += mapRangeOffset.x;
					mapRange.x.max += mapRangeOffset.x;
					mapRange.y.min += mapRangeOffset.y;
					mapRange.y.max += mapRangeOffset.y;

					{
						VectorInt2 startAt = piow->get_start_at();
						if ((mapRange.x.does_contain(startAt.x) && (mapRange.y.max + 1 == startAt.y || mapRange.y.min - 1 == startAt.y)) ||
							(mapRange.y.does_contain(startAt.y) && (mapRange.x.max + 1 == startAt.x || mapRange.x.min - 1 == startAt.x)))
						{
							// include start if is just right beyond the visible range
							mapRange.include(startAt);
						}
					}

					bool showMarkPresent = false;

					{
						ARRAY_STACK(CachedCellQueryResult, cqrs, mapRange.x.length() * mapRange.y.length());
						cqrs.set_size(cqrs.get_max_size());

						// gather cqrs
						{
							for_range(int, y, mapRange.y.min, mapRange.y.max)
							{
								for_range(int, x, mapRange.x.min, mapRange.x.max)
								{
									VectorInt2 at(x, y);
									int cqrIdx = cqr_index(at, mapRange);
									an_assert(cqrIdx != NONE);
									auto& cqr = cqrs[cqrIdx];
									cqr = piow->query_cell_at(at);
								}
							}
						}

						// calculate scale
						{
							float scale = 1.0f;
							if (GameSettings::get().difficulty.autoMapOnly)
							{
								// move the showCentreAt it to where we are
								showCentreAt += mapOffset;

								scale = hardcoded 0.5f;
							}
							else
							{
								// get valid range for scale
								RangeInt2 validRange = RangeInt2::empty;
								if (mapRangeAtPilgrim == mapRange)
								{
									for_every(cqr, cqrs)
									{
										if (cqr->knownExits || cqr->known || !cqr->knownPilgrimageDevices.is_empty() ||
											cqr->lineModelAlwaysOtherwise)
										{
											validRange.include(cqr->at);
										}
									}
								}
								else
								{
									for_range(int, y, mapRangeAtPilgrim.y.min, mapRangeAtPilgrim.y.max)
									{
										for_range(int, x, mapRangeAtPilgrim.x.min, mapRangeAtPilgrim.x.max)
										{
											VectorInt2 at(x, y);
											CachedCellQueryResult cqr;
											cqr = piow->query_cell_at(at);
											if (cqr.knownExits || cqr.known || !cqr.knownPilgrimageDevices.is_empty() ||
												cqr.lineModelAlwaysOtherwise)
											{
												validRange.include(at);
											}
										}
									}
								}

								// if empty, include current location and a bit to have something, anything
								if (GameSettings::get().difficulty.autoMapOnly)
								{
									if (validRange.is_empty())
									{
										validRange.include(showCentreAt);
									}
								}

								// make it symmetrical
								{
									if (!largerX)
									{
										int maxOff = max(abs(validRange.x.min - showCentreAt.x), abs(validRange.x.max - showCentreAt.x));
										validRange.x = RangeInt(showCentreAt.x - maxOff, showCentreAt.x + maxOff);
									}
									if (!largerY)
									{
										int maxOff = max(abs(validRange.y.min - showCentreAt.y), abs(validRange.y.max - showCentreAt.y));
										validRange.y = RangeInt(showCentreAt.y - maxOff, showCentreAt.y + maxOff);
									}
								}

								if (auto* owdef = piow->get_pilgrimage()->open_world__get_definition())
								{
									if (owdef->should_always_shop_whole_map())
									{
										RangeInt2 mapSize = owdef->get_size();
										if (!mapSize.x.is_empty()) { validRange.x.include(mapSize.x); }
										if (!mapSize.y.is_empty()) { validRange.y.include(mapSize.y); }
									}
								}

								// move the validRange it to where we are
								{
									validRange.x.min += mapOffset.x;
									validRange.x.max += mapOffset.x;
									validRange.y.min += mapOffset.y;
									validRange.y.max += mapOffset.y;
								}

								// update show centre and scale using valid range 
								showCentreAt = validRange.centre();

								{
									scale = min(scale, (float)(mapMaxSize / 2) / (float)validRange.x.length());
									scale = min(scale, (float)(mapMaxSize / 2) / (float)validRange.y.length());
								}
							}

							showMap.rd.set_scale(scale);
						}

						if (markPresentAt.z == 0)
						{
							// we may not know the actual location of centre if we just check cqrs
							CachedCellQueryResult cqr;
							cqr = piow->query_cell_at(markPresentAt.to_vector_int2());
							cqr.fill(showCentreAt);
							markPresentAtCentre += cqr.relCentre.to_vector3();
						}
						else
						{
							// keep markPresentAtCentre as it was
						}

						for_every(cqr, cqrs)
						{
							cqr->fill(showCentreAt, true);

							if (cqr->priority <= 1)
							{
								cqr->colour = GameSettings::get().difficulty.autoMapOnly? colourKnownExitsAutoMapOnly : colourKnownExits;
							}
							if (cqr->priority == 2)
							{
								cqr->colour = GameSettings::get().difficulty.autoMapOnly ? colourKnownDevicesAutoMapOnly : colourKnownDevices;
							}
							if (cqr->priority >= 3)
							{
								cqr->colour = GameSettings::get().difficulty.autoMapOnly ? colourKnownDevicesAutoMapOnly /* yes, reuse */ : colourVisited;
							}
						}

						float scaleIcon = hardcoded 0.2f;

						Box mapBox;
						Range3 mapBoxRange;
						{
							mapBoxRange.z.min = -10.0f;
							mapBoxRange.z.max = 10.0f;
							mapBoxRange.x.min = (float)(mapRange.x.min - showCentreAt.x) - 0.5f;
							mapBoxRange.x.max = (float)(mapRange.x.max - showCentreAt.x) + 0.5f;
							mapBoxRange.y.min = (float)(mapRange.y.min - showCentreAt.y) - 0.5f;
							mapBoxRange.y.max = (float)(mapRange.y.max - showCentreAt.y) + 0.5f;
							mapBox.setup(mapBoxRange);
						}

						{
							Concurrency::ScopedSpinLock lock(showMap.highlightAtLock);
							for_every(hAt, showMap.highlightAt)
							{
								// rhi - render highlight info
								Vector3 rhiFrom;
								Vector3 rhiTo;
								float rhiSize;
								// get info
								{
									{
										VectorInt3 highlightFrom = markPresentAt;
										Vector3 highlightFromCentre = markPresentAtCentre;
										if (GameSettings::get().difficulty.autoMapOnly)
										{
											if (!hAt->from.is_set() &&
												showMap.lastKnownLocation.is_set())
											{
												hAt->from = showMap.lastKnownLocation.get().to_vector_int3();
											}
											if (!hAt->from.is_set())
											{
												warn(TXT("hAt->from should be known at the time"));
												hAt->from = markPresentAt;
											}
											highlightFrom = hAt->from.get();
											if (markPresentAt.z == 0)
											{
												highlightFromCentre = piow->calculate_rel_centre_for(markPresentAt.to_vector_int2()).to_vector3();
											}
											else
											{
												highlightFromCentre = Vector3::zero;
											}
										}
										if (highlightFrom == hAt->to)
										{
											// skip
											continue;
										}
										rhiFrom = (highlightFrom - showCentreAt.to_vector_int3()).to_vector3() + highlightFromCentre;
									}
									{
										CachedCellQueryResult cqr;
										cqr = piow->query_cell_at(hAt->to.to_vector_int2());
										cqr.fill(showCentreAt);
										rhiTo = (hAt->to - showCentreAt.to_vector_int3()).to_vector3() + cqr.relCentre.to_vector3();

										for_every(kpd, cqr.knownPilgrimageDevices)
										{
											// we assume one special device, so we get the first one
											if (kpd->device->is_special())
											{
												Vector2 off = kpd->device->get_slot_offset();
												if (!off.is_zero())
												{
													PilgrimageInstanceOpenWorld::rotate_slot(off, cqr.rotateSlots);
													float scaleOff = cqr.calculate_scale_off();
													rhiTo += (off * scaleOff).to_vector3();
													break;
												}
											}
										}
									}
									rhiSize = scaleIcon * 1.5f;
								}

								// render
								{
									float zOffset = 0.05f;
									Vector3 relAt = rhiTo;
									relAt.z += zOffset;
									float halfSize = rhiSize * 0.5f;
									Range3 range = Range3::empty;
									range.include(Vector3(relAt.x - halfSize, relAt.y - halfSize, relAt.z));
									range.include(Vector3(relAt.x + halfSize, relAt.y + halfSize, relAt.z));
									if (mapBoxRange.does_contain(rhiTo))
									{
										showMap.rd.add_line(Vector3(range.x.min, range.y.min, relAt.z), Vector3(range.x.min, range.y.max, relAt.z), colourHighlight);
										showMap.rd.add_line(Vector3(range.x.min, range.y.max, relAt.z), Vector3(range.x.max, range.y.max, relAt.z), colourHighlight);
										showMap.rd.add_line(Vector3(range.x.max, range.y.max, relAt.z), Vector3(range.x.max, range.y.min, relAt.z), colourHighlight);
										showMap.rd.add_line(Vector3(range.x.max, range.y.min, relAt.z), Vector3(range.x.min, range.y.min, relAt.z), colourHighlight);
									}

									if (rhiFrom != rhiTo)
									{
										Box box;
										range.z.min = -1.0f;
										range.z.max =  1.0f;
										box.setup(range);

										Segment s(((markPresentAt - showCentreAt.to_vector_int3()).to_vector3() + markPresentAtCentre), range.centre());

										if (box.check_segment(s))
										{
											Vector3 start = s.get_start();
											Vector3 end = s.get_hit();

											if (!mapBoxRange.does_contain(start))
											{
												Segment s(start, end);
												if (mapBox.check_segment(s))
												{
													start = s.get_hit();
												}
												else
												{
													// skip, we're not hitting/intersect box at all
													continue;
												}
											}

											if (!mapBoxRange.does_contain(end))
											{
												Segment s(end, start);
												if (mapBox.check_segment(s))
												{
													end = s.get_hit();
												}
											}

											showMap.rd.add_line(Vector3(start.x, start.y, start.z + zOffset), Vector3(end.x, end.y, end.z + zOffset), colourHighlight);
										}
									}
								}

							}
						}

						bool showsSomething = false;
						for_every(cqr, cqrs)
						{
							if (cqr->at == markPresentAt.to_vector_int2() && showMap.knownLocation)
							{
								showMarkPresent = true;
							}
							if (cqr->known && cqr->lineModel)
							{
								showsSomething = true;

								float z = 0.0f;
								Vector3 c = cqr->c.to_vector3(z);
								Vector3 f = DirFourClockwise::vector_to_world(Vector2::yAxis, cqr->dir).to_vector3();
								Transform p = look_matrix(c, f, Vector3::zAxis).to_transform();
								for_every(line, cqr->lineModel->get_lines())
								{
									showMap.rd.add_line(p.location_to_world(line->a), p.location_to_world(line->b), line->colour.get(colourLineModel));
								}
								if (cqr->known && cqr->lineModelAdditional)
								{
									showsSomething = true;

									float z = 0.0f;
									Vector3 c = cqr->c.to_vector3(z);
									Vector3 f = DirFourClockwise::vector_to_world(Vector2::yAxis, cqr->dir).to_vector3();
									Transform p = look_matrix(c, f, Vector3::zAxis).to_transform();
									for_every(line, cqr->lineModelAdditional->get_lines())
									{
										showMap.rd.add_line(p.location_to_world(line->a), p.location_to_world(line->b), line->colour.get(colourLineModel));
									}
								}
							}
							else if (auto* lm = cqr->lineModelAlwaysOtherwise)
							{
								showsSomething = true;

								float z = 0.0f;
								Vector3 c = cqr->c.to_vector3(z);
								Vector3 f = DirFourClockwise::vector_to_world(Vector2::yAxis, cqr->dir).to_vector3();
								Transform p = look_matrix(c, f, Vector3::zAxis).to_transform();
								for_every(line, lm->get_lines())
								{
									showMap.rd.add_line(p.location_to_world(line->a), p.location_to_world(line->b), line->colour.get(colourLineModel));
								}
							}
							if (! cqr->objectiveLineModels.is_empty()) // being known is handled with query result
							{
								showsSomething = true;

								float z = 0.0f;
								Vector3 c = cqr->c.to_vector3(z);
								Vector3 f = Vector3::yAxis;
								Transform p = look_matrix(c, f, Vector3::zAxis).to_transform();
								for_every_ptr(lm, cqr->objectiveLineModels)
								{
									for_every(line, lm->get_lines())
									{
										showMap.rd.add_line(p.location_to_world(line->a), p.location_to_world(line->b), line->colour.get(colourLineModel));
									}
								}
							}
							if (cqr->knownExits)
							{
								showsSomething = true;

								float z = 0.0f;
								if (cqr->cl != cqr->cr)
								{
									if (cqr->mapColour.is_set())
									{
										float c = min(0.075f, (cqr->cr.x - cqr->cl.x) * 0.25f);
										showMap.rd.add_triangle(cqr->cd.to_vector3(z) + Vector3(0.0f,  c, 0.0f), cqr->cl.to_vector3(z) + Vector3( c, 0.0f, 0.0f), cqr->cu.to_vector3(z) + Vector3(0.0f, -c, 0.0f), cqr->mapColour.get());
										showMap.rd.add_triangle(cqr->cu.to_vector3(z) + Vector3(0.0f, -c, 0.0f), cqr->cr.to_vector3(z) + Vector3(-c, 0.0f, 0.0f), cqr->cd.to_vector3(z) + Vector3(0.0f,  c, 0.0f), cqr->mapColour.get());
									}
									// inner diamond
									showMap.rd.add_line(cqr->cl.to_vector3(z), cqr->cu.to_vector3(z), cqr->colour);
									showMap.rd.add_line(cqr->cu.to_vector3(z), cqr->cr.to_vector3(z), cqr->colour);
									showMap.rd.add_line(cqr->cr.to_vector3(z), cqr->cd.to_vector3(z), cqr->colour);
									showMap.rd.add_line(cqr->cd.to_vector3(z), cqr->cl.to_vector3(z), cqr->colour);
								}
								if (cqr->exits[DirFourClockwise::Up])		showMap.rd.add_line(cqr->eu.to_vector3(z), cqr->bu.to_vector3(z), cqr->colour);
								if (cqr->exits[DirFourClockwise::Right])	showMap.rd.add_line(cqr->er.to_vector3(z), cqr->br.to_vector3(z), cqr->colour);
								if (cqr->exits[DirFourClockwise::Down])		showMap.rd.add_line(cqr->ed.to_vector3(z), cqr->bd.to_vector3(z), cqr->colour);
								if (cqr->exits[DirFourClockwise::Left])		showMap.rd.add_line(cqr->el.to_vector3(z), cqr->bl.to_vector3(z), cqr->colour);

								// perpendicular lines joining cells
								if (cqr->exits[DirFourClockwise::Left])
								{
									int leftIdx = cqr_index(VectorInt2(cqr->at.x - 1, cqr->at.y), mapRange);
									if (leftIdx != NONE)
									{
										auto& cqrLeft = cqrs[leftIdx];
										if (cqrLeft.knownExits)
										{
											Colour colour = cqrLeft.priority < cqr->priority ? cqrLeft.colour : cqr->colour;
											showMap.rd.add_line(cqr->bl.to_vector3(z), cqrLeft.br.to_vector3(z), colour);
										}
									}
								}
								if (cqr->exits[DirFourClockwise::Down])
								{
									int downIdx = cqr_index(VectorInt2(cqr->at.x, cqr->at.y - 1), mapRange);
									if (downIdx != NONE)
									{
										auto& cqrDown = cqrs[downIdx];
										if (cqrDown.knownExits)
										{
											Colour colour = cqrDown.priority > cqr->priority ? cqrDown.colour : cqr->colour;
											showMap.rd.add_line(cqr->bd.to_vector3(z), cqrDown.bu.to_vector3(z), colour);
										}
									}
								}
							}
							if (!cqr->knownPilgrimageDevices.is_empty())
							{
								float scaleOff = cqr->calculate_scale_off();
								Vector2 cc = (cqr->cr + cqr->cl) * 0.5f;

								for_every(kpd, cqr->knownPilgrimageDevices)
								{
									bool appearAsDepleted = kpd->depleted;
									if (!kpd->device->may_appear_on_map())
									{
										continue;
									}
									if (kpd->device->may_appear_on_map_only_if_not_depleted() && appearAsDepleted)
									{
										continue; // do not show it at all
									}
									if (kpd->device->may_appear_on_map_only_if_depleted() && !appearAsDepleted)
									{
										continue; // do not show it at all
									}
									if (kpd->device->may_appear_on_map_only_if_visited() && !kpd->visited)
									{
										continue; // do not show it at all
									}
									{
										// do this AFTER the checks as the checks determine if it should be visible or not and this is how it appears
										if (kpd->device->should_reverse_appear_depleted())
										{
											appearAsDepleted = ! appearAsDepleted;
										}
										else if (!kpd->device->may_appear_depleted())
										{
											appearAsDepleted = false;
										}
										else if (GameSettings::get().difficulty.autoMapOnly && !kpd->device->may_appear_depleted_for_auto_map_only())
										{
											appearAsDepleted = false;
										}
									}
									float alpha = appearAsDepleted ? 0.3f : 1.0f;

									if (auto* lm = piow->get_pilgrimage_device_line_model_for_map(kpd->device.get(), kpd))
									{
										showsSomething = true;

										Vector2 off = kpd->device->get_slot_offset();
										{
											PilgrimageInstanceOpenWorld::rotate_slot(off, cqr->rotateSlots);
											float z = 0.05f;
											Vector3 pdAt = (cc + scaleOff * off).to_vector3(z);
											Transform p = Transform(pdAt, Quat::identity, scaleIcon);
											for_every(line, lm->get_lines())
											{
												Colour colour = line->colour.get(colourLineModel);
												colour.a *= alpha;
												showMap.rd.add_line(p.location_to_world(line->a), p.location_to_world(line->b), colour);
											}
										}
									}
								}
							}
						}

						for_every(lmom, piow->get_pilgrimage()->open_world__get_definition()->get_line_models_always_on_map())
						{
							showsSomething = true;

							if (!lmom->storyFactsRequired.is_empty())
							{
								if (auto* gd = TeaForGodEmperor::GameDirector::get_active())
								{
									Concurrency::ScopedMRSWLockRead lock(gd->access_story_execution().access_facts_lock());
									if (!lmom->storyFactsRequired.check(gd->access_story_execution().get_facts()))
									{
										continue;
									}
								}
							}

							if (auto* lm = lmom->lineModel.get())
							{
								Vector3 lmomAt = lmom->at - showCentreAt.to_vector2().to_vector3();
								Transform p = Transform(lmomAt, Quat::identity);
								for_every(line, lm->get_lines())
								{
									Colour colour = line->colour.get(colourLineModel);
									showMap.rd.add_line(p.location_to_world(line->a), p.location_to_world(line->b), colour);
								}
							}
						}

						// draw circles for long distance detection, circles start at where we were detecting and go at radius of what we detected
						{
							PilgrimageInstanceOpenWorld::LongDistanceDetection tempLongDistanceDetection;
							tempLongDistanceDetection.copy_from(piow->get_long_distance_detection());

							struct DetectedDevice
							{
								VectorInt2 at;
								bool depleted = false;
								int detectionDistanceCount = 0;
								int detectionDirCount = 0;

								static float detection_by_direction_radius() { return 5.0f; }

								bool is_detected_by_distance() const { return detectionDistanceCount >= 3; }
								bool is_detected_by_direction() const { return detectionDirCount >= 2; }
								bool is_detected() const { return is_detected_by_distance() || is_detected_by_direction(); }

								static DetectedDevice const* get(Array<DetectedDevice> const& dd, VectorInt2 const& at)
								{
									for_every(d, dd)
									{
										if (d->at == at)
										{
											return d;
										}
									}
									return nullptr;
								}
							};
							Array<DetectedDevice> detectedDevices;

							bool triangulationAssist = ! GameSettings::get().difficulty.noTriangulationHelp;

							if (triangulationAssist)
							{
								for_every(d, tempLongDistanceDetection.detections)
								{
									for_every(e, d->elements)
									{
										VectorInt2 deviceAt = e->deviceAt;
										bool found = false;
										for_every(dd, detectedDevices)
										{
											if (dd->at == deviceAt)
											{
												if (e->directionTowards.is_set())
												{
													if (e->radius <= DetectedDevice::detection_by_direction_radius())
													{
														++dd->detectionDirCount;
													}
												}
												else
												{
													++dd->detectionDistanceCount;
												}
												found = true;
												break;
											}
										}
										if (!found)
										{
											detectedDevices.push_back();
											auto& dd = detectedDevices.get_last();
											dd.at = deviceAt;
											if (e->directionTowards.is_set())
											{
												if (e->radius <= DetectedDevice::detection_by_direction_radius())
												{
													dd.detectionDirCount = 1;
												}
											}
											else
											{
												dd.detectionDistanceCount = 1;
											}
											dd.depleted = piow->is_pilgrim_device_state_depleted_long_distance_detectable(deviceAt);
										}
									}
								}
							}

							for_every(d, tempLongDistanceDetection.detections)
							{
								float const zOffset = 0.1f;
								Vector3 cd = d->at.to_vector2().to_vector3();
								cd += piow->calculate_rel_centre_for(d->at).to_vector3();
								cd.x -= showCentreAt.x;
								cd.y -= showCentreAt.y;
								// try to use real / offset loc
								{
									for_every(cqr, cqrs)
									{
										if (cqr->at == d->at)
										{
											cd = (cqr->c + cqr->relCentre).to_vector3();
										}
									}
								}
								cd.z = zOffset;

								float a = (float)for_everys_index(d) / (float)PilgrimageInstanceOpenWorld::LongDistanceDetection::MAX_DETECTIONS;
								a = remap_value(a, 0.0f, 1.0f, 1.0f, 0.1f, true);
								a = a * a;

								Colour colour = colourLongDistanceDetection.with_set_alpha(a);
								Colour colourDir = colourLongDistanceDetectionByDirection.with_set_alpha(a);

								for_every(e, d->elements)
								{
									Colour useColour = e->directionTowards.is_set()? colourDir : colour;

									bool detectedDevice = false;

									if (triangulationAssist)
									{
										if (DetectedDevice const* dd = DetectedDevice::get(detectedDevices, e->deviceAt))
										{
											if (dd->depleted)
											{
												// skip this one
												continue;
											}

											if (e->directionTowards.is_set())
											{
												if (dd->is_detected() && e->radius <= DetectedDevice::detection_by_direction_radius())
												{
													detectedDevice = true;
													useColour = useColour.with_set_alpha(0.8f);
												}
												else
												{
													useColour = useColour.with_set_alpha(0.5f);
												}
											}
											else
											{
												if (dd->is_detected())
												{
													detectedDevice = true;
													useColour = useColour.with_set_alpha(0.6f);
												}
												else
												{
													useColour = useColour.with_set_alpha(0.25f);
												}
											}
										}
										else
										{
											continue;
										}
									}

									if (e->directionTowards.is_set())
									{
										Vector3 l0 = cd;
										Vector3 l1 = e->directionTowards.get().to_vector3();
										l1.x -= showCentreAt.x;
										l1.y -= showCentreAt.y;
										l1.z = l0.z;

										{
											float length = DetectedDevice::detection_by_direction_radius();
											if (detectedDevice)
											{
												length = (l1 - l0).length();
												length = max(length * 0.25f, length - 0.4f);
											}
											l1 = l0 + (l1 - l0).normal() * length;
										}

										showMap.rd.add_line_clipped(l0, l1, mapBoxRange, useColour);
									}
									else
									{
										float length = pi<float>() * 2.0f * e->radius;
										float const step = 0.3f;

										int stepCount = max(8, (int)(length / step));

										Vector3 v = Vector3::yAxis * e->radius;

										float stepCountInv = 1.0f / (float)stepCount;
										Vector3 prevP = cd + v;
										for (int i = 0; i < stepCount; ++i)
										{
											float a = (float)(i + 1) * stepCountInv * 360.0f;

											Vector3 p = cd + v.rotated_by_yaw(a);

											showMap.rd.add_line_clipped(prevP, p, mapBoxRange, useColour);

											prevP = p;
										}
									}
								}
							}

							if (triangulationAssist)
							{
								for_every(dd, detectedDevices)
								{
									if (dd->is_detected() &&
										! dd->depleted)
									{
										Vector3 cd = dd->at.to_vector2().to_vector3();
										cd += piow->calculate_rel_centre_for(dd->at).to_vector3();
										cd.x -= showCentreAt.x;
										cd.y -= showCentreAt.y;
										// try to use real / offset loc
										{
											for_every(cqr, cqrs)
											{
												if (cqr->at == dd->at)
												{
													cd = (cqr->c + cqr->relCentre).to_vector3();
												}
											}
										}

										Colour colour = dd->is_detected_by_direction()? colourLongDistanceDetectionByDirection : colourLongDistanceDetection;

										Vector3 px = Vector3::xAxis * 0.3f;
										Vector3 py = Vector3::yAxis * 0.3f;
										showMap.rd.add_line_clipped(cd + py, cd + px, mapBoxRange, colour);
										showMap.rd.add_line_clipped(cd - py, cd + px, mapBoxRange, colour);
										showMap.rd.add_line_clipped(cd + py, cd - px, mapBoxRange, colour);
										showMap.rd.add_line_clipped(cd - py, cd - px, mapBoxRange, colour);
									}
								}
							}
						}

						if (showsSomething)
						{
							showMap.lastValidLocation = showCentreAt;
						}
					}

					// highlight where we are
					if (showMarkPresent)
					{
						float zOffset = 0.1f;
						Vector3 relAt = (markPresentAt - showCentreAt.to_vector_int3()).to_vector3() + markPresentAtCentre;
						relAt.z += zOffset;
						float size = 0.4f;
						showMap.rd.add_line(Vector3(relAt.x - size, relAt.y, relAt.z), Vector3(relAt.x, relAt.y + size, relAt.z), colourHere);
						showMap.rd.add_line(Vector3(relAt.x + size, relAt.y, relAt.z), Vector3(relAt.x, relAt.y + size, relAt.z), colourHere);
						showMap.rd.add_line(Vector3(relAt.x - size, relAt.y, relAt.z), Vector3(relAt.x, relAt.y - size, relAt.z), colourHere);
						showMap.rd.add_line(Vector3(relAt.x + size, relAt.y, relAt.z), Vector3(relAt.x, relAt.y - size, relAt.z), colourHere);
					}
				}
			}
		}

		showMap.rd.mark_new_data_done();
		showMap.rd.mark_data_available_to_render();
	}

	update_speaking(_deltaTime);
}

void PilgrimOverlayInfo::add_main(Framework::IModulesOwner* _owner, OverlayInfo::System* _overlayInfoSystem)
{
	add_pilgrim_stats(_owner, _overlayInfoSystem);

	equipmentsStats.copy_equipment_stats_from(EquipmentsStats());
	update_equipment_stats(_owner);
}

void get_extra_exms(Framework::IModulesOwner* _owner, ArrayStatic<Name, 8>& extraEXMs)
{
	if (auto* mp = _owner->get_gameplay_as<ModulePilgrim>())
	{
		extraEXMs.push_back(NAME(exmMeleeDamage));
		if (!PilgrimBlackboard::get_health_for_melee_kill(_owner).is_zero())
		{
			extraEXMs.push_back(NAME(exmMeleeKillHealth));
		}
		if (!PilgrimBlackboard::get_ammo_for_melee_kill(_owner).is_zero())
		{
			extraEXMs.push_back(NAME(exmMeleeKillAmmo));
		}
		if (mp->get_equipped_exm(EXMID::Passive::vampire_fists()))
		{
			extraEXMs.push_back(EXMID::Passive::vampire_fists());
		}
		if (mp->get_equipped_exm(EXMID::Passive::beggar_fists()))
		{
			extraEXMs.push_back(EXMID::Passive::beggar_fists());
		}
	}
}

bool PilgrimOverlayInfo::should_use_wide_stats() const
{
	return (!PlayerPreferences::should_show_map() || !showMap.visible) && showMainLog.lines.is_empty() && !GameDirector::get()->are_wide_stats_forced();
}

void PilgrimOverlayInfo::add_pilgrim_stats(Framework::IModulesOwner* _owner, OverlayInfo::System* _overlayInfoSystem)
{
	_overlayInfoSystem->deactivate(NAME(pilgrimStat));

	pilgrimStats = PilgrimStats();

	Game::get_as<Game>()->update_game_stats();
	pilgrimStats.timeSinceUpdateGameStats = 0.0f;

	float size = 0.025f;
	float verticalSpacingPt = 0.2f;
	Vector3 locDown = Vector3::zAxis * (-size * (1.0f + verticalSpacingPt));
	Vector3 locSide = Vector3::xAxis * (size * 4.0f);
	//Transform offsetUp = Transform(-locDown, Quat::identity);
	Transform offsetDown = Transform(locDown, Quat::identity);

	Vector3 dir = Vector3(0.3f, 0.5f, 0.0f).normal();

	wideStats = should_use_wide_stats();

	if (wideStats)
	{
		dir = Vector3(0.2f, 0.5f, 0.0f).normal();
	}

	// right side 
	{
		int side = 1;
		Vector3 useDir = dir;
		Transform placement = get_vr_placement_for_local(_owner, matrix_from_up_forward(useDir * statsDist + Vector3::zAxis * (0.0f), Vector3(0.0f, 0.0f, 1.0f), useDir).to_transform());
		Transform placementSide = placement;
		Transform offsetSide = Transform(locSide, Quat::identity);

#ifdef WITH_CRYSTALITE
		if (GameSettings::get().difficulty.withCrystalite)
		{
			add_pilgrim_stat_at(_overlayInfoSystem, pilgrimStats.crystalite.element, side, placement, size, nullptr, oipCrystalite.get());
			placement = placement.to_world(offsetDown);
		}
#endif
		if (showMainHealthStatus && showHealthStatus && is_at_boot_level(3)) add_pilgrim_stat_at(_overlayInfoSystem, pilgrimStats.healthMain.element, side, placement, size, nullptr, oipHealth.get());
		placement = placement.to_world(offsetDown);
		if (showHealthStatus && is_at_boot_level(3)) add_pilgrim_stat_at(_overlayInfoSystem, pilgrimStats.health.element, side, placement, size, oipStorage.get(), oipHealth.get());
		placementSide = placement.to_world(offsetSide);
		if (showHealthStatus && is_at_boot_level(3)) add_pilgrim_stat_at(_overlayInfoSystem, pilgrimStats.maxHealth.element, side, placementSide, size);// , nullptr, oipHealth.get());
		placement = placement.to_world(offsetDown);
		if (showAmmoStatus && is_at_boot_level(3)) add_pilgrim_stat_at(_overlayInfoSystem, pilgrimStats.leftHand.element, side, placement, size, GameSettings::get().difficulty.commonHandEnergyStorage ? nullptr : oipLeft.get(), oipAmmo.get());
		placementSide = placement.to_world(offsetSide);
		if (showAmmoStatus && is_at_boot_level(3)) add_pilgrim_stat_at(_overlayInfoSystem, pilgrimStats.maxAmmoLeft.element, side, placementSide, size);//, nullptr, oipAmmo.get());
		placement = placement.to_world(offsetDown);
		if (!GameSettings::get().difficulty.commonHandEnergyStorage)
		{
			if (showAmmoStatus && is_at_boot_level(3)) add_pilgrim_stat_at(_overlayInfoSystem, pilgrimStats.rightHand.element, side, placement, size, nullptr, oipAmmo.get(), oipRight.get());
			placementSide = placement.to_world(offsetSide);
			if (showAmmoStatus && is_at_boot_level(3)) add_pilgrim_stat_at(_overlayInfoSystem, pilgrimStats.maxAmmoRight.element, side, placementSide, size);// , nullptr, oipAmmo.get());
			placement = placement.to_world(offsetDown);
		}
		placement = placement.to_world(offsetDown); // blank line
	
		get_extra_exms(_owner, pilgrimStats.extraEXMs);
		{
			bool showNow = is_at_boot_level(4) && showAmmoStatus;
			for_every(extraEXM, pilgrimStats.extraEXMs)
			{
				if (*extraEXM == NAME(exmMeleeDamage))
				{
					if (showNow) add_pilgrim_stat_at(_overlayInfoSystem, pilgrimStats.meleeDamage.element, side, placement, size, oiMelee.get(), oigDamage.get());
					placement = placement.to_world(offsetDown);
				}
				if (*extraEXM == NAME(exmMeleeKillHealth))
				{
					if (showNow) add_pilgrim_stat_at(_overlayInfoSystem, pilgrimStats.meleeKillHealth.element, side, placement, size, oiMelee.get(), oiKill.get(), oipHealth.get());
					placement = placement.to_world(offsetDown);
				}
				if (*extraEXM == NAME(exmMeleeKillAmmo))
				{
					if (showNow) add_pilgrim_stat_at(_overlayInfoSystem, pilgrimStats.meleeKillAmmo.element, side, placement, size, oiMelee.get(), oiKill.get(), oipAmmo.get());
					placement = placement.to_world(offsetDown);
				}
				if (*extraEXM == EXMID::Passive::vampire_fists())
				{
					RefCountObjectPtr<OverlayInfo::Element> element;
					if (showNow) add_pilgrim_stat_at(_overlayInfoSystem, element, side, placement, size, oiMelee.get(), oipHealth.get());
					update_stat_overlay_element(element, (String(TXT("+")) + GameplayBalance::health_for_melee_hit().as_string(0)) + String::space());
					placement = placement.to_world(offsetDown);
				}
				if (*extraEXM == EXMID::Passive::beggar_fists())
				{
					RefCountObjectPtr<OverlayInfo::Element> element;
					if (showNow) add_pilgrim_stat_at(_overlayInfoSystem, element, side, placement, size, oiMelee.get(), oipAmmo.get());
					update_stat_overlay_element(element, (String(TXT("+")) + GameplayBalance::ammo_for_melee_hit().as_string(0)) + String::space());
					placement = placement.to_world(offsetDown);
				}
			}
		}
	}

	Transform rawLeftSidePlacement;
	{
		Vector3 useDir = dir * Vector3(-1.0f, 1.0f, 1.0f);
		rawLeftSidePlacement = matrix_from_up_forward(useDir * statsDist + Vector3::zAxis * (0.0f), Vector3(0.0f, 0.0f, 1.0f), useDir).to_transform();
	}

	// left side 
	{
		int side = -1;
		Transform placement = get_vr_placement_for_local(_owner, rawLeftSidePlacement);
		//placement = placement.to_world(Transform(-Vector3::xAxis * size * 8.0f - Vector3::yAxis * size * 1.0f, Quat::identity));
		Transform placementSide = placement;

		{
			Transform p = placement;// .to_world(offsetUp);

			// experience gained is displayed in multiple lines
			p = p.to_world(Transform(Vector3::zAxis * (size * (0.5f + verticalSpacingPt)), Quat::identity)); // just by spacing
			if (is_at_boot_level(3)) add_pilgrim_stat_at(_overlayInfoSystem, pilgrimStats.experienceGainedElement, side, p, size, nullptr, nullptr);
			if (auto* t = fast_cast<OverlayInfo::Elements::Text>(pilgrimStats.experienceGainedElement.get()))
			{
				t->with_vertical_align(0.0f);
				t->with_colour(Colour::greyLight);
			}
		}

		if (is_at_boot_level(3)) add_pilgrim_stat_at(_overlayInfoSystem, pilgrimStats.experienceToSpend.element, side, placement, size, nullptr, oipExperience.get());
		placement = placement.to_world(offsetDown);
		if (auto* gs = TeaForGodEmperor::PlayerSetup::access_current().access_active_game_slot())
		{
			// always show missions
			if (gs->get_game_slot_mode() == TeaForGodEmperor::GameSlotMode::Missions)
			{
				if (is_at_boot_level(3)) add_pilgrim_stat_at(_overlayInfoSystem, pilgrimStats.meritPointsToSpend.element, side, placement, size, nullptr, oipMeritPoints.get());
				placement = placement.to_world(offsetDown);
			}
		}
		placement = placement.to_world(offsetDown); // blank line
		if (is_at_boot_level(2)) add_pilgrim_stat_at(_overlayInfoSystem, pilgrimStats.time.element, side, placement, size, nullptr, oipTime.get());
		placement = placement.to_world(offsetDown);
		if (is_at_boot_level(2)) add_pilgrim_stat_at(_overlayInfoSystem, pilgrimStats.distance.element, side, placement, size, nullptr, oipDistance.get());
		placement = placement.to_world(offsetDown);

		placement = placement.to_world(offsetDown);
		if (PlayerSetup::get_current().get_preferences().showRealTime)
		{
			if (is_at_boot_level(2)) add_pilgrim_stat_at(_overlayInfoSystem, pilgrimStats.realTime.element, side, placement, size, nullptr, nullptr);

			if (auto* t = fast_cast<OverlayInfo::Elements::Text>(pilgrimStats.realTime.element.get()))
			{
				t->with_colour(Colour::greyLight);
			}
		}
		{
			if (is_at_boot_level(2)) add_pilgrim_stat_at(_overlayInfoSystem, pilgrimStats.lockedToHead.element, side, placement, size, nullptr, nullptr);

			if (auto* t = fast_cast<OverlayInfo::Elements::Text>(pilgrimStats.lockedToHead.element.get()))
			{
				t->with_horizontal_align(0.0f);
			}
		}
		placement = placement.to_world(offsetDown);

		{
			bool showTurnCounter = PlayerSetup::get_current().get_preferences().turnCounter == Option::On;
			if (PlayerSetup::get_current().get_preferences().turnCounter == Option::Allow)
			{
				if (auto* vr = VR::IVR::get())
				{
					showTurnCounter = ! vr->is_wireless();
				}
			}
			if (showTurnCounter)
			{
				placement = placement.to_world(offsetDown);
				if (is_at_boot_level(2)) add_pilgrim_stat_at(_overlayInfoSystem, pilgrimStats.turnCounter.element, side, placement, size, nullptr, nullptr);
				placement = placement.to_world(offsetDown);

				if (auto* t = fast_cast<OverlayInfo::Elements::Text>(pilgrimStats.turnCounter.element.get()))
				{
					t->with_colour(Colour::greyLight);
				}
			}
		}
	}

	// left side far
	if (is_at_boot_level(4))
	{
		Transform placement = rawLeftSidePlacement;
		placement = Transform(Vector3::zero, Rotator3(0.0f, -20.0f, 0.0f).to_quat()).to_world(placement);
		placement = get_vr_placement_for_local(_owner, placement);

		if (auto* ms = MissionState::get_current())
		{
			MissionUtils utils(this, _overlayInfoSystem, placement, offsetDown, size);

			VisibleValue<Energy> meritPointsToSpend;

			if (auto* m = ms->get_mission())
			{
				{
					missionInfo.missionTypeElement = utils.add(m->get_mission_type());
					missionInfo.missionTypeElement->with_colour(colourMissionType);
				}

				{
					String obj = m->get_poi_objective();
					List<String> lines;
					obj.split(String(TXT("~")), lines);
					for_every(l, lines)
					{
						utils.add(*l);
					}
				}

				// add objectives and their current state
				{
					auto& obj = m->get_mission_objectives();
					{
						Reward reward(obj.get_experience_for_visited_cell().adjusted_plus_one(GameSettings::get().experienceModifier), obj.get_merit_points_for_visited_cell());
						if (!reward.is_empty())
						{
							missionInfo.visitedCells.reward = utils.add(Framework::StringFormatter::format_sentence_loc_str(NAME(lsMissionObjectiveVisitedCellReward), Framework::StringFormatterParams().add(NAME(value), reward.as_string())));
							missionInfo.visitedCells.resultInt.element = utils.add(String::empty());
							missionInfo.visitedCells.resultInt.value = NONE;
						}
					}
					{
						Reward reward(obj.get_experience_for_visited_interface_box().adjusted_plus_one(GameSettings::get().experienceModifier), obj.get_merit_points_for_visited_interface_box());
						if (!reward.is_empty())
						{
							missionInfo.visitedInterfaceBoxes.reward = utils.add(Framework::StringFormatter::format_sentence_loc_str(NAME(lsMissionObjectiveVisitedInterfaceBoxReward), Framework::StringFormatterParams().add(NAME(value), reward.as_string())));
							missionInfo.visitedInterfaceBoxes.resultInt.element = utils.add(String::empty());
							missionInfo.visitedInterfaceBoxes.resultInt.value = NONE;
						}
					}
					{
						Reward reward(obj.get_experience_for_brought_item().adjusted_plus_one(GameSettings::get().experienceModifier), obj.get_merit_points_for_brought_item());
						if (!reward.is_empty())
						{
							missionInfo.bringItems.reward = utils.add(Framework::StringFormatter::format_sentence_loc_str(NAME(lsMissionObjectiveBringItemsReward), Framework::StringFormatterParams().add(NAME(value), reward.as_string())));
							missionInfo.bringItems.resultInt.element = utils.add(String::empty());
							missionInfo.bringItems.resultInt.value = NONE;
						}
					}
					{
						int count = obj.get_required_hacked_box_count();
						if (count > 0)
						{
							missionInfo.hackBoxes.requirement = utils.add(Framework::StringFormatter::format_sentence_loc_str(NAME(lsMissionObjectiveHackBoxesRequirement), Framework::StringFormatterParams().add(NAME(value), count)));
						}
					}
					{
						Reward reward(obj.get_experience_for_hacked_box().adjusted_plus_one(GameSettings::get().experienceModifier), obj.get_merit_points_for_hacked_box());
						if (!reward.is_empty())
						{
							missionInfo.hackBoxes.reward = utils.add(Framework::StringFormatter::format_sentence_loc_str(NAME(lsMissionObjectiveHackBoxesReward), Framework::StringFormatterParams().add(NAME(value), reward.as_string())));
							missionInfo.hackBoxes.resultInt.element = utils.add(String::empty());
							missionInfo.hackBoxes.resultInt.value = NONE;
						}
					}
					{
						int count = obj.get_required_stopped_infestations_count();
						if (count > 0)
						{
							missionInfo.infestations.requirement = utils.add(Framework::StringFormatter::format_sentence_loc_str(NAME(lsMissionObjectiveInfestationsRequirement), Framework::StringFormatterParams().add(NAME(value), count)));
						}
					}
					{
						Reward reward(obj.get_experience_for_stopped_infestation().adjusted_plus_one(GameSettings::get().experienceModifier), obj.get_merit_points_for_stopped_infestation());
						if (!reward.is_empty())
						{
							missionInfo.infestations.reward = utils.add(Framework::StringFormatter::format_sentence_loc_str(NAME(lsMissionObjectiveInfestationsReward), Framework::StringFormatterParams().add(NAME(value), reward.as_string())));
							missionInfo.infestations.resultInt.element = utils.add(String::empty());
							missionInfo.infestations.resultInt.value = NONE;
						}
					}
					{
						int count = obj.get_required_refills_count();
						if (count > 0)
						{
							missionInfo.refills.requirement = utils.add(Framework::StringFormatter::format_sentence_loc_str(NAME(lsMissionObjectiveRefillsRequirement), Framework::StringFormatterParams().add(NAME(value), count)));
						}
					}
					{
						Reward reward(obj.get_experience_for_refill().adjusted_plus_one(GameSettings::get().experienceModifier), obj.get_merit_points_for_refill());
						if (!reward.is_empty())
						{
							missionInfo.refills.reward = utils.add(Framework::StringFormatter::format_sentence_loc_str(NAME(lsMissionObjectiveRefillsReward), Framework::StringFormatterParams().add(NAME(value), reward.as_string())));
							missionInfo.refills.resultInt.element = utils.add(String::empty());
							missionInfo.refills.resultInt.value = NONE;
						}
					}
				}
			}

			if (!MissionStateObjectives::compare(missionInfo.missionItemsDoneFor, ms->get_objectives()))
			{
				missionInfo.missionItemsDoneFor = ms->get_objectives();
				missionInfo.missionItemsDoneFor.prepare_to_render(missionInfo.missionItemsRenderableData, Vector3::xAxis, Vector3::zAxis, Vector2(-0.6f, -0.5f), 0.15f);
			}

			{
				if (!missionInfo.missionItemsRenderableData.is_empty())
				{
					float useSize = 0.5f;

					auto* e = new OverlayInfo::Elements::CustomDraw();
					e->with_id(NAME(pilgrimStat));
					e->with_usage(OverlayInfo::Usage::World);

					::SafePtr<PilgrimOverlayInfo> safeThis(this);

					e->with_draw([safeThis](float _active, float _pulse, Colour const& _colour)
						{
							if (auto* self = safeThis.get())
							{
								if (!self->missionInfo.missionItemsRenderableData.is_empty())
								{
									self->missionInfo.missionItemsRenderableData.render(_active, _pulse, _colour, NP, self->get_overlay_shader_instance());
								}
							}
						});
					e->with_size(useSize, false);

					setup_overlay_element_placement(e, placement);

					_overlayInfoSystem->add(e);
				}
			}
		}
	}

	// fill

	{
		pilgrimStats.distance.value = 0.0f; // will be updated
		pilgrimStats.time.value = 0;
		pilgrimStats.experienceToSpend.value = Persistence::get_current().get_experience_to_spend();
		update_stat_overlay_element(pilgrimStats.experienceToSpend.element, (pilgrimStats.experienceToSpend.value).as_string(2));
		pilgrimStats.meritPointsToSpend.value = Persistence::get_current().get_merit_points_to_spend();
		update_stat_overlay_element(pilgrimStats.meritPointsToSpend.element, String::printf(TXT("%i "), pilgrimStats.meritPointsToSpend.value));
		pilgrimStats.experienceGainedVer = NONE;
	}

	if (auto* h = _owner->get_custom<CustomModules::Health>())
	{
		pilgrimStats.healthMain.value = max(Energy::zero(), h->get_health().mul(GameDirector::get()->get_health_status()));
		update_stat_overlay_element(pilgrimStats.healthMain.element, GameDirector::get()->is_health_status_blocked() || h->is_super_health_storage()? String::empty() : pilgrimStats.healthMain.value.as_string(2));
		pilgrimStats.health.value = h->get_total_health();
		update_stat_overlay_element(pilgrimStats.health.element, GameDirector::get()->is_health_status_blocked() ? String::empty() : pilgrimStats.health.value.as_string(2));
		pilgrimStats.maxHealth.value = h->get_max_total_health();
		update_stat_overlay_element(pilgrimStats.maxHealth.element, (! is_at_boot_level(3)) ? String::empty() : (stringStatOf + pilgrimStats.maxHealth.value.as_string(0)) + String::space());
	}
	if (auto* mp = _owner->get_gameplay_as<ModulePilgrim>())
	{
		pilgrimStats.leftHand.value = mp->get_hand_energy_storage(Hand::Left).mul(GameDirector::get()->get_ammo_status());
		update_stat_overlay_element(pilgrimStats.leftHand.element, GameDirector::get()->is_ammo_status_blocked() ? String::empty() : pilgrimStats.leftHand.value.as_string(2));
		if (!GameSettings::get().difficulty.commonHandEnergyStorage)
		{
			pilgrimStats.rightHand.value = mp->get_hand_energy_storage(Hand::Right).mul(GameDirector::get()->get_ammo_status());
			update_stat_overlay_element(pilgrimStats.rightHand.element, GameDirector::get()->is_ammo_status_blocked() ? String::empty() : pilgrimStats.rightHand.value.as_string(2));
		}
		pilgrimStats.maxAmmoLeft.value = mp->get_hand_energy_max_storage((Hand::Left));
		pilgrimStats.maxAmmoRight.value = mp->get_hand_energy_max_storage((Hand::Right));
		update_stat_overlay_element(pilgrimStats.maxAmmoLeft.element, (!is_at_boot_level(3)) ? String::empty() : (stringStatOf + pilgrimStats.maxAmmoLeft.value.as_string(0)) + String::space());
		update_stat_overlay_element(pilgrimStats.maxAmmoRight.element, (!is_at_boot_level(3)) ? String::empty() : (stringStatOf + pilgrimStats.maxAmmoRight.value.as_string(0)) + String::space());

#ifdef WITH_CRYSTALITE
		pilgrimStats.crystalite.value = mp->get_crystalilte();
		update_stat_overlay_element(pilgrimStats.crystalite.element, String::printf(TXT("%i"), pilgrimStats.crystalite.value));
#endif

		pilgrimStats.meleeDamage.value = get_physical_violence_ref(mp);
		update_stat_overlay_element(pilgrimStats.meleeDamage.element, get_physical_violence_str(mp) + String::space());

		pilgrimStats.meleeKillHealth.value = PilgrimBlackboard::get_health_for_melee_kill(_owner);
		update_stat_overlay_element(pilgrimStats.meleeKillHealth.element, (String(TXT("+")) + pilgrimStats.meleeKillHealth.value.as_string(0)) + String::space());
		pilgrimStats.meleeKillAmmo.value = PilgrimBlackboard::get_ammo_for_melee_kill(_owner);
		update_stat_overlay_element(pilgrimStats.meleeKillAmmo.element, (String(TXT("+")) + pilgrimStats.meleeKillAmmo.value.as_string(0)) + String::space());
	}
}

#define IF_DIFFER_SET(dest, src) bool temp_variable_named(diff) = (dest) != (src); dest = (src); if (temp_variable_named(diff))

void PilgrimOverlayInfo::update_pilgrim_stats(Framework::IModulesOwner* _owner)
{
	ArrayStatic<Name, 8> newExtraEXMs; SET_EXTRA_DEBUG_INFO(newExtraEXMs, TXT("PilgrimOverlayInfo::update_pilgrim_stats.newExtraEXMs"));
	{
		get_extra_exms(_owner, newExtraEXMs);

		if (pilgrimStats.extraEXMs != newExtraEXMs)
		{
			if (auto* game = Game::get_as<Game>())
			{
				if (auto* overlayInfoSystem = game->access_overlay_info_system())
				{
					add_pilgrim_stats(_owner, overlayInfoSystem);
				}
			}
		}
	}

	{
		auto const& gs = GameStats::get();
		IF_DIFFER_SET(pilgrimStats.distance.value, gs.distance)
		{
			String distanceString;
			MeasurementSystem::Type ms = TeaForGodEmperor::PlayerSetup::get_current().get_preferences().get_measurement_system();
			if (ms == MeasurementSystem::Imperial)
			{
				distanceString = String::printf(TXT("%.0fyd"), MeasurementSystem::to_yards(gs.distance));
			}
			else
			{
				distanceString = String::printf(TXT("%.0fm"), gs.distance);
			}
			update_stat_overlay_element(pilgrimStats.distance.element, distanceString);
		}
		if (PlayerSetup::get_current().get_preferences().showRealTime)
		{
			time_t currentTime = time(0);
			tm currentTimeInfo;
			tm* pcurrentTimeInfo = &currentTimeInfo;
#ifdef AN_CLANG
			pcurrentTimeInfo = localtime(&currentTime);
#else
			localtime_s(&currentTimeInfo, &currentTime);
#endif
			int realTimeSeconds = (pcurrentTimeInfo->tm_hour * 60 + pcurrentTimeInfo->tm_min) * 60 + pcurrentTimeInfo->tm_sec;

			IF_DIFFER_SET(pilgrimStats.realTime.value, realTimeSeconds)
			{
				update_stat_overlay_element(pilgrimStats.realTime.element, Framework::Time::from_seconds(realTimeSeconds).get_full_hour_minute_string() + String::space());
			}
		}
		IF_DIFFER_SET(pilgrimStats.lockedToHead.value, lockedToHead)
		{
			update_stat_overlay_element(pilgrimStats.lockedToHead.element, lockedToHead? (String::space() + LOC_STR(NAME(lsCharsPilgrimOverlayLockedToHead))) : String::empty());
		}
		IF_DIFFER_SET(pilgrimStats.time.value, gs.timeInSeconds)
		{
			update_stat_overlay_element(pilgrimStats.time.element, Framework::Time::from_seconds(gs.timeInSeconds).get_simple_compact_hour_minute_second_string() + String::space());
		}
		if (auto* tce = pilgrimStats.turnCounter.element.get())
		{
			int turnCount = 0;
			if (auto* vr = VR::IVR::get())
			{
				turnCount = vr->get_turn_count();
			}
			IF_DIFFER_SET(pilgrimStats.turnCounter.value, turnCount)
			{
				String tc = String::printf(TXT("%c %i %c"), turnCount < 0 ? '<' : ' ', abs(turnCount), turnCount > 0 ? '>' : ' ');
				update_stat_overlay_element(pilgrimStats.turnCounter.element, tc);
				if (abs(turnCount) < 5)
				{
					tce->with_colour(Colour::greyLight);
				}
				else if (abs(turnCount) < 10)
				{
					tce->with_colour(Colour::yellow);
				}
				else
				{
					tce->with_colour(Colour::red);
				}
			}
		}
		IF_DIFFER_SET(pilgrimStats.experienceToSpend.value, Persistence::get_current().get_experience_to_spend())
		{
			update_stat_overlay_element(pilgrimStats.experienceToSpend.element, (pilgrimStats.experienceToSpend.value).as_string(2)); 
		}
		IF_DIFFER_SET(pilgrimStats.meritPointsToSpend.value, Persistence::get_current().get_merit_points_to_spend())
		{
			update_stat_overlay_element(pilgrimStats.meritPointsToSpend.element, String::printf(TXT("%i "), pilgrimStats.meritPointsToSpend.value));
		}
		if (pilgrimStats.experienceGainedVer != Persistence::get_current().get_experience_gained_ver())
		{
			pilgrimStats.experienceGainedVer = Persistence::get_current().get_experience_gained_ver();
			String xpGained;
			Persistence::access_current().do_for_every_experience_gained(hardcoded 5, [&xpGained](Energy const& _added)
				{
					String newLine = String::printf(TXT("+%S"), _added.as_string_auto_decimals().to_char());
					if (newLine.get_length() <= 3 || newLine[newLine.get_length()] != '.')
					{
						// if no trailing numbers, put an empty space
						newLine += String::space();
					}
					xpGained = newLine + (xpGained.is_empty()? TXT("") : TXT("~")) + xpGained;
				});
			update_stat_overlay_element(pilgrimStats.experienceGainedElement, xpGained);
		}
	}

	if (auto* h = _owner->get_custom<CustomModules::Health>())
	{
		IF_DIFFER_SET(pilgrimStats.healthMain.value, max(Energy::zero(), h->is_super_health_storage()? Energy::zero() : h->get_health().mul(GameDirector::get()->get_health_status())))
		{
			update_stat_overlay_element(pilgrimStats.healthMain.element, h->is_super_health_storage()? String::empty() : pilgrimStats.healthMain.value.as_string(2));
		}
		IF_DIFFER_SET(pilgrimStats.health.value, h->get_total_health().mul(GameDirector::get()->get_health_status()))
		{
			update_stat_overlay_element(pilgrimStats.health.element, pilgrimStats.health.value.as_string(2));
		}
		IF_DIFFER_SET(pilgrimStats.maxHealth.value, h->get_max_total_health())
		{
			update_stat_overlay_element(pilgrimStats.maxHealth.element, pilgrimStats.maxHealth.value.as_string(0) + String::space());
		}
		// ignore game director's show status for blinking
		update_stat_overlay_element_blinking_low(pilgrimStats.healthMain.element, h->get_health(), h->get_max_health());
		update_stat_overlay_element_blinking_low(pilgrimStats.health.element, h->get_total_health(), h->get_max_total_health());
	}
	if (auto* mp = _owner->get_gameplay_as<ModulePilgrim>())
	{
		IF_DIFFER_SET(pilgrimStats.leftHand.value, max(Energy::zero(), mp->get_hand_energy_storage(Hand::Left).mul(GameDirector::get()->get_ammo_status())))
		{
			update_stat_overlay_element(pilgrimStats.leftHand.element, pilgrimStats.leftHand.value.as_string(2));
		}
		if (!GameSettings::get().difficulty.commonHandEnergyStorage)
		{
			IF_DIFFER_SET(pilgrimStats.rightHand.value, max(Energy::zero(), mp->get_hand_energy_storage(Hand::Right).mul(GameDirector::get()->get_ammo_status())))
			{
				update_stat_overlay_element(pilgrimStats.rightHand.element, pilgrimStats.rightHand.value.as_string(2));
			}
		}
		IF_DIFFER_SET(pilgrimStats.maxAmmoLeft.value, mp->get_hand_energy_max_storage((Hand::Left)))
		{
			update_stat_overlay_element(pilgrimStats.maxAmmoLeft.element, pilgrimStats.maxAmmoLeft.value.as_string(0) + String::space());
		}
		IF_DIFFER_SET(pilgrimStats.maxAmmoRight.value, mp->get_hand_energy_max_storage((Hand::Right)))
		{
			update_stat_overlay_element(pilgrimStats.maxAmmoRight.element, pilgrimStats.maxAmmoRight.value.as_string(0) + String::space());
		}
		// ignore game director's show status for blinking
		update_stat_overlay_element_blinking_low(pilgrimStats.leftHand.element, mp->get_hand_energy_storage(Hand::Left), pilgrimStats.maxAmmoLeft.value);
		update_stat_overlay_element_blinking_low(pilgrimStats.rightHand.element, mp->get_hand_energy_storage(Hand::Right), pilgrimStats.maxAmmoRight.value);

#ifdef WITH_CRYSTALITE
		IF_DIFFER_SET(pilgrimStats.crystalite.value, mp->get_crystalilte())
		{
			update_stat_overlay_element(pilgrimStats.crystalite.element, String::printf(TXT("%i"), pilgrimStats.crystalite.value) + String::space());
		}
#endif

		IF_DIFFER_SET(pilgrimStats.meleeKillHealth.value, get_physical_violence_ref(mp))
		{
			update_stat_overlay_element(pilgrimStats.meleeKillHealth.element, get_physical_violence_str(mp) + String::space());
		}

		IF_DIFFER_SET(pilgrimStats.meleeKillHealth.value, PilgrimBlackboard::get_health_for_melee_kill(_owner))
		{
			update_stat_overlay_element(pilgrimStats.meleeKillHealth.element, (String(TXT("+")) + pilgrimStats.meleeKillHealth.value.as_string(0)) + String::space());
		}
		IF_DIFFER_SET(pilgrimStats.meleeKillAmmo.value, PilgrimBlackboard::get_ammo_for_melee_kill(_owner))
		{
			update_stat_overlay_element(pilgrimStats.meleeKillAmmo.element, (String(TXT("+")) + pilgrimStats.meleeKillAmmo.value.as_string(0)) + String::space());
		}
	}

	// mission objectives
	if (auto* ms = MissionState::get_current())
	{
		if (missionInfo.visitedCells.resultInt.element.is_set())
		{
			int visitedCells = ms->get_visited_cells();
			IF_DIFFER_SET(missionInfo.visitedCells.resultInt.value, visitedCells)
			{
				update_stat_overlay_element(missionInfo.visitedCells.resultInt.element,
					Framework::StringFormatter::format_sentence_loc_str(NAME(lsMissionObjectiveVisitedCellResult), Framework::StringFormatterParams()
					.add(NAME(value), visitedCells)));
			}
		}
		if (missionInfo.visitedInterfaceBoxes.resultInt.element.is_set())
		{
			int visitedInterfaceBoxes = ms->get_visited_interface_boxes();
			IF_DIFFER_SET(missionInfo.visitedInterfaceBoxes.resultInt.value, visitedInterfaceBoxes)
			{
				update_stat_overlay_element(missionInfo.visitedInterfaceBoxes.resultInt.element,
					Framework::StringFormatter::format_sentence_loc_str(NAME(lsMissionObjectiveVisitedInterfaceBoxResult), Framework::StringFormatterParams()
					.add(NAME(value), visitedInterfaceBoxes)));
			}
		}
		if (missionInfo.bringItems.resultInt.element.is_set())
		{
			int heldItemsAndBrought = ms->get_held_items_to_bring(_owner) + ms->get_already_brough_items();
			IF_DIFFER_SET(missionInfo.bringItems.resultInt.value, heldItemsAndBrought)
			{
				update_stat_overlay_element(missionInfo.bringItems.resultInt.element,
					Framework::StringFormatter::format_sentence_loc_str(NAME(lsMissionObjectiveBringItemsResult), Framework::StringFormatterParams()
					.add(NAME(value), heldItemsAndBrought)));
			}
		}
		if (missionInfo.hackBoxes.resultInt.element.is_set())
		{
			int hackedBoxes = ms->get_hacked_boxes();
			IF_DIFFER_SET(missionInfo.hackBoxes.resultInt.value, hackedBoxes)
			{
				update_stat_overlay_element(missionInfo.hackBoxes.resultInt.element,
					Framework::StringFormatter::format_sentence_loc_str(NAME(lsMissionObjectiveHackBoxesResult), Framework::StringFormatterParams()
					.add(NAME(value), hackedBoxes)));
			}
		}
		if (missionInfo.infestations.resultInt.element.is_set())
		{
			int stoppedInfestations = ms->get_stopped_infestations();
			int infestationsToStop = 0;
			if (auto* m = ms->get_mission())
			{
				infestationsToStop = m->get_mission_objectives().get_required_stopped_infestations_count();
			}
			IF_DIFFER_SET(missionInfo.infestations.resultInt.value, stoppedInfestations)
			{
				update_stat_overlay_element(missionInfo.infestations.resultInt.element,
					Framework::StringFormatter::format_sentence_loc_str(NAME(lsMissionObjectiveInfestationsResult), Framework::StringFormatterParams()
					.add(NAME(value), stoppedInfestations)
					.add(NAME(limit), infestationsToStop)));
			}
		}
		if (missionInfo.refills.resultInt.element.is_set())
		{
			int refills = ms->get_refills();
			int refillsToDo = 0;
			if (auto* m = ms->get_mission())
			{
				refillsToDo = m->get_mission_objectives().get_required_refills_count();
			}
			IF_DIFFER_SET(missionInfo.refills.resultInt.value, refills)
			{
				update_stat_overlay_element(missionInfo.refills.resultInt.element,
					Framework::StringFormatter::format_sentence_loc_str(NAME(lsMissionObjectiveRefillsResult), Framework::StringFormatterParams()
					.add(NAME(value), refills)
					.add(NAME(limit), refillsToDo)));
			}
		}
	}
}

void PilgrimOverlayInfo::update_stat_overlay_element(RefCountObjectPtr<OverlayInfo::Element>& element, String const& _text)
{
	if (auto* e = fast_cast<OverlayInfo::Elements::Text>(element.get()))
	{
		e->with_text(_text);
	}
}

void PilgrimOverlayInfo::update_stat_overlay_element_blinking_low(RefCountObjectPtr<OverlayInfo::Element>& element, Energy const& _current, Energy const& _max)
{
	if (auto* e = fast_cast<OverlayInfo::Elements::Text>(element.get()))
	{
		if (_current < _max.adjusted(EnergyCoef(0.5f)))
		{
			if (_current < _max.adjusted(EnergyCoef(0.125f)))
			{
				if (::System::Core::get_timer_mod(0.5f) < 0.35f || !PlayerPreferences::are_currently_flickering_lights_allowed())
				{
					e->with_colour(Colour::red);
				}
				else
				{
					e->with_colour(Colour::black);
				}
			}
			else if (_current < _max.adjusted(EnergyCoef(0.25f)))
			{
				if (::System::Core::get_timer_mod(1.0f) < 0.5f || !PlayerPreferences::are_currently_flickering_lights_allowed())
				{
					e->with_colour(Colour::red);
				}
				else
				{
					e->with_colour(Colour::orange);
				}
			}
			else if (_current < _max.adjusted(EnergyCoef(0.333333f)))
			{
				e->with_colour(Colour::orange);
			}
			else
			{
				e->with_colour(Colour::yellow);
			}
		}
		else
		{
			e->with_colour(Colour::white);
		}
	}
}

void PilgrimOverlayInfo::setup_overlay_element_placement(OverlayInfo::Element* e, Transform const& _vrPlacement, bool _forceVRPlacement)
{
	if (lockedToHead && ! _forceVRPlacement)
	{
		Transform placement = _vrPlacement;

		{
			set_reference_placement_if_not_set();
			Transform placementWS = referencePlacement.get().vrAnchorPlacement.to_world(placement);
			Transform ownerPlacement = referencePlacement.get().ownerPlacement;
			Transform eyesRelative = referencePlacement.get().eyesRelative;
			eyesRelative.set_orientation(Quat::identity);
			placement = ownerPlacement.to_world(eyesRelative).to_local(placementWS);
		}

		// anchor based
		e->with_location(OverlayInfo::Element::Relativity::RelativeToAnchorHUD/*RelativeToCameraFlat*/, placement.get_translation());
		e->with_follow_camera_location_z(0.1f);
		e->with_rotation(OverlayInfo::Element::Relativity::RelativeToAnchorHUD, placement.get_orientation().to_rotator());
		e->with_distance(0.0f);
	}
	else
	{
		e->with_vr_placement(_vrPlacement);
	}
}

void PilgrimOverlayInfo::add_pilgrim_stat_at(OverlayInfo::System* _overlayInfoSystem, RefCountObjectPtr<OverlayInfo::Element>& element, int _side, Transform const& _placement, float _size,
	Framework::TexturePart* _left, Framework::TexturePart* _centre, Framework::TexturePart* _right)
{
	Transform placement = _placement;

	float fSide = (float)_side;

	float horizontalSpacingPt = 0.2f;
	Transform textOffset = Transform(Vector3::xAxis * (_size * (_side >= 0.0f? 3.5f : 0.5f)), Quat::identity);
	Transform offset = Transform(Vector3::xAxis * (_size * (1.0f + horizontalSpacingPt) * fSide), Quat::identity);
	Transform halfOffset = Transform(Vector3::xAxis * (_size * 0.5f) * fSide + Vector3::zAxis * (_size * hardcoded 0.125f /* to align properly with font */), Quat::identity);

	for_count(int, i, 3)
	{
		auto* current = i == 0 ? _left : (i == 2 ? _right : _centre);
		if (current)
		{
			auto* e = new OverlayInfo::Elements::TexturePart(current);
			e->with_id(NAME(pilgrimStat));
			e->with_usage(OverlayInfo::Usage::World);

			setup_overlay_element_placement(e, placement.to_world(halfOffset));

			e->with_size(_size);

			_overlayInfoSystem->add(e);
		}

		placement = placement.to_world(offset);
	}

	{
		auto* e = new OverlayInfo::Elements::Text();
		e->with_id(NAME(pilgrimStat));
		e->with_usage(OverlayInfo::Usage::World);
		e->with_font(oiFontStats.get());
		e->with_smaller_decimals();

		setup_overlay_element_placement(e, placement.to_world(textOffset));

		e->with_horizontal_align(1.0f);
		e->with_vertical_align(0.5f);

		e->with_letter_size(_size);

		element = e;

		_overlayInfoSystem->add(e);
	}
}

void PilgrimOverlayInfo::add_main_log(String const& _text, Optional<Colour> const& _colour, Optional<int> const& _atLine)
{
#ifdef OUTPUT_OVERLAY_INFO
	output(TXT("[POvIn] add main log \"%S\""), _text.to_char());
#endif
	if (_atLine.is_set())
	{
		// add empty lines
		while (showMainLog.lines.get_size() < _atLine.get())
		{
			showMainLog.lines.push_back(ShowLog::Element());
		}
	}
	if (_text.does_contain('~'))
	{
		String text = _text;
		bool multiLineContinuation = false;
		while (true)
		{
			int at = text.find_first_of('~');
			if (at != NONE)
			{
				ShowLog::Element e;
				e.text = text.get_left(at);
				e.colour = _colour;
				e.multiLineContinuation = multiLineContinuation;
				showMainLog.lines.push_back(e);
				text = text.get_sub(at + 1);
			}
			else
			{
				ShowLog::Element e;
				e.text = text;
				e.colour = _colour;
				e.multiLineContinuation = multiLineContinuation;
				showMainLog.lines.push_back(e);
				break;
			}
			multiLineContinuation = true;
		}
	}
	else
	{
		ShowLog::Element e;
		e.text = _text;
		e.colour = _colour;
		showMainLog.lines.push_back(e);
	}

	update_main_log(false, true);
}

void PilgrimOverlayInfo::clear_main_log()
{
	showMainLog.lines.clear();
	showMainLog.clearMainLogLineIndicatorOnNoVoiceoverActor = Name::invalid();
	update_main_log(true, false);
}

void PilgrimOverlayInfo::redo_main_overlay_elements()
{
	if (currentlyShowing == Main &&
		requestedToShow != None)
	{
		if (auto* game = Game::get_as<Game>())
		{
			add_pilgrim_stats(owner.get(), game->access_overlay_info_system());
			internal_hide_exms(NAME(pilgrimOverlayInfoMain));
			internal_hide_permanent_exms(NAME(pilgrimOverlayInfoMain));
			show_exms(NAME(pilgrimOverlayInfoMain));
			show_permanent_exms(NAME(pilgrimOverlayInfoMain));
		}
	}
}

void PilgrimOverlayInfo::clear_main_log_line_indicator()
{
	update_main_log(true, false);
}

void PilgrimOverlayInfo::clear_main_log_line_indicator_on_no_voiceover_actor(Name const & _voActor)
{
	showMainLog.clearMainLogLineIndicatorOnNoVoiceoverActor = _voActor;
}

void PilgrimOverlayInfo::update_main_log(bool _clear, Optional<bool> const& _withLineIndicator)
{
	update_log(Main, _clear, _withLineIndicator);
}

void PilgrimOverlayInfo::update_investigate_log(bool _clear)
{
	update_log(Investigate, _clear);
}

void PilgrimOverlayInfo::show_main_log_at_camera(bool _showMainLogAtCamera)
{
	showMainLogAtCamera = _showMainLogAtCamera;
	update_log(Main);
}

void PilgrimOverlayInfo::update_log(WhatToShow _whichLog, bool _clear, Optional<bool> const& _withLineIndicator)
{
	auto& showLog = _whichLog == Main ? showMainLog : showInvestigateLog;
	Name showLogId = _whichLog == Main ? NAME(mainLog) : NAME(investigateLog);

	if (showMainLogAtCamera && _whichLog == Investigate)
	{
		_clear = true;
	}
	if (GameSettings::get().difficulty.noOverlayInfo)
	{
		showLog.lines.clear();
		return;
	}

#ifdef OUTPUT_OVERLAY_INFO
	output(TXT("[POvIn] update %S log > %S"), _whichLog == Main? TXT("main") : TXT("investigate"), _clear ? TXT("clear") : TXT("fill"));
#endif
	if (auto* game = Game::get_as<Game>())
	{
		if (auto* overlayInfoSystem = game->access_overlay_info_system())
		{
			int lineLimit = _whichLog == Investigate? showLog.lines.get_size() : 10;

			showLog.withLineIndicator = _withLineIndicator.get(showLog.withLineIndicator);
			if (_whichLog != Main)
			{
				showLog.withLineIndicator = false;
			}

			// we should clear when changing to Main and back

			if (_clear ||
				currentShowMainLogAtCamera != showMainLogAtCamera ||
				showLog.lines.get_size() > lineLimit ||
				showLog.lines.get_size() < showLog.displayedLines ||
				(_whichLog == Investigate && showLog.lines.get_size() != showLog.displayedLines))
			{
				overlayInfoSystem->deactivate_but_keep_placement(showLogId);
				showLog.displayedLines = 0;
				for_every(line, showLog.lines)
				{
					line->element.clear();
				}
			}
			else
			{
				// update elements, if required
				for_every(line, showLog.lines)
				{
					if (line->elementText != line->text)
					{
						line->elementText = line->text;
						if (auto* e = fast_cast<OverlayInfo::Elements::Text>(line->element.get()))
						{
							e->with_text(line->text);
						}
					}
				}
			}

			currentShowMainLogAtCamera = showMainLogAtCamera;

			overlayInfoSystem->deactivate(NAME(mainLogLineIndicator));

			for_every(line, showLog.lines)
			{
				line->indicatorElement.clear();
			}

			bool followPitch = TeaForGodEmperor::PlayerPreferences::should_hud_follow_pitch();

			if (! _clear)
			{
				float letterSize = _whichLog == Investigate? 0.015f : 0.025f;
				float scaleDistances = 0.025f / letterSize;
				float lineOffset = letterSize * 1.2f;

				Vector3 loc = Vector3(-0.0225f, 0.05f, 0.02f).normal() * statsDist * 1.1f;
				Transform placement = matrix_from_up_forward(loc, Vector3(0.0f, 0.0f, 1.0f), Vector3(0.0f, 1.0f, 0.0f)).to_transform();
				float atCameraPitch = 20.0f;
				float atCameraPitchOffset = 2.0f;
				if (currentlyShowing == Main)
				{
					placement = get_vr_placement_for_local(owner.get(), placement);
				}
				else
				{
					if (followPitch)
					{
						Vector3 offset = placement.get_translation();
						offset.y -= statsDist * 1.1f;
						placement.set_translation(offset); // if using anchor and camera flat
					}
				}

				//placement = placement.to_world(Transform(Vector3::zAxis * (-lineOffset * (float)lineLimit * 0.5f + (lineOffset - letterSize) * 0.5f), Quat::identity));

				Transform offset = Transform(-Vector3::zAxis * lineOffset, Quat::identity);

				// lower by 3 lines
				for_count(int, i, 3)
				{
					placement = placement.to_world(offset);
				}

				// for investigate, start from the bottom
				if (_whichLog == Investigate)
				{
					int lowerLinesTotal = TypeConversions::Normal::f_i_closest((float)14 * scaleDistances);
					int nonEmptyLines = 0;
					for_every(l, showLog.lines)
					{
						if (!l->text.is_empty())
						{
							++nonEmptyLines;
						}
					}
					int lowerByLines = max(0, lowerLinesTotal - nonEmptyLines);
					while (lowerByLines > 0)
					{
						placement = placement.to_world(offset);
						--lowerByLines;
					}
				}

				int lineIndicatorAtIdx = showLog.lines.get_size() - 1;
				while (showLog.lines.is_index_valid(lineIndicatorAtIdx))
				{
					if (!showLog.lines[lineIndicatorAtIdx].multiLineContinuation)
					{
						break;
					}
					--lineIndicatorAtIdx;
				}
				lineIndicatorAtIdx = max(0, lineIndicatorAtIdx);

				for (int lineIdx = max(0, showLog.lines.get_size() - lineLimit); lineIdx < showLog.lines.get_size(); ++ lineIdx)
				{
					auto& line = showLog.lines[lineIdx];

					if (lineIdx >= showLog.displayedLines)
					{
						Transform usePlacement = placement;

						auto* e = new OverlayInfo::Elements::Text(line.text);
						e->with_id(showLogId);
						e->with_font(oiFontStats.get());
						// no smaller decimals!
						if (line.colour.is_set())
						{
							e->with_colour(line.colour.get());
						}
						if (_whichLog == Investigate)
						{
							e->with_smaller_decimals();
						}

						if (showMainLogAtCamera && _whichLog == Main)
						{
							Rotator3 offset = Rotator3(atCameraPitch, 0.0f, 0.0f);

							e->with_location(OverlayInfo::Element::Relativity::RelativeToCamera);
							e->with_rotation(OverlayInfo::Element::Relativity::RelativeToCamera, offset);
							e->with_distance(1.2f);
						}
						else
						{
							e->with_usage(OverlayInfo::Usage::World);
							if (currentlyShowing == Main)
							{
								setup_overlay_element_placement(e, usePlacement);
							}
							else
							{
								if (followPitch)
								{
									e->with_location(OverlayInfo::Element::Relativity::RelativeToAnchorPY/*RelativeToCameraFlat*/, usePlacement.get_translation());
									e->with_follow_camera_location_z(0.1f);
									e->with_rotation(OverlayInfo::Element::Relativity::RelativeToAnchorPY, usePlacement.get_orientation().to_rotator());
									e->with_distance(statsDist * 1.1f);
								}
								else
								{
									e->with_placement(OverlayInfo::Element::RelativeToCamera, usePlacement);
									e->with_follow_camera_yaw(5.0f);
									e->with_follow_camera_pitch(5.0f);
									e->with_distance(0.0f);
								}
							}
						}

						if (showMainLogAtCamera && _whichLog == Main)
						{
							e->with_horizontal_align(0.5f);
							e->with_vertical_align(0.5f);

							e->with_letter_size(letterSize * 2.0f);
						}
						else
						{
							e->with_horizontal_align(0.0f);
							e->with_vertical_align(0.5f);

							e->with_letter_size(letterSize);
						}

						overlayInfoSystem->add(e);

						line.element = e;
						line.elementText = line.text;
					}
					else if (_whichLog == Investigate)
					{
						Transform usePlacement = placement;

						if (auto* e = line.element.get())
						{
							if (followPitch)
							{
								e->with_location(OverlayInfo::Element::Relativity::RelativeToAnchorPY/*RelativeToCameraFlat*/, usePlacement.get_translation());
								e->with_follow_camera_location_z(0.1f);
								e->with_rotation(OverlayInfo::Element::Relativity::RelativeToAnchorPY, usePlacement.get_orientation().to_rotator());
								e->with_distance(statsDist * 1.1f);
							}
							else
							{
								e->with_placement(OverlayInfo::Element::RelativeToCamera, usePlacement);
								e->with_follow_camera_yaw(5.0f);
								e->with_follow_camera_pitch(5.0f);
								e->with_distance(0.0f);
							}
						}
					}

					if (currentlyShowing == Main &&
						lineIdx == lineIndicatorAtIdx && !line.text.is_empty() &&
						showLog.withLineIndicator)
					{
						Transform usePlacement = placement;

						auto* e = new OverlayInfo::Elements::Text(String(TXT("> ")));
						e->with_id(NAME(mainLogLineIndicator));
						e->with_usage(OverlayInfo::Usage::World);
						e->with_font(oiFontStats.get());
						// no smaller decimals!
						if (line.colour.is_set())
						{
							e->with_colour(line.colour.get());
						}

						if (currentlyShowing == Main)
						{
							setup_overlay_element_placement(e, usePlacement);
						}
						else
						{
							if (followPitch)
							{
								e->with_location(OverlayInfo::Element::Relativity::RelativeToAnchorPY/*RelativeToCameraFlat*/, usePlacement.get_translation());
								e->with_follow_camera_location_z(0.1f);
								e->with_rotation(OverlayInfo::Element::Relativity::RelativeToAnchorPY, usePlacement.get_orientation().to_rotator());
								e->with_distance(statsDist * 1.1f);
							}
							else
							{
								e->with_placement(OverlayInfo::Element::RelativeToCamera, usePlacement);
								e->with_follow_camera_yaw(5.0f);
								e->with_follow_camera_pitch(5.0f);
								e->with_distance(0.0f);
							}
						}

						e->with_horizontal_align(1.0f);
						e->with_vertical_align(0.5f);

						e->with_letter_size(letterSize);

						overlayInfoSystem->add(e);

						line.indicatorElement = e;
					}

					if (_whichLog != Investigate ||
						!line.text.is_empty())
					{
						// for investigate, don't show empty lines
						placement = placement.to_world(offset);
						atCameraPitch -= atCameraPitchOffset;
					}
				}

				showLog.displayedLines = showLog.lines.get_size();
			}
		}
	}
}

void PilgrimOverlayInfo::switch_show_investigate()
{
	requestedToShow = currentlyShowing == Investigate ? None : Investigate;
}

void PilgrimOverlayInfo::update_investigate_info()
{
	struct HandleLog
	{
		ShowLog& handleLog;
		bool requiresUpdate = false;
		HandleLog(ShowLog& _log) :handleLog(_log) {}
		void clear()
		{
			requiresUpdate = true;
			handleLog.lines.clear();
		}
		int add()
		{
			requiresUpdate = true;
			handleLog.lines.push_back(ShowLog::Element());
			return handleLog.lines.get_size() - 1;
		}
		void update(int _lineIdx, String const & _text)
		{
			requiresUpdate = true;
			while (handleLog.lines.get_size() <= _lineIdx)
			{
				handleLog.lines.push_back(ShowLog::Element());
			}
			auto& line = handleLog.lines[_lineIdx];
			if (_text.does_contain('~'))
			{
				if (handleLog.lines.get_size() == _lineIdx + 1)
				{
					String text = _text;
					bool multiLineContinuation = false;
					line.text = text;
					while (! text.is_empty())
					{
						int at = text.find_first_of('~');
						ShowLog::Element e;
						
						if (at != NONE)
						{
							e.text = text.get_left(at);
							text = text.get_sub(at + 1);
						}
						else
						{
							e.text = text;
							text = String::empty();
						}

						e.multiLineContinuation = multiLineContinuation;
						if (!multiLineContinuation)
						{
							line = e;
						}
						else
						{
							handleLog.lines.push_back(e);
						}

						multiLineContinuation = true;
					}
				}
				else
				{
					error(TXT("multiline are supported only for last log entry"));
					int at = _text.find_first_of('~');
					line.text = _text.get_left(at) + TXT("...");
				}
			}
			else
			{
				line.text = _text;
			}
			// actual element will be updated with update_investigate_log
		}
	};

	HandleLog handleLog(showInvestigateLog);
	
	auto* shouldShow = scanForObjectDescriptorKeepData.investigate.chosen.get();
	if (investigateLogInfo.imo != shouldShow ||
		(investigateLogInfo.imo.is_pointing_at_something() && ! shouldShow) || // disappeared/died/deleted
		showInvestigateLog.lines.is_empty()) // to show "no object"
	{
		investigateLogInfo.imo = shouldShow;
		showInvestigateLog.lines.clear();
		update_investigate_log(true);

		// create lines
		if (! shouldShow)
		{
			int lineIdx = handleLog.add();
			handleLog.update(lineIdx, LOC_STR(NAME(lsInvestigateNoObject)));			
		}
		else
		{
			investigateLogInfo.health.lineIdx = handleLog.add();
			investigateLogInfo.health.cachedData.clear();
			investigateLogInfo.shield.lineIdx = handleLog.add();
			investigateLogInfo.shield.cachedData.clear();
			investigateLogInfo.armour.lineIdx = handleLog.add();
			investigateLogInfo.armour.cachedData.clear();
			investigateLogInfo.ricochet.lineIdx = handleLog.add();
			investigateLogInfo.ricochet.cachedData.clear();
			investigateLogInfo.proneToExplosions.lineIdx = handleLog.add();
			investigateLogInfo.proneToExplosions.cachedData.clear();
			investigateLogInfo.social.lineIdx = handleLog.add();
			investigateLogInfo.social.cachedData.clear();
			investigateLogInfo.extraEffects.set_size(investigateLogInfo.extraEffects.get_max_size());
			for_every(ee, investigateLogInfo.extraEffects)
			{
				ee->lineIdx = handleLog.add();
				ee->cachedData.clear();
			}
			investigateLogInfo.confused.lineIdx = handleLog.add();
			investigateLogInfo.confused.cachedData.clear();
			investigateLogInfo.noSight.lineIdx = handleLog.add();
			investigateLogInfo.noSight.cachedData.clear();
			investigateLogInfo.noHearing.lineIdx = handleLog.add();
			investigateLogInfo.noHearing.cachedData.clear();
#ifdef WITH_DETAILED_HEALTH_RULES_INFO
			investigateLogInfo.healthRules.clear();
			if (auto* h = shouldShow->get_custom<CustomModules::Health>())
			{
				investigateLogInfo.healthRules.set_size(h->get_rule_count());
				for_every(li, investigateLogInfo.healthRules)
				{
					li->lineIdx = handleLog.add();
					li->cachedData.clear();
				}
			}
#endif
			investigateLogInfo.investigatorInfoProvider.clear();
			if (auto* iip = shouldShow->get_custom<CustomModules::InvestigatorInfoProvider>())
			{
				investigateLogInfo.investigatorInfoProvider.set_size(iip->get_info_count());
				for_every(li, investigateLogInfo.investigatorInfoProvider)
				{
					li->lineIdx = handleLog.add();
					li->cachedData.clear();
				}
			}
		}
	}

	if (auto* imo = investigateLogInfo.imo.get())
	{
		MODULE_OWNER_LOCK_FOR_IMO(imo, TXT("investigate log"));
		auto* healthModule = imo->get_custom<CustomModules::Health>();
		if (auto* h = healthModule)
		{
			Energy showHealth = max(Energy::zero(), h->get_total_health());
			if (investigateLogInfo.health.cachedData.check_update(showHealth))
			{
				handleLog.update(investigateLogInfo.health.lineIdx,
					Framework::StringFormatter::format_sentence_loc_str(investigateInfo.health.info, Framework::StringFormatterParams()
						.add(NAME(value), showHealth.as_string())));
			}
			if (investigateLogInfo.proneToExplosions.cachedData.check_update('E')) // check just once to fill
			{
				Optional<EnergyCoef> proneToExplosions = h->get_prone_to_explosions();
				if (proneToExplosions.is_set())
				{
					handleLog.update(investigateLogInfo.proneToExplosions.lineIdx,
						Framework::StringFormatter::format_sentence_loc_str(investigateInfo.proneToExplosions.info, Framework::StringFormatterParams()
							.add(NAME(value), proneToExplosions.get().as_string_auto_decimals())));
				}
				else
				{
					handleLog.update(investigateLogInfo.proneToExplosions.lineIdx, String::empty());
				}
			}
#ifdef WITH_DETAILED_HEALTH_RULES_INFO
			{
				int destHRIdx = 0;
				for_count(int, srcHRIdx, investigateLogInfo.healthRules.get_size()) // they match in size
				{
					Optional<Energy> showHealth = h->get_rule_health(srcHRIdx);
					if (showHealth.is_set())
					{
						an_assert(investigateLogInfo.healthRules.is_index_valid(destHRIdx));
						auto& hr = investigateLogInfo.healthRules[destHRIdx];
						if (hr.cachedData.cachedData.check_update(showHealth))
						{
							handleLog.update(hr.lineIdx,
								Framework::StringFormatter::format_sentence_loc_str(investigateInfo.healthRule.info, Framework::StringFormatterParams()
									.add(NAME(value), showHealth.get().as_string())));
						}
						++destHRIdx;
					}
				}
				while (destHRIdx < investigateLogInfo.healthRules.get_size())
				{
					auto& hr = investigateLogInfo.healthRules[destHRIdx];
					if (hr.cachedData.cachedData.check_update_empty())
					{
						handleLog.update(hr.lineIdx, String::empty());
					}
					++destHRIdx;
				}
			}
#endif
		}
		{
			Energy shieldHealth = Energy::zero();
			{
				::SafePtr<Framework::IModulesOwner> spawnedShield = imo->get_variables().get_value<::SafePtr<Framework::IModulesOwner>>(NAME(spawnedShield), ::SafePtr<Framework::IModulesOwner>());
				if (auto* s = spawnedShield.get())
				{
					if (auto* h = s->get_custom<CustomModules::Health>())
					{
						shieldHealth = h->get_health();
					}
				}
			}
			if (investigateLogInfo.shield.cachedData.check_update(shieldHealth)) // update once
			{
				if (shieldHealth.is_positive())
				{
					handleLog.update(investigateLogInfo.shield.lineIdx,
						Framework::StringFormatter::format_sentence_loc_str(investigateInfo.shield.info, Framework::StringFormatterParams()
							.add(NAME(value), shieldHealth.as_string())));
				}
				else
				{
					handleLog.update(investigateLogInfo.shield.lineIdx, String::empty());
				}
			}
		}
		{
			int eeIdx = 0;
			if (healthModule)
			{
				for_every(ee, healthModule->get_extra_effects())
				{
					if (ee->params.investigatorInfo.info.is_valid())
					{
						if (investigateLogInfo.extraEffects.is_index_valid(eeIdx))
						{
							auto& iliee = investigateLogInfo.extraEffects[eeIdx];
							if (iliee.cachedData.check_update(ee->params.investigatorInfo.id)) // update once
							{
								Framework::StringFormatterParams formatterParams;
								ee->params.setup_formatter_params(REF_ formatterParams);

								handleLog.update(iliee.lineIdx,
									Framework::StringFormatter::format_sentence_loc_str(ee->params.investigatorInfo.info, formatterParams));
							}
						}
						++eeIdx;
					}
				}
			}
			// clear everything else
			while (eeIdx < investigateLogInfo.extraEffects.get_size())
			{
				Name noEE;
				auto& iliee = investigateLogInfo.extraEffects[eeIdx];
				if (iliee.cachedData.check_update(noEE)) // update once
				{
					handleLog.update(iliee.lineIdx, String::empty());
				}
				++eeIdx;
			}
		}
		{
			bool confused = imo->get_variables().get_value<bool>(NAME(confused), false);
			if (investigateLogInfo.confused.cachedData.check_update(confused)) // update once
			{
				if (confused)
				{
					handleLog.update(investigateLogInfo.confused.lineIdx, investigateInfo.confused.info.get());
				}
				else
				{
					handleLog.update(investigateLogInfo.confused.lineIdx, String::empty());
				}
			}
		}
		{
			float armourInfo = imo->get_variables().get_value<float>(NAME(armourMaterialInfo), 0.0f);
			if (healthModule)
			{
				for_every(ee, healthModule->get_extra_effects())
				{
					if (ee->params.armourPiercing.is_set())
					{
						armourInfo *= ee->params.armourPiercing.get().as_float();
					}
				}
			}
			if (GameSettings::get().difficulty.armourIneffective)
			{
				armourInfo = 0.0f;
			}
			armourInfo = min(armourInfo, clamp(GameSettings::get().difficulty.armourEffectivenessLimit, 0.0f, 1.0f));
			if (investigateLogInfo.armour.cachedData.check_update(armourInfo))
			{
				if (armourInfo > 0.0f)
				{
					handleLog.update(investigateLogInfo.armour.lineIdx,
						Framework::StringFormatter::format_sentence_loc_str(investigateInfo.armour100.info, Framework::StringFormatterParams()
							.add(NAME(value), armourInfo * 100.0f)));
				}
				else
				{
					handleLog.update(investigateLogInfo.armour.lineIdx, String::empty());
				}
			}
		}
		{
			float ricochetInfo = imo->get_variables().get_value<float>(NAME(ricochetMaterialInfo), 0.0f);
			if (GameSettings::get().difficulty.armourIneffective)
			{
				ricochetInfo = 0.0f;
			}
			if (healthModule && !healthModule->do_extra_effects_allow_ricochets())
			{
				ricochetInfo = 0.0f;
			}
			ricochetInfo = min(ricochetInfo, clamp(GameSettings::get().difficulty.armourEffectivenessLimit, 0.0f, 1.0f));
			if (investigateLogInfo.ricochet.cachedData.check_update(ricochetInfo))
			{
				if (ricochetInfo > 0.0f)
				{
					handleLog.update(investigateLogInfo.ricochet.lineIdx,
						Framework::StringFormatter::format_sentence_loc_str(investigateInfo.ricochet.info, Framework::StringFormatterParams()
							.add(NAME(value), ricochetInfo * 100.0f)));
				}
				else
				{
					handleLog.update(investigateLogInfo.ricochet.lineIdx, String::empty());
				}
			}
		}
		{
			int valueNoSight = (imo->get_variables().get_value<bool>(NAME(perceptionNoSight), false)? 1 : 0)
							|| (imo->get_variables().get_value<bool>(NAME(perceptionSightImpaired), false)? 1 : 0);
			int valueSightMovementBased = (imo->get_variables().get_value<bool>(NAME(perceptionSightMovementBased), false)? 1 : 0);
			int value = (valueNoSight ? bit(1) : 0)
					  | (valueSightMovementBased ? bit(2) : 0);
			if (investigateLogInfo.noSight.cachedData.check_update(value))
			{
				if (is_flag_set(value, bit(1)))
				{
					handleLog.update(investigateLogInfo.noSight.lineIdx, investigateInfo.noSight.info.get());
				}
				else if (is_flag_set(value, bit(2)))
				{
					handleLog.update(investigateLogInfo.noSight.lineIdx, investigateInfo.sightMovementBased.info.get());
				}
				else
				{
					handleLog.update(investigateLogInfo.noSight.lineIdx, String::empty());
				}
			}
		}
		{
			int value = imo->get_variables().get_value<bool>(NAME(perceptionNoHearing), false) ? 1 : 0;
			if (investigateLogInfo.noHearing.cachedData.check_update(value))
			{
				if (value)
				{
					handleLog.update(investigateLogInfo.noHearing.lineIdx, investigateInfo.noHearing.info.get());
				}
				else
				{
					handleLog.update(investigateLogInfo.noHearing.lineIdx, String::empty());
				}
			}
		}
		{
			int value = 0;
			bool ignoreSocial = false;
			if (auto* iip = imo->get_custom<CustomModules::InvestigatorInfoProvider>())
			{
				ignoreSocial = iip->should_ignore_social_info();
			}
			if (! ignoreSocial)
			{
				if (auto* ai = imo->get_ai())
				{
					if (auto* mind = ai->get_mind())
					{
						if (mind->get_social().is_ally(owner.get()))
						{
							value = 1;
						}
						if (mind->get_social().is_enemy(owner.get()))
						{
							value = -1;
						}
					}
				}
			}
			if (investigateLogInfo.social.cachedData.check_update(value))
			{
				if (value > 0)
				{
					handleLog.update(investigateLogInfo.social.lineIdx, investigateInfo.ally.info.get());
				}
				else if (value < 0)
				{
					handleLog.update(investigateLogInfo.social.lineIdx, investigateInfo.enemy.info.get());
				}
				else
				{
					handleLog.update(investigateLogInfo.social.lineIdx, String::empty());
				}
			}
		}
		{
			if (auto* iip = imo->get_custom<CustomModules::InvestigatorInfoProvider>())
			{
				for_every(destiip, investigateLogInfo.investigatorInfoProvider)
				{
					int idx = for_everys_index(destiip);
					if (iip->does_require_update(idx, destiip->cachedData))
					{
						if (iip->is_active(idx))
						{
							handleLog.update(destiip->lineIdx, iip->get_info(idx));
						}
						else
						{
							handleLog.update(destiip->lineIdx, String::empty());
						}
					}
				}
			}
		}
	}

	if (handleLog.requiresUpdate)
	{
		update_investigate_log(false);
	}
}

void PilgrimOverlayInfo::update_subtitle_log()
{
	return; // no subtitle log ATM
	if (auto* s = SubtitleSystem::get())
	{
		if (showSubtitleLog.ver != s->get_user_log_ver())
		{
			if (auto* game = Game::get_as<Game>())
			{
				if (auto* overlayInfoSystem = game->access_overlay_info_system())
				{
					overlayInfoSystem->deactivate(NAME(subtitleLog));

					showSubtitleLog.lines.clear();

					float letterSize = 0.025f * 0.5f;
					float lineOffset = letterSize * 1.2f;
					int lineLimit = 20;

					s->trim_user_log(lineLimit);
					s->for_user_log(lineLimit, [this](String const& _line) { showSubtitleLog.lines.push_back(_line); });

					Vector3 loc = Vector3(0.5f, 0.05f, 0.0f).normal() * statsDist * 1.1f;
					Transform placement = get_vr_placement_for_local(owner.get(), matrix_from_up_forward(loc, Vector3(0.0f, 0.0f, 1.0f), Vector3(0.8f, 0.0f, 0.0f)).to_transform());

					placement = placement.to_world(Transform(Vector3::zAxis * (-lineOffset * (float)lineLimit * 0.5f + (lineOffset - letterSize) * 0.5f), Quat::identity));

					Transform offset = Transform(Vector3::zAxis * lineOffset, Quat::identity);

					for_every_reverse(line, showSubtitleLog.lines)
					{
						Transform usePlacement = placement;

						auto* e = new OverlayInfo::Elements::Text(*line);
						e->with_id(NAME(subtitleLog));
						e->with_usage(OverlayInfo::Usage::World);
						e->with_font(oiFontStats.get());
						e->with_smaller_decimals();

						setup_overlay_element_placement(e, usePlacement);

						e->with_horizontal_align(0.0f);
						e->with_vertical_align(0.5f);

						e->with_letter_size(letterSize);

						overlayInfoSystem->add(e);

						placement = placement.to_world(offset);
					}
				}
			}

			// update at the and as trimming may update ver
			showSubtitleLog.ver = s->get_user_log_ver();
		}
	}
}

void PilgrimOverlayInfo::update_equipment_stats(Framework::IModulesOwner* _owner)
{
	if (showAmmoStatus && !fullUpdateEquipmentStatsRequired)
	{
		return;
	}

	workingEquipmentsStats.reset();

	if (showAmmoStatus && fullUpdateEquipmentStatsRequired)
	{
		fullUpdateEquipmentStatsRequired = false;
		if (auto* mp = _owner->get_gameplay_as<ModulePilgrim>())
		{
			for_count(int, iHand, Hand::MAX)
			{
				Hand::Type hand = (Hand::Type)iHand;
#ifdef WITH_SHORT_EQUIPMENT_STATS
				auto* me = mp->get_main_equipment(hand);
				if (me)
				{
					EquipmentStats & gs = workingEquipmentStats;
					gs.reset();
					gs.equipment = me;
					gs.update_stats();
					workingEquipmentsStats.mainEquipment[hand].copy_stats_from(gs);
				}
#endif
				auto* he = mp->get_hand_equipment(hand);
				if (he)
				{
					EquipmentStats& gs = workingEquipmentStats;
					gs.reset();
					gs.equipment = he;
					gs.update_stats();
					workingEquipmentsStats.usedEquipment[hand].copy_stats_from(gs);
					workingEquipmentsStats.usedEquipmentInHand[hand].copy_stats_from(gs);
				}
#ifdef WITH_SHORT_EQUIPMENT_STATS
				ArrayStatic<Framework::IModulesOwner*, 16> inPockets; SET_EXTRA_DEBUG_INFO(inPockets, TXT("PilgrimOverlayInfo::update_equipment_stats.inPockets"));
				mp->get_in_pockets(inPockets, hand, true, true);
				for_every(ip, inPockets)
				{
					EquipmentStats& gs = workingEquipmentStats;
					gs.reset();
					gs.equipment = *ip;
					gs.update_stats();
					workingEquipmentsStats.pockets[hand].push_back(gs);
				}
#endif
			}
		}
	}

	if (equipmentsStats == workingEquipmentsStats)
	{
#ifdef WITH_SHORT_EQUIPMENT_STATS
		for_count(int, iHand, Hand::MAX)
		{
			update_equipment_stats(equipmentsStats.mainEquipment[iHand], 8);
			update_equipment_stats(equipmentsStats.usedEquipment[iHand], 8);
			for_every(pes, equipmentsStats.pockets[iHand])
			{
				update_equipment_stats(*pes, 8);
			}
		};
#endif
	}
	else
	{
		equipmentsStats.copy_equipment_stats_from(workingEquipmentsStats);
		if (auto* game = Game::get_as<Game>())
		{
			if (auto* overlayInfoSystem = game->access_overlay_info_system())
			{
				add_equipment_stats(_owner, overlayInfoSystem);
			}
		}
	}
}

void PilgrimOverlayInfo::set_may_request_looking_for_pickups(bool _lookForPickups)
{
	mayRequestLookingForPickups = !noExplicitTutorials  && _lookForPickups;
}

void PilgrimOverlayInfo::update_in_hand_equipment_stats(float _deltaTime)
{
	Concurrency::ScopedSpinLock lock(equipmentsStats.showInHandReasonsLock);

	OverlayInfo::System* overlayInfoSystem = nullptr;
	if (auto* game = Game::get_as<Game>())
	{
		overlayInfoSystem = game->access_overlay_info_system();
	}

	if (!overlayInfoSystem)
	{
		return;
	}

	for_count(int, iHand, Hand::MAX)
	{
		auto& reasons = equipmentsStats.showInHandReasons[iHand];
		if (!reasons.is_empty())
		{
			bool forceUpdate = false;
			for (int ir = 0; ir < reasons.get_size(); ++ir)
			{
				auto& reason = reasons[ir];
				if (reason.timeLeft.is_set())
				{
					reason.timeLeft = reason.timeLeft.get() - _deltaTime;
					if (reason.timeLeft.get() <= 0.0f)
					{
						reasons.remove_fast_at(ir);
						if (reasons.get_size() == 1 && reasons[0].reason == NAME(gunDamagedShortInfo))
						{
							forceUpdate = true;
						}
					}
				}
			}

			if (reasons.is_empty() || GameDirector::get()->is_pilgrim_overlay_blocked() || GameDirector::get()->is_pilgrim_overlay_in_hand_stats_blocked())
			{
				overlayInfoSystem->deactivate(iHand == Hand::Left ? NAME(pilgrimEquipmentInHandLeftStat) : NAME(pilgrimEquipmentInHandRightStat));
			}
			else
			{
				auto& uesInHand = equipmentsStats.usedEquipmentInHand[iHand];

				// update only if we changed equipment or we're forced
				Framework::IModulesOwner* forHandEquipment = nullptr;
				{
					if (owner.get())
					{
						if (auto* mp = owner->get_gameplay_as<ModulePilgrim>())
						{
							forHandEquipment = mp->get_hand_equipment((Hand::Type)iHand);
						}
					}
				}

				bool uesDiffrent = false;
				if (uesInHand.equipment != forHandEquipment || forceUpdate)
				{
					EquipmentStats& currentUESInHand = workingEquipmentStats;
					if (!forceUpdate)
					{
						currentUESInHand.reset();
						currentUESInHand = uesInHand;
					}

					uesInHand.reset();
					uesInHand.equipment = forHandEquipment;
					uesInHand.update_stats();

					uesDiffrent = forceUpdate || currentUESInHand != uesInHand;
				}

				if (uesDiffrent || forceUpdate)
				{
#ifdef OUTPUT_OVERLAY_INFO_IN_HAND
					output(TXT("update in hand %S"), Hand::to_char((Hand::Type)iHand));
#endif

					if (overlayInfoSystem)
					{
						overlayInfoSystem->deactivate(iHand == Hand::Left ? NAME(pilgrimEquipmentInHandLeftStat) : NAME(pilgrimEquipmentInHandRightStat));

						if (uesInHand.equipment.get())
						{
							uesInHand.reset(true);
							uesInHand.update_stats(true, true); // update full
							add_in_hand_equipment_stats(overlayInfoSystem, uesInHand, iHand == Hand::Left ? NAME(pilgrimEquipmentInHandLeftStat) : NAME(pilgrimEquipmentInHandRightStat), (Hand::Type)iHand);
						}
					}
				}
				else
				{
					for_count(int, iHand, Hand::MAX)
					{
						update_equipment_stats(uesInHand, 5);
					}
				}
			}
		}
	}
}

void PilgrimOverlayInfo::clear_in_hand_equipment_stats(Hand::Type _hand)
{
#ifdef OUTPUT_OVERLAY_INFO_IN_HAND
	output(TXT("clear in hand %S"), Hand::to_char(_hand));
#endif
	Concurrency::ScopedSpinLock lock(equipmentsStats.showInHandReasonsLock);
	auto& reasons = equipmentsStats.showInHandReasons[_hand];

	bool wereNoReasons = reasons.is_empty();
	reasons.clear();

	if (!wereNoReasons)
	{
#ifdef OUTPUT_OVERLAY_INFO_IN_HAND
		output(TXT("clear in hand!"));
#endif

		OverlayInfo::System* overlayInfoSystem = nullptr;
		if (auto* game = Game::get_as<Game>())
		{
			overlayInfoSystem = game->access_overlay_info_system();
		}

		if (overlayInfoSystem)
		{
			overlayInfoSystem->deactivate(_hand == Hand::Left ? NAME(pilgrimEquipmentInHandLeftStat) : NAME(pilgrimEquipmentInHandRightStat));
		}
	}
}

void PilgrimOverlayInfo::show_in_hand_equipment_stats(Hand::Type _hand, Name const& _reason, Optional<float> const & _timeLeft)
{
	if (GameDirector::get()->is_pilgrim_overlay_blocked() ||
		GameDirector::get()->is_pilgrim_overlay_in_hand_stats_blocked())
	{
		// don't try to show at all (if something is visible, will be hidden)
		return;
	}

#ifdef OUTPUT_OVERLAY_INFO_IN_HAND
	output(TXT("show in hand %S %S %.3f"), Hand::to_char(_hand), _reason.to_char(), _timeLeft.get(0.0f));
#endif
	Concurrency::ScopedSpinLock lock(equipmentsStats.showInHandReasonsLock);
	auto& reasons = equipmentsStats.showInHandReasons[_hand];

	bool wereNoReasons = reasons.is_empty();

	{
		for_every(r, reasons)
		{
			if (r->reason == _reason)
			{
				if (!_timeLeft.is_set())
				{
					r->timeLeft.clear();
				}
				else
				{
					if (!r->timeLeft.is_set() || r->timeLeft.get() < _timeLeft.get())
					{
						r->timeLeft = _timeLeft;
					}
				}
				return; // no need to change anything
			}
		}
		reasons.push_back_unique(TimedReason(_reason, _timeLeft));
	}

	if ((reasons.get_size() == 2 && reasons[0].reason == NAME(gunDamagedShortInfo)) || // there was only one reason and it was gunDamagedShortInfo)
		wereNoReasons)
	{
#ifdef OUTPUT_OVERLAY_INFO_IN_HAND
		output(TXT("show in hand!"));
#endif

		OverlayInfo::System* overlayInfoSystem = nullptr;
		if (auto* game = Game::get_as<Game>())
		{
			overlayInfoSystem = game->access_overlay_info_system();
		}

		if (overlayInfoSystem)
		{
			internal_show_in_hand_equipment_stats(_hand, overlayInfoSystem);
		}
	}
}

void PilgrimOverlayInfo::internal_show_in_hand_equipment_stats(Hand::Type _hand, OverlayInfo::System* overlayInfoSystem)
{
	overlayInfoSystem->deactivate(_hand == Hand::Left ? NAME(pilgrimEquipmentInHandLeftStat) : NAME(pilgrimEquipmentInHandRightStat));

	auto& uesInHand = equipmentsStats.usedEquipmentInHand[_hand];

	EquipmentStats & newUseInHand = workingEquipmentStats;
	newUseInHand.reset();

	if (owner.get())
	{
		if (auto* mp = owner->get_gameplay_as<ModulePilgrim>())
		{
			newUseInHand.equipment = mp->get_hand_equipment(_hand);
		}
	}

	uesInHand.copy_stats_from(newUseInHand);
	uesInHand.update_stats(true, true);

	if (uesInHand.equipment.is_set())
	{
		add_in_hand_equipment_stats(overlayInfoSystem, uesInHand, _hand == Hand::Left ? NAME(pilgrimEquipmentInHandLeftStat) : NAME(pilgrimEquipmentInHandRightStat), (Hand::Type)_hand);
	}
}

void PilgrimOverlayInfo::hide_in_hand_equipment_stats(Hand::Type _hand, Name const& _reason)
{
#ifdef OUTPUT_OVERLAY_INFO_IN_HAND
	output(TXT("hide in hand %S %S %.3f"), Hand::to_char(_hand), _reason.to_char());
#endif

	Concurrency::ScopedSpinLock lock(equipmentsStats.showInHandReasonsLock);
	auto& reasons = equipmentsStats.showInHandReasons[_hand];

	reasons.remove_fast(TimedReason(_reason));
	if (reasons.is_empty())
	{
#ifdef OUTPUT_OVERLAY_INFO_IN_HAND
		output(TXT("hide in hand!"));
#endif
		OverlayInfo::System* overlayInfoSystem = nullptr;
		if (auto* game = Game::get_as<Game>())
		{
			overlayInfoSystem = game->access_overlay_info_system();
		}

		if (overlayInfoSystem)
		{
			overlayInfoSystem->deactivate(_hand == Hand::Left ? NAME(pilgrimEquipmentInHandLeftStat) : NAME(pilgrimEquipmentInHandRightStat));
		}
	}
	else if (reasons.get_size() == 1 && reasons[0].reason == NAME(gunDamagedShortInfo))
	{
		recreate_in_hand_equipment_stats(_hand);
	}
}

void PilgrimOverlayInfo::recreate_in_hand_equipment_stats(Optional<Hand::Type> const & _hand)
{
	for_count(int, iHand, Hand::MAX)
	{
		if (_hand.is_set() && _hand.get() != iHand)
		{
			continue;
		}

		Concurrency::ScopedSpinLock lock(equipmentsStats.showInHandReasonsLock, true);
		auto& reasons = equipmentsStats.showInHandReasons[iHand];

		if (reasons.is_empty())
		{
			continue;
		}

		OverlayInfo::System* overlayInfoSystem = nullptr;
		if (auto* game = Game::get_as<Game>())
		{
			overlayInfoSystem = game->access_overlay_info_system();
		}

		if (overlayInfoSystem)
		{
			internal_show_in_hand_equipment_stats((Hand::Type)iHand, overlayInfoSystem);
		}
	}
}

void PilgrimOverlayInfo::add_equipment_stats(Framework::IModulesOwner* _owner, OverlayInfo::System* _overlayInfoSystem)
{
#ifdef WITH_SHORT_EQUIPMENT_STATS
	float size = 0.025f;
	float verticalOffset = size * 7.0f;
	Transform offset = Transform(-Vector3::zAxis * verticalOffset, Quat::identity);

	int amount = 2 + max(equipmentsStats.pockets[Hand::Left].get_size(), equipmentsStats.pockets[Hand::Right].get_size());

	_overlayInfoSystem->deactivate(NAME(pilgrimEquipmentStat));

	for_count(int, iHand, Hand::MAX)
	{
		Vector3 forward = Vector3(-0.3f, 0.5f, 0.0f);
		Vector3 right = forward.rotated_right();
		Vector3 loc = forward + right * (iHand == Hand::Left? -0.7f : -0.3f);
		float mainAtZ = verticalOffset * ((float)amount) * 0.5f;
		Transform placement = get_vr_placement_for_local(_owner, matrix_from_up_forward(loc + Vector3::zAxis * mainAtZ, Vector3(0.0f, 0.0f, 1.0f), forward).to_transform());

		auto& mes = equipmentsStats.mainEquipment[iHand];
		auto& ues = equipmentsStats.usedEquipment[iHand];
		mes.update_stats();
		ues.update_stats();
		for_every(pes, equipmentsStats.pockets[iHand])
		{
			pes->update_stats();
		}
		if (mes.equipment.is_set() &&
			mes.equipment != ues.equipment)
		{
			add_one_equipment_stats(_overlayInfoSystem, mes, NAME(pilgrimEquipmentStat), placement, size);
		}
		placement = placement.to_world(offset);
		if (ues.equipment.is_set())
		{
			add_one_equipment_stats(_overlayInfoSystem, ues, NAME(pilgrimEquipmentStat), placement, size);
		}
		placement = placement.to_world(offset);
		for_every(pes, equipmentsStats.pockets[iHand])
		{
			add_one_equipment_stats(_overlayInfoSystem, *pes, NAME(pilgrimEquipmentStat), placement, size);
			placement = placement.to_world(offset);
		}
	}
#endif
}

void PilgrimOverlayInfo::update_equipment_stats(EquipmentStats& _stats, int _padLeft)
{
	if (_stats.equipment.is_set())
	{
		if (auto* gun = _stats.equipment->get_gameplay_as<ModuleEquipments::Gun>())
		{
			IF_DIFFER_SET(_stats.shots.value, gun->get_shots_left())
			{
				update_stat_overlay_element(_stats.shots.element, String::printf(TXT("%i"), _stats.shots.value));
			}
		}

		if (auto* h = _stats.equipment->get_custom<CustomModules::Health>())
		{
			IF_DIFFER_SET(_stats.health.value, h->get_total_health())
			{
				update_stat_overlay_element(_stats.health.element, _stats.health.value.as_string(2).pad_left(_padLeft));
			}
		}
	}
}

void PilgrimOverlayInfo::add_one_equipment_stats(OverlayInfo::System* _overlayInfoSystem, EquipmentStats & _stats, Name const& _id, Transform const& _at, float _size)
{
#ifdef WITH_SHORT_EQUIPMENT_STATS
	Transform placement = _at;

	float verticalSpacingPt = 0.2f;
	Transform offset = Transform(-Vector3::zAxis * (_size * (1.0f + verticalSpacingPt)), Quat::identity);

	if (_stats.equipment.is_set())
	{
		if (auto* gun = _stats.equipment->get_gameplay_as<ModuleEquipments::Gun>())
		{
			add_stat_at(_overlayInfoSystem, _id, NP, LOC_STR(TeaForGodEmperor::WeaponCoreKind::to_localised_string_id_short(_stats.gunStats.coreKind)), nullptr, placement, _size, nullptr);
			placement = placement.to_world(offset);
			add_stat_at(_overlayInfoSystem, _id, NP, _stats.gunStats.totalDamage.as_string(2).pad_left(8), nullptr, placement, _size, oigDamage.get());
			placement = placement.to_world(offset);
			add_stat_at(_overlayInfoSystem, _id, NP, _stats.gunStats.shotCost.as_string(2).pad_left(8), nullptr, placement, _size, oigShotCost.get());
			placement = placement.to_world(offset);
			String additional;
			int maxLength = 0;
			Optional<Colour> additionalsColour;
			if (!_stats.gunStats.damaged.is_zero())
			{
				additional = LOC_STR(NAME(lsEquipmentDamaged));
				additionalsColour = RegisteredColours::get_colour(NAME(pilgrim_overlay_weaponPart_damaged));
			}
			else
			{
				maxLength = 8;
				String add(TXT("+"));
				if (!_stats.gunStats.magazineSize.is_zero()) additional += add;
				if (!_stats.gunStats.armourPiercing.is_zero()) additional += add;
				if (_stats.gunStats.antiDeflection != 0.0f) additional += add;
				if (_stats.gunStats.continuousFire) additional += add;
				for_count(int, i, _stats.gunStats.specialFeatures.get_size()) { additional += add; }
				for_every(locStr, _stats.gunStats.statInfoLocStrs)
				{
					String t = LOC_STR(*locStr);
					int count = 1 + t.count('~');
					for_count(int, i, count) { additional += add; }
				}
				for_every(locStr, _stats.gunStats.projectileStatInfoLocStrs)
				{
					String t = LOC_STR(*locStr);
					int count = 1 + t.count('~');
					for_count(int, i, count) { additional += add; }
				}
			}
			int linesLeft = 2;
			while (!additional.is_empty() && linesLeft > 0)
			{
				String addNow = additional;
				if (maxLength > 0 && additional.get_length() > maxLength)
				{
					addNow = additional.get_left(maxLength);
					additional = additional.get_sub(maxLength);
				}
				else
				{
					additional = String::empty();
				}
				add_stat_at(_overlayInfoSystem, _id, NP, addNow, nullptr, placement, _size, nullptr, String::empty(), additionalsColour);
				placement = placement.to_world(offset);
				--linesLeft;
			}

			/*
			if (!_stats.gunStats.magazineSize.is_zero() && linesLeft > 0)
			{
				add_stat_at(_overlayInfoSystem, _id, NP, ((_stats.gunStats.magazineSize / _stats.gunStats.shotCost).floored() + Energy::one() / * in chamber * /).as_string(0).pad_left(5), nullptr, placement, _size, nullptr, LOC_STR(NAME(lsEquipmentMagazineShort)));
				placement = placement.to_world(offset);
				--linesLeft;
			}
			if (!_stats.gunStats.armourPiercing.is_zero() && linesLeft > 0)
			{
				add_stat_at(_overlayInfoSystem, _id, NP, _stats.gunStats.armourPiercing.as_string_percentage(0).pad_left(6), nullptr, placement, _size, nullptr, LOC_STR(NAME(lsEquipmentArmourPiercingShort)));
				placement = placement.to_world(offset);
				--linesLeft;
			}
			*/
		}
		if (auto* h = _stats.equipment->get_custom<CustomModules::Health>())
		{
			add_stat_at(_overlayInfoSystem, _id, NP, String::empty(), &_stats.health.element, placement, _size, oieHealth.get());
			placement = placement.to_world(offset);
			_stats.health.value = h->get_total_health();
			update_stat_overlay_element(_stats.health.element, _stats.health.value.as_string(2).pad_left(8));
		}
	}
#endif
}

void PilgrimOverlayInfo::add_in_hand_equipment_stats(OverlayInfo::System* _overlayInfoSystem, EquipmentStats& _stats, Name const& _id, Hand::Type _inHand)
{
	Transform placement = look_matrix(Vector3::zero, -Vector3::zAxis, Vector3::yAxis).to_transform();
	Transform orgPlacement = placement;

	float _size = 0.008f;
	float verticalSpacingPt = 0.2f;
	Transform offset = Transform(-Vector3::zAxis * (_size * (1.0f + verticalSpacingPt)), Quat::identity);

	float nodesSize = 0.016f;
	float nodesHorizontalSpacingPt = 0.2f;
	float nodesVerticalSpacingPt = 0.2f;

	Transform nodesOffsetStartZ = Transform( Vector3::zAxis * (nodesSize * (0.6f)), Quat::identity);
	Transform nodesOffsetStartX = Transform(-Vector3::xAxis * (nodesSize * (0.5f + nodesHorizontalSpacingPt)), Quat::identity);
	Transform nodesOffsetX = Transform(-Vector3::xAxis * (nodesSize * (1.0f + nodesVerticalSpacingPt)), Quat::identity);
#ifndef WEAPON_PARTS_IN_EACH_COLUMN
	Transform nodesOffsetZ = Transform(-Vector3::zAxis * (nodesSize * (1.0f + nodesVerticalSpacingPt)), Quat::identity);
#endif

	if (auto* eq = _stats.equipment.get())
	{
		if (auto* gun = eq->get_gameplay_as<ModuleEquipments::Gun>())
		{
			auto& reasons = equipmentsStats.showInHandReasons[_inHand];
			bool shortInfoForGunDamaged = reasons.get_size() == 1 && reasons[0].reason == NAME(gunDamagedShortInfo);

			Transform useOffset = offset;
			float useSize = _size;
			if (shortInfoForGunDamaged)
			{
				useSize *= 2.0f;
				useOffset.set_translation(useOffset.get_translation() * 2.0f);
				placement = placement.to_world(useOffset);
			}

			if (!shortInfoForGunDamaged)
			{
				float ammoSize = useSize * 2.0f;
				Transform placementAmmo = placement.to_world(Transform(/*Vector3::xAxis * useSize * 16.0f + */Vector3::zAxis * useSize * 0.0f, Quat::identity));
				add_stat_at(_overlayInfoSystem, _id, _inHand, String::empty(), &_stats.shots.element, placementAmmo, ammoSize, nullptr);
				if (auto* t = fast_cast<OverlayInfo::Elements::Text>(_stats.shots.element.get()))
				{
					t->with_horizontal_align(0.0f);
					t->with_vertical_align(-0.1f); // higher
				}
				_stats.shots.value = gun->get_shots_left();
				update_stat_overlay_element(_stats.shots.element, String::printf(TXT("%i"), _stats.shots.value));
			}
			OverlayInfo::TextColours textColours;
			List<ModuleEquipments::GunChosenStats::StatInfo> builtStats;
			_stats.gunStats.build_overlay_stats_info(OUT_ builtStats, shortInfoForGunDamaged, OPTIONAL_ OUT_ &textColours);
			for_every(stat, builtStats)
			{
				Colour lineColour = Colour::white;
				auto statColour = textColours.process_line(stat->text);
				OverlayInfo::TextColours::alter_colour(lineColour, statColour);
				add_stat_at(_overlayInfoSystem, _id, _inHand, stat->text, nullptr, placement, useSize, nullptr, String::empty(), lineColour);
	#ifdef WEAPON_PARTS_IN_EACH_COLUMN
				WeaponStatAffection::ready_to_use();
				for_every_ref(wp, _stats.weaponParts)
				{
					for_every(affectedBy, stat->affectedBy)
					{
						if (affectedBy->how == WeaponStatAffection::Unknown) break;

						if (affectedBy->weaponPart == wp)
						{
							Transform place = placement;

							{
								place = place.to_world(nodesOffsetStartX);
								int column = for_everys_index(wp);
								for_count(int, i, column)
								{
									place = place.to_world(nodesOffsetX);
								}
							}

							RefCountObjectPtr<OverlayInfo::Element> e;
							add_stat_at(_overlayInfoSystem, _id, _inHand, WeaponStatAffection::to_ui_symbol(affectedBy->how), &e, place, useSize * 1.4f, nullptr, String::empty(), lineColour);
							if (auto* t = fast_cast<OverlayInfo::Elements::Text>(e.get()))
							{
								t->with_horizontal_align(0.5f);
							}
						}
					}
				}
	#endif
				placement = placement.to_world(useOffset);
			}

			if (!shortInfoForGunDamaged)
			{
				Transform place = orgPlacement;

	#ifndef WEAPON_PARTS_IN_EACH_COLUMN
				int column = 0;
				int inColumn = 0;
	#endif
				place = place.to_world(nodesOffsetStartZ);
				place = place.to_world(nodesOffsetStartX);
				for_every_ref(wp, _stats.weaponParts)
				{
	#ifdef WEAPON_PARTS_IN_EACH_COLUMN
					place = orgPlacement;
					place = place.to_world(nodesOffsetStartZ);
					place = place.to_world(nodesOffsetStartX);
					int column = for_everys_index(wp);
					for_count(int, i, column)
					{
						place = place.to_world(nodesOffsetX);
					}
	#else
					{
						bool nextColumn = false;
						if (column == 0)
						{
							if (auto* wpt = wp->get_weapon_part_type())
							{
								if (wpt->get_type() == WeaponPartType::GunNode)
								{
									nextColumn = true;
								}
							}
						}
						else if (inColumn >= 3)
						{
							nextColumn = true;
						}
						if (nextColumn)
						{
							place = orgPlacement;
							place = place.to_world(nodesOffsetStartZ);
							place = place.to_world(nodesOffsetStartX);
							++column;
							for_count(int, i, column)
							{
								place = place.to_world(nodesOffsetX);
							}
							inColumn = 0;
						}
					}
	#endif
					if (auto* mi = wp->get_schematic_mesh_instance_for_overlay())
					{
						Transform usePlacement = place;

						auto* e = new OverlayInfo::Elements::Mesh(*mi);
						e->with_id(_id);
						e->with_usage(OverlayInfo::Usage::World);

						e->with_active_shader_param(NAME(useAlpha));

						setup_overlay_element_placement(e, usePlacement);

						e->with_scale(_size * 0.015f);
						e->with_max_dist_blend(_size * 10.0f);

						e->with_front_axis(-Vector3::yAxis);

						{
							::SafePtr<PilgrimOverlayInfo> safePOI(this);
							e->with_custom_vr_placement([_inHand, usePlacement, safePOI](OverlayInfo::System const* _system, OverlayInfo::Element const* _element) { return calculate_equipment_placement_for_overlay(_inHand, safePOI.get(), usePlacement); });
						}

						_overlayInfoSystem->add(e);

	#ifndef WEAPON_PARTS_IN_EACH_COLUMN
						place = place.to_world(nodesOffsetZ);
						++ inColumn;
	#endif
					}
				}
			}
		}
		if (auto* h = eq->get_custom<CustomModules::Health>())
		{
			add_stat_at(_overlayInfoSystem, _id, _inHand, String::empty(), &_stats.health.element, placement, _size, oieHealth.get());
			placement = placement.to_world(offset);
			_stats.health.value = h->get_total_health();
			update_stat_overlay_element(_stats.health.element, _stats.health.value.as_string(2).pad_left(5));
		}
	}
}

void PilgrimOverlayInfo::add_stat_at(OverlayInfo::System* _overlayInfoSystem, Name const& _id, Optional<Hand::Type> _inHand, String const& _text, OUT_ RefCountObjectPtr<OverlayInfo::Element>* out_element, Transform const& _placement, float _size, Framework::TexturePart* _texturePart, String const& _header, Optional<Colour> const& _colour)
{
	Transform placement = _placement;

	float horizontalSpacingPt = 0.2f;
	Transform offset = Transform(Vector3::xAxis * (_size * (1.0f + horizontalSpacingPt)), Quat::identity);
	Transform halfOffset = Transform(Vector3::xAxis * (_size * 0.5f) + Vector3::zAxis * (_size * hardcoded 0.125f /* to align properly with font */), Quat::identity);

	if (_texturePart)
	{
		Transform usePlacement = placement.to_world(halfOffset);

		auto* e = new OverlayInfo::Elements::TexturePart(_texturePart);
		e->with_id(_id);
		e->with_usage(OverlayInfo::Usage::World);

		setup_overlay_element_placement(e, usePlacement);

		e->with_size(_size);

		if (_inHand.is_set())
		{
			::SafePtr<PilgrimOverlayInfo> safePOI(this);
			e->with_custom_vr_placement([_inHand, usePlacement, safePOI](OverlayInfo::System const* _system, OverlayInfo::Element const* _element) { return calculate_equipment_placement_for_overlay(_inHand.get(), safePOI.get(), usePlacement); });
		}

		if (_colour.is_set())
		{
			e->with_colour(_colour.get());
		}

		_overlayInfoSystem->add(e);

		placement = placement.to_world(offset);
	}

	if (!_header.is_empty())
	{
		Transform usePlacement = placement;

		auto* e = new OverlayInfo::Elements::Text(_header);
		e->with_id(_id);
		e->with_usage(OverlayInfo::Usage::World);
		e->with_font(oiFontStats.get());
		e->with_smaller_decimals();

		setup_overlay_element_placement(e, usePlacement);

		e->with_horizontal_align(0.0f);
		e->with_vertical_align(0.5f);

		e->with_letter_size(_size);

		if (_inHand.is_set())
		{
			::SafePtr<PilgrimOverlayInfo> safePOI(this);
			e->with_custom_vr_placement([_inHand, usePlacement, safePOI](OverlayInfo::System const* _system, OverlayInfo::Element const* _element) { return calculate_equipment_placement_for_overlay(_inHand.get(), safePOI.get(), usePlacement); });
		}

		if (_colour.is_set())
		{
			e->with_colour(_colour.get());
		}

		_overlayInfoSystem->add(e);

		placement = placement.to_world(offset);
	}

	if (! _text.is_empty() || out_element)
	{
		Transform usePlacement = placement;

		auto* e = new OverlayInfo::Elements::Text(_text);
		e->with_id(_id);
		e->with_usage(OverlayInfo::Usage::World);
		e->with_font(oiFontStats.get());
		e->with_smaller_decimals();

		setup_overlay_element_placement(e, usePlacement);

		e->with_horizontal_align(0.0f);
		e->with_vertical_align(0.5f);

		e->with_letter_size(_size);

		if (_inHand.is_set())
		{
			::SafePtr<PilgrimOverlayInfo> safePOI(this);
			e->with_custom_vr_placement([_inHand, usePlacement, safePOI](OverlayInfo::System const* _system, OverlayInfo::Element const* _element) { return calculate_equipment_placement_for_overlay(_inHand.get(), safePOI.get(), usePlacement); });
		}

		if (_colour.is_set())
		{
			e->with_colour(_colour.get());
		}

		if (out_element)
		{
			*out_element = e;
		}

		_overlayInfoSystem->add(e);
	}
}

void PilgrimOverlayInfo::hide_map(OverlayInfo::System* _overlayInfoSystem)
{
	showMap.visible = false;

#ifdef OUTPUT_OVERLAY_INFO
	output(TXT("[POvIn] hide map, clear highlights"));
#endif

	_overlayInfoSystem->deactivate(NAME(map));
	disableMapWhenMovesTooFarFrom.clear();

	{
		Concurrency::ScopedSpinLock lock(showMap.highlightAtLock);
		showMap.highlightsLastVisibleAt.reset();
	}
}

void PilgrimOverlayInfo::show_map(Framework::IModulesOwner* _owner, OverlayInfo::System* _overlayInfoSystem)
{
	{
		Concurrency::ScopedSpinLock lock(showMap.highlightAtLock);
		if (showMap.resetHighlightsLastVisibleAt)
		{
			showMap.highlightsLastVisibleAt.reset();
			showMap.resetHighlightsLastVisibleAt = false;
		}
		if (showMap.highlightsLastVisibleAt.get_time_since() > 30.0f)
		{
			showMap.highlightAt.clear();
		}
	}

	bool shouldShowMap = true;
	if (!PlayerPreferences::should_show_map() || GameDirector::get()->is_map_blocked())
	{
		shouldShowMap = false;
	}
	else if (auto* piow = PilgrimageInstanceOpenWorld::get())
	{
		if (!piow->get_pilgrimage()->open_world__get_definition()->is_map_available())
		{
			shouldShowMap = false;
		}
	}
	else
	{
		shouldShowMap = false;
	}

	if (!shouldShowMap)
	{
		showMap.visible = false;
		_overlayInfoSystem->deactivate(NAME(map));
		disableMapWhenMovesTooFarFrom.clear();
		return;
	}

#ifdef OUTPUT_OVERLAY_INFO
	output(TXT("[POvIn] show map (%i)"), showMap.highlightAt.get_size());
#endif

	showMap.visible = true;
	showMap.rd.mark_new_data_required();

	_overlayInfoSystem->deactivate(NAME(map));

	auto* e = new OverlayInfo::Elements::CustomDraw();
	e->with_id(NAME(map));
	e->with_usage(OverlayInfo::Usage::World); 
	if (lockedToHead)
	{
		// use reference from now
		ReferencePlacement refPlace;
		setup_reference_placement(_owner, REF_ refPlace);

		Transform placement = get_vr_placement_for_local(_owner, matrix_from_up_right(Vector3(0.0f, 0.5f, -0.2f), Vector3(0.0f, -1.0f, 0.6f), Vector3::xAxis).to_transform(), &refPlace);
		setup_overlay_element_placement(e, placement, true);
	}
	else
	{
		//Transform placement = get_vr_placement_for_local(_owner, matrix_from_up_right(Vector3(0.0f, 0.5f, -0.4f), Vector3(0.0f, -0.6f, 1.0f), Vector3::xAxis).to_transform());
		Transform placement = get_vr_placement_for_local(_owner, matrix_from_up_right(Vector3(0.0f, 0.5f, -0.2f), Vector3(0.0f, -1.0f, 0.6f), Vector3::xAxis).to_transform());
		setup_overlay_element_placement(e, placement, true);
	}
	e->with_size(0.1f, false);
	::SafePtr<PilgrimOverlayInfo> sp(this);
	e->with_draw([sp](float _active, float _pulse, Colour const& _colour)
	{
		if (auto* poi = sp.get())
		{
			poi->showMap.rd.render(_active, _pulse, _colour, NP, poi->overlayShaderInstance);
		}
	});
	_overlayInfoSystem->add(e);

	update_disable_map_when_moves_too_far_from(_overlayInfoSystem);
}

void PilgrimOverlayInfo::update_disable_map_when_moves_too_far_from(OverlayInfo::System * _overlayInfoSystem)
{
	if (showMap.visible && lockedToHead) // map should be shown in the same spot, so we can look at it from any angle
	{
		float dist = 0.2f;
		disableMapWhenMovesTooFarFrom = Transform(_overlayInfoSystem->get_camera_placement().get_translation() + _overlayInfoSystem->get_camera_placement().get_axis(Axis::Forward) * dist, _overlayInfoSystem->get_camera_placement().get_orientation());
		disableMapWhenMovesTooFarFromDist = 0.45f;
	}
	else
	{
		disableMapWhenMovesTooFarFrom.clear();
	}
}

void PilgrimOverlayInfo::add_info_about_cell(Framework::IModulesOwner* _owner, OverlayInfo::System* _overlayInfoSystem, float _time)
{
#ifndef SHOW_INFO_ABOUT_CELL_WHEN_CHANGING_CELLS
	return;
#endif

	if (auto* piow = PilgrimageInstanceOpenWorld::get())
	{
		if (auto* owdef = piow->get_pilgrimage()->open_world__get_definition())
		{
			if (!owdef->has_tile_change_notifications())
			{
				return;
			}
		}
	}

	if (GameSettings::get().difficulty.showDirectionsOnlyToObjective)
	{
		// don't show it if we're just showing where we should go
		return;
	}

	_overlayInfoSystem->deactivate(NAME(infoAboutCell));

	{
		// placement
		Transform placement;
		float letterSize = 0.1f;
		float const nominalDistance = 1.0f;
		float const defaultDistance = 2.0f;
		{
			auto* presence = _owner->get_presence();
			Transform ownerPlacement = presence->get_placement();
			Transform eyesRelative = presence->get_eyes_relative_look();
			Transform eyesWS = ownerPlacement.to_world(eyesRelative);

			placement = get_vr_placement_for_local(_owner, Transform(Vector3::yAxis * 2.0f, Quat::identity));

			Transform startTrace = matrix_from_up_forward(eyesWS.get_translation(), ownerPlacement.get_axis(Axis::Up), eyesWS.get_axis(Axis::Forward)).to_transform();

			// find placement of the door, so we display info in front of the door
			//FOR_EVERY_DOOR_IN_ROOM(dir, _owner->get_presence()->get_in_room())
			for_every_ptr(dir, _owner->get_presence()->get_in_room()->get_doors())
			{
				if (dir->get_door() &&
					dir->is_visible() &&
					dir->get_door()->is_world_separator_door_due_to(WorldSeparatorReason::CellSeparator))
				{
					auto* aCell = dir->get_door()->get_variables().get_existing<VectorInt2>(NAME(aCell));
					auto* bCell = dir->get_door()->get_variables().get_existing<VectorInt2>(NAME(bCell));
					{
						if ((aCell && inOpenWorldCell.get() == *aCell) ||
							(bCell && inOpenWorldCell.get() == *bCell))
						{
							Vector3 startAt = dir->get_hole_centre_WS();
							Vector3 up = ownerPlacement.get_axis(Axis::Up);
							startAt += up * Vector3::dot(up, (eyesWS.get_translation() - startAt));
							Vector3 forward = dir->get_inbound_matrix().get_y_axis();
							// check if we can show information more in the direction we're looking to
							{
								Vector3 eyesForward = eyesWS.get_axis(Axis::Forward);
								eyesForward = eyesForward.drop_using(up).normal();
								if (Vector3::dot(forward, eyesForward) > -0.3f)
								{
									forward = (forward * 0.5f + eyesForward * 0.5f).normal();
								}
							}
							startTrace = matrix_from_up_forward(startAt, up, forward).to_transform();
							placement = get_vr_placement_for_world(_owner, look_matrix(startTrace.location_to_world(Vector3::yAxis * defaultDistance), startTrace.get_axis(Axis::Forward), up).to_transform());
							letterSize = 0.05f * max(defaultDistance / nominalDistance, 1.0f);
							break;
						}
					}
				}
			}

	#ifndef PLACE_INFO_ABOUT_CELL_IN_PILGRIM_OVERLAY_INFO__ALWAYS_IN_FRONT
			if (MainConfig::global().should_be_immobile_vr())
			{
				Vector3 relPlacement(0.0f, 1.5f, 0.5f);
				Vector3 loc = startTrace.get_translation() + ownerPlacement.vector_to_world(relPlacement);
				Vector3 dir = ownerPlacement.vector_to_world(relPlacement).normal();
				Vector3 up = presence->get_placement().get_axis(Axis::Up);
				placement = get_vr_placement_for_world(_owner, look_matrix(loc, dir, up).to_transform());
			}
			else
			{
				// try to place on a wall - if not possible, place anywhere
	#ifdef PLACE_INFO_ABOUT_CELL_IN_PILGRIM_OVERLAY_INFO__AT_ANY_WALL
				for_count(int, lineIdx, 10)
	#endif
				{
					float offZ = 0.0f;
	#ifdef PLACE_INFO_ABOUT_CELL_IN_PILGRIM_OVERLAY_INFO__AT_ANY_WALL
					float maxOffX = lineIdx == 0 ? 0.0f : (lineIdx <= 2 ? 0.3f : 0.5f);
					float maxDist = lineIdx <= 2 ? 2.0f : 8.0f;
	#else
					float maxOffX = 0.0f;
					float maxDist = 8.0f;
	#endif

					Framework::CheckCollisionContext checkCollisionContext;
					checkCollisionContext.collision_info_needed();
					checkCollisionContext.use_collision_flags(pilgrimOverlayInfoTrace);

					checkCollisionContext.start_with_nothing_to_check();
					checkCollisionContext.check_scenery();
					checkCollisionContext.check_room();
					checkCollisionContext.check_room_scenery();

					Framework::CollisionTrace collisionTrace(Framework::CollisionTraceInSpace::WS);
					{
						collisionTrace.add_location(presence->get_centre_of_presence_WS());
						collisionTrace.add_location(startTrace.get_translation());
						Vector3 dirLocal = (Vector3(Random::get_float(-maxOffX, maxOffX), 1.0f, offZ)) * maxDist;
						collisionTrace.add_location(startTrace.location_to_world(dirLocal));
					}

					Framework::CheckSegmentResult result;
					if (presence->trace_collision(AgainstCollision::Movement, collisionTrace, result, Framework::CollisionTraceFlag::ResultInPresenceRoom, checkCollisionContext, nullptr) && !result.hitNormal.is_zero())
					{
						letterSize = 0.05f * max((result.hitLocation - eyesWS.get_translation()).length() / nominalDistance, 1.0f);
						Vector3 up = presence->get_placement().get_axis(Axis::Up);
						if (abs(Vector3::dot(result.hitNormal, up)) > 0.9f)
						{
							Vector3 right = presence->get_placement().get_axis(Axis::Right);
							placement = get_vr_placement_for_world(_owner, look_matrix_with_right(result.hitLocation, -result.hitNormal, right).to_transform());
						}
						else
						{
							placement = get_vr_placement_for_world(_owner, look_matrix(result.hitLocation, -result.hitNormal, up).to_transform());
						}
	#ifdef PLACE_INFO_ABOUT_CELL_IN_PILGRIM_OVERLAY_INFO__AT_ANY_WALL
						break;
	#endif
					}
				}
			}
	#endif
		}

		/*
		{
			String text = String::printf(TXT("%S%i/%S%i"),
				inOpenWorldCell.get().x > 0 ? TXT("+") : TXT(""), inOpenWorldCell.get().x,
				inOpenWorldCell.get().y > 0 ? TXT("+") : TXT(""), inOpenWorldCell.get().y);
			auto* e = new OverlayInfo::Elements::Text(text);
			e->with_id(NAME(infoAboutCell));
			e->with_usage(OverlayInfo::Usage::World);
			e->with_font(oiFont.get());
			e->with_smaller_decimals();

			e->with_vr_placement(placement);
			e->apply_vr_anchor_displacement();

			e->auto_deactivate(_time);

			e->with_letter_size(letterSize);

			_overlayInfoSystem->add(e);
		}
		*/
		{
			float iconSize = letterSize * 2.0f;
			for_every(cit, inOpenWorldCellInfoTextures)
			{
				auto* e = new OverlayInfo::Elements::AltTexturePart(cit->texturePartUse);
				e->with_id(NAME(infoAboutCell));
				e->with_usage(OverlayInfo::Usage::World);

				e->with_vr_placement(placement.to_world(Transform(iconSize * cit->offset.to_vector3_xz() * 1.2f, Quat::identity)));
				e->apply_vr_anchor_displacement();

				if (cit->colourOverride.is_set())
				{
					e->with_colour(cit->colourOverride.get());
				}

				if (cit->altTexturePartUse.texturePart)
				{
					e->with_alt_texture_part_use(cit->altTexturePartUse);
					e->with_timing(cit->periodTime, cit->altPt, cit->altFirst);
					e->with_alt_on_top(cit->altOnTop);
					e->with_alt_offset_pt(cit->altOffsetPt);
					if (cit->altColourOverride.is_set())
					{
						e->with_alt_colour(cit->altColourOverride.get());
					}
				}

				e->auto_deactivate(_time);

				e->with_size(iconSize);

				_overlayInfoSystem->add(e);
			}
		}
	}

	if (auto* s = _owner->get_sound())
	{
		s->play_sound(NAME(changedOpenWorldCell));
	}
}

void PilgrimOverlayInfo::add_show_info(Framework::IModulesOwner* _owner, OverlayInfo::System* _overlayInfoSystem)
{
	Concurrency::ScopedSpinLock lock(showInfosLock);

	updateShowInfos = false;

#ifdef OUTPUT_OVERLAY_INFO_INFO
	output(TXT("[POvIn] add show info"));
#endif

	for_every(showInfo, showInfos)
	{
		if (showInfo->visible)
		{
#ifdef OUTPUT_OVERLAY_INFO_INFO
			output(TXT("[POvIn]  show info \"%S\" already visible"), showInfo->id.to_char());
#endif
			continue;
		}
#ifdef OUTPUT_OVERLAY_INFO_INFO
		output(TXT("[POvIn]  show info \"%S\" make visible now"), showInfo->id.to_char());
#endif
		showInfo->visible = true;

		auto* presence = _owner->get_presence();
		Transform ownerPlacement = presence->get_placement();
		Transform eyesRelative = presence->get_eyes_relative_look();
		Transform eyesWS = ownerPlacement.to_world(eyesRelative);

		Transform placement = get_vr_placement_for_world(_owner, showInfo->at);

		if (showInfo->setup_element)
		{
			auto* e = showInfo->setup_element();

			e->with_id(showInfo->id);
			e->with_usage(OverlayInfo::Usage::World);
			e->with_vr_placement(placement);
			e->apply_vr_anchor_displacement();

			showInfo->element = e;
			_overlayInfoSystem->add(e);
		}
		else
		{
			float size = 0.025f;

			if (showInfo->size.is_set())
			{
				size = showInfo->size.get();
			}
			else
			{
				size = 0.025f * max((showInfo->at.get_translation() - eyesWS.get_translation()).length() / 0.5f, 1.0f);
				showInfo->size = size;
			}

			{
				String text = showInfo->info;
				auto* e = new OverlayInfo::Elements::Text(text);
				e->with_id(showInfo->id);
				e->with_usage(OverlayInfo::Usage::World);
				e->with_font(oiFont.get());
				if (showInfo->allowSmallerDecimals.get(true))
				{
					e->with_smaller_decimals();
				}
				e->with_colours(showInfo->colours);

				if (showInfo->width.is_set())
				{
					int maxChars = max(4, TypeConversions::Normal::f_i_closest(showInfo->width.get() / (showInfo->size.get())));
					e->with_max_line_width(maxChars);
				}
				if (showInfo->pt.is_set())
				{
					e->with_horizontal_align(showInfo->pt.get().x);
					e->with_vertical_align(showInfo->pt.get().y);
				}

				e->with_vr_placement(placement);
				e->apply_vr_anchor_displacement();

				e->with_letter_size(size);

				showInfo->element = e;

				_overlayInfoSystem->add(e);
			}
		}

	}
}

Transform PilgrimOverlayInfo::get_vr_placement_for_world(Framework::IModulesOwner* _owner, Transform const& _worldPlacement) const
{
	auto* presence = _owner->get_presence();

	return presence->get_vr_anchor().to_local(_worldPlacement);
}

Transform PilgrimOverlayInfo::get_vr_placement_for_local(Framework::IModulesOwner* _owner, Transform const& _localPlacement, ReferencePlacement const * _usingRefPlacement)
{
	if (!_usingRefPlacement)
	{
		set_reference_placement_if_not_set();
		_usingRefPlacement = &referencePlacement.get();
	}
	Transform ownerPlacement = _usingRefPlacement->ownerPlacement;
	Transform eyesRelative = _usingRefPlacement->eyesRelative;
	eyesRelative.set_orientation(Quat::identity);
	Transform placement = ownerPlacement.to_world(eyesRelative).to_world(_localPlacement);

	return _usingRefPlacement->vrAnchorPlacement.to_local(placement);
}

void PilgrimOverlayInfo::set_reference_placement_if_not_set()
{
#ifdef OUTPUT_OVERLAY_INFO_REFERENCE
	output(TXT("[POvIn] set_reference_placement_if_not_set (%S)"), referencePlacement.is_set()? TXT("is set") : TXT("is not set"));
#endif
	if (!referencePlacement.is_set())
	{
		set_reference_placement();
	}
}

void PilgrimOverlayInfo::set_reference_placement()
{
#ifdef OUTPUT_OVERLAY_INFO_REFERENCE
	output(TXT("[POvIn] set_reference_placement [actual]"));
#endif
	referencePlacement.set_if_not_set(ReferencePlacement());
	setup_reference_placement(owner.get(), REF_ referencePlacement.access());
}

bool PilgrimOverlayInfo::setup_reference_placement(Framework::IModulesOwner* _owner, REF_ ReferencePlacement & _refPlace)
{
	if (_owner)
	{
		if (auto* presence = _owner->get_presence())
		{
			_refPlace.ownerPlacement = presence->get_placement();
			_refPlace.eyesRelative = presence->get_eyes_relative_look();
			_refPlace.eyesRelative.set_orientation(Quat::identity);
			_refPlace.vrAnchorPlacement = presence->get_vr_anchor();

			return true;
		}
	}

	return false;
}

void PilgrimOverlayInfo::clear_reference_placement_if_possible()
{
#ifdef OUTPUT_OVERLAY_INFO_REFERENCE
	output(TXT("[POvIn] clear_reference_placement_if_possible"));
#endif
	if (referencePlacement.is_set()
	 && (currentlyShowing == None || currentlyShowing == ShowInfo || currentlyShowing == Investigate)
	//&& (showEXMs.showReasons.is_empty() && showEXMs.keepShowingReasons.is_empty()) &&
	 && (showPermanentEXMs.showReasons.is_empty() && showPermanentEXMs.keepShowingReasons.is_empty())
		)
	{
#ifdef OUTPUT_OVERLAY_INFO_REFERENCE
		output(TXT("[POvIn] clear_reference_placement"));
#endif
		referencePlacement.clear();
	}
	else
	{
#ifdef OUTPUT_OVERLAY_INFO_REFERENCE
		output(TXT("[POvIn] clear_reference_placement NOT POSSIBLE"));
#endif
	}
}

bool PilgrimOverlayInfo::is_showing_info(Name const& _id)
{
	Concurrency::ScopedSpinLock lock(showInfosLock);

	for_every(si, showInfos)
	{
		if (si->id == _id)
		{
			return true;
		}
	}
	return false;
}

void PilgrimOverlayInfo::show_info(Optional<Name> const& _id, Transform const& _at, std::function<OverlayInfo::Element* ()> _setup_element, Optional<bool> const& _allowAutoHide)
{
	show_info_ex(_id, _at, String::empty(), _setup_element, NP, NP, NP, _allowAutoHide, NP, NP);
}

void PilgrimOverlayInfo::show_info(Optional<Name> const& _id, Transform const& _at, String const& _info, Optional<OverlayInfo::TextColours> const& _colours, Optional<float> const& _size, Optional<float> const& _width, Optional<bool> const& _allowAutoHide, Optional<Vector2> const& _pt, Optional<bool> const& _allowSmallerDecimals)
{
	show_info_ex(_id, _at, _info, nullptr, _colours, _size, _width, _allowAutoHide, _pt, _allowSmallerDecimals);
}

void PilgrimOverlayInfo::show_info_ex(Optional<Name> const& _id, Transform const& _at, String const& _info, std::function<OverlayInfo::Element* ()> _setup_element, Optional<OverlayInfo::TextColours> const & _colours, Optional<float> const& _size, Optional<float> const& _width, Optional<bool> const& _allowAutoHide, Optional<Vector2> const& _pt, Optional<bool> const& _allowSmallerDecimals)
{
	ShowInfoInfo showInfo;
	showInfo.id = _id.get(Name::invalid());
	showInfo.allowAutoHide = _allowAutoHide.get(true);
	showInfo.info = _info;
	showInfo.setup_element = _setup_element;
	if (_colours.is_set())
	{
		showInfo.colours = _colours.get();
	}
	showInfo.at = _at;
	showInfo.size = _size;
	showInfo.width = _width;
	showInfo.pt = _pt;
	showInfo.allowSmallerDecimals = _allowSmallerDecimals;
	if (showInfo.allowAutoHide)
	{
		OverlayInfo::System* overlayInfoSystem = nullptr;
		if (auto* game = Game::get_as<Game>())
		{
			overlayInfoSystem = game->access_overlay_info_system();
		}
		if (overlayInfoSystem)
		{
			float dist = 0.2f;
			showInfo.hideWhenMovesTooFarFrom = Transform(overlayInfoSystem->get_camera_placement().get_translation() + overlayInfoSystem->get_camera_placement().get_axis(Axis::Forward) * dist, overlayInfoSystem->get_camera_placement().get_orientation());
			showInfo.hideWhenMovesTooFarFromDist = 0.45f;
		}
	}
	{
		Concurrency::ScopedSpinLock lock(showInfosLock);
		showInfos.push_back(showInfo);
	}
	if (requestedToShow != Main || !allowShowInfoInMain)
	{
		requestedToShow = ShowInfo;
	}
	updateShowInfos = true;
	if (!showInfo.allowAutoHide)
	{
		an_assert(showInfo.id.is_valid());
		Concurrency::ScopedSpinLock lock(keepOverlayInfoElementsLock);
		keepOverlayInfoElements.push_back_unique(showInfo.id);
	}
#ifdef OUTPUT_OVERLAY_INFO_INFO
	output(TXT("[POvIn] show info \"%S\"%S: %S"), showInfo.id.to_char(), showInfo.allowAutoHide? TXT(" [autohide]") : TXT(" [no autohide]"), _info.to_char());
#endif
}

bool PilgrimOverlayInfo::update_info(Name const& _id, String const& _info)
{
	Concurrency::ScopedSpinLock lock(showInfosLock);
	bool updated = false;
	an_assert(_id.is_valid());
	for_every(si, showInfos)
	{
		if (si->id == _id)
		{
			if (auto* oie = fast_cast<OverlayInfo::Elements::Text>(si->element.get()))
			{
				oie->with_text(_info);
				updated = true;
			}
		}
	}
	return updated;
}

void PilgrimOverlayInfo::hide_show_info(Optional<Name> const& _id)
{
#ifdef OUTPUT_OVERLAY_INFO_INFO
	output(TXT("[POvIn] hide show info \"%S\""), _id.get(Name::invalid()).to_char());
#endif
	{
		Concurrency::ScopedSpinLock lock(showInfosLock);
		OverlayInfo::System* overlayInfoSystem = nullptr;
		if (auto* game = Game::get_as<Game>())
		{
			overlayInfoSystem = game->access_overlay_info_system();
		}
		for (int i = 0; i < showInfos.get_size(); ++i)
		{
			auto& showInfo = showInfos[i];
			if ((!_id.is_set() && showInfo.allowAutoHide) || showInfo.id == _id.get())
			{
				if (overlayInfoSystem)
				{
					overlayInfoSystem->deactivate(showInfo.id);
				}
				if (showInfo.allowAutoHide)
				{
					Concurrency::ScopedSpinLock lock(keepOverlayInfoElementsLock);
					keepOverlayInfoElements.remove_fast(showInfo.id);
				}
				showInfos.remove_at(i);
				--i;
			}
		}
	}
	if (showInfos.is_empty() && currentlyShowing == ShowInfo)
	{
#ifdef OUTPUT_OVERLAY_INFO_HIDE
		if (requestedToShow != None)
		{
			output(TXT("[POvIn:hide] not showing info anymore"));
		}
#endif
		requestedToShow = None;
	}
}

void PilgrimOverlayInfo::show_permanent_exms(Name const& _reason)
{
#ifdef OUTPUT_OVERLAY_INFO
	output(TXT("[POvIn] show permanent exms \"%S\""), _reason.to_char());
#endif
	Concurrency::ScopedSpinLock lock(showPermanentEXMs.showReasonsLock);

	if ((showPermanentEXMs.showReasons.is_empty() && showPermanentEXMs.keepShowingReasons.is_empty()) || ! _reason.is_valid())
	{
#ifdef OUTPUT_OVERLAY_INFO
		output(TXT("[POvIn] do show permanent exms!"));
#endif
		{
			Concurrency::ScopedSpinLock lock(keepOverlayInfoElementsLock);
			keepOverlayInfoElements.push_back_unique(NAME(permanentEXM));
		}

		OverlayInfo::System* overlayInfoSystem = nullptr;
		if (auto* game = Game::get_as<Game>())
		{
			overlayInfoSystem = game->access_overlay_info_system();
		}

		if (overlayInfoSystem)
		{
			internal_show_permanent_exms(overlayInfoSystem);
		}
	}

	if (_reason.is_valid())
	{
		showPermanentEXMs.showReasons.push_back_unique(_reason);
		showPermanentEXMs.keepShowingReasons.remove(_reason);
	}
#ifdef OUTPUT_OVERLAY_INFO
	output_reasons(TXT("reasons"), showPermanentEXMs.showReasons);
	output_reasons(TXT("keep showing reasons"), showPermanentEXMs.keepShowingReasons);
#endif
}

void PilgrimOverlayInfo::internal_show_permanent_exms(OverlayInfo::System * overlayInfoSystem)
{
	overlayInfoSystem->deactivate(NAME(permanentEXM));

	if (is_at_boot_level(4))
	{
		showPermanentEXMs.pilgrimEXMs.clear();
		{
			auto const& psp = PlayerSetup::get_current().get_preferences();
			if (psp.useVignetteForMovement)
			{
				showPermanentEXMs.pilgrimEXMs.push_back(EXMID::Integral::anti_motion_sickness());
			}
		}
		if (auto* pa = owner.get())
		{
			if (auto* mp = pa->get_gameplay_as<ModulePilgrim>())
			{
				auto const& inventory = mp->get_pilgrim_inventory();
				Concurrency::ScopedMRSWLockRead lock(inventory.exmsLock);
				for_every(exmId, inventory.permanentEXMs)
				{
					showPermanentEXMs.pilgrimEXMs.push_back(*exmId);
				}
			}
		}

		struct EXMToShow
		{
			EXMType const* exm = nullptr;
			int count = 1;
		};
		ArrayStatic<EXMToShow, 32> exmsToShow; SET_EXTRA_DEBUG_INFO(exmsToShow, TXT("PilgrimOverlayInfo::internal_show_permanent_exms.exmsToShow"));
		for_every(exmId, showPermanentEXMs.pilgrimEXMs)
		{
			if (auto* exm = EXMType::find(*exmId))
			{
				bool existing = false;
				for_every(e, exmsToShow)
				{
					if (e->exm == exm)
					{
						++e->count;
						existing = true;
						break;
					}
				}
				if (!existing)
				{
					EXMToShow e2s;
					e2s.exm = exm;
					exmsToShow.push_back(e2s);
				}
			}
		}
		int count = exmsToShow.get_size();
		showPermanentEXMs.exms.set_size(count);

		::SafePtr<PilgrimOverlayInfo> safeThis(this);

		for_count(int, i, count)
		{
			float rescale = 1.7f; // this way it matches pilgrim stats
			//float size = 0.025f;
			float letterSize = 0.006f;
			float infoWidth = 0.15f; // this is calculated to fit
			int infoCharWidth = TypeConversions::Normal::f_i_closest(infoWidth / letterSize);

			float rel = (float)i - (float)(count - 1) / 2.0f;
			Vector3 loc = Vector3(0.35f, 0.2f, 0.0f).normal() * statsDist * 1.1f + Vector3::zAxis * (-0.02f + (-0.055f * rel) * rescale);
			Transform placement = get_vr_placement_for_local(owner.get(), matrix_from_up_forward(loc, Vector3(0.0f, 0.0f, 1.0f), Vector3(0.8f, 0.5f, 0.0f)).to_transform());
		
			Transform infoOffset = matrix_from_up_forward(Vector3::xAxis * hardcoded 0.032f * rescale + Vector3::zAxis * hardcoded 0.023f * rescale, Vector3::zAxis, Vector3::yAxis).to_transform();

			auto& showEXM = showPermanentEXMs.exms[i];
			showEXM.currentEXMType = exmsToShow[i].exm;
			showEXM.count = exmsToShow[i].count;

			showEXM.visible.element = (new OverlayInfo::Elements::CustomDraw())
				->with_draw([safeThis, i](float _active, float _pulse, Colour const& _colour) { draw_permanent_exm_for_overlay(i, safeThis.get(), _active, _pulse, _colour); })
				->with_size(rescale, false)
				->with_id(NAME(permanentEXM))
				->with_usage(OverlayInfo::Usage::World)
				;
			setup_overlay_element_placement(showEXM.visible.element.get(), placement);
			showEXM.info.element = (new OverlayInfo::Elements::Text())
				->with_letter_size(letterSize * rescale)
				->with_line_spacing(0.1f)
				->with_horizontal_align(0.0f)
				->with_vertical_align(1.0f)
				->with_max_line_width(infoCharWidth)
				->with_id(NAME(permanentEXM))
				->with_usage(OverlayInfo::Usage::World)
				;
			setup_overlay_element_placement(showEXM.info.element.get(), placement.to_world(infoOffset));

			showEXM.info.value = Name::invalid();

			overlayInfoSystem->add(showEXM.visible.element.get());
			overlayInfoSystem->add(showEXM.info.element.get());
		}
	}
}

void PilgrimOverlayInfo::hide_permanent_exms(Name const& _reason)
{
	internal_hide_permanent_exms(_reason);

	clear_reference_placement_if_possible();
}

void PilgrimOverlayInfo::internal_hide_permanent_exms(Name const & _reason)
{
#ifdef OUTPUT_OVERLAY_INFO
	output(TXT("[POvIn] hide permanent  exms \"%S\""), _reason.to_char());
#endif
	Concurrency::ScopedSpinLock lock(showPermanentEXMs.showReasonsLock);

	showPermanentEXMs.showReasons.remove(_reason);
	showPermanentEXMs.keepShowingReasons.remove(_reason);

	if (showPermanentEXMs.showReasons.is_empty())
	{
		showPermanentEXMs.keepShowingReasons.clear();
	}
#ifdef OUTPUT_OVERLAY_INFO
	output_reasons(TXT("reasons"), showPermanentEXMs.showReasons);
	output_reasons(TXT("keep showing reasons"), showPermanentEXMs.keepShowingReasons);
#endif
	if (showPermanentEXMs.showReasons.is_empty() && showPermanentEXMs.keepShowingReasons.is_empty())
	{
#ifdef OUTPUT_OVERLAY_INFO
		output(TXT("[POvIn] do hide permanent exms!"));
#endif
		{
			Concurrency::ScopedSpinLock lock(keepOverlayInfoElementsLock);
			keepOverlayInfoElements.remove_fast(NAME(permanentEXM));
		}

		OverlayInfo::System* overlayInfoSystem = nullptr;
		if (auto* game = Game::get_as<Game>())
		{
			overlayInfoSystem = game->access_overlay_info_system();
		}

		if (overlayInfoSystem)
		{
			overlayInfoSystem->deactivate(NAME(permanentEXM));
		}

		showPermanentEXMs.exms.clear();
	}
}

void PilgrimOverlayInfo::keep_showing_permanent_exms(Name const & _reason)
{
#ifdef OUTPUT_OVERLAY_INFO
	output(TXT("[POvIn] keep showing permanent exms reason \"%S\""), _reason.to_char());
#endif

	Concurrency::ScopedSpinLock lock(showPermanentEXMs.showReasonsLock);

	showPermanentEXMs.showReasons.remove(_reason);
	showPermanentEXMs.keepShowingReasons.push_back_unique(_reason);

#ifdef OUTPUT_OVERLAY_INFO
	output_reasons(TXT("reasons"), showPermanentEXMs.showReasons);
	output_reasons(TXT("keep showing reasons"), showPermanentEXMs.keepShowingReasons);
#endif
}

void PilgrimOverlayInfo::draw_permanent_exm_for_overlay(int _idx, PilgrimOverlayInfo* _poi, float _active, float _pulse, Colour const& _colour)
{
	if (!_poi ||
		!_poi->showPermanentEXMs.exms.is_index_valid(_idx))
	{
		return;
	}

	ShowPermanentEXMs::ShowEXM& showEXM = _poi->showPermanentEXMs.exms[_idx];

	Name shouldShowEXM = showEXM.currentEXMType ? showEXM.currentEXMType->get_id() : Name::invalid();

	if (showEXM.visible.value != shouldShowEXM ||
		showEXM.visibleCount != showEXM.count)
	{
		showEXM.visible.value = shouldShowEXM;
		showEXM.visibleCount = showEXM.count;
		showEXM.rd.mark_new_data_required();
	}
	if (showEXM.info.value != shouldShowEXM)
	{
		showEXM.info.value = shouldShowEXM;
		String desc = showEXM.currentEXMType ? showEXM.currentEXMType->get_ui_desc() : String::empty();
		if (showEXM.count > 1)
		{
			if (!desc.is_empty()) desc += String(TXT("~"));
			desc += String::printf(TXT("x%i"), showEXM.count);
		}
		update_stat_overlay_element(showEXM.info.element, desc);
	}

	if (showEXM.rd.should_do_new_data_to_render())
	{
		showEXM.rd.clear();

		float size = hardcoded 0.045f;

		Matrix44 offset = matrix_from_up_forward(Vector3::zero, -Vector3::yAxis, Vector3::zAxis);

		{
			float useSize = size;
			auto* lm = _poi->oiPermanentEXM.get();
			bool allowP1 = true;
			if (showEXM.currentEXMType &&
				showEXM.currentEXMType->is_integral())
			{
				lm = _poi->oiIntegralEXM.get();
				allowP1 = false;
			}
			if (lm)
			{
				for_every(l, lm->get_lines())
				{
					Colour c = l->colour.get(Colour::white);
					Vector3 a = offset.location_to_world(l->a * useSize);
					Vector3 b = offset.location_to_world(l->b * useSize);

					showEXM.rd.add_line(a, b, c);
				}

				if (allowP1)
				{
					if (auto* lm = _poi->oiPermanentEXMp1.get())
					{
						Vector3 addOffset = -Vector3::xAxis * 0.2f;

						for_count(int, i, showEXM.count - 1)
						{
							Vector3 addOffsetNow = (float)(i + 1) * addOffset;
							for_every(l, lm->get_lines())
							{
								Colour c = l->colour.get(Colour::white);
								Vector3 a = offset.location_to_world((l->a + addOffsetNow) * useSize);
								Vector3 b = offset.location_to_world((l->b + addOffsetNow) * useSize);

								showEXM.rd.add_line(a, b, c);
							}
						}
					}
				}

				useSize *= 0.8f;
			}

			if (showEXM.currentEXMType)
			{
				if (auto* lm = showEXM.currentEXMType->get_line_model())
				{
					for_every(l, lm->get_lines())
					{
						Colour c = l->colour.get(Colour::white);
						Vector3 a = offset.location_to_world(l->a * useSize);
						Vector3 b = offset.location_to_world(l->b * useSize);

						showEXM.rd.add_line(a, b, c);
					}
				}
			}
		}

		showEXM.rd.mark_new_data_done();
		showEXM.rd.mark_data_available_to_render();
	}

	showEXM.rd.render(_active, _pulse, _colour, NP, _poi->overlayShaderInstance);
}

void PilgrimOverlayInfo::update_exm_infos(Framework::IModulesOwner* _owner)
{
	// normal
	{
		bool showDesc = showEXMs.showActiveReasons.get_size() < (showEXMs.showReasons.get_size() + showEXMs.keepShowingReasons.get_size());

		for_count(int, iHand, Hand::MAX)
		{
			auto& showHand = showEXMs.hands[iHand];
			if (showHand.activeEXM.info.element.is_set())
			{
				IF_DIFFER_SET(showHand.activeEXM.info.value, showDesc && showHand.activeEXM.currentEXMType? showHand.activeEXM.visible.value : Name::invalid())
				{
#ifdef OUTPUT_OVERLAY_INFO
					output(TXT("[POvIn] update active at %S to %S"), Hand::to_char((Hand::Type)iHand), showHand.activeEXM.info.value.to_char());
					output(TXT("[POvIn] %S showDesc"), showDesc? TXT("Y") : TXT("n"));
					output(TXT("[POvIn] %S showHand.activeEXM.currentEXMType"), showHand.activeEXM.currentEXMType ? TXT("Y") : TXT("n"));
					output_reasons(TXT("reasons"), showEXMs.showReasons);
					output_reasons(TXT("active reasons"), showEXMs.showActiveReasons);
					output_reasons(TXT("keep showing reasons"), showEXMs.keepShowingReasons);
#endif
					String desc = showDesc && showHand.activeEXM.currentEXMType ? showHand.activeEXM.currentEXMType->get_ui_desc() : String::empty();
					update_stat_overlay_element(showHand.activeEXM.info.element, desc);
					update_stat_overlay_element(showHand.activeEXM.infoReverse.element, desc);
				}
			}
			for_every(passiveEXM, showHand.passiveEXMs)
			{
				if (passiveEXM->info.element.is_set())
				{
					IF_DIFFER_SET(passiveEXM->info.value, showDesc && passiveEXM->currentEXMType ? passiveEXM->visible.value : Name::invalid())
					{
#ifdef OUTPUT_OVERLAY_INFO
						output(TXT("[POvIn] update passive at %S to %S"), Hand::to_char((Hand::Type)iHand), passiveEXM->info.value.to_char());
						output(TXT("[POvIn] %S showDesc"), showDesc ? TXT("Y") : TXT("n"));
						output(TXT("[POvIn] %S passiveEXM->currentEXMType"), passiveEXM->currentEXMType ? TXT("Y") : TXT("n"));
						output_reasons(TXT("reasons"), showEXMs.showReasons);
						output_reasons(TXT("active reasons"), showEXMs.showActiveReasons);
						output_reasons(TXT("keep showing reasons"), showEXMs.keepShowingReasons);
#endif
						String desc = showDesc && passiveEXM->currentEXMType ? passiveEXM->currentEXMType->get_ui_desc() : String::empty();
						update_stat_overlay_element(passiveEXM->info.element, desc);
						update_stat_overlay_element(passiveEXM->infoReverse.element, desc);
					}
				}
			}
		}
	}

	// permanent
	{
		Concurrency::ScopedSpinLock lock(showPermanentEXMs.showReasonsLock);

		if (! showPermanentEXMs.showReasons.is_empty() || ! showPermanentEXMs.keepShowingReasons.is_empty())
		{
			ArrayStatic<Name, 32> exmsToShow; SET_EXTRA_DEBUG_INFO(exmsToShow, TXT("PilgrimOverlayInfo::update_exm_infos.exmsToShow"));
			{
				auto const& psp = PlayerSetup::get_current().get_preferences();
				if (psp.useVignetteForMovement)
				{
					exmsToShow.push_back(EXMID::Integral::anti_motion_sickness());
				}
			}
			if (auto* pa = owner.get())
			{
				if (auto* mp = pa->get_gameplay_as<ModulePilgrim>())
				{
					auto const& inventory = mp->get_pilgrim_inventory();
					Concurrency::ScopedMRSWLockRead lock(inventory.exmsLock);
					for_every(exmId, inventory.permanentEXMs)
					{
						exmsToShow.push_back(*exmId);
					}
				}
			}

			bool updateRequired = false;
			if (exmsToShow.get_size() != showPermanentEXMs.pilgrimEXMs.get_size())
			{
				updateRequired = true;
			}
			else
			{
				for_every(exm, exmsToShow)
				{
					if (showPermanentEXMs.pilgrimEXMs[for_everys_index(exm)] != *exm)
					{
						updateRequired = true;
						break;
					}
				}
			}

			if (updateRequired)
			{
				OverlayInfo::System* overlayInfoSystem = nullptr;
				if (auto* game = Game::get_as<Game>())
				{
					overlayInfoSystem = game->access_overlay_info_system();
				}

				if (overlayInfoSystem)
				{
					internal_show_permanent_exms(overlayInfoSystem);
				}
			}
		}
	}
}

void PilgrimOverlayInfo::show_exms(Name const& _reason, bool _onlyActive)
{
#ifdef OUTPUT_OVERLAY_INFO
	output(TXT("[POvIn] show exms \"%S\" %S"), _reason.to_char(), _onlyActive? TXT("only active") : TXT(""));
#endif
	Concurrency::ScopedSpinLock lock(showEXMs.showReasonsLock);

	if ((showEXMs.showReasons.is_empty() && showEXMs.keepShowingReasons.is_empty()) || ! _reason.is_valid())
	{
#ifdef OUTPUT_OVERLAY_INFO
		output(TXT("[POvIn] do show exms!"));
#endif
		{
			Concurrency::ScopedSpinLock lock(keepOverlayInfoElementsLock);
			keepOverlayInfoElements.push_back_unique(NAME(exm));
		}

		OverlayInfo::System* overlayInfoSystem = nullptr;
		if (auto* game = Game::get_as<Game>())
		{
			overlayInfoSystem = game->access_overlay_info_system();
		}


		if (overlayInfoSystem)
		{
			overlayInfoSystem->deactivate(NAME(exm));

			if (is_at_boot_level(5))
			{
				::SafePtr<PilgrimOverlayInfo> safeThis(this);

				for_count(int, iHand, Hand::MAX)
				{
					Hand::Type hand = (Hand::Type)iHand;
					Transform infoOffset = look_matrix(Vector3::xAxis * hardcoded 0.032f + Vector3::yAxis * hardcoded 0.023f, -Vector3::zAxis, Vector3::yAxis).to_transform();
					Transform otherSide = Transform(Vector3::zero, Rotator3(0.0f, 0.0f, 180.0f).to_quat());
					float letterSize = 0.006f;
					float infoWidth = 0.22f;
					int infoCharWidth = TypeConversions::Normal::f_i_closest(infoWidth / letterSize);
					auto& showHand = showEXMs.hands[iHand];

					showHand.passiveEXMs.clear();
					while (showHand.passiveEXMs.has_place_left())
					{
						int passiveEXMIdx = showHand.passiveEXMs.get_size();
						showHand.passiveEXMs.grow_size(1);
						auto& passiveEXM = showHand.passiveEXMs.get_last();
						passiveEXM.proposalEXM = Name::invalid();
						passiveEXM.currentEXMType = nullptr;
						passiveEXM.proposalEXMType = nullptr;

						for_count(int, side, 2)
						{
							auto& visible = side == 0 ? passiveEXM.visible : passiveEXM.visibleReverse;
							auto& info = side == 0 ? passiveEXM.info : passiveEXM.infoReverse;
							Transform sideT = side == 0 ? Transform::identity : otherSide;
							Transform useInfoOffset = sideT.to_world(infoOffset);
							visible.element = (new OverlayInfo::Elements::CustomDraw())
								->one_sided()
								->with_draw([hand, safeThis, passiveEXMIdx](float _active, float _pulse, Colour const& _colour) { draw_exm_for_overlay(hand, passiveEXMIdx, safeThis.get(), _active, _pulse, _colour); })
								->with_size(1.0f, false)
								->with_id(NAME(exm))
								->with_usage(OverlayInfo::Usage::World)
								->with_custom_vr_placement([hand, safeThis, passiveEXMIdx, sideT](OverlayInfo::System const* _system, OverlayInfo::Element const* _element) { return calculate_exm_placement_for_overlay(hand, passiveEXMIdx, safeThis.get(), sideT); });
							info.element = (new OverlayInfo::Elements::Text())
								->with_letter_size(letterSize)
								->with_line_spacing(0.1f)
								->with_horizontal_align(0.0f)
								->with_vertical_align(1.0f)
								->with_max_line_width(infoCharWidth)
								->with_id(NAME(exm))
								->with_usage(OverlayInfo::Usage::World)
								->with_custom_vr_placement([hand, safeThis, passiveEXMIdx, useInfoOffset](OverlayInfo::System const* _system, OverlayInfo::Element const* _element) { return calculate_exm_placement_for_overlay(hand, passiveEXMIdx, safeThis.get(), useInfoOffset); });
							info.value = Name::invalid();

							overlayInfoSystem->add(visible.element.get());
							overlayInfoSystem->add(info.element.get());
						}

						passiveEXM.rd.mark_new_data_required();
					}

					{
						for_count(int, side, 2)
						{
							auto& visible = side == 0 ? showHand.activeEXM.visible : showHand.activeEXM.visibleReverse;
							auto& info = side == 0 ? showHand.activeEXM.info : showHand.activeEXM.infoReverse;
							Transform sideT = side == 0 ? Transform::identity : otherSide;
							Transform useInfoOffset = sideT.to_world(infoOffset);
							visible.element = (new OverlayInfo::Elements::CustomDraw())
								->one_sided()
								->with_draw([hand, safeThis](float _active, float _pulse, Colour const& _colour) { draw_exm_for_overlay(hand, NP, safeThis.get(), _active, _pulse, _colour); })
								->with_size(1.0f, false)
								->with_id(NAME(exm))
								->with_usage(OverlayInfo::Usage::World)
								->with_custom_vr_placement([hand, safeThis, sideT](OverlayInfo::System const* _system, OverlayInfo::Element const* _element) { return calculate_exm_placement_for_overlay(hand, NP, safeThis.get(), sideT); });
							info.element = (new OverlayInfo::Elements::Text())
								->with_letter_size(letterSize)
								->with_line_spacing(0.1f)
								->with_horizontal_align(0.0f)
								->with_vertical_align(1.0f)
								->with_max_line_width(infoCharWidth)
								->with_id(NAME(exm))
								->with_usage(OverlayInfo::Usage::World)
								->with_custom_vr_placement([hand, safeThis, useInfoOffset](OverlayInfo::System const* _system, OverlayInfo::Element const* _element) { return calculate_exm_placement_for_overlay(hand, NP, safeThis.get(), useInfoOffset); });
							info.value = Name::invalid();

							overlayInfoSystem->add(visible.element.get());
							overlayInfoSystem->add(info.element.get());
						}

						showHand.activeEXM.rd.mark_new_data_required();
					}
				}
			}
		}
	}

	if (_reason.is_valid())
	{
		showEXMs.showReasons.push_back_unique(_reason);
		showEXMs.keepShowingReasons.remove(_reason);
		if (_onlyActive)
		{
			showEXMs.showActiveReasons.push_back_unique(_reason);
		}
	}
#ifdef OUTPUT_OVERLAY_INFO
	output_reasons(TXT("reasons"), showEXMs.showReasons);
	output_reasons(TXT("active reasons"), showEXMs.showActiveReasons);
	output_reasons(TXT("keep showing reasons"), showEXMs.keepShowingReasons);
#endif
}

void PilgrimOverlayInfo::hide_exms(Name const& _reason)
{
	internal_hide_exms(_reason);

	clear_reference_placement_if_possible();
}

void PilgrimOverlayInfo::internal_hide_exms(Name const & _reason)
{
#ifdef OUTPUT_OVERLAY_INFO
	output(TXT("[POvIn] hide exms \"%S\""), _reason.to_char());
#endif
	Concurrency::ScopedSpinLock lock(showEXMs.showReasonsLock);

	showEXMs.showReasons.remove(_reason);
	showEXMs.showActiveReasons.remove(_reason);
	showEXMs.keepShowingReasons.remove(_reason);

	if (showEXMs.showReasons.is_empty())
	{
		showEXMs.keepShowingReasons.clear();
	}
#ifdef OUTPUT_OVERLAY_INFO
	output_reasons(TXT("reasons"), showEXMs.showReasons);
	output_reasons(TXT("active reasons"), showEXMs.showActiveReasons);
	output_reasons(TXT("keep showing reasons"), showEXMs.keepShowingReasons);
#endif
	if (showEXMs.showReasons.is_empty() && showEXMs.keepShowingReasons.is_empty())
	{
#ifdef OUTPUT_OVERLAY_INFO
		output(TXT("[POvIn] do hide exms!"));
#endif
		{
			Concurrency::ScopedSpinLock lock(keepOverlayInfoElementsLock);
			keepOverlayInfoElements.remove_fast(NAME(exm));
		}

		OverlayInfo::System* overlayInfoSystem = nullptr;
		if (auto* game = Game::get_as<Game>())
		{
			overlayInfoSystem = game->access_overlay_info_system();
		}

		if (overlayInfoSystem)
		{
			overlayInfoSystem->deactivate(NAME(exm));
		}

		for_count(int, iHand, Hand::MAX)
		{
			auto& showHand = showEXMs.hands[iHand];

			showHand.passiveEXMs.clear();
			showHand.activeEXM.visible.element.clear();
			showHand.activeEXM.visibleReverse.element.clear();
			showHand.activeEXM.info.element.clear();
			showHand.activeEXM.infoReverse.element.clear();
		}
	}
}

void PilgrimOverlayInfo::keep_showing_exms(Name const & _reason)
{
#ifdef OUTPUT_OVERLAY_INFO
	output(TXT("[POvIn] keep showing exms reason \"%S\""), _reason.to_char());
#endif

	Concurrency::ScopedSpinLock lock(showEXMs.showReasonsLock);

	showEXMs.showReasons.remove(_reason);
	showEXMs.keepShowingReasons.push_back_unique(_reason);

#ifdef OUTPUT_OVERLAY_INFO
	output_reasons(TXT("reasons"), showEXMs.showReasons);
	output_reasons(TXT("active reasons"), showEXMs.showActiveReasons);
	output_reasons(TXT("keep showing reasons"), showEXMs.keepShowingReasons);

#endif
}

void PilgrimOverlayInfo::show_exm_proposal(Hand::Type _hand, bool _passiveEXM, Name const& _exm)
{
	ShowEXMs::ShowHand& showHand = showEXMs.hands[_hand];

	if (_passiveEXM && showHand.passiveEXMs.is_empty())
	{
		return;
	}

	ShowEXMs::ShowEXM& showEXM = _passiveEXM ? showHand.passiveEXMs[0] : showHand.activeEXM;

	showEXM.proposalEXM = _exm;
}

void PilgrimOverlayInfo::draw_exm_for_overlay(Hand::Type _hand, Optional<int> const& _passiveEXMIdx, PilgrimOverlayInfo* _poi, float _active, float _pulse, Colour const& _colour)
{
	if (!_poi)
	{
		return;
	}

	bool onlyActive = _poi->showEXMs.showActiveReasons.get_size() == (_poi->showEXMs.showReasons.get_size() + _poi->showEXMs.keepShowingReasons.get_size()); // this way if we deactivate, nothing will appear out of nowhere
	bool currentIsActive = false;
	Optional<float> cooldownLeftPt;

	ShowEXMs::ShowHand& showHand = _poi->showEXMs.hands[_hand];

	if (_passiveEXMIdx.is_set() &&
		!showHand.passiveEXMs.is_index_valid(_passiveEXMIdx.is_set()))
	{
		return;
	}
	
	ShowEXMs::ShowEXM& showEXM = (_passiveEXMIdx.is_set() && showHand.passiveEXMs.is_index_valid(_passiveEXMIdx.get())) ? showHand.passiveEXMs[_passiveEXMIdx.get()] : showHand.activeEXM;
	ShowEXMs::ShowEXM& showEXMForProposalSource = (_passiveEXMIdx.is_set() && ! showHand.passiveEXMs.is_empty()) ? showHand.passiveEXMs[0] : showHand.activeEXM;
	bool mainProposal = ! _passiveEXMIdx.is_set() || _passiveEXMIdx.get() == 0;

	if (auto* pa = _poi->owner.get())
	{
		if (auto* mp = pa->get_gameplay_as<ModulePilgrim>())
		{
			if (!mp->get_hand(_hand))
			{
				// no hand, don't draw
				return;
			}
			auto const& pilgrimHand = mp->get_pilgrim_setup().get_hand(_hand);
			if (_passiveEXMIdx.is_set() &&
				!pilgrimHand.passiveEXMs.is_index_valid(_passiveEXMIdx.get()))
			{
				if (showEXM.info.element.is_set())
				{
					if (auto* oie = fast_cast<OverlayInfo::Elements::Text>(showEXM.info.element.get()))
					{
						oie->with_text(String::empty());
					}
					if (auto* oie = fast_cast<OverlayInfo::Elements::Text>(showEXM.infoReverse.element.get()))
					{
						oie->with_text(String::empty());
					}
				}
				showEXM.rd.mark_new_data_required();
				showEXM.rd.clear();
				return; // just don't render
			}

			auto const& pilgrimEXM = _passiveEXMIdx.is_set() ? pilgrimHand.passiveEXMs[_passiveEXMIdx.get()] : pilgrimHand.activeEXM;

			showEXM.visible.value = pilgrimEXM.exm;

			if (!_passiveEXMIdx.is_set())
			{
				currentIsActive = mp->does_equipped_exm_appear_active(_hand, &cooldownLeftPt);
			}
		}
	}

	if (onlyActive && !currentIsActive && !cooldownLeftPt.is_set())
	{
		showEXM.rd.mark_new_data_required();
		showEXM.rd.clear(); // no need do draw it anymore
		return;
	}

	{
		bool newDataRequired = false;
		newDataRequired |= update_exm(showEXM.visible.value, REF_ showEXM.currentEXMType);
		newDataRequired |= update_exm(showEXMForProposalSource.proposalEXM, REF_ showEXM.proposalEXMType);
		if (newDataRequired || showEXM.rd.is_empty())
		{
			showEXM.rd.mark_new_data_required();
		}
	}

	float renderSize = hardcoded 0.045f;

	if (showEXM.rd.should_do_new_data_to_render())
	{
		showEXM.rd.clear();

		{
			float useSize = renderSize;
			if (auto* lm = _passiveEXMIdx.is_set() ? _poi->oiPassiveEXM.get() : _poi->oiActiveEXM.get())
			{
				for_every(l, lm->get_lines())
				{
					Colour c = l->colour.get(Colour::white);
					Vector3 a = l->a * useSize;
					Vector3 b = l->b * useSize;

					showEXM.rd.add_line(a, b, c);
				}
				
				useSize *= 0.8f;
			}

			if (showEXM.currentEXMType)
			{
				if (auto* lm = showEXM.currentEXMType->get_line_model())
				{
					for_every(l, lm->get_lines())
					{
						Colour c = l->colour.get(Colour::white);
						Vector3 a = l->a * useSize;
						Vector3 b = l->b * useSize;

						showEXM.rd.add_line(a, b, c);
					}
				}
			}
		}

		if (showEXM.proposalEXMType)
		{
			//Vector3 offset = Vector3::zAxis * 0.05f;
			Vector3 offset = -Vector3::xAxis * 0.1f;

			if (mainProposal)
			{
				if (auto* lm = _poi->oiInstallEXM.get())
				{
					float useSize = renderSize * 0.8f;
					Vector3 useOffset = offset * 0.5f;
					Colour colour = showEXM.currentEXMType ? _poi->colourReplaceEXM : _poi->colourInstallEXM;
					for_every(l, lm->get_lines())
					{
						Colour c = l->colour.get(colour);
						Vector3 a = l->a * useSize + useOffset;
						Vector3 b = l->b * useSize + useOffset;

						showEXM.rd.add_line(a, b, c);
					}
				}
			}
			else
			{
				if (auto* lm = _poi->oiMoveEXMThroughStack.get())
				{
					float useSize = renderSize * 0.8f;
					Vector3 useOffset = Vector3::yAxis * /* exm offset */ EXM_OFFSET * /* half */ 0.5f;
					Colour colour = showEXM.currentEXMType ? _poi->colourReplaceEXM : _poi->colourInstallEXM;
					for_every(l, lm->get_lines())
					{
						Colour c = l->colour.get(colour);
						Vector3 a = l->a * useSize + useOffset;
						Vector3 b = l->b * useSize + useOffset;

						showEXM.rd.add_line(a, b, c);
					}
				}
			}

			if (mainProposal)
			{
				float useSize = renderSize;

				if (auto* lm = showEXM.proposalEXMType->is_active() ? _poi->oiActiveEXM.get() : _poi->oiPassiveEXM.get())
				{
					for_every(l, lm->get_lines())
					{
						Colour c = l->colour.get(Colour::white);
						Vector3 a = l->a * useSize + offset;
						Vector3 b = l->b * useSize + offset;

						showEXM.rd.add_line(a, b, c);
					}

					useSize *= 0.8f;

				}
				if (auto* lm = showEXM.proposalEXMType->get_line_model())
				{
					for_every(l, lm->get_lines())
					{
						Colour c = l->colour.get(Colour::white);
						Vector3 a = l->a * useSize + offset;
						Vector3 b = l->b * useSize + offset;

						showEXM.rd.add_line(a, b, c);
					}
				}
			}
		}

		showEXM.rd.mark_new_data_done();
		showEXM.rd.mark_data_available_to_render();
	}

	Colour useColour = _colour;
	Colour overrideColour = Colour::none;
	if (currentIsActive)
	{
		overrideColour = _poi->colourActiveEXM;
	}

	if (onlyActive && !currentIsActive)
	{
		// this may happen only if cooldown is active
		// with useColour being none, only cooldown part will be displayed
		useColour = Colour::none;
	}

	Optional<Colour> cooldownColour;
	Optional<float> cooldownY;
	if (cooldownLeftPt.is_set())
	{
		cooldownColour = _poi->colourActiveEXMCooldown;
		// we assume EXM line models have y in range of [-0.5, 0.5]
		cooldownY = renderSize * (-0.5f + cooldownLeftPt.get());
	}

	showEXM.rd.render(_active, _pulse, useColour, overrideColour, _poi->overlayShaderInstance, cooldownColour, cooldownY);
}

Transform PilgrimOverlayInfo::calculate_exm_placement_for_overlay(Hand::Type _hand, Optional<int> const& _passiveEXMIdx, PilgrimOverlayInfo* _poi, Transform const & _offset)
{
	if (_poi)
	{
		if (auto* pa = _poi->owner.get())
		{
			if (auto* mp = pa->get_gameplay_as<ModulePilgrim>())
			{
				if (auto* h = mp->get_hand(_hand))
				{
					if (auto* ap = h->get_appearance())
					{
						Transform atOS = ap->calculate_socket_os(_passiveEXMIdx.is_set() ? NAME(overlayEXMPassive) : NAME(overlayEXMActive));
						Transform atWS = h->get_presence()->get_placement().to_world(atOS);
						Transform atVR = h->get_presence()->get_vr_anchor().to_local(atWS);
						Transform idxOffset = Transform(-Vector3::yAxis * EXM_OFFSET * (float)_passiveEXMIdx.get(0), Quat::identity);
						return atVR.to_world(idxOffset).to_world(_offset);
					}
				}
			}
		}
	}
	return _offset;
}

Transform PilgrimOverlayInfo::calculate_equipment_placement_for_overlay(Hand::Type _hand, PilgrimOverlayInfo* _poi, Transform const & _offset)
{
	if (_poi)
	{
		if (auto* pa = _poi->owner.get())
		{
			if (auto* mp = pa->get_gameplay_as<ModulePilgrim>())
			{
				if (auto* h = mp->get_hand(_hand))
				{
					if (auto* ap = h->get_appearance())
					{
						Transform atOS = ap->calculate_socket_os(NAME(overlayEquipment));
						Transform atWS = h->get_presence()->get_placement().to_world(atOS);
						Transform atVR = h->get_presence()->get_vr_anchor().to_local(atWS);
						return atVR.to_world(_offset);
					}
				}
			}
		}
	}
	return _offset;
}

//

PilgrimOverlayInfoRenderableData::PilgrimOverlayInfoRenderableData()
{
	vertexFormat.with_location_3d().with_colour_rgba().calculate_stride_and_offsets();
}

void PilgrimOverlayInfoRenderableData::mark_new_data_required()
{
	newRequired.increase();
}

void PilgrimOverlayInfoRenderableData::mark_new_data_done()
{
	newRequired = 0;
}

void PilgrimOverlayInfoRenderableData::mark_data_available_to_render()
{
	dataAvailableToRender.increase();
}

bool PilgrimOverlayInfoRenderableData::should_do_new_data_to_render()
{
	if (newRequired.get())
	{
		bool doNow = false;
		{
			Concurrency::ScopedSpinLock lock(dataFlagsLock, true);
			dataAvailableToRender = 0;
			doNow = !dataBeingRendered;
		}
		return doNow;
	}
	return false;
}

void PilgrimOverlayInfoRenderableData::clear()
{
	linesData.clear();
	trianglesData.clear();
}

void PilgrimOverlayInfoRenderableData::add_line(Vector3 const& _a, Vector3 const& _b, Colour const& _colour)
{
	int stride = vertexFormat.get_stride();
	int atLDIdx = linesData.get_size();
	linesData.grow_size(2 * stride);

	int locationOffset = vertexFormat.get_location_offset();
	int colourOffset = vertexFormat.get_colour_offset();

	byte* atPtr = &linesData[atLDIdx];
	*(Vector3*)(atPtr + locationOffset) = _a;
	*(Vector3*)(atPtr + locationOffset + stride) = _b;
	*(Colour*)(atPtr + colourOffset) = _colour;
	*(Colour*)(atPtr + colourOffset + stride) = _colour;
}

void PilgrimOverlayInfoRenderableData::add_line_clipped(Vector3 const& _a, Vector3 const& _b, Range3 const & _to, Colour const& _colour)
{
	Segment s(_a, _b);
	if (s.get_inside(_to))
	{
		add_line(s.get_at_start_t(), s.get_at_end_t(), _colour);
	}
}

void PilgrimOverlayInfoRenderableData::add_triangle(Vector3 const& _a, Vector3 const& _b, Vector3 const& _c, Colour const& _colour)
{
	int stride = vertexFormat.get_stride();
	int atTDIdx = trianglesData.get_size();
	trianglesData.grow_size(3 * stride);

	int locationOffset = vertexFormat.get_location_offset();
	int colourOffset = vertexFormat.get_colour_offset();

	byte* atPtr = &trianglesData[atTDIdx];
	*(Vector3*)(atPtr + locationOffset) = _a;
	*(Vector3*)(atPtr + locationOffset + stride) = _b;
	*(Vector3*)(atPtr + locationOffset + stride * 2) = _c;
	*(Colour*)(atPtr + colourOffset) = _colour;
	*(Colour*)(atPtr + colourOffset + stride) = _colour;
	*(Colour*)(atPtr + colourOffset + stride * 2) = _colour;
}

void PilgrimOverlayInfoRenderableData::render(float _active, float _pulse, Colour const& _colour, Optional<Colour> const & _overlayOverrideColour, ::System::ShaderProgramInstance* _withShaderProgramInstance,
	Optional<Colour> const & _cooldownColour, Optional<float> const& _cooldownY)
{
	{
		Concurrency::ScopedSpinLock lock(dataFlagsLock);
		if (dataAvailableToRender.get())
		{
			dataBeingRendered = true;
		}
	}
	if (dataBeingRendered)
	{
		if (!linesData.is_empty() ||
			!trianglesData.is_empty())
		{
			assert_rendering_thread();
			::System::Video3D* v3d = ::System::Video3D::get();
			auto& mvms = v3d->access_model_view_matrix_stack();
			mvms.push_to_world(scale_matrix(Vector3(scale)));
			v3d->mark_enable_blend();
			v3d->ready_for_rendering();
			v3d->set_fallback_colour(_colour);
			if (_withShaderProgramInstance)
			{
				Colour useOverlayColour = _colour.with_alpha(_active).mul_rgb(_pulse);
				Colour overlayOverrideColour = _overlayOverrideColour.get(Colour::none);
				_withShaderProgramInstance->set_uniform(NAME(overlayColour), useOverlayColour.to_vector4());
				_withShaderProgramInstance->set_uniform(NAME(overlayOverrideColour), overlayOverrideColour.to_vector4());
				_withShaderProgramInstance->set_uniform(NAME(cooldownColour), _cooldownColour.get(Colour::none).to_vector4());
				_withShaderProgramInstance->set_uniform(NAME(cooldownY), _cooldownY.get(0.0f));
			}
			if (!linesData.is_empty())
			{
				int lineCount = (linesData.get_data_size() / vertexFormat.get_stride()) / 2;
				Meshes::Mesh3D::render_data(v3d, _withShaderProgramInstance, nullptr,
					Meshes::Mesh3DRenderingContext().with_existing_face_display().with_existing_blend(),
					linesData.get_data(), ::Meshes::Primitive::Line,
					lineCount,
					vertexFormat);
			}
			if (!trianglesData.is_empty())
			{
				int triangleCount = (trianglesData.get_data_size() / vertexFormat.get_stride()) / 3;
				Meshes::Mesh3D::render_data(v3d, _withShaderProgramInstance, nullptr,
					Meshes::Mesh3DRenderingContext().with_existing_face_display().with_existing_blend(),
					trianglesData.get_data(), ::Meshes::Primitive::Triangle,
					triangleCount,
					vertexFormat);
			}
			v3d->set_fallback_colour();
			v3d->mark_disable_blend();
			mvms.pop();
			//
		}
		{
			Concurrency::ScopedSpinLock lock(dataFlagsLock);
			dataBeingRendered = false;
		}
	}
}

//

bool PilgrimOverlayInfo::EquipmentStats::operator ==(EquipmentStats const& _gs) const
{
	return equipment == _gs.equipment &&
		   (! equipment.is_set() || gunStats == _gs.gunStats); // compare stats only if there is actual equipment (previous check makes sure there are both)
}

void PilgrimOverlayInfo::EquipmentStats::copy_stats_from(EquipmentStats const& _stats)
{
	equipment = _stats.equipment;
	gunStats = _stats.gunStats;
	weaponParts = _stats.weaponParts;
}

void PilgrimOverlayInfo::EquipmentStats::reset(bool _keepEquipment)
{
	if (!_keepEquipment)
	{
		equipment.clear();
	}
	gunStats.reset();
	weaponParts.clear();
}

void PilgrimOverlayInfo::EquipmentStats::update_stats(bool _fillAffection, bool _fillWeaponParts)
{
	gunStats.reset();
	weaponParts.clear();
	if (auto* e = equipment.get())
	{
		if (auto* gun = e->get_gameplay_as<ModuleEquipments::Gun>())
		{
			gun->build_chosen_stats(REF_ gunStats, _fillAffection, nullptr, NP);
			if (_fillWeaponParts)
			{
				auto& ws = gun->get_weapon_setup();
				for_every(wp, ws.get_parts())
				{
					weaponParts.push_back(wp->part);
				}
			}
		}
	}
	if (_fillWeaponParts)
	{
		struct SortWeaponParts
		{
			static int compare(void const* _a, void const* _b)
			{
				RefCountObjectPtr<WeaponPart> const& a = *((RefCountObjectPtr<WeaponPart>*)_a);
				RefCountObjectPtr<WeaponPart> const& b = *((RefCountObjectPtr<WeaponPart>*)_b);
				auto* awpt = a->get_weapon_part_type();
				auto* bwpt = b->get_weapon_part_type();
				if (awpt && bwpt)
				{
					if (awpt->get_type() < bwpt->get_type()) return A_BEFORE_B;
					if (awpt->get_type() > bwpt->get_type()) return B_BEFORE_A;
				}
				return A_AS_B;
			}
		};
		sort(weaponParts, SortWeaponParts::compare);
	}
}

//

void PilgrimOverlayInfo::EquipmentsStats::reset()
{
	for_count(int, iHand, Hand::MAX)
	{
#ifdef WITH_SHORT_EQUIPMENT_STATS
		mainEquipment[iHand].reset();
#endif
		usedEquipment[iHand].reset();
		usedEquipmentInHand[iHand].reset();
#ifdef WITH_SHORT_EQUIPMENT_STATS
		pockets[iHand].clear();
#endif
	}
}

void PilgrimOverlayInfo::EquipmentsStats::copy_equipment_stats_from(EquipmentsStats const& _source)
{
	for_count(int, iHand, Hand::MAX)
	{
#ifdef WITH_SHORT_EQUIPMENT_STATS
		mainEquipment[iHand].copy_stats_from(_source.mainEquipment[iHand]);
#endif
		usedEquipment[iHand].copy_stats_from(_source.usedEquipment[iHand]);
#ifdef WITH_SHORT_EQUIPMENT_STATS
		pockets[iHand].set_size(_source.pockets[iHand].get_size());
		for_every(p, pockets[iHand])
		{
			p->copy_stats_from(_source.pockets[iHand][for_everys_index(p)]);
		}
#endif
	}
}

void PilgrimOverlayInfo::EquipmentsStats::copy_in_hand_equipment_stats_from(EquipmentsStats const& _source)
{
	for_count(int, iHand, Hand::MAX)
	{
		usedEquipmentInHand[iHand].copy_stats_from(_source.usedEquipmentInHand[iHand]);
	}
}

bool PilgrimOverlayInfo::EquipmentsStats::operator ==(EquipmentsStats const& _gs) const
{
	for_count(int, iHand, Hand::MAX)
	{
		if (
#ifdef WITH_SHORT_EQUIPMENT_STATS
			mainEquipment[iHand] != _gs.mainEquipment[iHand] ||
#endif
			usedEquipment[iHand] != _gs.usedEquipment[iHand])
		{
			return false;
		}
#ifdef WITH_SHORT_EQUIPMENT_STATS
		if (pockets[iHand].get_size() != _gs.pockets[iHand].get_size())
		{
			return false;
		}
		for_count(int, i, pockets[iHand].get_size())
		{
			if (pockets[iHand][i] != _gs.pockets[iHand][i])
			{
				return false;
			}
		}
#endif
	}
	return true;
}

#ifdef AN_ALLOW_EXTENSIVE_LOGS
tchar const* PilgrimOverlayInfo::to_char(WhatToShow _what)
{
	if (_what == Main) return TXT("main");
	if (_what == ShowInfo) return TXT("show info");
	if (_what == Investigate) return TXT("investigate");
	if (_what == None) return TXT("none");
	todo_implement;
	return TXT("??");
}
#endif

void PilgrimOverlayInfo::show_symbol_replacement(Framework::TexturePart* _from, Framework::TexturePart* _to)
{
	if (!_to)
	{
		// nothing to show
		return;
	}
	SymbolReplacement sr;
	sr.from = _from;
	sr.to = _to;
	{
		Concurrency::ScopedSpinLock lock(symbolReplacementsLock);
		symbolReplacements.push_back(sr);
	}

	speak(speakSymbolUnobfuscated.get());
}

void PilgrimOverlayInfo::update_symbol_replacements(float _deltaTime)
{
	if (symbolReplacements.is_empty())
	{
		return;
	}
	Concurrency::ScopedSpinLock lock(symbolReplacementsLock);
	auto& sr = symbolReplacements.get_first();
	if (!sr.element.get())
	{
		if (auto* game = Game::get_as<Game>())
		{
			if (auto* overlayInfoSystem = game->access_overlay_info_system())
			{
				auto* e = new OverlayInfo::Elements::TexturePart(sr.from.get());
				e->with_id(NAME(symbolReplacement));
				e->with_usage(OverlayInfo::Usage::World);

				Rotator3 offset(5.0f, 0.0f, 0.0f);

				e->with_location(OverlayInfo::Element::Relativity::RelativeToAnchorPY/*RelativeToCameraFlat*/);
				e->with_follow_camera_location_z(0.1f);
				e->with_rotation(OverlayInfo::Element::Relativity::RelativeToAnchorPY, offset);
				e->with_distance(0.55f);

				e->with_size(0.1f);

				e->set_cant_be_deactivated_by_system();

				sr.element = e;
				overlayInfoSystem->add(e);
			}
		}
	}

	sr.time += _deltaTime;
	float const showFor = 3.0f;
	float const blink = 0.3f;
	float const blinkFrom = showFor * 0.25f;
	float const blinkTo = showFor * 0.75f;
	float const pt = clamp((sr.time - blinkFrom) / (blinkTo - blinkFrom), 0.2f, 0.8f);
	bool showFrom = sr.time < blinkFrom || (sr.time < blinkTo && (mod(sr.time, blink) < blink* (1.0f - pt)));
	if (auto* tpe = fast_cast<OverlayInfo::Elements::TexturePart>(sr.element.get()))
	{
		tpe->with_texture_part(showFrom ? sr.from.get() : sr.to.get());
	}
	if (sr.time >= showFor)
	{
		if (auto* e = sr.element.get())
		{
			e->deactivate();
		}
		symbolReplacements.pop_front();
	}
}

void PilgrimOverlayInfo::update_turn_counter_info()
{
	if (auto* vr = VR::IVR::get())
	{
		bool showTurnCounter = PlayerSetup::get_current().get_preferences().turnCounter == Option::On;
		if (PlayerSetup::get_current().get_preferences().turnCounter == Option::Allow)
		{
			showTurnCounter = !vr->is_wireless();
		}
		int turnCount = 0;
		if (showTurnCounter)
		{
			turnCount = vr->get_turn_count();
			if (abs(turnCount) >= 10)
			{
				showTurnCounter = true;
			}
			else if (turnCounterInfo.get() && abs(turnCount) < 5)
			{
				showTurnCounter = false;
			}
			else
			{
				showTurnCounter = turnCounterInfo.is_set();
			}
		}
		if (showTurnCounter)
		{
			if (!turnCounterInfo.get())
			{
				if (auto* game = Game::get_as<Game>())
				{
					if (auto* overlayInfoSystem = game->access_overlay_info_system())
					{
						auto* t = new OverlayInfo::Elements::Text();
						t->with_id(NAME(turnCounterInfo));
						t->with_usage(OverlayInfo::Usage::World);
						t->with_pulse();
						t->with_vertical_align(1.0f);
						t->with_scale(1.0f);
						configure_tip_oie_element(t, Rotator3(5.0f, 0.0f, 0.0f));

						turnCounterInfoShownForTurnCount = 0;
						turnCounterInfo = t;
						overlayInfoSystem->add(t);
					}
				}
			}
			if (auto* t = fast_cast<OverlayInfo::Elements::Text>(turnCounterInfo.get()))
			{
				if (turnCounterInfoShownForTurnCount != turnCount)
				{
					turnCounterInfoShownForTurnCount = turnCount;
					if (turnCount > 0)
					{
						t->with_text(LOC_STR(NAME(lsTurnCounterInfoTurnLeft)));
					}
					else if (turnCount < 0)
					{
						t->with_text(LOC_STR(NAME(lsTurnCounterInfoTurnRight)));
					}
					else
					{
						t->with_text(String::empty());
					}
				}
			}
		}
		else if (turnCounterInfo.get())
		{
			turnCounterInfo->deactivate();
			turnCounterInfo.clear();
		}
	}
}

void PilgrimOverlayInfo::speak(PilgrimOverlayInfoSpeakLine::Type _line, SpeakParams const& _params)
{
	Framework::Sample* line = nullptr;
	switch (_line)
	{
	case PilgrimOverlayInfoSpeakLine::Success: line = speakSuccess.get(); break;
	case PilgrimOverlayInfoSpeakLine::ReceivingData: line = speakReceivingData.get(); break;
	case PilgrimOverlayInfoSpeakLine::TransferComplete: line = speakTransferComplete.get(); break;
	case PilgrimOverlayInfoSpeakLine::InterfaceBoxFound: line = speakInterfaceBoxFound.get(); break;
	case PilgrimOverlayInfoSpeakLine::InfestationEnded: line = speakInfestationEnded.get(); break;
	case PilgrimOverlayInfoSpeakLine::SystemStabilised: line = speakSystemStabilised.get(); break;
	default:break;
	}
	speak(line, _params);
}

void PilgrimOverlayInfo::speak(Framework::Sample* _line, SpeakParams const & _params)
{
	if (!_line)
	{
		return;
	}

	if (GameSettings::get().difficulty.noOverlayInfo)
	{
		return;
	}

#ifdef OUTPUT_OVERLAY_INFO
	output(TXT("[POvIn] speak (queue) \"%S\""), _line->get_text().to_char());
#endif

	Concurrency::ScopedSpinLock lock(speaking.speakingLock);

	// if we already have such a line, remove it from the list and add it at the end
	for_every(lts, speaking.linesToSpeak)
	{
		if (lts->line == _line)
		{
			speaking.linesToSpeak.remove_at(for_everys_index(lts));
			break;
		}
	}

	Speaking::LineToSpeak lts;
	lts.line = _line;
	lts.colour = _params.colour;
	lts.delay = _params.speakDelay;
	lts.clearLogOnSpeak = _params.speakClearLogOnSpeak.get(false);
	lts.atLine = _params.atLine;
	lts.performOnSpeak = _params.performOnSpeak;
	speaking.linesToSpeak.push_back(lts);
	speaking.shouldUpdateSpeaking = true;
	speaking.allowAutoClear = _params.allowAutoClear;

#ifdef OUTPUT_OVERLAY_INFO
	output(TXT("[POvIn] queued lines to speak:"));
	for_every(lts, speaking.linesToSpeak)
	{
		if (lts->line)
		{
			output(TXT("[POvIn]   %i \"%S\""), for_everys_index(lts), lts->line->get_text().to_char());
		}
	}
#endif
}

bool PilgrimOverlayInfo::is_speaking_or_pending() const
{
	bool result = false;
	{
		Concurrency::ScopedSpinLock lock(speaking.speakingLock);
		result = speaking.speakingLine || ! speaking.linesToSpeak.is_empty();
	}
	return result;
}

void PilgrimOverlayInfo::set_auto_energy_speak_blocked(bool _blocked)
{
	Concurrency::ScopedSpinLock lock(speakingContext.speakingLock);

	speakingContext.blocked = _blocked;
}

void PilgrimOverlayInfo::update_speaking(float _deltaTime)
{
	if (!owner.get())
	{
		return;
	}

	// energy levels, auto speak
	// we ignore game director's status blocking here as it is used only for startup sequence
	if (owner->get_as_object() &&
		owner->get_as_object()->is_world_active())
	{
		todo_hack(TXT("availableFor variables are used to avoid situations when particular things are set a frame after we were spawned, etc"));
		Concurrency::ScopedSpinLock lock(speakingContext.speakingLock);
		// health
		{
			pilgrimEnergyState.healthCouldBeTopped = false;
			pilgrimEnergyState.healthHalf = false;
			pilgrimEnergyState.healthLow = false;
			pilgrimEnergyState.healthVeryLow = false;
			pilgrimEnergyState.healthCritical = false;
			if (is_at_boot_level(4) && !GameDirector::get()->is_health_status_blocked() && ! speakingContext.blocked)
			{
				if (auto* h = owner->get_custom<CustomModules::Health>())
				{
					Energy newHealth = h->get_health();
					Energy newHealthTotal = h->get_total_health();
					Energy maxHealth = h->get_max_health();
					Energy maxHealthTotal = h->get_max_total_health();

					speakingContext.healthAvailableFor += _deltaTime;

					if (speakingContext.health.is_set() &&
						speakingContext.healthTotal.is_set() &&
						speakingContext.healthAvailableFor > 0.5f)
					{
						Energy thresholdCritical = hardcoded Energy(3);
						Energy thresholdLow = hardcoded maxHealth / 2;
						Energy thresholdTotalVeryLow = min(hardcoded maxHealthTotal / 3, hardcoded maxHealth * 2);
						Energy thresholdTotalHalf = hardcoded maxHealthTotal / 2;
						Energy thresholdTotalLow = min(hardcoded maxHealthTotal / 2, hardcoded maxHealth * 3);
						Energy thresholdTotalFull = hardcoded maxHealthTotal - Energy::one();

						pilgrimEnergyState.healthCouldBeTopped |= newHealthTotal < maxHealthTotal - hardcoded Energy(25);
						pilgrimEnergyState.healthHalf |= newHealthTotal < thresholdTotalHalf;
						pilgrimEnergyState.healthLow |= newHealthTotal < thresholdTotalLow;
						pilgrimEnergyState.healthVeryLow |= newHealthTotal < thresholdTotalVeryLow;
						pilgrimEnergyState.healthCritical |= newHealthTotal < thresholdTotalVeryLow;

						bool speakOnLowHealth = !TeaForGodEmperor::GameSettings::get().difficulty.playerImmortal;

						if (newHealth <= thresholdCritical && speakingContext.health.get() > thresholdCritical)
						{
							if (speakOnLowHealth)
							{
								speak(speakHealthCritical.get(), SpeakParams().with_colour(speakWarningColour).at_line(PILGRIM_LOG_AUTO_AT_LINE));
							}
						}
						else if (newHealth <= thresholdLow && speakingContext.health.get() > thresholdLow)
						{
							if (speakOnLowHealth)
							{
								speak(speakLowHealth.get(), SpeakParams().with_colour(speakWarningColour).at_line(PILGRIM_LOG_AUTO_AT_LINE));
							}
						}
						else if (newHealthTotal <= thresholdTotalHalf && speakingContext.healthTotal.get() > thresholdTotalHalf)
						{
							if (speakOnLowHealth)
							{
								speak(speakHalfHealth.get(), SpeakParams().with_colour(speakWarningColour).at_line(PILGRIM_LOG_AUTO_AT_LINE));
							}
						}
						else if (newHealthTotal <= thresholdTotalVeryLow && speakingContext.healthTotal.get() > thresholdTotalVeryLow)
						{
							if (speakOnLowHealth)
							{
								speak(speakLowHealth.get(), SpeakParams().with_colour(speakWarningColour).at_line(PILGRIM_LOG_AUTO_AT_LINE));
							}
						}
						if (newHealthTotal >= thresholdTotalFull && speakingContext.healthTotal.get() < thresholdTotalFull)
						{
							if (pilgrimEnergyState.healthFullTS.get_time_since() > 2.0f)
							{
								if (auto* mp = owner->get_gameplay_as<ModulePilgrim>())
								{
									if (mp->should_pilgrim_overlay_speak_full_health())
									{
										speak(speakFullHealth.get(), SpeakParams().at_line(PILGRIM_LOG_AUTO_AT_LINE));
									}
								}
							}
							pilgrimEnergyState.healthFullTS.reset();
						}
					}

					speakingContext.health = newHealth;
					speakingContext.healthTotal = newHealthTotal;
				}
			}
			else
			{
				speakingContext.health.clear();
				speakingContext.healthTotal.clear();
				speakingContext.healthAvailableFor = 0.0f;
			}
		}
		// energy storages
		{
			pilgrimEnergyState.ammoCouldBeTopped = false;
			pilgrimEnergyState.ammoHalf = false;
			pilgrimEnergyState.ammoLow = false;
			pilgrimEnergyState.ammoCritical = false;
			if (is_at_boot_level(4) && !GameDirector::get()->is_ammo_status_blocked() && !speakingContext.blocked)
			{
				if (auto* mp = owner->get_gameplay_as<ModulePilgrim>())
				{
					for_count(int, hIdx, GameSettings::get().difficulty.commonHandEnergyStorage ? 1 : Hand::MAX)
					{
						Hand::Type hand = (Hand::Type)hIdx;
						Energy newAmmo = mp->get_hand_energy_storage(hand);
						Energy maxAmmo = mp->get_hand_energy_max_storage(hand);

						speakingContext.energyStoragesAvailableFor += _deltaTime;

						if (speakingContext.energyStorages[hIdx].is_set() &&
							speakingContext.energyStoragesAvailableFor > 0.5f)
						{
							Energy thresholdHalf = hardcoded maxAmmo / 2;
							Energy thresholdLow = min(hardcoded maxAmmo / 3, Energy(50));
							Energy thresholdFull = hardcoded maxAmmo - Energy::one();
							Energy thresholdCritical = Energy::zero();
							if (auto* eqimo = mp->get_hand_equipment(hand))
							{
								if (auto* gun = eqimo->get_gameplay_as<ModuleEquipments::Gun>())
								{
									thresholdCritical = gun->get_chamber_size() * 2;
								}
							}

							pilgrimEnergyState.ammoCouldBeTopped |= newAmmo < maxAmmo - hardcoded Energy(50);
							pilgrimEnergyState.ammoHalf |= newAmmo < thresholdHalf;
							pilgrimEnergyState.ammoLow |= newAmmo < thresholdLow;
							pilgrimEnergyState.ammoCritical |= newAmmo < thresholdCritical;

							bool speakOnLowEnergy = !TeaForGodEmperor::GameSettings::get().difficulty.weaponInfinite;

							if ((pilgrimEnergyState.playNoAmmo[hIdx] && pilgrimEnergyState.playNoAmmoTS[hIdx].get_time_since() > 5.0f) ||
								(newAmmo < thresholdCritical && speakingContext.energyStorages[hIdx].get() >= thresholdCritical))
							{
								pilgrimEnergyState.playNoAmmoTS[hIdx].reset();
								if (speakOnLowEnergy)
								{
									speak(GameSettings::get().difficulty.commonHandEnergyStorage ? speakAmmoCritical.get() : speakAmmoCriticalHand[hIdx].get(), SpeakParams().with_colour(speakWarningColour).at_line(PILGRIM_LOG_AUTO_AT_LINE));
									if (followGuidanceSpokenTS.get_time_since() >= 10.0f && 
										GameSettings::get().difficulty.showDirectionsOnlyToObjective &&
										PlayerPreferences::should_allow_follow_guidance() &&
										!GameDirector::get()->is_follow_guidance_blocked() &&
										!PilgrimageInstance::get()->is_follow_guidance_blocked())
									{
										speak(speakFindEnergySource.get(), SpeakParams().with_colour(speakWarningColour).at_line(PILGRIM_LOG_AUTO_AT_LINE));
										speak(speakFollowGuidanceAmmo.get(), SpeakParams().with_colour(speakWarningColour).at_line(PILGRIM_LOG_AUTO_AT_LINE));
										followGuidanceSpokenTS.reset();
									}
								}
							}
							if (newAmmo <= thresholdHalf && speakingContext.energyStorages[hIdx].get() > thresholdHalf)
							{
								if (speakOnLowEnergy)
								{
									speak(GameSettings::get().difficulty.commonHandEnergyStorage? speakHalfAmmo.get() : speakHalfAmmoHand[hIdx].get(), SpeakParams().with_colour(speakWarningColour).at_line(PILGRIM_LOG_AUTO_AT_LINE));
								}
							}
							if (newAmmo <= thresholdLow && speakingContext.energyStorages[hIdx].get() > thresholdLow)
							{
								if (speakOnLowEnergy)
								{
									speak(GameSettings::get().difficulty.commonHandEnergyStorage? speakLowAmmo.get() : speakLowAmmoHand[hIdx].get(), SpeakParams().with_colour(speakWarningColour).at_line(PILGRIM_LOG_AUTO_AT_LINE));
								}
							}
							if (newAmmo >= thresholdFull && speakingContext.energyStorages[hIdx].get() < thresholdFull)
							{
								if (pilgrimEnergyState.ammoFullTS[hIdx].get_time_since() > 2.0f)
								{
									if (mp->should_pilgrim_overlay_speak_full_ammo())
									{
										speak(GameSettings::get().difficulty.commonHandEnergyStorage ? speakFullAmmo.get() : speakFullAmmoHand[hIdx].get(), SpeakParams().at_line(PILGRIM_LOG_AUTO_AT_LINE));
									}
								}
								pilgrimEnergyState.ammoFullTS[hIdx].reset();
							}
						}

						speakingContext.energyStorages[hIdx] = newAmmo;
					}
				}
			}
			else
			{
				speakingContext.energyStorages[Hand::Left].clear();
				speakingContext.energyStorages[Hand::Right].clear();
				speakingContext.energyStoragesAvailableFor = 0.0f;
			}
			for_count(int, i, Hand::MAX) pilgrimEnergyState.playNoAmmo[i] = false;
		}
	}

	if (! speaking.shouldUpdateSpeaking)
	{
		return;
	}
	Concurrency::ScopedSpinLock lock(speaking.speakingLock);

	if (auto* vos = VoiceoverSystem::get())
	{
		float const speakingInterval = hardcoded 0.5f;
		float const autoClearTime = hardcoded 3.0f;

		if (speaking.speakingLine)
		{
			if (! vos->is_speaking(NAME(pilgrimOverlayInfo), speaking.speakingLine))
			{
				speaking.speakingLine = nullptr;
			}
		}

		if (speaking.speakingLine)
		{
			speaking.timeSinceLastSpeak = 0.0f;
		}
		else
		{
			speaking.timeSinceLastSpeak += _deltaTime;
		}

		if (!speaking.linesToSpeak.is_empty() &&
			(speaking.timeSinceLastSpeak >= speakingInterval || speaking.linesToSpeak.get_first().delay.is_set()))
		{
			if (speaking.linesToSpeak.get_first().delay.is_set())
			{
				speaking.timeSinceLastSpeak = max(speaking.timeSinceLastSpeak, speakingInterval);
				speaking.linesToSpeak.get_first().delay = speaking.linesToSpeak.get_first().delay.get() - _deltaTime;
				if (speaking.linesToSpeak.get_first().delay.get() <= 0.0f)
				{
					speaking.linesToSpeak.get_first().delay.clear();
				}
			}
			else
			{
				auto lts = speaking.linesToSpeak.get_first();
				if (lts.clearLogOnSpeak)
				{
					clear_main_log();
				}
				speaking.speakingLine = lts.line;
				speaking.linesToSpeak.pop_front();
				speaking.timeSinceLastSpeak = 0.0f;
#ifdef OUTPUT_OVERLAY_INFO
				output(TXT("[POvIn] speak line \"%S\""), speaking.speakingLine->get_text().trim().to_char());
#endif
				vos->speak(NAME(pilgrimOverlayInfo), 0 /* no audio duck */, speaking.speakingLine, false /* we use log to show text */);
				add_main_log(speaking.speakingLine->get_text().trim(), lts.colour, lts.atLine);
				clear_main_log_line_indicator_on_no_voiceover_actor(NAME(pilgrimOverlayInfo));
				if (lts.performOnSpeak)
				{
					lts.performOnSpeak();
				}

#ifdef OUTPUT_OVERLAY_INFO
				output(TXT("[POvIn] queued lines to speak left:"));
				for_every(lts, speaking.linesToSpeak)
				{
					if (lts->line)
					{
						output(TXT("[POvIn]   %i \"%S\""), for_everys_index(lts), lts->line->get_text().to_char());
					}
				}
#endif
			}

		}

		if (speaking.timeSinceLastSpeak >= autoClearTime)
		{
			if (speaking.allowAutoClear)
			{
				clear_main_log();
			}
			if (speaking.linesToSpeak.is_empty())
			{
				speaking.shouldUpdateSpeaking = false;
			}
		}
	}
}

void PilgrimOverlayInfo::log(LogInfoContext& _log)
{
	_log.log(TXT("pilgrim overlay info"));
	LOG_INDENT(_log);
	{
		_log.log(TXT("going in wrong direction"));
		LOG_INDENT(_log);
		_log.log(TXT("count : %i"), goingInWrongDirectionCount);
		_log.log(TXT("TS    : %.3f"), goingInWrongDirectionTS.get_time_since());
	}
	{
		_log.log(TXT("pilgrim energy state"));
		LOG_INDENT(_log);
		_log.log(TXT("health : %S"), pilgrimEnergyState.healthCritical ? TXT("critical")
								   : pilgrimEnergyState.healthVeryLow ? TXT("very low")
								   : pilgrimEnergyState.healthLow ? TXT("low")
								   : pilgrimEnergyState.healthHalf ? TXT("half")
								   : pilgrimEnergyState.healthCouldBeTopped ? TXT("could be topped")
								   : TXT("full or almost full"));
		_log.log(TXT("ammo   : %S"), pilgrimEnergyState.ammoCritical ? TXT("critical")
								   : pilgrimEnergyState.ammoLow ? TXT("low")
								   : pilgrimEnergyState.ammoHalf ? TXT("half")
								   : pilgrimEnergyState.ammoCouldBeTopped ? TXT("could be topped")
								   : TXT("full or almost full"));
		_log.log(TXT("healthFullTS  : %.3f"), pilgrimEnergyState.healthFullTS.get_time_since());
		_log.log(TXT("ammoFullTS[L] : %.3f"), pilgrimEnergyState.ammoFullTS[Hand::Left].get_time_since());
		_log.log(TXT("ammoFullTS[R] : %.3f"), pilgrimEnergyState.ammoFullTS[Hand::Right].get_time_since());
	}
}

void PilgrimOverlayInfo::clear_object_descriptions()
{
	for_every_ref(o, objectDescriptor.objects)
	{
		o->deactivate_descriptor_element();
	}
	objectDescriptor.objects.clear();
}

void PilgrimOverlayInfo::collect_health_damage(Framework::IModulesOwner* _imo, Energy const& _damage)
{
	if (_imo == owner.get())
	{
		// do not show health damage for owner
		return;
	}
	float const keepForTime = 2.0f;
	Concurrency::ScopedSpinLock lock(collectedHealthDamageLock);
	for_every(rp, collectedHealthDamage)
	{
		if (rp->unsafeIMOForReference == _imo)
		{
			rp->damage += _damage;
			rp->timeLeft = keepForTime;
			return;
		}
	}

	if (collectedHealthDamage.has_place_left())
	{
		ReportedDamage rp;
		rp.unsafeIMOForReference = _imo;
		rp.damage = _damage;
		rp.timeLeft = keepForTime;
		collectedHealthDamage.push_back(rp);
	}
}

void PilgrimOverlayInfo::advance_object_descriptor(float _deltaTime)
{
	objectDescriptor.timeSinceLastUpdate += _deltaTime;
	objectDescriptor.timeToUpdate -= _deltaTime;
	if (objectDescriptor.timeToUpdate >= 0.0f)
	{
		return;
	}

	{
		float timeStep = 0.2f;
		float advanceTime = objectDescriptor.timeSinceLastUpdate;
		objectDescriptor.timeSinceLastUpdate = 0.0f;

		objectDescriptor.timeToUpdate = timeStep;

		for (int i = 0; i < objectDescriptor.objects.get_size(); ++i)
		{
			auto& od = *objectDescriptor.objects[i].get();
			od.forceKeepTimeLeft = max(od.forceKeepTimeLeft - advanceTime, 0.0f);
		}

		{
			Concurrency::ScopedSpinLock lock(collectedHealthDamageLock);
			for (int i = 0; i < collectedHealthDamage.get_size(); ++i)
			{
				auto& rp = collectedHealthDamage[i];
				rp.timeLeft -= advanceTime;
				if (rp.timeLeft <= 0.0f)
				{
					collectedHealthDamage.remove_fast_at(i);
					--i;
				}
			}
		}
	}

	struct SetupText
	{
		static void damage(OUT_ tchar * t, int tlen, Energy const& _damage)
		{
#ifdef AN_CLANG
			tsprintf(t, tlen, TXT("-%S"),
#else
			tsprintf(t, tlen, TXT("-%s"),
#endif
				_damage.as_string_auto_decimals().to_char());
		}
	};

	auto* r = owner->get_presence()->get_in_room();
	Transform pilgrimViewPlacement = owner->get_presence()->get_placement().to_world(owner->get_presence()->get_eyes_relative_look());	

	for (int i = 0; i < objectDescriptor.objects.get_size(); ++i)
	{
		auto& od = *objectDescriptor.objects[i].get();
		od.valid = false;
		if (od.description == ObjectDescriptor::Object::Description::Investigate)
		{
			if (auto* imo = od.path.get_target())
			{
				MODULE_OWNER_LOCK_FOR_IMO(imo, TXT("investigate icons"));
				struct IconFlags
				{
					enum Flag
					{
						NoSight				= 1 << 0,
						SightMovementBased	= 1 << 1,
						NoHearing			= 1 << 2,
						Ally				= 1 << 3,
						Enemy				= 1 << 4,
						Confused			= 1 << 5,
						
						InvestigatorInfoProviderStartingBit = 6
					};
				};
				Optional<Energy> health;
				int iconsValue = 0;
				int extraEffects = 0; // sum of name values
				if (imo->get_variables().get_value<bool>(NAME(perceptionNoSight), false) ||
					imo->get_variables().get_value<bool>(NAME(perceptionSightImpaired), false))
				{
					iconsValue |= IconFlags::NoSight;
				}
				else if (imo->get_variables().get_value<bool>(NAME(perceptionSightMovementBased), false))
				{
					iconsValue |= IconFlags::SightMovementBased;
				}
				else
				if (imo->get_variables().get_value<bool>(NAME(perceptionNoHearing), false))
				{
					iconsValue |= IconFlags::NoHearing;
				}
				if (imo->get_variables().get_value<bool>(NAME(confused), false))
				{
					iconsValue |= IconFlags::Confused;
				}
				{
					bool ignoreSocial = false;
					if (auto* iip = imo->get_custom<CustomModules::InvestigatorInfoProvider>())
					{
						ignoreSocial = iip->should_ignore_social_info();
					}
					if (!ignoreSocial)
					{
						if (auto* ai = imo->get_ai())
						{
							if (auto* mind = ai->get_mind())
							{
								if (mind->get_social().is_ally(owner.get()))
								{
									iconsValue |= IconFlags::Ally;
								}
								else if (mind->get_social().is_enemy(owner.get()))
								{
									iconsValue |= IconFlags::Enemy;
								}
							}
						}
					}
				}
				if (auto* iip = imo->get_custom<CustomModules::InvestigatorInfoProvider>())
				{
					for_every(cd, od.investigatorInfoProviderCachedData)
					{
						int idx = for_everys_index(cd);
						if (iip->has_icon(idx) &&
							iip->is_active(idx) &&
							! iip->get_icon_at_socket(idx).is_set()) // if at socket, don't show it here
						{
							iconsValue |= 1 << (IconFlags::InvestigatorInfoProviderStartingBit + idx);
						}
					}
				}
				auto* healthModule = imo->get_custom<CustomModules::Health>();
				if (healthModule)
				{
					health = max(Energy::zero(), healthModule->get_total_health());
					for_every(ee, healthModule->get_extra_effects())
					{
						extraEffects += ee->name.get_index();
					}
				}
				if (od.descriptorElements.is_index_valid(ObjectDescriptor::Object::DEI_Health))
				{
					auto& ode = od.descriptorElements[ObjectDescriptor::Object::DEI_Health];
					if (auto* e = fast_cast<OverlayInfo::Elements::Text>(ode.element.get()))
					{
						if (ode.cachedData.check_update(health))
						{
							if (health.is_set())
							{
								String healthInfo = String::printf(TXT("%lc%S"), investigateInfo.health.icon, health.get().as_string().to_char());
								e->with_text(healthInfo);
							}
							else
							{
								e->with_text(TXT(""));
							}
						}
					}
				}
				if (od.descriptorElements.is_index_valid(ObjectDescriptor::Object::DEI_Armour))
				{
					auto& ode = od.descriptorElements[ObjectDescriptor::Object::DEI_Armour];
					if (auto* e = fast_cast<OverlayInfo::Elements::Text>(ode.element.get()))
					{
						::SafePtr<Framework::IModulesOwner> spawnedShield = imo->get_variables().get_value<::SafePtr<Framework::IModulesOwner>>(NAME(spawnedShield), ::SafePtr<Framework::IModulesOwner>());
						struct ShieldArmour
						{
							bool hasShield;
							float armourInfoValue;
						} shieldArmour;
						shieldArmour.hasShield = spawnedShield.get();
						shieldArmour.armourInfoValue = imo->get_variables().get_value<float>(NAME(armourMaterialInfo), 0.0f);
						if (healthModule)
						{
							for_every(ee, healthModule->get_extra_effects())
							{
								if (ee->params.armourPiercing.is_set())
								{
									shieldArmour.armourInfoValue *= ee->params.armourPiercing.get().as_float();
								}
							}
						}
						if (ode.cachedData.check_update(shieldArmour))
						{
							if (GameSettings::get().difficulty.armourIneffective)
							{
								shieldArmour.armourInfoValue = 0.0f;
							}
							shieldArmour.armourInfoValue = min(shieldArmour.armourInfoValue, clamp(GameSettings::get().difficulty.armourEffectivenessLimit, 0.0f, 1.0f));

							tchar armourInfo[8];
							int at = 0;
							if (shieldArmour.hasShield)
							{
								armourInfo[at] = investigateInfo.shield.icon;
								++at;
							}
							if (shieldArmour.armourInfoValue > 0.0f)
							{
								if (shieldArmour.armourInfoValue >= 0.625f)
								{
									armourInfo[at] = shieldArmour.armourInfoValue >= 1.000f ? investigateInfo.armour100.icon : investigateInfo.armour75.icon;
								}
								else
								{
									armourInfo[at] = shieldArmour.armourInfoValue >= 0.375f ? investigateInfo.armour50.icon : investigateInfo.armour25.icon;
								}
								++at;
							}
							armourInfo[at] = 0;
							e->with_text(armourInfo);
						}
					}
				}
				if (od.descriptorElements.is_index_valid(ObjectDescriptor::Object::DEI_Ricochet))
				{
					auto& ode = od.descriptorElements[ObjectDescriptor::Object::DEI_Ricochet];
					if (auto* e = fast_cast<OverlayInfo::Elements::Text>(ode.element.get()))
					{
						float ricochetInfoValue = imo->get_variables().get_value<float>(NAME(ricochetMaterialInfo), 0.0f);

						if (GameSettings::get().difficulty.armourIneffective)
						{
							ricochetInfoValue = 0.0f;
						}
						if (healthModule && !healthModule->do_extra_effects_allow_ricochets())
						{
							ricochetInfoValue = 0.0f;
						}

						if (ode.cachedData.check_update(ricochetInfoValue))
						{
							ricochetInfoValue = min(ricochetInfoValue, clamp(GameSettings::get().difficulty.armourEffectivenessLimit, 0.0f, 1.0f));

							tchar ricochetInfo[8];
							int at = 0;
							if (ricochetInfoValue > 0.0f)
							{
								ricochetInfo[at] = investigateInfo.ricochet.icon;
								++at;
							}
							ricochetInfo[at] = 0;
							e->with_text(ricochetInfo);
						}
					}
				}
				if (od.descriptorElements.is_index_valid(ObjectDescriptor::Object::DEI_Icons))
				{
					auto& ode = od.descriptorElements[ObjectDescriptor::Object::DEI_Icons];
					if (auto* e = fast_cast<OverlayInfo::Elements::Text>(ode.element.get()))
					{
						if (ode.cachedData.check_update(iconsValue))
						{
							tchar iconInfo[16];
							int at = 0;
							if (iconsValue & IconFlags::Confused)
							{
								iconInfo[at] = investigateInfo.confused.icon;
								++at;
							}
							if (iconsValue & IconFlags::Ally)
							{
								iconInfo[at] = investigateInfo.ally.icon;
								++at;
							}
							if (iconsValue & IconFlags::Enemy)
							{
								iconInfo[at] = investigateInfo.enemy.icon;
								++at;
							}
							if (iconsValue & IconFlags::NoSight)
							{
								iconInfo[at] = investigateInfo.noSight.icon;
								++ at;
							}
							if (iconsValue & IconFlags::SightMovementBased)
							{
								iconInfo[at] = investigateInfo.sightMovementBased.icon;
								++ at;
							}
							if (iconsValue & IconFlags::NoHearing)
							{
								iconInfo[at] = investigateInfo.noHearing.icon;
								++at;
							}
							if (auto* iip = imo->get_custom<CustomModules::InvestigatorInfoProvider>())
							{
								for_every(cd, od.investigatorInfoProviderCachedData)
								{
									int idx = for_everys_index(cd);
									if (iconsValue & (1 << (IconFlags::InvestigatorInfoProviderStartingBit + idx)))
									{
										auto iipicon = iip->get_icon(idx);
										if (iipicon.is_set())
										{
											iconInfo[at] = iipicon.get();
											++at;
										}
									}
								}
							}
							iconInfo[at] = 0;
							an_assert(at < 16);
							e->with_text(iconInfo);
						}
					}
				}
				if (od.descriptorElements.is_index_valid(ObjectDescriptor::Object::DEI_ExtraEffects))
				{
					auto& ode = od.descriptorElements[ObjectDescriptor::Object::DEI_ExtraEffects];
					if (auto* e = fast_cast<OverlayInfo::Elements::Text>(ode.element.get()))
					{
						if (ode.cachedData.check_update(extraEffects))
						{
							tchar iconInfo[16];
							int at = 0;
							if (healthModule)
							{
								for_every(ee, healthModule->get_extra_effects())
								{
									if (ee->params.investigatorInfo.icon.is_valid())
									{
										auto const & iipiconString = ee->params.investigatorInfo.icon.get();
										if (! iipiconString.is_empty())
										{
											iconInfo[at] = iipiconString[0];
											++at;
										}
									}
								}
							}
							iconInfo[at] = 0;
							an_assert(at < 16);
							e->with_text(iconInfo);
						}
					}
				}
				{
					for (int deIdx = od.startIndices.healthRules; deIdx < od.startIndices.icons; ++deIdx)
					{
						auto& ode = od.descriptorElements[deIdx];
						if (auto* e = fast_cast<OverlayInfo::Elements::Text>(ode.element.get()))
						{
							int hrIdx = ode.healthRuleIdx.get(deIdx - od.startIndices.healthRules);

							Optional<Energy> hrHealth;
							if (healthModule)
							{
								hrHealth = healthModule->get_rule_health(hrIdx);
							}
							if (ode.cachedData.check_update(hrHealth))
							{
								if (hrHealth.is_set())
								{
									String healthInfo = String::printf(TXT("%lc%S"), investigateInfo.healthRule.icon, hrHealth.get().as_string().to_char());
									e->with_text(healthInfo);
								}
								else
								{
									e->with_text(TXT(""));
								}
							}
						}
					}
				}
			}
			if (!od.path.get_target())
			{
				od.shouldDeactivate = true;
				if (od.forceKeepTimeLeft > 0.0f &&
					od.collectedDamageShouldBeVisible)
				{
					// make it remain visible
					od.valid = true;
				}
			}
		}
		if (od.description == ObjectDescriptor::Object::Description::HealthDamage)
		{
			if (auto* imo = od.unsafeIMOForReference)
			{
				Optional<Energy> reportDamage;
				for (int i = 0; i < collectedHealthDamage.get_size(); ++i)
				{
					auto& rp = collectedHealthDamage[i];
					if (rp.unsafeIMOForReference == imo)
					{
						reportDamage = rp.damage;
						od.forceKeepTimeLeft = rp.timeLeft;
						break;
					}
				}
				{
					od.collectedDamageShouldBeVisible = true;// reportDamage.is_set();
					if (od.descriptorElements.get_size() > 0)
					{
						auto& ode = od.descriptorElements[0];
						if (auto* e = fast_cast<OverlayInfo::Elements::Text>(ode.element.get()))
						{
							if (ode.cachedData.check_update(reportDamage))
							{
								if (reportDamage.is_set())
								{
									if (reportDamage.get() < Energy(1000))
									{
										tchar t[100];
										SetupText::damage(OUT_ t, 100, reportDamage.get());
										e->with_text(t);
									}
									else
									{
										e->with_text(TXT("---"));
									}
								}
								else
								{
									e->with_text(TXT(""));
								}
							}
						}
					}
				}
			}
			if (!od.path.get_target())
			{
				od.shouldDeactivate = true;
				if (od.forceKeepTimeLeft > 0.0f &&
					od.collectedDamageShouldBeVisible)
				{
					// make it remain visible
					od.valid = true;
				}
			}
		}
		if (od.shouldDeactivate && od.forceKeepTimeLeft <= 0.0f)
		{
			od.deactivate_descriptor_element();
			objectDescriptor.objects.remove_at(i);
			--i;
		}
	}
	ScanForObjectDescriptorContext context;
	context.lookForGuns = false;
	context.lookForPickups = false;
	context.lookForInvestigate = false;
	context.lookForHealthDamage = false;
	context.lookForNoDoorMarkers = false;
	if (auto* mp = owner->get_gameplay_as<ModulePilgrim>())
	{
		if (!noExplicitTutorials)
		{
			for_count(int, hIdx, Hand::MAX)
			{
				if (auto* e = mp->get_main_equipment((Hand::Type)hIdx))
				{
					if (auto* g = e->get_gameplay_as<ModuleEquipments::Gun>())
					{
						if (g->is_damaged())
						{
							context.lookForGuns = true;
							break;
						}
					}
				}
			}
		}
		if (!noExplicitTutorials)
		{
			if (mp->should_pilgrim_overlay_look_for_pickups())
			{
				context.lookForPickups = true;
			}
		}
		if (mp->has_exm_equipped(EXMID::Active::investigator()))
		{
			context.lookForInvestigate = true;
			context.lookForHealthDamage = true;
		}
	}
	if (auto* piow = PilgrimageInstanceOpenWorld::get())
	{
		context.lookForNoDoorMarkers = piow->are_there_no_door_markers_pending();
	}
	s_collectHealthDamageForLocalPlayer = context.lookForHealthDamage;
	if (r && context.should_look_for_anything())
	{
		System::ViewFrustum viewFrustum;
		ArrayStatic<Framework::DoorInRoom*, ObjectDescriptor::MAX_ROOM_DEPTH > throughDoors;
		context.pilgrimInRoom = r;
		context.originalPilgrimViewPlacement = pilgrimViewPlacement;
		context.throughDoorsLimit = 1;
		if (context.lookForGuns ||
			context.lookForPickups)
		{
			context.throughDoorsLimit = max<int>(context.throughDoorsLimit, ObjectDescriptor::MaxRoomDepth::PickUp);
		}
		if (context.lookForInvestigate)
		{
			context.throughDoorsLimit = max<int>(context.throughDoorsLimit, ObjectDescriptor::MaxRoomDepth::Investigate);
		}
		if (context.lookForHealthDamage)
		{
			context.throughDoorsLimit = max<int>(context.throughDoorsLimit, ObjectDescriptor::MaxRoomDepth::HealthDamage);
		}
		if (context.lookForNoDoorMarkers)
		{
			context.throughDoorsLimit = max<int>(context.throughDoorsLimit, ObjectDescriptor::MaxRoomDepth::NoDoorMarkers);
		}
		context.throughDoorsLimit = min(context.throughDoorsLimit, ObjectDescriptor::MAX_ROOM_DEPTH);
		scan_room_object_descriptor(REF_ context, scanForObjectDescriptorKeepData, r, pilgrimViewPlacement, throughDoors, viewFrustum, Transform::identity);
	}
	// to keep the last result (so we stay with it and less likely switch to something else - and consume
	{
		scanForObjectDescriptorKeepData.investigate.bestMatch = context.investigate.bestMatch;
		scanForObjectDescriptorKeepData.investigate.chosen = context.investigate.chosen;
		if (context.investigate.chosen)
		{
			use_object_descriptor(context.investigate.chosen, ObjectDescriptor::Object::Investigate, context.investigate.chosenThroughDoors);
		}
	}
	// if objects are not marked as valid at this moment, they will be removed
	for (int i = 0; i < objectDescriptor.objects.get_size(); ++i)
	{
		auto& od = *objectDescriptor.objects[i].get();
		if (!od.valid) // was not visible, remove it!
		{
			od.deactivate_descriptor_element();
			objectDescriptor.objects.remove_at(i);
			--i;
			continue;
		}
		else
		{
			// if no descriptorElement, make it
			// if we're to change it, it is done in scan_rooom_object_descriptor (ie. descriptorElement is deactivated and cleared
			if (od.descriptorElements.is_empty())
			{
				struct CustomVRPlacementForDescriptor
				{
					Optional<int> atBoneIdx;
					Optional<int> atSocketIdx;

					static Transform calculate(OverlayInfo::System const* _system, OverlayInfo::Element const* _element, Framework::IModulesOwner* owner, ObjectDescriptor::Object* od, Optional<CustomVRPlacementForDescriptor> const & _info = NP)
					{
						bool isOk = false;
						if (od)
						{
							if (od->path.is_active())
							{
								if (auto* imo = owner)
								{
									if (auto* t = od->path.get_target())
									{
										float closerDistance = 0.05f;
										float collisionRadius = 0.0f;
										if (auto* c = t->get_collision())
										{
											collisionRadius = c->get_collision_primitive_radius();
										}
										if (collisionRadius > 0.0f)
										{
											closerDistance = min(closerDistance, collisionRadius * 0.3f);
										}
										Transform targetOS = t->get_presence()->get_centre_of_presence_os();
										if (_info.is_set())
										{
											if (_info.get().atSocketIdx.is_set())
											{
												if (auto* a = t->get_appearance())
												{
													targetOS = a->calculate_socket_os(_info.get().atSocketIdx.get());
												}
											}
											else if (_info.get().atBoneIdx.is_set())
											{
												if (auto* a = t->get_appearance())
												{
													if (auto* pose = a->get_final_pose_MS())
													{
														auto& bones = pose->get_bones();
														if (bones.is_index_valid(_info.get().atBoneIdx.get()))
														{
															targetOS = a->get_ms_to_os_transform().to_world(bones[_info.get().atBoneIdx.get()]);
														}
													}
												}
											}
										}
										else if (auto* ai = fast_cast<ModuleAI>(t->get_ai()))
										{
											targetOS = ai->calculate_investigate_placement_os();
											closerDistance += collisionRadius;
										}

										// Owner Room Space
										Transform vrAnchorORS = imo->get_presence()->get_vr_anchor();
										Transform placementORS = od->path.get_placement_in_owner_room();
										placementORS = placementORS.to_world(targetOS);
										Vector3 placementLocVR = vrAnchorORS.location_to_local(placementORS.get_translation());

										// orientate
										Transform cameraVR = _system->get_camera_placement();
										Vector3 cameraLocVR = cameraVR.get_translation();
										//Transform placementVR = look_matrix(placementLocVR, (placementLocVR - cameraLocVR).normal(), cameraVR.get_axis(Axis::Z)).to_transform();
										Vector3 camToPlaceDir = (placementLocVR - cameraLocVR).normal();
										float camToPlaceDist = (placementLocVR - cameraLocVR).length();
										closerDistance = min(closerDistance, max(max(0.0f, camToPlaceDist - 0.1f), camToPlaceDist * 0.5f));
										Transform placementVR = look_matrix(placementLocVR - camToPlaceDir * closerDistance, (cameraVR.get_axis(Axis::Y) + camToPlaceDir).normal(), cameraVR.get_axis(Axis::Z)).to_transform();

										od->vrPlacement = placementVR;
										isOk = true;
									}
								}
							}
							if (!isOk)
							{
								od->shouldDeactivate = true;

								// reorientate towards camera
								Transform cameraVR = _system->get_camera_placement();
								Vector3 cameraLocVR = cameraVR.get_translation();
								Vector3 placementLocVR = od->vrPlacement.get_translation();

								Transform placementVR = look_matrix(placementLocVR, (cameraVR.get_axis(Axis::Y) + (placementLocVR - cameraLocVR).normal()).normal(), cameraVR.get_axis(Axis::Z)).to_transform();

								od->vrPlacement = placementVR;
							}
							return od->vrPlacement;
						}
						return _element->get_placement(); // as it is
					}
				};
				if (od.description == ObjectDescriptor::Object::Description::Investigate)
				{
					RefCountObjectPtr<ObjectDescriptor::Object> odref(&od);

					// create element that is placed where object is placed and shows info about investigation
					an_assert(od.descriptorElements.get_size() == ObjectDescriptor::Object::DEI_Health);
					od.descriptorElements.push_back(
						ObjectDescriptor::Object::DescriptorElement((new OverlayInfo::Elements::Text())
						->with_use_additional_scale(DESCRIPTOR_ELEMENT_ADDITIONAL_SCALE)
						->with_char_offset(Vector2(-3.0f, -0.5f))
						->with_horizontal_align(0.0f)
						));
					an_assert(od.descriptorElements.get_size() == ObjectDescriptor::Object::DEI_Armour);
					od.descriptorElements.push_back(
						ObjectDescriptor::Object::DescriptorElement((new OverlayInfo::Elements::Text())
						->with_use_additional_scale(DESCRIPTOR_ELEMENT_ADDITIONAL_SCALE)
						->with_char_offset(Vector2(3.0f, -0.5f))
						->with_horizontal_align(1.0f)
						));
					an_assert(od.descriptorElements.get_size() == ObjectDescriptor::Object::DEI_Ricochet);
					od.descriptorElements.push_back(
						ObjectDescriptor::Object::DescriptorElement((new OverlayInfo::Elements::Text())
						->with_use_additional_scale(DESCRIPTOR_ELEMENT_ADDITIONAL_SCALE)
						->with_char_offset(Vector2(3.0f, -0.5f))
						->with_horizontal_align(0.0f)
						));
					an_assert(od.descriptorElements.get_size() == ObjectDescriptor::Object::DEI_Icons);
					od.descriptorElements.push_back(
						ObjectDescriptor::Object::DescriptorElement((new OverlayInfo::Elements::Text())
						->with_use_additional_scale(DESCRIPTOR_ELEMENT_ADDITIONAL_SCALE)
						->with_char_offset(Vector2(-3.0f, -1.5f))
						->with_horizontal_align(0.0f)
						));
					an_assert(od.descriptorElements.get_size() == ObjectDescriptor::Object::DEI_ExtraEffects);
					od.descriptorElements.push_back(
						ObjectDescriptor::Object::DescriptorElement((new OverlayInfo::Elements::Text())
						->with_use_additional_scale(DESCRIPTOR_ELEMENT_ADDITIONAL_SCALE)
						->with_char_offset(Vector2(-3.0f, 0.5f))
						->with_horizontal_align(0.0f)
						->with_pulse(0.5f, 0.5f) // extra effects should be pulsating
						));
					an_assert(od.descriptorElements.get_size() == ObjectDescriptor::Object::DEI_Mix);
					od.startIndices.healthRules = od.descriptorElements.get_size();
					if (auto* imo = od.path.get_target())
					{
						if (auto* h = imo->get_custom<CustomModules::Health>())
						{
							for_count(int, hrIdx, h->get_rule_count())
							{
								od.descriptorElements.push_back(ObjectDescriptor::Object::DescriptorElement((new OverlayInfo::Elements::Text())).for_health_rule(hrIdx));
							}
						}
					}
					od.startIndices.icons = od.descriptorElements.get_size();
					if (auto* imo = od.path.get_target())
					{
						if (auto* iip = imo->get_custom<CustomModules::InvestigatorInfoProvider>())
						{
							for_count(int, idx, iip->get_info_count())
							{
								if (iip->has_icon(idx) &&
									iip->get_icon_at_socket(idx).is_set())
								{
									od.descriptorElements.push_back(ObjectDescriptor::Object::DescriptorElement((new OverlayInfo::Elements::Text())).for_icon(idx));
								}
							}
						}
					}
					od.startIndices.end = od.descriptorElements.get_size();
					od.investigatorInfoProviderCachedData.clear();
					if (auto* imo = od.path.get_target())
					{
						if (auto* iip = imo->get_custom<CustomModules::InvestigatorInfoProvider>())
						{
							od.investigatorInfoProviderCachedData.set_size(iip->get_info_count());
						}
					}

					for_every(ode, od.descriptorElements)
					{
						if (auto* e = fast_cast<OverlayInfo::Elements::Text>(ode->element.get()))
						{
							if (ode->healthRuleIdx.is_set())
							{
								int hrIdx = ode->healthRuleIdx.get();
								CustomVRPlacementForDescriptor info;
								if (auto* imo = od.path.get_target())
								{
									if (auto* h = imo->get_custom<CustomModules::Health>())
									{
										Optional<Name> atBone;
										Optional<Name> atSocket;
										if (h->get_rule_hit_info(hrIdx, OUT_ atBone, OUT_ atSocket))
										{
											if (atSocket.is_set())
											{
												if (auto* a = imo->get_appearance())
												{
													info.atSocketIdx = a->find_socket_index(atSocket.get());
												}
											}
											if (!info.atSocketIdx.is_set() && atBone.is_set())
											{
												if (auto* a = imo->get_appearance())
												{
													if (auto* as = a->get_skeleton())
													{
														if (auto *s = as->get_skeleton())
														{
															info.atBoneIdx = s->find_bone_index(atBone.get());
														}
													}
												}
											}
										}
									}
								}
								e
								->with_smaller_decimals()
								->with_scale(1.5f)
								->with_use_additional_scale(DESCRIPTOR_ELEMENT_ADDITIONAL_SCALE)
								->with_line_spacing(0.1f)
								->with_char_offset(Vector2(-0.5f, 0.0f))
								->with_horizontal_align(0.0f)
								->with_vertical_align(0.5f)
								->with_id(NAME(objectDescriptor))
								->with_usage(OverlayInfo::Usage::World)
								->with_custom_vr_placement([odref, this, info](OverlayInfo::System const* _system, OverlayInfo::Element const* _element)
									{
										return CustomVRPlacementForDescriptor::calculate(_system, _element, owner.get(), odref.get(), info);
									});
							}
							else if (ode->iconIdx.is_set())
							{
								int iIdx = ode->iconIdx.get();
								CustomVRPlacementForDescriptor info;
								String iconText;
								if (auto* imo = od.path.get_target())
								{
									if (auto* iip = imo->get_custom<CustomModules::InvestigatorInfoProvider>())
									{
										auto iipicon = iip->get_icon(iIdx);
										if (iipicon.is_set())
										{
											iconText = String::single(iipicon.get());
										}
										auto atSocket = iip->get_icon_at_socket(iIdx);
										if (atSocket.is_set() &&
											atSocket.get().is_valid())
										{
											info.atSocketIdx = atSocket.get().get_index();
										}
									}
								}
								e
								->with_text(iconText)
								->with_smaller_decimals()
								->with_scale(1.5f)
								->with_use_additional_scale(DESCRIPTOR_ELEMENT_ADDITIONAL_SCALE)
								->with_line_spacing(0.1f)
								->with_char_offset(Vector2(-0.5f, 0.0f))
								->with_horizontal_align(0.0f)
								->with_vertical_align(0.5f)
								->with_id(NAME(objectDescriptor))
								->with_usage(OverlayInfo::Usage::World)
								->with_custom_vr_placement([odref, this, info](OverlayInfo::System const* _system, OverlayInfo::Element const* _element)
									{
										return CustomVRPlacementForDescriptor::calculate(_system, _element, owner.get(), odref.get(), info);
									});
							}
							else
							{
								e
								->with_smaller_decimals()
								->with_scale(1.5f)
								->with_use_additional_scale(DESCRIPTOR_ELEMENT_ADDITIONAL_SCALE)
								->with_line_spacing(0.1f)
								->with_vertical_align(1.0f)
								->with_id(NAME(objectDescriptor))
								->with_usage(OverlayInfo::Usage::World)
								->with_custom_vr_placement([odref, this](OverlayInfo::System const* _system, OverlayInfo::Element const* _element)
									{
										return CustomVRPlacementForDescriptor::calculate(_system, _element, owner.get(), odref.get());
									});
							}
						}
					}

					objectDescriptor.timeToUpdate = 0.0f; // update next frame
				}
				else if (od.description == ObjectDescriptor::Object::Description::HealthDamage && od.collectedDamageShouldBeVisible)
				{
					RefCountObjectPtr<ObjectDescriptor::Object> odref(&od);

					auto* e = new OverlayInfo::Elements::Text();
					// create element that is placed where object is placed and shows info about investigation
					od.descriptorElements.push_back(
						ObjectDescriptor::Object::DescriptorElement((e)
						->with_smaller_decimals()
						->with_scale(2.0f)
						->with_use_additional_scale(DESCRIPTOR_ELEMENT_ADDITIONAL_SCALE)
						->with_line_spacing(0.1f)
						->with_horizontal_align(0.5f)
						->with_vertical_align(-0.5f)
						->with_id(NAME(objectDescriptor))
						->with_usage(OverlayInfo::Usage::World)
						->with_colour(colourInvestigateDamage)
						->with_custom_vr_placement([odref, this](OverlayInfo::System const* _system, OverlayInfo::Element const* _element)
							{
								return CustomVRPlacementForDescriptor::calculate(_system, _element, owner.get(), odref.get());
							})
					));

					objectDescriptor.timeToUpdate = 0.0f; // update next frame
				}
				else if (od.description == ObjectDescriptor::Object::Description::PickUp)
				{
					float letterSize = 0.025f;

					RefCountObjectPtr<ObjectDescriptor::Object> odref(&od);

					// create element that is placed where object is placed and shows "pick up"
					od.descriptorElements.push_back(
						ObjectDescriptor::Object::DescriptorElement((new OverlayInfo::Elements::Text())
						->with_text(LOC_STR(NAME(lsObjectDescriptorPickUp)))
						->with_letter_size(letterSize)
						->with_line_spacing(0.1f)
						->with_horizontal_align(0.5f)
						->with_vertical_align(0.5f)
						->with_id(NAME(objectDescriptor))
						->with_usage(OverlayInfo::Usage::World)
						->with_custom_vr_placement([odref, this](OverlayInfo::System const* _system, OverlayInfo::Element const* _element)
							{
								return CustomVRPlacementForDescriptor::calculate(_system, _element, owner.get(), odref.get());
							})
					));
				}
				else if (od.description == ObjectDescriptor::Object::Description::NoDoorMarker)
				{
					float letterSize = 0.025f;

					RefCountObjectPtr<ObjectDescriptor::Object> odref(&od);

					// create element that is placed where object is placed and shows "please wait"
					od.descriptorElements.push_back(
						ObjectDescriptor::Object::DescriptorElement((new OverlayInfo::Elements::Text())
						->with_text(LOC_STR(NAME(lsPleaseWait)))
						->with_letter_size(letterSize)
						->with_line_spacing(0.1f)
						->with_horizontal_align(0.5f)
						->with_vertical_align(0.5f)
						->with_id(NAME(objectDescriptor))
						->with_usage(OverlayInfo::Usage::World)
						->with_custom_vr_placement([odref, this](OverlayInfo::System const* _system, OverlayInfo::Element const* _element)
							{
								return CustomVRPlacementForDescriptor::calculate(_system, _element, owner.get(), odref.get());
							})
					));
				}
				for_every(ode, od.descriptorElements)
				{
					if (auto* e = ode->element.get())
					{
						if (auto* game = Game::get_as<Game>())
						{
							if (auto* overlayInfoSystem = game->access_overlay_info_system())
							{
								overlayInfoSystem->add(e);
							}
						}
					}
				}
			}
		}
	}
}

void PilgrimOverlayInfo::use_object_descriptor(Framework::IModulesOwner* o, ObjectDescriptor::Object::Description desc, ArrayStatic<Framework::DoorInRoom*, ObjectDescriptor::MAX_ROOM_DEPTH > const & _throughDoors)
{
	bool found = false;
	for_every_ref(od, objectDescriptor.objects)
	{
		if (od->path.get_target() == o)
		{
			if (od->description == ObjectDescriptor::Object::HealthDamage && desc == ObjectDescriptor::Object::HealthDamage)
			{
				// keep existing one
				od->valid = true;
				found = true;
				break;
			}
			if (desc == ObjectDescriptor::Object::HealthDamage)
			{
				// HealthDamage should not replace anything else
				continue;
			}
			if (od->description != ObjectDescriptor::Object::HealthDamage && od->description > desc)
			{
				// ignore this one, let the existing one be kept
				return;
			}
			if (od->description != desc)
			{
				od->description = desc;
				od->deactivate_descriptor_element();
			}
			od->valid = true;
			found = true;
			break;
		}
	}
	if (!found)
	{
		ObjectDescriptor::Object* od = ObjectDescriptor::Object::get_one();
		od->valid = true;
		od->description = desc;
		od->path.set_owner(owner.get());
		od->path.set_target(o);
		for_every_ptr(d, _throughDoors)
		{
			od->path.push_through_door(d);
		}
		od->unsafeIMOForReference = o;
		objectDescriptor.objects.push_back(RefCountObjectPtr<ObjectDescriptor::Object>(od));
	}
}

void PilgrimOverlayInfo::scan_room_object_descriptor(REF_ ScanForObjectDescriptorContext & _context, ScanForObjectDescriptorKeepData const& _keepData, Framework::Room* room, Transform const& _pilgrimViewPlacement, ArrayStatic<Framework::DoorInRoom*, ObjectDescriptor::MAX_ROOM_DEPTH > & _throughDoors, System::ViewFrustum const & _viewFrustum, Transform const & _thisRoomTransform)
{
	an_assert(room);
	Framework::DoorInRoom* enteredThroughDoor = !_throughDoors.is_empty() ? _throughDoors.get_last()->get_door_on_other_side() : nullptr;
	FOR_EVERY_OBJECT_IN_ROOM(o, room)
	{
		ObjectDescriptor::Object::Description desc = ObjectDescriptor::Object::Description::None;
		if (is_relevant_to_object_descriptor(REF_ _context, _keepData, o, _pilgrimViewPlacement, _throughDoors, OUT_ desc))
		{
			use_object_descriptor(o, desc, _throughDoors);
		}
	}
	if (_throughDoors.get_size() < _context.throughDoorsLimit)
	{
		FOR_EVERY_DOOR_IN_ROOM(door, room)
		{
			if (door != enteredThroughDoor && door->has_world_active_room_on_other_side() &&
				door->is_visible() &&
				door->can_see_through_it_now() &&
				door->get_placement().location_to_local(_pilgrimViewPlacement.get_translation()).y < 0.0f)
			{
				System::ViewFrustum doorVF;
				bool isVisible = true;
			
				{
					// only if still visible
					System::ViewFrustum wholeDoorVF;
					door->get_hole()->setup_frustum_placement(door->get_side(), door->get_hole_scale(), wholeDoorVF, _pilgrimViewPlacement, door->get_outbound_transform());

					if (_viewFrustum.is_empty())
					{
						doorVF = wholeDoorVF;
					}
					else
					{
						if (!doorVF.clip(&wholeDoorVF, &_viewFrustum))
						{
							isVisible = false;
						}
					}
				}

				if (isVisible)
				{
					_throughDoors.push_back(door);
					scan_room_object_descriptor(REF_ _context, _keepData, door->get_room_on_other_side(), door->get_other_room_transform().to_local(_pilgrimViewPlacement), _throughDoors, doorVF, _thisRoomTransform.to_world(door->get_other_room_transform()));
					_throughDoors.pop_back();
				}
			}
		}
	}
}

bool PilgrimOverlayInfo::is_relevant_to_object_descriptor(REF_ ScanForObjectDescriptorContext & _context, ScanForObjectDescriptorKeepData const& _keepData, Framework::IModulesOwner* imo, Transform const& _pilgrimViewPlacement, ArrayStatic<Framework::DoorInRoom*, ObjectDescriptor::MAX_ROOM_DEPTH >const& _throughDoors, OUT_ ObjectDescriptor::Object::Description& _description) const
{
	bool result = false;
	Transform imoPlacement = imo->get_presence()->get_placement();
	if (imoPlacement.get_scale() <= 0.0f)
	{
		return false;
	}
	if (auto* a = imo->get_appearance())
	{
		if (! a->is_visible())
		{
			return false;
		}
	}
	if (_context.lookForGuns ||
		_context.lookForPickups ||
		_context.lookForHealthDamage ||
		_context.lookForNoDoorMarkers)
	{
		Transform tgtPlacementOS = imo->get_presence()->get_centre_of_presence_os();
		Transform tgtPlacement = imoPlacement.to_world(tgtPlacementOS);
		Vector3 pilgrimToIMO = tgtPlacement.get_translation() - _pilgrimViewPlacement.get_translation();
		float dist = (pilgrimToIMO).length();
		if (_context.lookForGuns || 
			_context.lookForPickups)
		{
			if (_throughDoors.get_size() <= ObjectDescriptor::MaxRoomDepth::PickUp)
			{
				if (dist < 2.5f)
				{
					if (auto* p = imo->get_custom<CustomModules::Pickup>())
					{
						if (p->can_be_picked_up() &&
							!p->is_in_pocket() &&
							!p->is_held())
						{
							auto* g = imo->get_gameplay_as<ModuleEquipments::Gun>();
							if (_context.lookForGuns &&
								g && !g->is_main_equipment())
							{
								_description = ObjectDescriptor::Object::Description::PickUp;

								result = true;
							}
							if (_context.lookForPickups &&
								!g)
							{
								_description = ObjectDescriptor::Object::Description::PickUp;

								result = true;
							}
						}
					}
				}
			}
		}
		if (_context.lookForNoDoorMarkers)
		{
			if (_throughDoors.get_size() <= ObjectDescriptor::MaxRoomDepth::NoDoorMarkers)
			{
				if (dist < 2.5f)
				{
					if (auto* o = imo->get_as_object())
					{
						if (o->get_tags().get_tag(NAME(noDoorMarkerPending)))
						{
							_description = ObjectDescriptor::Object::Description::NoDoorMarker;

							result = true;
						}
					}
				}
			}
		}
		if (_context.lookForHealthDamage)
		{
			if (_throughDoors.get_size() <= ObjectDescriptor::MaxRoomDepth::HealthDamage)
			{
				if (auto* h = imo->get_custom<CustomModules::Health>())
				{
					_description = ObjectDescriptor::Object::Description::HealthDamage;
					result = true;
				}
			}
		}
		if (result)
		{
			AI::CastInfo castInfo;
			if (!AI::check_clear_ray_cast(castInfo, _context.originalPilgrimViewPlacement.get_translation(), owner.get(),
					imo, imo->get_presence()->get_in_room(), _context.originalPilgrimViewPlacement.to_world(_pilgrimViewPlacement.to_local(imoPlacement)), tgtPlacementOS))
			{
				result = false;
			}
		}
	}
	if (_context.lookForInvestigate &&
		imo->get_as_object())
	{
		float investigateTargetTag = imo->get_as_object()->get_tags().get_tag(NAME(investigateTarget));
		if (investigateTargetTag > 0.0f)
		{
			Transform invPlacementOS;
			if (auto* ai = fast_cast<ModuleAI>(imo->get_ai()))
			{
				invPlacementOS = ai->calculate_investigate_placement_os();
			}
			else
			{
				invPlacementOS = imo->get_presence()->get_centre_of_presence_os();
			}
			Transform invPlacement = imoPlacement.to_world(invPlacementOS);
			Vector3 imoVS = _pilgrimViewPlacement.location_to_local(invPlacement.get_translation());
			float off = (imoVS * Vector3(1.0f, 0.0f, 1.0f)).length();
			float distFwd = imoVS.y;
			float dist = imoVS.length();
			bool forceOk = false;
#ifdef SHOW_INVESTIGATE_LOCKS_TARGET
			if (currentlyShowing == Investigate &&
				investigateLogInfo.imo == imo)
			{
				forceOk = true;
			}
#endif

			if (distFwd > 0.0f || forceOk) // only if in front
			{
				float maxOffClose = 0.4f;
				float angThreshold = 45.0f;
				float dot = Vector3::dot(Vector3::yAxis, imoVS.normal());
				float ang = acos_deg(dot);
				float offNorm = off / maxOffClose;
				float angNorm = (1.0f - ang) / (1.0f - angThreshold);

				if (offNorm < 1.0f || angNorm < 1.0f || forceOk)
				{
					float match = 0.3f / max(0.5f, dist); // to make distance a bit less relevant
					match -= min(offNorm, angNorm) * 0.4f * (1.2f - 0.2f * investigateTargetTag); // make less important matches fall off quicker

					if (forceOk)
					{
						match = 1.0f;
					}

					if (!_keepData.investigate.chosen.get() || _keepData.investigate.chosen == imo || forceOk || match >= _keepData.investigate.bestMatch + 0.025f /* extra bit to avoid switching between two */)
					{
						if (!_context.investigate.chosen || match >= _context.investigate.bestMatch || forceOk)
						{
							AI::CastInfo castInfo;
							if (AI::check_clear_ray_cast(castInfo, _context.originalPilgrimViewPlacement.get_translation(), owner.get(),
									imo, imo->get_presence()->get_in_room(), _context.originalPilgrimViewPlacement.to_world(_pilgrimViewPlacement.to_local(imoPlacement)), invPlacementOS))
							{
								_context.investigate.chosen = imo;
								_context.investigate.chosenThroughDoors = _throughDoors;
								_context.investigate.bestMatch = match;
							}
						}
					}
				}
			}
		}
	}
	return result;
}

void PilgrimOverlayInfo::on_transform_vr_anchor(Transform const& _localToVRAnchorTransform)
{
	struct TransformVRAnchor
	{
		Transform localToVRAnchorTransform = Transform::identity;

		TransformVRAnchor(Transform const& _localToVRAnchorTransform)
		{
			localToVRAnchorTransform = _localToVRAnchorTransform;// .inverted();
		}
		void apply_to(REF_ Transform& _transform)
		{
			_transform = _transform.to_world(localToVRAnchorTransform);
			_transform.normalise_orientation();
		}
	} transformVRAnchor(_localToVRAnchorTransform);
	if (referencePlacement.is_set())
	{
		transformVRAnchor.apply_to(REF_ referencePlacement.access().vrAnchorPlacement);
	}
	if (disableWhenMovesTooFarFrom.is_set())
	{
		transformVRAnchor.apply_to(REF_ disableWhenMovesTooFarFrom.access());
	}
	if (disableMapWhenMovesTooFarFrom.is_set())
	{
		transformVRAnchor.apply_to(REF_ disableMapWhenMovesTooFarFrom.access());
	}
	{
		Concurrency::ScopedSpinLock lock(showInfosLock);
		for_every(si, showInfos)
		{
			transformVRAnchor.apply_to(REF_ si->at);
			if (si->hideWhenMovesTooFarFrom.is_set())
			{
				transformVRAnchor.apply_to(REF_ si->hideWhenMovesTooFarFrom.access());
			}
		}
	}
	for_every_ref(o, objectDescriptor.objects)
	{
		transformVRAnchor.apply_to(REF_ o->vrPlacement);
	}
}

//--

void PilgrimOverlayInfo::ObjectDescriptor::Object::deactivate_descriptor_element()
{
	for_every(ode, descriptorElements)
	{
		if (auto* e = ode->element.get())
		{
			e->deactivate();
		}
	}
	descriptorElements.clear();
}

void PilgrimOverlayInfo::ObjectDescriptor::Object::on_get()
{
	base::on_get();
	valid = true;
	shouldDeactivate = false;
	description = None;
}

void PilgrimOverlayInfo::ObjectDescriptor::Object::on_release()
{
	base::on_release();
	path.reset();
	deactivate_descriptor_element();
}

//--

void PilgrimOverlayInfo::InvestigateInfo::Icon::setup(Name const& _lsIcon, Name const& _info)
{
	if (_lsIcon.is_valid())
	{
		String iconStr = LOC_STR(_lsIcon);
		icon = iconStr.is_empty() ? ' ' : iconStr[0];
	}
	else
	{
		icon = ' ';
	}
	if (_info.is_valid())
	{
		info = AS_LOC_STR(_info);
	}
	else
	{
		info = Framework::LocalisedString();
	}
}
