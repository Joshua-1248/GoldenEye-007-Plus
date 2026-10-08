#!/usr/bin/env python3
from pathlib import Path
import sys

root=Path(__file__).resolve().parents[1]

def read(p): return (root/p).read_text(errors='replace')
BOT=read('src/game/mpbots.c')
BOTH=read('src/game/mpbots.h')
FRONT=read('src/game/front.c')
MP=read('src/game/mpmenu.c')
AI=read('src/game/chrai.c')
CHR=read('src/game/chr.c')
CHRPROP=read('src/game/chrprop.c')
ACT=read('src/game/chraction.c')
TITLE=read('assets/obseg/text/LtitleE.c')

checks=[]
def ck(name, cond):
    checks.append((name,bool(cond)))

ck('Simulants cannot enter guard AI interpreter',
   'modMpBotsGetSlotForChr(ChrEntityp) >= 0' in AI and 'return;' in AI[AI.find('modMpBotsGetSlotForChr(ChrEntityp) >= 0'):AI.find('modMpBotsGetSlotForChr(ChrEntityp) >= 0')+100])
ck('bot shared/full tick gates split-screen animation/action',
   'botMode' in CHR and 'if (botMode && !coopFullTick)' in CHR and 'if (coopFullTick)\n                    chrlvActionTick(chr);' in CHR)
ck('spawn height snaps to STAN floor',
   'spawnpos.y = stanGetPositionYValue(pad->stan, spawnpos.x, spawnpos.z);' in BOT)
ck('corpse free notification drives delayed respawn',
   'modMpBotsNotifyPropFreed(prop);' in CHRPROP and 'runtime->respawntimer = runtime->deathrecorded ? -1 : 0;' in BOT and 'runtime->respawntimer < 0' in BOT)
ck('one-human score page reads only P1 in dedicated branch',
   'if (player_count == 1)' in MP and 'scores[0], current_colour' in MP)
ck('true one-player Co-Op scenario is allowed',
   'return playercount >= 1 && playercount <= 4;' in FRONT)
ck('generic six difficulty names are exact',
   '"Very Easy", "Easy", "Normal", "Hard", "Very Hard", "Extreme"' in BOT)
ck('Extreme aim row is zero-error while Very Hard keeps PerfectSim settle',
   '{ 0, MODBOT_PD_DTOR(0.0f),   MODBOT_PD_DTOR(2.0f),   45' in BOT and '{ 0, MODBOT_PD_DTOR(0.0f),   MODBOT_PD_DTOR(0.0f),    0' in BOT)
ck('default bot names are Bot 1 through Bot 8',
   all(f'"Bot {i}"' in BOT for i in range(1,9)))
ck('all 13 independent trait bits exist',
   'MODBOT_TRAIT_COUNT          = 13' in BOTH and 'MODBOT_TRAIT_CHEAP          = 1 << 12' in BOTH)
ck('Cheap alone supplies automatic starting weapon',
   'if (config->traits & MODBOT_TRAIT_CHEAP)\n        weaponindex = modMpBotsChooseWeaponIndex(slot);' in BOT)
ck('Speedy and Cheap stack with no cap',
   'if (config->traits & MODBOT_TRAIT_SPEEDY) speed *= 2.0f;' in BOT
   and 'if (config->traits & MODBOT_TRAIT_CHEAP) speed *= 2.0f;' in BOT
   and 'Traits intentionally stack without a cap.' in BOT)
ck('Juggernaut uses PD TurtleSim-relative slowdown',
   'if (config->traits & MODBOT_TRAIT_JUGGERNAUT) speed *= (3.5f / 7.6f);' in BOT)
ck('Armor Specialist actively selects armour pickup targets',
   'modMpBotsPdFindArmorTarget' in BOT and 'moveprop = modMpBotsPdFindArmorTarget(slot);' in BOT)
ck('targeting traits combine rather than exclusive else-if',
   'Each targeting trait casts one vote' in BOT and '++votes[participant]' in BOT)
ck('trait submenu has bottom descriptions and exact Cheap text',
   'TITLE_STR_BOTTRAIT_PACIFIST_DESC+g_ModMpBotTraitChoice' in FRONT and 'Cheats without any consequences to it. Uses unfair tactics such as starting armed and moving at twice the normal speed.' in TITLE)
ck('Rename Bot supports Z mode cycle, B delete, L/R caret and D-pad-only grid',
   'g_ModMpBotRenameMode=(g_ModMpBotRenameMode+1)%3' in FRONT and 'if(g_ModMpBotConfigs[g_ModMpBotEditSlot].name[0]) modMpBotRenameDelete();' in FRONT and 'if(pressed&L_TRIG)' in FRONT and 'if(pressed&R_TRIG)' in FRONT and 'U_JPAD|D_JPAD|L_JPAD|R_JPAD' in FRONT)
ck('rename keyboard begins lowercase and has 26 reachable symbol/number keys',
   'return "abcdefghijklmnopqrstuvwxyz";' in FRONT and 'return "0123456789.,!?-+=/\\\\:;()@&#";' in FRONT)
ck('bot character selector uses separate crosshair arrows without D-pad cycling',
   'Character arrows are separate crosshair targets; no D-pad cycling.' in FRONT and 'modMpBotCycleCharacter(g_ModMpBotEditSlot,-1)' in FRONT and 'modMpBotCycleCharacter(g_ModMpBotEditSlot,1)' in FRONT)
ck('P1 analog-right paths use direct positive stick threshold',
   'joyGetStickX(0) > 30' in FRONT and 'joyGetStickX(i) > 30' in FRONT)
ck('bot local audio includes hit, reload, swap/pickup and armour',
   'BOND_GET_HIT1_SFX' in BOT and 'GUN_RIFLECOCK_SFX' in BOT and 'modMpBotsPlayLocalSfx' in BOT and 'ARMOUR_COLLECT_SFX' in BOT)
ck('bot melee matches GE player reach and miss sound',
   'return runtime->weaponnum == ITEM_SNIPERRIFLE ? 100.0f : 50.0f;' in BOT
   and 'modMpBotsPlayLocalSfx(runtime->chr, PUNCHING_AIR_SFX);' in BOT)
ck('bot damage adds gender-appropriate local vocalization',
   'modMpBotsPlayLocalSfx(victim, BOND_GET_HIT1_SFX);' in BOT and 'play_sound_for_shot_actor(victim);' in BOT)
ck('door probe extends through STAN steering portal',
   'endx += gx * (220.0f / len);' in BOT and 'endz += gz * (220.0f / len);' in BOT
   and 'scan->type != PROP_TYPE_DOOR' in BOT and 'along > 280.0f' in BOT)
ck('Co-Op bots are enabled and use hostile-guard ally targeting',
   'modMpBotsCoopChooseTarget' in BOT and 'chrCoopIsEscortCandidate(candidate)' in BOT
   and 'scenario != SCENARIO_COOP' not in BOT[BOT.find('static s32 modMpBotsRuntimeAllowed'):BOT.find('s32 modMpBotsGetCount')])
ck('Personality Traits uses two-column no-scroll cheat layout',
   'x=i<7?0x37:0xdc' in FRONT and 'g_ModMpBotTraitScroll+visible' not in FRONT)
ck('Rename D-pad draws menu highlight without moving crosshair',
   'g_ModMpBotRenameDpad=TRUE;' in FRONT and 'microcode_constructor_related_to_menus(DL,hx1,hy1,hx2,hy2,0x32)' in FRONT
   and 'cursor_h_pos=88.0f+g_ModMpBotRenameCol*60.0f' not in FRONT)

ck('crosshair highlights bot character arrows',
   'cursor_h_pos>=226.0f && cursor_h_pos<250.0f' in FRONT and 'DL=microcode_constructor_related_to_menus(DL,0xe1,y-1,0xf5,y+0xe,0x32);' in FRONT
   and 'DL=microcode_constructor_related_to_menus(DL,0x16a,y-1,0x17e,y+0xe,0x32);' in FRONT)
ck('Personality Traits row has no decorative arrow',
   'else value=(char*)">";' not in FRONT)
ck('empty Rename Bot B restores entry name and returns',
   'g_ModMpBotRenameOriginal' in FRONT and 'strcpy(g_ModMpBotConfigs[g_ModMpBotEditSlot].name,g_ModMpBotRenameOriginal); frontChangeMenu(MENU_MP_BOT_EDIT,FALSE); return;' in FRONT)
ck('Rename Bot crosshair also draws key highlight',
   'g_ModMpBotRenameDpad || (g_ModMpBotRenameRow<5' in FRONT and 'cursor_h_pos>=hx1 && cursor_h_pos<hx2' in FRONT)


ck('Simulants use the human damage grace table and reject damage during grace',
   'damagegracetimer60' in BOT and 'g_DamageTypes[damagetype].field_0x8' in BOT
   and 'g_DamageTypes[damagetype].flashEndFrame' in BOT
   and 'modMpBotsCanTakeDamage(self)' in ACT)
ck('dead Simulants cannot emit damage audio',
   'victim->actiontype == ACT_DIE || victim->actiontype == ACT_DEAD' in BOT
   and ACT.find('!modMpBotsCanTakeDamage(self)') < ACT.find('modMpBotsNotifyHumanHit(self'))
ck('explosion damage uses Simulant pain audio but death launch is cleared',
   'modMpBotsOnDamageAccepted(self);' in ACT
   and 'self->fallspeed.f[0] = 0.0f;' in ACT
   and 'self->fallspeed.f[1] = 0.0f;' in ACT
   and 'self->fallspeed.f[2] = 0.0f;' in ACT)
ck('bot melee range is horizontal like player melee',
   'modMpBotsPdMeleeDistanceSq' in BOT
   and 'return dx * dx + dz * dz;' in BOT
   and 'modMpBotsPdMeleeDistanceSq(runtime, target) > reach * reach' in BOT)
ck('Co-Op follow uses dedicated close-follow navigation instead of combat standoff',
   'Following a teammate is not combat range management.' in BOT
   and '(180.0f * 180.0f)' in BOT
   and 'modMpBotsPdNavTick(slot, followprop);' in BOT)
ck('Co-Op close-follow still probes doors',
   'modMpBotsPdCheckDoor(runtime, &followprop->pos);' in BOT)

failed=[n for n,ok in checks if not ok]
for n,ok in checks:
    print(f"[{'PASS' if ok else 'FAIL'}] {n}")
print(f"\nR30 P12/P13 SIMULANT FEATURE AUDIT: {'PASS' if not failed else 'FAIL'} ({len(checks)-len(failed)}/{len(checks)})")
if failed: sys.exit(1)
