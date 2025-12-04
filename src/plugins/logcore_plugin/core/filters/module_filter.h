#pragma once

#include <mutex>
#include <regex>
#include <shared_mutex>
#include <string>
#include <string_view>
#include <unordered_map>
#include "core/logger/log_data_wrapper.h"
#include "util/string_util.h"

namespace aimrt::plugins::logcore::filters {


class ModuleFilter {
 public:
  ModuleFilter() = default;
  explicit ModuleFilter(const std::string& pattern) : pattern_(pattern) {
    module_filter_regex_ = std::regex(pattern_);
  }

  /**
   * @brief Check if a log should pass the filter
   * @param log_data Log data wrapper
   * @return true if module name matches pattern
   */
  bool ShouldLog(const runtime::core::logger::LogDataWrapper& log_data) noexcept {
    try {
      std::string_view module_name = log_data.module_name;

      // Fast path: check cache
      {
        std::shared_lock lock(cache_mutex_);
        auto it = module_filter_map_.find(module_name);
        if (it != module_filter_map_.end()) {
          return it->second;
        }
      }

      // Slow path: regex match and cache result
      bool result = std::regex_match(
          module_name.begin(), module_name.end(), module_filter_regex_);

      {
        std::unique_lock lock(cache_mutex_);
        module_filter_map_.emplace(module_name, result);
      }

      return result;

    } catch (const std::exception&) {
      return true;  // On error, allow the log
    }
  }

  /**
   * @brief Set filter pattern
   */
  void SetPattern(const std::string& pattern) {
    pattern_ = pattern;
    module_filter_regex_ = std::regex(pattern_);

    // Clear cache when pattern changes
    std::unique_lock lock(cache_mutex_);
    module_filter_map_.clear();
  }

  /**
   * @brief Get current pattern
   */
  const std::string& GetPattern() const { return pattern_; }

 private:
  std::string pattern_ = "(.*)";  // Default: match all
  std::regex module_filter_regex_;

  // Cache for module name -> match result
  mutable std::shared_mutex cache_mutex_;
  std::unordered_map<std::string, bool,
                     aimrt::common::util::StringHash,
                     std::equal_to<>> module_filter_map_;
};

}  // namespace aimrt::plugins::logcore::filters
