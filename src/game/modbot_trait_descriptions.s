/* Traits presentation strings: game .text in Expansion Pak RDRAM.
 * This intentionally does NOT place these long messages in LTITLE's boot-time
 * MEMPOOL_PERMANENT allocation or in the fixed-size compressed CDATA slot.
 * The table is data (not executed) and is referenced by front.c. */
    .section .text
    .balign 4
    .globl g_ModMpBotTraitDescriptions
    .type g_ModMpBotTraitDescriptions, @object
g_ModMpBotTraitDescriptions:
    .word .Lbottrait_pacifist
    .word .Lbottrait_vindictive
    .word .Lbottrait_psychotic
    .word .Lbottrait_juggernaut
    .word .Lbottrait_armor_specialist
    .word .Lbottrait_pyromaniac
    .word .Lbottrait_bully
    .word .Lbottrait_fearful
    .word .Lbottrait_equalizer
    .word .Lbottrait_stalker
    .word .Lbottrait_melee
    .word .Lbottrait_speedy
    .word .Lbottrait_cheap
    .word .Lbottrait_opportunist

.Lbottrait_pacifist:
    .asciz "Prioritizes collecting as many weapons & ammo as possible to keep other players from getting them."
.Lbottrait_vindictive:
    .asciz "When killed by another player, a bot with this trait will seek out that player to kill them."
.Lbottrait_psychotic:
    .asciz "Prioritizes merciless extermination by any means necessary. This also means a complete disregard for it's own life & will even use explosives at point blank range to eliminate their target."
.Lbottrait_juggernaut:
    .asciz "Carries ballistic plate armor that is much more durable than standard body armor at the cost of moving much more slowly due to the weight concentration."
.Lbottrait_armor_specialist:
    .asciz "Starts with Body Armor every time & will prioritize keeping it's defenses up as much as possible."
.Lbottrait_pyromaniac:
    .asciz "Prefers to use weapons that cause explosions."
.Lbottrait_bully:
    .asciz "Goes after players weaker than itself."
.Lbottrait_fearful:
    .asciz "Runs away from players carrying strong weapons."
.Lbottrait_equalizer:
    .asciz "Balances the playing field so other players can score easier by going after the player with the highest score."
.Lbottrait_stalker:
    .asciz "Picks a random player in the game & actively pursues that player mercilessly."
.Lbottrait_melee:
    .asciz "Never uses weapons other than it's melee attacks."
.Lbottrait_speedy:
    .asciz "Moves at two times the speed."
.Lbottrait_cheap:
    .asciz "Cheats without any consequences to it. The ways it cheats are starting armed & moving at twice the normal speed."
.Lbottrait_opportunist:
    .asciz "Attacks targets with their backs turned away from the bot. If their target goes to face them, they will begin evading them while still attacking."
    .balign 4
