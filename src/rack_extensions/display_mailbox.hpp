// Nonblocking single-producer/single-consumer display snapshots.
//
// Copyright 2026 Arhythmetic Units
//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
// You should have received a copy of the GNU General Public License
// along with this program.  If not, see <http://www.gnu.org/licenses/>.
//

#ifndef ARHYTHMETIC_UNITS_FOURIER_RACK_EXTENSIONS_DISPLAY_MAILBOX_HPP_
#define ARHYTHMETIC_UNITS_FOURIER_RACK_EXTENSIONS_DISPLAY_MAILBOX_HPP_

#include <array>
#include <atomic>

namespace Fourier {

/// @brief A latest-value mailbox with separate producer and consumer storage.
/// @details One engine producer and one UI consumer only. Each owns one slot;
/// an atomic exchange transfers the third slot. Release/acquire orders both
/// publication and reuse, so the producer cannot overwrite a held UI snapshot.
/// Publication never waits or allocates. Intermediate values may be superseded.
/// Construct before either thread uses it; destroy only after both have stopped.
template<typename T>
class DisplayMailbox {
    static_assert(ATOMIC_INT_LOCK_FREE == 2, "Display publication must be lock-free");
    std::array<T, 3> slots{};
    unsigned producer = 0;
    unsigned consumer = 1;
    std::atomic<unsigned> middle{2};
    static constexpr unsigned DIRTY = 4;

 public:
    /// @brief Engine-only storage for the next complete snapshot.
    T& writable() { return slots[producer]; }

    /// @brief Publish the complete write slot and acquire a reusable slot.
    void publish() {
        producer = middle.exchange(producer | DIRTY, std::memory_order_acq_rel) & ~DIRTY;
    }

    /// @brief UI-only snapshot retained by the consumer, even after widget recreation.
    /// @returns Storage valid until the next successful consume().
    const T& current() const { return slots[consumer]; }

    /// @brief UI-only: acquire the latest snapshot, or return null if unchanged.
    /// @returns Storage valid until the next successful consume() on this mailbox.
    const T* consume() {
        if (!(middle.load(std::memory_order_acquire) & DIRTY)) return nullptr;
        consumer = middle.exchange(consumer, std::memory_order_acq_rel) & ~DIRTY;
        return &slots[consumer];
    }
};

}  // namespace Fourier
#endif  // ARHYTHMETIC_UNITS_FOURIER_RACK_EXTENSIONS_DISPLAY_MAILBOX_HPP_
