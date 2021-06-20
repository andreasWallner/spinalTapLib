#pragma once

#include "spdlog/fmt/bin_to_hex.h"
#include "spdlog/spdlog.h"

namespace spinaltap::logging {

inline auto logger = std::make_shared<spdlog::logger>("spinaltap");

}
