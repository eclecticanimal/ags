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
#include "core/platform.h"
#include "ac/spritecache.h"
#include "ac/gamestructdefines.h"
#include "debug/out.h"
#include "gfx/bitmap.h"

using namespace AGS::Common;

// Tells that the sprite is found in the game resources.
#define SPRCACHEFLAG_ISASSET        0x01
// Tells that the sprite is assigned externally and cannot be autodisposed.
#define SPRCACHEFLAG_EXTERNAL       0x02
// Tells that the sprite index was remapped to the placeholder (sprite 0).
#define SPRCACHEFLAG_REMAP0         0x04
// Locked sprites are ones that should not be freed when out of cache space.
#define SPRCACHEFLAG_LOCKED         0x08

// High-verbosity sprite cache log
#if DEBUG_SPRITECACHE
#define SprCacheLog(...) Debug::Printf(kDbgGroup_SprCache, kDbgMsg_Debug, __VA_ARGS__)
#else
#define SprCacheLog(...)
#endif


namespace AGS
{
namespace Common
{

SpriteCache::SpriteCache(std::vector<SpriteInfo> &sprInfos, const Callbacks &callbacks)
    : ResourceCache(DEFAULTCACHESIZE_KB * 1024u)
    , _sprInfos(sprInfos)
{
    _callbacks.AdjustSize = (callbacks.AdjustSize) ? callbacks.AdjustSize : DummyAdjustSize;
    _callbacks.InitSprite = (callbacks.InitSprite) ? callbacks.InitSprite : DummyInitSprite;
    _callbacks.PostInitSprite = (callbacks.PostInitSprite) ? callbacks.PostInitSprite : DummyPostInitSprite;
    _callbacks.PrewriteSprite = (callbacks.PrewriteSprite) ? callbacks.PrewriteSprite : DummyPrewriteSprite;
}

SpriteCache::~SpriteCache()
{
    Reset();
}

size_t SpriteCache::GetSpriteSlotCount() const
{
    return _spriteData.size();
}

bool SpriteCache::HasFreeSlots() const
{
    return !((_spriteData.size() == SIZE_MAX) || (_spriteData.size() > MAX_SPRITE_INDEX));
}

bool SpriteCache::IsAssetSprite(sprkey_t index) const
{
    return index >= 0 && (size_t)index < _spriteData.size() && // in the valid range
        _spriteData[index].IsAssetSprite(); // found in the game resources
}

void SpriteCache::Reset()
{
    _file.Close();
    ResourceCache::Clear();
    _spriteData.clear();
}

bool SpriteCache::SetSprite(sprkey_t index, std::unique_ptr<Bitmap> image, int flags)
{
    assert(index >= 0); // out of positive range indexes are valid to fail
    assert(image);
    if (index < 0 || EnlargeTo(index) != index)
    {
        Debug::Printf(kDbgGroup_SprCache, kDbgMsg_Error, "SetSprite: unable to use index %d", index);
        return false;
    }
    if (!image)
    {
        Debug::Printf(kDbgGroup_SprCache, kDbgMsg_Error, "SetSprite: attempt to assign nullptr to index %d", index);
        return false;
    }

    const int spf_flags = flags
        | (SPF_HICOLOR * image->GetColorDepth() > 8)
        | (SPF_TRUECOLOR * image->GetColorDepth() > 16);
    _sprInfos[index] = SpriteInfo(image->GetWidth(), image->GetHeight(), spf_flags);
    _spriteData[index].Flags = SPRCACHEFLAG_EXTERNAL | SPRCACHEFLAG_LOCKED; // NOT from asset file
    Put(index, std::move(image), kCacheItem_External | kCacheItem_Locked);
    SprCacheLog("SetSprite: (external) %d", index);
    return true;
}

void SpriteCache::SetEmptySprite(sprkey_t index, bool as_asset)
{
    assert(index >= 0); // out of positive range indexes are valid to fail
    if (index < 0 || EnlargeTo(index) != index)
    {
        Debug::Printf(kDbgGroup_SprCache, kDbgMsg_Error, "SetEmptySprite: unable to use index %d", index);
        return;
    }
    ResourceCache::Dispose(index); // make sure it's free
    if (as_asset)
        _spriteData[index].Flags = SPRCACHEFLAG_ISASSET;
    RemapSpriteToSprite0(index);
}

Bitmap *SpriteCache::RemoveSprite(sprkey_t index)
{
    assert(index >= 0); // out of positive range indexes are valid to fail
    if (index < 0 || (size_t)index >= _spriteData.size())
        return nullptr;
    std::unique_ptr<Bitmap> image = ResourceCache::Remove(index);
    InitNullSprite(index);
    SprCacheLog("RemoveSprite: %d", index);
    return image.release();
}

void SpriteCache::DisposeSprite(sprkey_t index)
{
    assert(index >= 0); // out of positive range indexes are valid to fail
    if (index < 0 || (size_t)index >= _spriteData.size())
        return;
    ResourceCache::Dispose(index);
    InitNullSprite(index);
    SprCacheLog("RemoveAndDispose: %d", index);
}

sprkey_t SpriteCache::EnlargeTo(sprkey_t topmost)
{
    assert(topmost >= 0);
    if (topmost < 0 || topmost > MAX_SPRITE_INDEX)
        return -1;
    if ((size_t)topmost < _spriteData.size())
        return topmost;

    size_t newsize = topmost + 1;
    _sprInfos.resize(newsize);
    _spriteData.resize(newsize);
    return topmost;
}

sprkey_t SpriteCache::GetFreeIndex()
{
    // FIXME: inefficient if large number of sprites were created in game;
    // use "available ids" stack, see managed pool for an example;
    // IMPORTANT: must keep in mind that SpriteCache's interface allows
    // to set any arbitrary sprite ID with SetSprite and SetEmptySprite!
    for (size_t i = MIN_SPRITE_INDEX; i < _spriteData.size(); ++i)
    {
        // slot empty
        if (!DoesSpriteExist(i))
        {
            _sprInfos[i] = SpriteInfo();
            _spriteData[i] = SpriteData();
            return i;
        }
    }
    // enlarge the sprite bank to find a free slot and return the first new free slot
    return EnlargeTo(_spriteData.size());
}

bool SpriteCache::SpriteData::IsAssetSprite() const
{
    return (Flags & SPRCACHEFLAG_ISASSET) != 0; // found in game resources
}

bool SpriteCache::SpriteData::IsRemapped() const
{
    return (Flags & SPRCACHEFLAG_REMAP0) != 0; // was remapped to placeholder (sprite 0)
}

bool SpriteCache::SpriteData::IsExternalSprite() const
{
    return (Flags & SPRCACHEFLAG_EXTERNAL) != 0; // assigned externally
}

bool SpriteCache::SpriteData::IsLocked() const
{
    return (Flags & SPRCACHEFLAG_LOCKED) != 0;
}

bool SpriteCache::DoesSpriteExist(sprkey_t index) const
{
    return index >= 0 && (size_t)index < _spriteData.size() && // in the valid range
        _spriteData[index].IsValid(); // has assigned sprite
}

Size SpriteCache::GetSpriteResolution(sprkey_t index) const
{
    return DoesSpriteExist(index) ? _sprInfos[index].GetResolution() : Size();
}

Bitmap *SpriteCache::operator [] (sprkey_t index)
{
    // invalid sprite slot
    assert(index >= 0); // out of positive range indexes are valid to fail
    if (index < 0 || (size_t)index >= _spriteData.size())
        return nullptr;

    // Resolve potentially remapped sprites
    index = GetDataIndex(index);
    // Try get image from cache
    auto &image = ResourceCache::Get(index);
    if (image)
        return image.get();
    // If no ready image, but has an asset, then try loading one
    if (_spriteData[index].IsAssetSprite())
        return LoadSprite(index);
    return nullptr;
}

void SpriteCache::DisposeAllCached()
{
    ResourceCache::DisposeFreeItems();
}

void SpriteCache::Precache(sprkey_t index)
{
    assert(index >= 0); // out of positive range indexes are valid to fail
    if (index < 0 || (size_t)index >= _spriteData.size())
        return;
    if (!_spriteData[index].IsAssetSprite())
        return; // cannot precache a non-asset sprite

    if (!ResourceCache::Exists(index))
        LoadSprite(index);

    // make sure locked sprites can't fill the cache
    ResourceCache::Lock(index);
    _spriteData[index].Flags |= SPRCACHEFLAG_LOCKED;
    SprCacheLog("Precached %d", index);
}

sprkey_t SpriteCache::GetDataIndex(sprkey_t index)
{
    assert((index >= 0) && ((size_t)index < _spriteData.size()));
    return (_spriteData[index].Flags & SPRCACHEFLAG_REMAP0) == 0 ? index : 0;
}

size_t SpriteCache::CalcSize(const std::unique_ptr<Bitmap> &item)
{
    assert(item);
    return item ? (item->GetWidth() * item->GetHeight() * item->GetBPP()) : 0u;
}

Bitmap *SpriteCache::LoadSprite(sprkey_t index)
{
    assert((index >= 0) && ((size_t)index < _spriteData.size()));
    if (index < 0 || (size_t)index >= _spriteData.size())
        return nullptr;
    assert((_spriteData[index].Flags & SPRCACHEFLAG_ISASSET) != 0);

    Bitmap *image{};
    HError err = _file.LoadSprite(index, image);
    if (!image)
    {
        Debug::Printf(kDbgGroup_SprCache, kDbgMsg_Warn,
            "LoadSprite: failed to load sprite %d:\n%s\n - remapping to sprite 0.", index,
            err ? "Sprite does not exist." : err->FullMessage().GetCStr());
        RemapSpriteToSprite0(index);
        return nullptr;
    }

    // Let the external user convert this sprite's image for their needs
    image = _callbacks.InitSprite(index, image, _sprInfos[index].Flags);
    if (!image)
    {
        Debug::Printf(kDbgGroup_SprCache, kDbgMsg_Warn,
            "LoadSprite: failed to initialize sprite %d, remapping to sprite 0.", index);
        RemapSpriteToSprite0(index);
        return nullptr;
    }

    // save the stored sprite info
    _sprInfos[index].Width = image->GetWidth();
    _sprInfos[index].Height = image->GetHeight();

    // Add to the cache
    ResourceCache::Put(index, std::unique_ptr<Bitmap>(image));
    _spriteData[index].Flags = SPRCACHEFLAG_ISASSET;
    if (index == 0) // keep sprite 0 locked
        _spriteData[index].Flags |= SPRCACHEFLAG_LOCKED;
    SprCacheLog("Loaded %d, size now %zu KB", index, _cacheSize / 1024);

    // Let the external user to react to the new sprite;
    // note that this callback is allowed to modify the sprite's pixels,
    // but not its size or flags.
    _callbacks.PostInitSprite(index);
    return image;
}

void SpriteCache::RemapSpriteToSprite0(sprkey_t index)
{
    assert((index > 0) && ((size_t)index < _spriteData.size()));
    if (index <= 0)
        return; // don't remap sprite 0 to itself
    _sprInfos[index] = _sprInfos[0];
    _spriteData[index].Flags |= SPRCACHEFLAG_REMAP0;
    SprCacheLog("RemapSpriteToSprite0: %d", index);
}

void SpriteCache::InitNullSprite(sprkey_t index)
{
    assert(index >= 0);
    _sprInfos[index] = SpriteInfo();
    _spriteData[index] = SpriteData();
}

int SpriteCache::SaveToFile(const String &filename, int store_flags, SpriteCompression compress, SpriteFileIndex &index)
{
    // Gather a list of sprites;
    // the list contains pairs, where first element tells whether the sprites
    // exists at all (either have a ready image, or found in a input file).
    // SaveSpriteFile will either use a ready image or load missing images
    // before saving to the destination.
    std::vector<std::pair<bool, Bitmap*>> sprites;
    for (size_t i = 0; i < _spriteData.size(); ++i)
    {
        auto &image = ResourceCache::Get(i);
        if (image) // optionally convert a sprite's pixel data for the saving
            _callbacks.PrewriteSprite(image.get());
        sprites.push_back(std::make_pair(
            (image || _spriteData[i].IsAssetSprite()),
            image.get()));
    }
    return SaveSpriteFile(filename, sprites, &_file, store_flags, compress, index);
}

HError SpriteCache::InitFile(const String &filename, const String &sprindex_filename)
{
    Reset();

    std::vector<Size> metrics;
    HError err = _file.OpenFile(filename, sprindex_filename, metrics);
    if (!err)
        return err;

    // Initialize sprite infos
    size_t newsize = metrics.size();
    _sprInfos.resize(newsize);
    _spriteData.resize(newsize);
    for (size_t i = 0; i < metrics.size(); ++i)
    {
        if (!metrics[i].IsNull())
        {
            // Existing sprite
            _spriteData[i].Flags = SPRCACHEFLAG_ISASSET;
            Size newsz = _callbacks.AdjustSize(Size(metrics[i].Width, metrics[i].Height), _sprInfos[i].Flags);
            _sprInfos[i].Width = newsz.Width;
            _sprInfos[i].Height = newsz.Height;
        }
        else
        {
            // Mark as empty slot
            InitNullSprite(i);
        }
    }
    return HError::None();
}

void SpriteCache::DetachFile()
{
    _file.Close();
}

} // namespace Common
} // namespace AGS
