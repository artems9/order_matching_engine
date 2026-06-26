#pragma once

#include "Types.hpp"

#include <algorithm>
#include <cstddef>
#include <cassert>

// MANAGES WHERE ORDERS LIVE IN MEMORY
template <std::size_t PoolCapacity>
class OrderPool {

public:
    // Initializes a free-list over a raw memory buffer.
    // Each slot in the buffer is treated as a FreeSlot node.
    OrderPool() {
        for (std::size_t slotN = 0; slotN < PoolCapacity; slotN++) {

            // Compute address of the N-th slot inside the raw memory pool
            auto slot = reinterpret_cast<FreeSlot*>(
                pool_ + (slotN * maxSlotSize)
            );

            // Insert this slot into the front of the free list
            slot->next = head_;
            head_ = slot;
        }

        // After construction, head_ points to the first available free slot
    }

    ~OrderPool() = default;

    // Pool owns raw memory; copying would duplicate internal state incorrectly.
    OrderPool(const OrderPool&) = delete;
    OrderPool& operator=(const OrderPool&) = delete;

    // Moving is disabled because raw pointers would become invalid or ambiguous.
    OrderPool(OrderPool&&) = delete;
    OrderPool& operator=(OrderPool&&) = delete;

    // Allocates a slot from the free list and returns it as an Order object.
    // The returned memory is uninitialized — caller must construct Order in-place.
    // one slot leaves front, head moves right
    Order* allocateSlot() {
        // Compiled out in release build
        assert(head_ && "OrderPool exhausted");

        // Take the first available slot from the free list
        FreeSlot* freeSlot = head_;
        head_ = head_->next;

        // Reinterpret the raw slot memory as an Order storage location
        return reinterpret_cast<Order*>(freeSlot);
    }

    // Returns a previously allocated slot back to the free list.
    // Caller must ensure the Order object has been destroyed first.
    // one slot joins front, head moves left
    void deallocateSlot(Order* ptr) {
        // Compiled out in release build
        assert(ptr && "Can't deallocate slot at nullptr");

        // Treat the memory as a FreeSlot node again
        auto* slot = reinterpret_cast<FreeSlot*>(ptr);

        // Push slot back onto free list
        slot->next = head_;
        head_ = slot;
    }

private:
    // Internal node used only when a slot is free.
    // When allocated, the same memory is treated as an Order.
    struct FreeSlot {
        FreeSlot* next;
    };

    // Size of each slot in the pool:
    // large enough to hold either an Order or a FreeSlot.
    static constexpr std::size_t maxSlotSize =
        std::max(sizeof(Order), sizeof(FreeSlot));

    // Raw storage buffer for all slots.
    // This memory is not constructed; it is manually managed.
    // Alignment depends on the largest member, not the total size.
    alignas(std::max(alignof(Order), alignof(FreeSlot))) std::byte pool_[
        PoolCapacity * maxSlotSize];

    // Head of the singly-linked free list.
    FreeSlot* head_ = nullptr;
};