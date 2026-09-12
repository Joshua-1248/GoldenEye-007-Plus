#include <ultra64.h>
#include "macro.h"
#include "ob.h"
#include <deb.h>
#include <memp.h>
#include <assets/obseg/obseg.h>
#include "decompress.h"
#include "indy_comms.h"
#include "assets/obseg/file_resource_id_enums.h"

//bss
//800888b0

 resource_lookup_data_entry resource_lookup_data_array[OBJ_INDEX_MAX];


// data
//D:80046050
s32 ob_c_debug_notice_list_entry = 0;


#include <assets/obseg/file_resource_table.inc.c>
/* struct fileentry file_resource_table[] =
   {
       blah;
    };
 */


//D:800482D4
s32 file_entry_max = OBJ_INDEX_END;

#ifdef GE_PHYSICAL_FASTPATHS
/*
 * R11: resource names in the retail table are immutable, yet fileGetIndex()
 * linearly strcmp's up to ~870 entries for every named load.  Stage loading
 * and the front end perform many such lookups.  Keep an open-addressed name
 * index in Expansion Pak scratch and retain the original linear path as an
 * exact fallback for debug/host-added resources.
 */
#define RESOURCE_NAME_HASH_SLOTS 2048
#define RESOURCE_NAME_HASH_MASK (RESOURCE_NAME_HASH_SLOTS - 1)
#define RESOURCE_NAME_HASH_MAGIC 0x47455248U /* "GERH" */
extern u8 _resourceNameHashStart[];

static u32 resourceNameHashValue(const u8 *name)
{
    u32 hash = 5381;

    while (*name)
    {
        hash = ((hash << 5) + hash) ^ *name++;
    }

    return hash;
}

static u16 *resourceNameHashSlots(void)
{
    return (u16 *)(_resourceNameHashStart + 4);
}

static void resourceNameHashInsert(s32 index)
{
    u16 *slots = resourceNameHashSlots();
    u32 slot;
    u32 probes;
    u8 *name = file_resource_table[index].filename;

    if (name == NULL)
    {
        return;
    }

    slot = resourceNameHashValue(name) & RESOURCE_NAME_HASH_MASK;

    for (probes = 0; probes < RESOURCE_NAME_HASH_SLOTS; probes++)
    {
        if (slots[slot] == 0 || slots[slot] == index)
        {
            slots[slot] = index;
            return;
        }

        slot = (slot + 1) & RESOURCE_NAME_HASH_MASK;
    }
}

static s32 resourceNameHashLookup(const u8 *name)
{
    u16 *slots;
    u32 slot;
    u32 probes;

    if (*(u32 *)_resourceNameHashStart != RESOURCE_NAME_HASH_MAGIC)
    {
        return 0;
    }

    slots = resourceNameHashSlots();
    slot = resourceNameHashValue(name) & RESOURCE_NAME_HASH_MASK;

    for (probes = 0; probes < RESOURCE_NAME_HASH_SLOTS; probes++)
    {
        s32 index = slots[slot];

        if (index == 0)
        {
            return 0;
        }

        if (file_resource_table[index].filename != NULL &&
            strcmp(name, file_resource_table[index].filename) == 0)
        {
            return index;
        }

        slot = (slot + 1) & RESOURCE_NAME_HASH_MASK;
    }

    return 0;
}
#endif



void load_resource(u8 *ptrdata, s32 bytes,  fileentry *srcfile,  resource_lookup_data_entry *lookupdata)
{
    u8 *source;
    u8  buffer[0x2100];
    s32 unused;


    if (bytes == 0)
    {
        romCopy(ptrdata, srcfile->hw_address, lookupdata->rom_size);
        return;
    }
    source = (ptrdata + bytes) - ((lookupdata->rom_size + 7) & -8);
    if ((u32) (source - ptrdata) < 8U)
    {
        lookupdata->poolRemaining = 0;
    }
    else
    {
#if DEBUG
        char sp54[128];
        u32  stack[2];
#endif

        romCopy(source, srcfile->hw_address, lookupdata->rom_size);
        lookupdata->poolRemaining = decompressdata(source, ptrdata, buffer);;
#if DEBUG
        if (result == 0)
        {
            sprintf(sp54, "DMA-Crash %s %d Ram: %02x%02x%02x%02x%02x%02x%02x%02x", "ob.c", 204, scratch[0], scratch[1], scratch[2], scratch[3], scratch[4], scratch[5], scratch[6], scratch[7]);
            crashSetMessage(sp54);
            CRASH();
        }
#endif

    }
}

void resource_load_from_indy(u8 *ptrdata, s32 bytes,  fileentry *srcfile,  resource_lookup_data_entry *lookupdata)
{
    u8 *pPayload;
    u8 buffer[8448];
    s32 size;
    static const u8 rz_header_1[] = {0x11, 0x72, 0x00, 0x00};
    static const u8 rz_header_2[] = {0x11, 0x72, 0x00, 0x00};

    if (!bytes)
    {
        indycommHostLoadFile(srcfile->filename, ptrdata);
        return;
    }
    indycommHostCheckFileExists(srcfile->filename, &lookupdata->pc_size);
    pPayload = (ptrdata + bytes) - ((lookupdata->pc_size + 7) & -8);
    if ((pPayload - ptrdata) < 8U)
    {
        lookupdata->poolRemaining = 0;
    }
    else
    {
        indycommHostLoadFile(srcfile->filename, pPayload);
        if ((pPayload[0] == rz_header_1[0]) && (pPayload[1] == rz_header_2[1]))
        {
            size = decompressdata(pPayload, ptrdata, &buffer);
        }
        else
        {
            bcopy(pPayload, ptrdata, lookupdata->pc_size);
            size = lookupdata->pc_size;
        }
        lookupdata->poolRemaining = size;
    }
}





void obInit(void)
{
    s32 size;
    s32 i;

    debTryAdd(&ob_c_debug_notice_list_entry, "ob_c_debug");
    
    for (i = 1; i < file_entry_max - 1; i++)
    {
        size = (file_resource_table[i + 1].hw_address - file_resource_table[i].hw_address);

        resource_lookup_data_array[i].rom_size = size;
        resource_lookup_data_array[i].poolRemaining = 0;
        resource_lookup_data_array[i].pc_size = 0;
        resource_lookup_data_array[i].rom_remaining = 0;

        if (size);
    }
#ifdef GE_PHYSICAL_FASTPATHS
    {
        u16 *slots = resourceNameHashSlots();

        for (i = 0; i < RESOURCE_NAME_HASH_SLOTS; i++)
        {
            slots[i] = 0;
        }

        *(u32 *)_resourceNameHashStart = RESOURCE_NAME_HASH_MAGIC;

        for (i = 1; i < file_entry_max; i++)
        {
            resourceNameHashInsert(i);
        }
    }
#endif
}


#if !defined(LEFTOVERDEBUG)
/* VERSION_EU */
/* same as below version, shuffled in EU */
void obLoadBGFileBytesAtOffset(u8 *bgname, u8 *target, s32 offset, s32 len)
{
  s32 index;
   fileentry *fileentry;

  index = fileGetIndex(bgname);
  fileentry = &file_resource_table[index];

  if (resource_lookup_data_array[index].rom_size != 0)
  {
    //if the size of offset data would exceed file size, loop forever
    if ((resource_lookup_data_array[index].rom_size + 0xF) < (offset + len))
    {
      while (1){};
    }
    romCopy(target, &fileentry->hw_address[offset], len, fileentry);
  }

}
#endif

#if defined(LEFTOVERDEBUG)
/* no VERSION_EU */
void *_fileIndexLoadToBank(s32 index, FILELOADMETHOD param_2, s32 size, u8 bank)
{
    return fileIndexLoadToBank(index, param_2, size, bank);
}
#endif

#if defined(LEFTOVERDEBUG)
/* no VERSION_EU */
void *_fileIndexLoadToAddr(int index, FILELOADMETHOD param_2, u8 *ptrdata, int size)
{
    return fileIndexLoadToAddr(index, param_2, ptrdata, size);
}
#endif

void *_fileNameLoadToBank(char *filename, FILELOADMETHOD loadMethod, s32 size, u8 bank)
{
    return fileIndexLoadToBank(fileGetIndex(filename), loadMethod, size, bank);
}

void * _fileNameLoadToAddr(char *filename, FILELOADMETHOD loadMethod, u8 *ptrdata, s32 size)
{
    return fileIndexLoadToAddr(fileGetIndex(filename), loadMethod, ptrdata, size);
}

#if defined(LEFTOVERDEBUG)
/* no VERSION_EU */
/**
 * 0F18AC 7F0BCD7C
 * loads data stored at an offset of a bg file
 */
void obLoadBGFileBytesAtOffset(u8 *bgname, u8 *target, s32 offset, s32 len)
{
  s32 index;
   fileentry *fileentry;

  index = fileGetIndex(bgname);
  fileentry = &file_resource_table[index];

  if (resource_lookup_data_array[index].rom_size != 0)
  {
    //if the size of offset data would exceed file size, loop forever
    if ((resource_lookup_data_array[index].rom_size + 0xF) < (offset + len))
    {
      while (1){};
    }
    romCopy(target, &fileentry->hw_address[offset], len, fileentry);
  }

}
#endif





void *fileIndexLoadToBank(s32 index, FILELOADMETHOD loadMethod, s32 size, u8 bank) //#MATCH https://decomp.me/scratch/uqiBe
{
    resource_lookup_data_entry *info = &resource_lookup_data_array[index];
    s32                         bytes;
    void                       *ptrdata = NULL;

    if (loadMethod == FILELOADMETHOD_EXTRAMEM || loadMethod == FILELOADMETHOD_DEFAULT || loadMethod == 2)
    {
        // bytes = info->poolRemaining;
        if (info->poolRemaining == 0)
        { // verify pool remaining is 0
            info->poolRemaining = mempGetBankSizeLeft(bank);
            // info->poolRemaining = bytes;
        }
        // bytes = info->poolRemaining;
        ptrdata             = mempAllocBytesInBank(info->poolRemaining, bank); // get pointer to allocated space in bank
        info->rom_remaining = info->poolRemaining;

        if (file_resource_table[index].hw_address == 0) //IF NULL, check indy
        {
            resource_load_from_indy(ptrdata, info->poolRemaining, &file_resource_table[index], info);
        }
        else
        {
            load_resource(ptrdata, info->poolRemaining, &file_resource_table[index], info);
        }
        if (loadMethod != FILELOADMETHOD_EXTRAMEM)
        {
            // mempRealloc
            mempAddEntryOfSizeToBank(ptrdata, info->poolRemaining, bank);
        }
    }
    else // skipped in PD
    {
        if (info->poolRemaining == 0)
        {
            if (info->rom_size != 0)
            {
                info->poolRemaining = info->rom_size;
            }
            else
            {
                info->poolRemaining = info->pc_size;
            }
        }
        ptrdata             = mempAllocBytesInBank(info->poolRemaining, bank);
        info->rom_remaining = info->poolRemaining;

        if (file_resource_table[index].hw_address == 0)
        {
            resource_load_from_indy(ptrdata, 0, &file_resource_table[index], info);
        }
        else
        {
            load_resource(ptrdata, 0, &file_resource_table[index], info);
        }
        if (size == 0)
        {
            info->loaded_bank = bank;
        }
    }
    return ptrdata;
}





void *fileIndexLoadToAddr(s32 index, FILELOADMETHOD loadMethod, void *ptrdata, s32 bytes) //#match https://decomp.me/scratch/YExRh
{
    resource_lookup_data_entry *info = &resource_lookup_data_array[index];

    if (!info->poolRemaining)
    {
        if (info->rom_size)
        {
            info->poolRemaining = info->rom_size;
        }
        else
        {
            info->poolRemaining = info->pc_size;
        }
    }
    if (loadMethod == FILELOADMETHOD_EXTRAMEM || loadMethod == FILELOADMETHOD_DEFAULT || loadMethod == 2)
    {
        if (!file_resource_table[index].hw_address)
        {
            info->rom_remaining = bytes;
            resource_load_from_indy(ptrdata, bytes, &file_resource_table[index], &resource_lookup_data_array[index]);
        }
        else
        {
            info->rom_remaining = bytes;
            //fix a1/a0 inversion by manual pointer "info"
            load_resource(ptrdata, bytes, &file_resource_table[index], &resource_lookup_data_array[index]);
        }
    }
    else
    {
        if (!file_resource_table[index].hw_address)
        {
            resource_load_from_indy(ptrdata, 0, &file_resource_table[index], &resource_lookup_data_array[index]);
        }
        else
        {
            load_resource(ptrdata, 0, &file_resource_table[index], &resource_lookup_data_array[index]);
        }
    }

    return ptrdata;
}






s32 get_pc_remaining_buffer_for_index(s32 index)
{
    return resource_lookup_data_array[index].poolRemaining;
}


s32 get_rom_remaining_buffer_for_index(s32 index)
{
    return resource_lookup_data_array[index].rom_remaining;
}


void fileSetSize(s32 filenum, u8* ptr, u32 size, s32 reallocate)
{
    resource_lookup_data_array[filenum].poolRemaining = size;
    resource_lookup_data_array[filenum].rom_remaining = size;
    if (reallocate != 0)
    {
        mempAddEntryOfSizeToBank(ptr, resource_lookup_data_array[filenum].poolRemaining, MEMPOOL_STAGE);
    }
}


s32 get_pc_buffer_remaining_value(u8 *name)
{
    int index;

    index = fileGetIndex(name);
    return resource_lookup_data_array[index].poolRemaining;
}


void obBlankResourcesLoadedInBank(u8 bank)
{
    int i;
    for (i = 1; i < file_entry_max; i++) {
        if (resource_lookup_data_array[i].loaded_bank <= bank) {
            resource_lookup_data_array[i].loaded_bank = '\0';
        }
        if (bank == 4) {
            resource_lookup_data_array[i].poolRemaining = 0;
        }
    }
}

void obBlankResourcesInBank5(void) {
  obBlankResourcesLoadedInBank(MEMPOOL_ME);
}




s32 fileGetIndex(u8 *resname)
{
    s32 i;
    s32 stack;
    s32 size;
    struct resource_lookup_data_entry *lookup;

#ifdef GE_PHYSICAL_FASTPATHS
    i = resourceNameHashLookup(resname);

    if (i != 0)
    {
        return i;
    }
#endif

    for (i = 1; i < file_entry_max; i++)
    {
        if (file_resource_table[i].filename != NULL)
        {
            if (strcmp(resname, file_resource_table[i].filename) == 0)
            {
                return i;
            }
        }
    }
    
    i = file_entry_max;
    
    //too many files exist
    if (i >= OBJ_INDEX_MAX)
    {
        return 0;
    }
    
    file_entry_max++;

    if (indycommHostCheckFileExists(resname, &size) == 0)
    {
        return 0;
    }

    file_resource_table[i].index = i; // offset 0
    file_resource_table[i].filename = resname;  // offset 4

    lookup = &resource_lookup_data_array[i];
    
    if(1);

    lookup->unk_11 = 0;
    file_resource_table[i].hw_address = 0;  // offset 8
    lookup->rom_size = 0;
    lookup->poolRemaining = 0;
    lookup->pc_size = ALIGN16_a(size);
    lookup->rom_remaining = 0;
    lookup->loaded_bank = 0;
#ifdef GE_PHYSICAL_FASTPATHS
    resourceNameHashInsert(i);
#endif

    return i;
}





void removed_handle_filetable_entry(u32 index)
{
    return;
}

void removed_loop_handle_filetable_entries(void)
{
    int i;
    for (i = 1; (i < file_entry_max); i++)
    {
        removed_handle_filetable_entry(i);
    }
}

void removed_loop_filetableentries(void)
{
    int i;

    for (i = 1; (i < file_entry_max); i++)
    {
        ;
    }
}




// removed
void sub_GAME_7F0BD410(void)
{
    s32 i;

    for (i = 1; i < file_entry_max; i++)
    {
        // unknown which property is referenced, just need to get the compiler to
        // correctly reference the parent object.
        if (resource_lookup_data_array[i].poolRemaining)
        {
            // removed
        }    
    }
}



