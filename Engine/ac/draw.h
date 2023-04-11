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
#ifndef __AGS_EE_AC__DRAW_H
#define __AGS_EE_AC__DRAW_H

#include <memory>
#include "core/types.h"
#include "ac/common_defines.h"
#include "gfx/bitmap.h"
#include "gfx/gfx_def.h"
#include "game/roomstruct.h"

namespace AGS
{
    namespace Common
    {
        typedef std::shared_ptr<Common::Bitmap> PBitmap;
    }
    namespace Engine { class IDriverDependantBitmap; }
}
using namespace AGS; // FIXME later

#define IS_ANTIALIAS_SPRITES usetup.enable_antialiasing && (play.disable_antialiasing == 0)

// Converts AGS color index to the actual bitmap color using game's color depth
int MakeColor(int color_index);

class Viewport;
class Camera;

// Initializes drawing methods and optimisation
void init_draw_method();
// Initializes global game drawing resources
void init_game_drawdata();
// Initializes drawing resources upon entering new room
void init_room_drawdata();
// Disposes resources related to the current drawing methods
void dispose_draw_method();
// Disposes global game drawing resources
void dispose_game_drawdata();
// Disposes any temporary resources on leaving current room
void dispose_room_drawdata();
// Releases all the cached textures of game objects
void clear_drawobj_cache();
// Releases all the textures used as render targets, if necessary;
// (this is primarily for resetting display mode of certain renderers).
void release_drawobj_rendertargets();
// Updates drawing settings depending on main viewport's size and position on screen
void on_mainviewport_changed();
// Notifies that a new room viewport was created
void on_roomviewport_created(int index);
// Notifies that a new room viewport was deleted
void on_roomviewport_deleted(int index);
// Updates drawing settings if room viewport's position or size has changed
void on_roomviewport_changed(Viewport *view);
// Detects overlapping viewports, starting from the given index in z-sorted array
void detect_roomviewport_overlaps(size_t z_index);
// Updates drawing settings if room camera's size has changed
void on_roomcamera_changed(Camera *cam);
// Marks particular object as need to update the texture
void mark_object_changed(int objid);
// Resets all object caches which reference this sprite
void reset_objcache_for_sprite(int sprnum, bool deleted);

// whether there are currently remnants of a DisplaySpeech
void mark_screen_dirty();
bool is_screen_dirty();

// marks whole screen as needing a redraw
void invalidate_screen();
// marks all the camera frame as needing a redraw
void invalidate_camera_frame(int index);
// marks certain rectangle on screen as needing a redraw
// in_room flag tells how to interpret the coordinates: as in-room coords or screen viewport coordinates.
void invalidate_rect(int x1, int y1, int x2, int y2, bool in_room);

void mark_current_background_dirty();

// Avoid freeing and reallocating the memory if possible
Common::Bitmap *recycle_bitmap(Common::Bitmap *bimp, int coldep, int wid, int hit, bool make_transparent = false);
void recycle_bitmap(std::unique_ptr<Common::Bitmap> &bimp, int coldep, int wid, int hit, bool make_transparent = false);
Engine::IDriverDependantBitmap* recycle_ddb_sprite(Engine::IDriverDependantBitmap *ddb, uint32_t sprite_id,
    Common::Bitmap *source, bool opaque = false);
inline Engine::IDriverDependantBitmap* recycle_ddb_bitmap(Engine::IDriverDependantBitmap *ddb,
    Common::Bitmap *source, bool opaque = false)
    { return recycle_ddb_sprite(ddb, UINT32_MAX, source, opaque); }
inline Engine::IDriverDependantBitmap* recycle_render_target(Engine::IDriverDependantBitmap *ddb, int width, int height, int col_depth, bool opaque = false);
// Draw everything 
void render_graphics(Engine::IDriverDependantBitmap *extraBitmap = nullptr, int extraX = 0, int extraY = 0);
// Construct game scene, scheduling drawing list for the renderer
void construct_game_scene(bool full_redraw = false);
// Construct final game screen elements; updates and draws mouse cursor
void construct_game_screen_overlay(bool draw_mouse = true);
// Construct engine overlay with debugging tools (fps, console)
void construct_engine_overlay();

void debug_draw_room_mask(RoomAreaMask mask);
void debug_draw_movelist(int charnum);
void update_room_debug();
void tint_image (Common::Bitmap *g, Common::Bitmap *source, int red, int grn, int blu, int light_level, int luminance=255);
void draw_sprite_support_alpha(Common::Bitmap *ds, int xpos, int ypos, Common::Bitmap *image,
                               Common::BlendMode blend_mode = Common::kBlend_Normal, int alpha = 0xFF);
void draw_sprite_slot_support_alpha(Common::Bitmap *ds, int xpos, int ypos, int src_slot,
                                    Common::BlendMode blend_mode = Common::kBlend_Normal, int alpha = 0xFF);
// CLNUP I'd like to put the default parameters to draw_gui_sprite, but the extern from guiman.h prevents it
void draw_gui_sprite(Common::Bitmap *ds, int pic, int x, int y, Common::BlendMode blend_mode);
void draw_gui_sprite(Common::Bitmap *ds, int xpos, int ypos,
    Common::Bitmap *image, Common::BlendMode blend_mode = Common::kBlend_Normal, int alpha = 0xFF);
void draw_gui_sprite_flipped(Common::Bitmap *ds, int pic, int x, int y, Common::BlendMode blend_mode, bool is_flipped);
void draw_gui_sprite_flipped(Common::Bitmap *ds, int xpos, int ypos,
    Common::Bitmap *image, Common::BlendMode blend_mode = Common::kBlend_Normal, int alpha = 0xFF, bool is_flipped = false);

// Render game on screen
void render_to_screen();
// Callbacks for the graphics driver
void draw_game_screen_callback();
void GfxDriverOnInitCallback(void *data);
bool GfxDriverSpriteEvtCallback(int evt, int data);
void putpixel_compensate (Common::Bitmap *g, int xx,int yy, int col);
// Create the actsps[aa] image with the object drawn correctly.
// Returns true if nothing at all has changed and actsps is still
// intact from last time; false otherwise.
// Hardware-accelerated do not require altering the raw bitmap itself,
// so they only detect whether the sprite ID itself has changed.
// Software renderers modify the cached bitmap whenever any visual
// effect changes (scaling, tint, etc).
// * alwaysUseSoftware option forces HW renderers to  construct the image
// in software mode as well.
bool construct_object_gfx(int aa, int *drawnWidth, int *drawnHeight, bool alwaysUseSoftware);
// Returns a cached character image prepared for the render
Common::Bitmap *get_cached_character_image(int charid);
// Returns a cached object image prepared for the render
Common::Bitmap *get_cached_object_image(int objid);
// Adds a walk-behind sprite to the list for the given slot
// (reuses existing texture if possible)
void add_walkbehind_image(size_t index, Common::Bitmap *bmp, int x, int y);
// Clears black game borders in legacy letterbox mode (CLNUP?)
void clear_letterbox_borders();

void draw_and_invalidate_text(Common::Bitmap *ds, int x1, int y1, int font, color_t text_color, const char *text);

void setpal();

// This function converts game coordinates coming from script to the actual game resolution.
extern AGS_INLINE void defgame_to_finalgame_coords(int &x, int &y);
// Creates bitmap of a format compatible with the gfxdriver;
// if col_depth is 0, uses game's native color depth.
Common::Bitmap *CreateCompatBitmap(int width, int height, int col_depth = 0);
// Checks if the bitmap is compatible with the gfxdriver;
// returns same bitmap or its copy of a compatible format.
Common::Bitmap *ReplaceBitmapWithSupportedFormat(Common::Bitmap *bitmap);
// Checks if the bitmap needs any kind of adjustments before it may be used
// in AGS sprite operations. Also handles number of certain special cases
// (old systems or uncommon gfx modes, and similar stuff).
// Original bitmap **gets deleted** if a new bitmap had to be created.
Common::Bitmap *PrepareSpriteForUse(Common::Bitmap *bitmap);
// Same as above, but compatible for std::shared_ptr.
Common::PBitmap PrepareSpriteForUse(Common::PBitmap bitmap);
// Makes a screenshot corresponding to the last screen render and returns it as a bitmap
// of the requested width and height and game's native color depth.
Common::Bitmap *CopyScreenIntoBitmap(int width, int height, bool at_native_res = false);

#endif // __AGS_EE_AC__DRAW_H
