#pragma once

#include "spdlog/spdlog.h"

#include "spdlog/fmt/bin_to_hex.h"

namespace spinaltap::logging {

inline auto logger = std::make_shared<spdlog::logger>("spinaltap");

}
