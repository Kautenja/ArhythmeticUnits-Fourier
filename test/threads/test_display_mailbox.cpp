// Standalone ownership and concurrency checks for display publication.
// Copyright 2026 Arhythmetic Units
// SPDX-License-Identifier: GPL-3.0-or-later
#include <array>
#include <atomic>
#include <thread>
#include "rack_extensions/display_mailbox.hpp"
#include "catch_amalgamated.hpp"

TEST_CASE("Display mailbox retains the latest complete publication without overwriting a reader") {
    struct Packet { std::array<unsigned, 64> values{}; };
    Fourier::DisplayMailbox<Packet> mailbox;
    CHECK(mailbox.consume() == nullptr);
    mailbox.writable().values.fill(1);
    mailbox.publish();
    const auto held = mailbox.consume();
    REQUIRE(held);
    for (unsigned i = 2; i < 20; ++i) {
        mailbox.writable().values.fill(i);
        mailbox.publish();
    }
    CHECK(held->values.front() == 1);
    CHECK(held->values.back() == 1);
    REQUIRE(mailbox.consume()->values.front() == 19);
    CHECK(mailbox.consume() == nullptr);
}

TEST_CASE("Display mailbox publishes coherent snapshots while the reader holds a slot") {
    struct Packet { std::array<unsigned, 64> values{}; };
    Fourier::DisplayMailbox<Packet> mailbox;
    std::atomic<bool> start{false};
    std::atomic<bool> done{false};
    std::thread producer([&]() {
        while (!start.load(std::memory_order_acquire)) std::this_thread::yield();
        for (unsigned i = 1; i <= 100000; ++i) {
            mailbox.writable().values.fill(i);
            mailbox.publish();
        }
        done.store(true, std::memory_order_release);
    });
    unsigned last = 0;
    start.store(true, std::memory_order_release);
    bool consistent = true;
    // Always inspect the currently owned storage, including while the producer
    // replaces pending snapshots. Reading only new slots can miss reuse races.
    do {
        mailbox.consume();
        const auto* packet = &mailbox.current();
        const unsigned value = packet->values.front();
        consistent = consistent && value >= last;
        for (auto element : packet->values) consistent = consistent && element == value;
        last = value;
        std::this_thread::yield();
    } while (!done.load(std::memory_order_acquire));
    producer.join();
    // Drain after the producer stops; do not spin forever if publication breaks.
    mailbox.consume();
    CHECK(consistent);
    CHECK(mailbox.current().values.front() == 100000);
    CHECK(mailbox.current().values.back() == 100000);
    CHECK(mailbox.consume() == nullptr);
}
