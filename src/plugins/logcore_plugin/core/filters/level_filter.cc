#include "logcore_plugin/core/filters/level_filter.h"
#include <algorithm>
#include <cctype>

namespace aimrt::plugins::logcore::filters {

aimrt_log_level_t LevelFilter::ParseLevel(const std::string& level_str) {
  // Convert to uppercase
  std::string upper_str = level_str;
  std::transform(upper_str.begin(), upper_str.end(), upper_str.begin(),
                 [](unsigned char c) { return std::toupper(c); });

  if (upper_str == "TRACE") return AIMRT_LOG_LEVEL_TRACE;
  if (upper_str == "DEBUG") return AIMRT_LOG_LEVEL_DEBUG;
  if (upper_str == "INFO") return AIMRT_LOG_LEVEL_INFO;
  if (upper_str == "WARN") return AIMRT_LOG_LEVEL_WARN;
  if (upper_str == "ERROR") return AIMRT_LOG_LEVEL_ERROR;
  if (upper_str == "FATAL") return AIMRT_LOG_LEVEL_FATAL;
  if (upper_str == "OFF") return AIMRT_LOG_LEVEL_OFF;

  // Default to TRACE if unknown
  return AIMRT_LOG_LEVEL_TRACE;
}

std::string LevelFilter::GetLevelName(aimrt_log_level_t lvl) {
  switch (lvl) {
    case AIMRT_LOG_LEVEL_TRACE:
      return "TRACE";
    case AIMRT_LOG_LEVEL_DEBUG:
      return "DEBUG";
    case AIMRT_LOG_LEVEL_INFO:
      return "INFO";
    case AIMRT_LOG_LEVEL_WARN:
      return "WARN";
    case AIMRT_LOG_LEVEL_ERROR:
      return "ERROR";
    case AIMRT_LOG_LEVEL_FATAL:
      return "FATAL";
    case AIMRT_LOG_LEVEL_OFF:
      return "OFF";
    default:
      return "UNKNOWN";
  }
}

}  // namespace aimrt::plugins::logcore::filters
