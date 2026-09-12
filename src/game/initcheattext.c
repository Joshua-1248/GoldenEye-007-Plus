#include <ultra64.h>
#include "initcheattext.h"
#include "player.h"
#include "cheat.h"

void initCheatTextBuffer(void) {
    int i;
    
    for(i=0;i<75;i++)
    {
        g_CheatPlayerTextRelated[i] = 0;
    }

}


void disableOnscreenCheatText(void)
{
  g_CurrentPlayer->cheatInputBufferIndex = 0;
  g_CurrentPlayer->cheatInputCount = 0;
}

