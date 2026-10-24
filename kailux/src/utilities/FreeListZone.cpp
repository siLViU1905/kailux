#include "FreeListZone.h"

#include <algorithm>
#include <stdexcept>

namespace kailux
{
    FreeListZone FreeListZone::create(vk::DeviceSize base, vk::DeviceSize capacity)
    {
        FreeListZone zone;
        zone.mBase = base;
        zone.mCapacity = capacity;
        zone.mFreeBlocks.emplace_back(base, capacity);
        return zone;
    }

    vk::DeviceSize FreeListZone::Alloc(vk::DeviceSize size, vk::DeviceSize alignment)
    {
        if (const auto offset{TryAlloc(size, alignment)})
            return *offset;
        throw std::runtime_error{"Asset zone out of memory"};
    }

    std::optional<vk::DeviceSize> FreeListZone::TryAlloc(vk::DeviceSize size, vk::DeviceSize alignment)
    {
        for (auto it{mFreeBlocks.begin()}; it != mFreeBlocks.end(); ++it)
        {
            const auto aligned{((it->offset + alignment - 1ul) / alignment) * alignment};
            const auto padding{aligned - it->offset};

            if (it->size >= size + padding)
            {
                if (aligned < mBase
                    || (aligned + size) > (mBase + mCapacity))
                    return std::nullopt;

                const auto offset{aligned};

                const auto remaining{it->size - size - padding};
                if (remaining > 64)
                {
                    const Block leftover{aligned + size, remaining};
                    mFreeBlocks.insert(std::next(it), leftover);
                }

                if (padding > 0)
                    it->size = padding;
                else
                    mFreeBlocks.erase(it);

                mUsedBlocks.emplace_back(offset, size);
                return offset;
            }
        }
        return std::nullopt;
    }

    bool FreeListZone::Free(vk::DeviceSize offset)
    {
        const auto it{std::ranges::find_if(
            mUsedBlocks,
            [offset](const auto& block)
            {
                return block.offset == offset;
            }
            )};

        if (it == mUsedBlocks.end())
            return false;

        const Block freed{*it};
        mUsedBlocks.erase(it);

        const auto pos{std::ranges::lower_bound(
            mFreeBlocks,
            freed.offset,
            {},
            &Block::offset
            )};

        const auto inserted{mFreeBlocks.insert(pos, freed)};
        const auto next{std::next(inserted)};
        if (next != mFreeBlocks.end()
            && inserted->offset + inserted->size == next->offset)
        {
            inserted->size += next->size;
            mFreeBlocks.erase(next);
        }

        if (inserted != mFreeBlocks.begin())
        {
            const auto prev{std::prev(inserted)};
            if (prev->offset + prev->size == inserted->offset)
            {
                prev->size += inserted->size;
                mFreeBlocks.erase(inserted);
            }
        }
        return true;
    }
}
