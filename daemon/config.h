#pragma once

#include <cstdint>

// These offsets are highly variable depending on the exact UE4 version and Game (Tencent ACE).
// You must discover and update these offsets using IDA Pro or a memory dumper!

namespace UE4Config {
    // Offset from the start of the UWorld object to the TArray<AAudioVolume*>
    // This is typically a very reliable structural anchor.
    constexpr uint32_t AUDIO_VOLUMES_OFFSET = 0x2A8;

    // Offset to the floating-point time keeping value (TimeSeconds)
    // Should be > 0.0f and actively increasing during normal gameplay.
    constexpr uint32_t TIME_SECONDS_OFFSET = 0x140;

    // Offset to the ULevel* PersistentLevel
    constexpr uint32_t PERSISTENT_LEVEL_OFFSET = 0x30;

    // Heuristics bounds
    constexpr uint32_t MAX_AUDIO_VOLUMES_COUNT = 500;
}
