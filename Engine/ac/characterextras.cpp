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
#include "ac/characterextras.h"
#include "ac/viewframe.h"
#include "util/stream.h"

using AGS::Common::Stream;

int CharacterExtras::GetFrameSoundVolume(CharacterInfo *chi) const
{
    return ::CalcFrameSoundVolume(
        anim_volume, cur_anim_volume,
        (chi->flags & CHF_SCALEVOLUME) ? zoom : 100);
}

void CharacterExtras::CheckViewFrame(CharacterInfo *chi)
{
    ::CheckViewFrame(chi->view, chi->loop, chi->frame, GetFrameSoundVolume(chi));
}

void CharacterExtras::ReadFromSavegame(Stream *in, int save_ver)
{
    in->ReadArrayOfInt16(invorder, MAX_INVORDER);
    invorder_count = in->ReadInt16();
    width = in->ReadInt16();
    height = in->ReadInt16();
    zoom = in->ReadInt16();
    xwas = in->ReadInt16();
    ywas = in->ReadInt16();
    tint_r = in->ReadInt16();
    tint_g = in->ReadInt16();
    tint_b = in->ReadInt16();
    tint_level = in->ReadInt16();
    tint_light = in->ReadInt16();
    process_idle_this_time = in->ReadInt8();
    slow_move_counter = in->ReadInt8();
    animwait = in->ReadInt16();
    if (save_ver >= 2) // expanded at ver 2
    {
        anim_volume = static_cast<uint8_t>(in->ReadInt8());
        cur_anim_volume = static_cast<uint8_t>(in->ReadInt8());
        in->ReadInt8(); // reserved to fill int32
        in->ReadInt8();
    }
}

void CharacterExtras::WriteToSavegame(Stream *out)
{
    out->WriteArrayOfInt16(invorder, MAX_INVORDER);
    out->WriteInt16(invorder_count);
    out->WriteInt16(width);
    out->WriteInt16(height);
    out->WriteInt16(zoom);
    out->WriteInt16(xwas);
    out->WriteInt16(ywas);
    out->WriteInt16(tint_r);
    out->WriteInt16(tint_g);
    out->WriteInt16(tint_b);
    out->WriteInt16(tint_level);
    out->WriteInt16(tint_light);
    out->WriteInt8(process_idle_this_time);
    out->WriteInt8(slow_move_counter);
    out->WriteInt16(animwait);
    out->WriteInt8(static_cast<uint8_t>(anim_volume));
    out->WriteInt8(static_cast<uint8_t>(cur_anim_volume));
    out->WriteInt8(0); // reserved to fill int32
    out->WriteInt8(0);
}
