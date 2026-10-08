#!/usr/bin/env python3
from pathlib import Path
import sys
R=Path(__file__).resolve().parents[1]
B=(R/'src/game/mpbots.c').read_text(); H=(R/'src/game/mpbots.h').read_text()
C=(R/'src/game/chr.c').read_text(); A=(R/'src/game/chraction.c').read_text()
M=(R/'src/game/mpmenu.c').read_text(); RDR=(R/'src/game/radar.c').read_text()
BC=(R/'src/bondconstants.h').read_text(); BV=(R/'src/game/bondview2.c').read_text(); LD=(R/'ge007.ld').read_text()
checks=[]
def ck(n,c):
    checks.append(bool(c)); print(('[PASS] ' if c else '[FAIL] ')+n)
ck('invented stuck strafe remains absent', 'navrecoveryside' not in B and 'navrecovery60' not in B)
ck('fixed 45 tick repath remains absent under STAN navigation', 'MODBOT_PD_NAV_REPATH_TICKS' not in B and 'runtime->navgoalstan != targetprop->stan' in B and 'waypointFindRoute(' not in B)
ck('PD ten-age door probe is present', '(runtime->navage % 10) != 0' in B and 'sub_GAME_7F0B1410' in B and 'doorsChooseSwingDirection' in B)
ck('bot death uses natural GE corpse lifecycle', 'MODBOT_RESPAWN_TICKS' not in B and 'modMpBotsQueueActorRemoval' not in B and 'chr->hidden |= CHRHIDDEN_REMOVE' not in B)
ck('bot model textures use reserved private upper pool',
   '&g_ModMpBotTexturePool' in B
   and '_modMpBotTextureCacheStart' in LD
   and '_modMpBotTextureCacheEnd - 0x10000' in LD
   and '== 0x10000' in LD)
ck('bots expose only live props to radar', 'modMpBotsGetBotProp' in B and 'chrIsDead(chr)' in B)
ck('bot action/model animation fulltick is split-screen safe even in fastpaths',
   'botMode' in C
   and C.count('if (!(coopMode || botMode) || coopFullTick)') >= 4
   and 'else if (botMode)' in C
   and 'if (botMode && !coopFullTick)' in C
   and 'tickamount = 0;' in C)
ck('bots bypass full guard argh reaction', 'modMpBotsGetSlotForChr(self) >= 0 && damageToCause > 0.0f' in A)
ck('bot rays include player props and use normal MP damage adapter',
   'CDTYPE_DOORS | botextracdtypes' in A
   and 'CDTYPE_PATHBLOCKER | botextracdtypes' in A
   and 'modMpBotsDamageViewer' in A
   and 'record_damage_kills(damage' in B)
ck('score renderer uses real char buffer', 'char text[16];' in M and 'sprintf(text, "%d", points);' in M and 'sprintf(&text' not in M)
ck('BOT SCORES enum/page/routing present',
   'MENU_BOT_SCORES' in BC and 'text = "BOT SCORES";' in M
   and 'modMpBotsGetBotKills(i)' in M
   and 'g_CurrentPlayer->mpmenumode = MENU_BOT_SCORES;' in M)
ck('normal SCORES remains the default watch opening page',
   'g_CurrentPlayer->mpmenumode = MENU_SCORES;' in M)
ck('one-human watch centering present', '(viGetViewWidth() - 160) >> 1' in M and '(viGetViewHeight() - 120) >> 1' in M)
ck('watch gameplay input is consumed', 'gamemode == GAMEMODE_MULTI && g_CurrentPlayer->mpmenuon' in BV)
ck('radar has PD-style second Simulant pass',
   'modMpBotsGetBotProp(i)' in RDR and 'for (i = 0; i < MOD_MP_BOT_MAX; i++)' in RDR
   and 'microcode_constructor_related_to_menus' in RDR)
ck('fixed 180-tick bot respawn timer remains absent',
   'MODBOT_RESPAWN_TICKS' not in B and 'respawntimer = 180' not in B)
passed=sum(checks)
print(f'\nR30 P9 SIMULANT INTEGRATION AUDIT: {"PASS" if passed==len(checks) else "FAIL"} ({passed}/{len(checks)})')
sys.exit(0 if passed==len(checks) else 1)
