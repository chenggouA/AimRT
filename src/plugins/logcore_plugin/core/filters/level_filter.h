#pragma once

#include <atomic>
#include <string>
#include "aimrt_module_c_interface/logger/logger_base.h"

namespace aimrt::plugins::logcore::filters {


class LevelFilter {
 public:
  LevelFilter() = default;
  explicit LevelFilter(aimrt_log_level_t min_level) : min_level_(min_level) {}

  /**
   * @brief Check if a log should pass the filter
   * @param lvl Log level to check
   * @return true if log level >= min_level
   */
  bool ShouldLog(aimrt_log_level_t lvl) const noexcept {
    return lvl >= min_level_.load(std::memory_order_relaxed);
  }

  /**
   * @brief Set minimum log level
   */
  void SetMinLevel(aimrt_log_level_t lvl) noexcept {
    min_level_.store(lvl, std::memory_order_relaxed);
  }

  /**
   * @brief Get minimum log level
   */
  aimrt_log_level_t GetMinLevel() const noexcept {
    return min_level_.load(std::memory_order_relaxed);
  }

  /**
   * @brief Parse log level from string
   * @param level_str "TRACE"/"DEBUG"/"INFO"/"WARN"/"ERROR"/"FATAL"/"OFF"
   * @return Corresponding log level enum
   */
  static aimrt_log_level_t ParseLevel(const std::string& level_str);

  /**
   * @brief Get log level name
   * @param lvl Log level enum
   * @return Level name string
   */
  static std::string GetLevelName(aimrt_log_level_t lvl);

 private:
  std::atomic<aimrt_log_level_t> min_level_{AIMRT_LOG_LEVEL_TRACE};
};

}  // namespace aimrt::plugins::logcore::filters
