#pragma once

namespace scratch {

struct adopt_lock_t { explicit adopt_lock_t() = default; };

inline constexpr adopt_lock_t adopt_lock{};

} // namespace scratch
