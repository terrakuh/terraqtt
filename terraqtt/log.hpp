#ifndef TERRAQTT_LOG_HPP_
#define TERRAQTT_LOG_HPP_

#include "config.hpp"

#if TERRAQTT_LOG_ENABLE
#	define SPDLOG_ACTIVE_LEVEL SPDLOG_LEVEL_##TERRAQTT_LOG_LEVEL
#	include <spdlog/spdlog.h>
#	define TERRAQTT_LOG(level, ...) SPDLOG_##level(__VA_ARGS__)
#else
#	define TERRAQTT_LOG(...) ((void) 0)
#endif

#endif
