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
#ifndef __AC_MOVE_H
#define __AC_MOVE_H
#include <allegro.h> // fixed math
#include "game/savegame.h"

// Forward declaration
namespace AGS { namespace Common { class Stream; } }
using namespace AGS; // FIXME later

#define MAXNEEDSTAGES 256
#define MAXNEEDSTAGES_LEGACY 40

struct MoveList {
    int   pos[MAXNEEDSTAGES]{};
    int   numstage = 0;
    fixed xpermove[MAXNEEDSTAGES]{}, ypermove[MAXNEEDSTAGES]{};
    int   fromx = 0, fromy = 0;
    int   onstage = 0, onpart = 0;
    int   lastx = 0, lasty = 0;
    char  doneflag = 0;
    char  direct = 0;  // MoveCharDirect was used or not

    void ReadFromFile_Legacy(Common::Stream *in);
    AGS::Engine::HSaveError ReadFromFile(Common::Stream *in, int32_t cmp_ver);
    void WriteToFile(Common::Stream *out);
};

#endif // __AC_MOVE_H
