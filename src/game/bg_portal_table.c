#include <ultra64.h>
#include "bg.h"

/* EU needs this separate object to preserve the BSS layout. */
#ifdef VERSION_EU
struct unk_portalstruct table_for_portals[PORTMAX];
#endif
