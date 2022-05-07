//=============================================================================
//
// Adventure Game Studio (AGS)
//
// Copyright (C) 1999-2011 Chris Jones and 2011-20xx others
// The full list of copyright holders can be found in the Copyright.txt
// file, which is part of this source code distribution.
//
// The AGS source code is provided under the Artistic License 2.0.
// A copy of this license can be found in the file License.txt and at
// http://www.opensource.org/licenses/artistic-license-2.0.php
//
//=============================================================================
#include "ac/dynobj/cc_character.h"
#include "ac/characterinfo.h"
#include "ac/global_character.h"
#include "ac/gamesetupstruct.h"
#include "ac/game_version.h"
#include "script/cc_common.h"
#include "util/stream.h"

using namespace AGS::Common;

extern GameSetupStruct game;

// return the type name of the object
const char *CCCharacter::GetType() {
    return "Character";
}

size_t CCCharacter::CalcSerializeSize()
{
    return sizeof(int32_t);
}

void CCCharacter::Serialize(const char *address, Stream *out) {
    CharacterInfo *chaa = (CharacterInfo*)address;
    out->WriteInt32(chaa->index_id);
}

void CCCharacter::Unserialize(int index, Stream *in, size_t /*data_sz*/) {
    int num = in->ReadInt32();
    ccRegisterUnserializedObject(index, &game.chars[num], this);
}

uint8_t CCCharacter::ReadInt8(const char *address, intptr_t offset)
{
    // The only supported variable remaining in 3.4.*
    const int on_offset = 28 * sizeof(int32_t) + 301 * sizeof(int16_t) /* inventory */ + sizeof(int32_t) + 40 + 20;
    if (offset == on_offset)
        return ((CharacterInfo*)address)->on;
    cc_error("CCCharacter: unsupported variable offset %d", offset);
    return 0;
}

void CCCharacter::WriteInt8(const char *address, intptr_t offset, uint8_t val)
{
    // The only supported variable remaining in 3.4.*
    const int on_offset = 28 * sizeof(int32_t) + 301 * sizeof(int16_t) /* inventory */ + sizeof(int32_t) + 40 + 20;
    if (offset == on_offset)
        ((CharacterInfo*)address)->on = val;
    else
        cc_error("CCCharacter: unsupported variable offset %d", offset);
}

void CCCharacter::WriteInt16(const char *address, intptr_t offset, int16_t val)
{
    *(int16_t*)(address + offset) = val;

    // Detect when a game directly modifies the inventory, which causes the displayed
    // and actual inventory to diverge since 2.70. Force an update of the displayed
    // inventory for older games that reply on the previous behaviour.
    if (loaded_game_file_version < kGameVersion_270)
    {
        const int invoffset = 28 * sizeof(int32_t);
        if (offset >= invoffset && offset < (invoffset + MAX_INV * sizeof(short)))
        {
            update_invorder();
        }
    }
}
