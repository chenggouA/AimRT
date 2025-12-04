#pragma once

#include <atomic>
#include <functional>
#include <list>
#include <string>
#include "aimrt_module_cpp_interface/executor/executor.h"
#include "core/logger/formatter.h"
#include "core/logger/logger_backend_base.h"
#include "logcore_plugin/core/filters/level_filter.h"
#include "logcore_plugin/core/filters/module_filter.h"
#include "logcore_plugin/global.h"

namespace aimrt::plugins::logcore::base {


class BackendBase : public runtime::core::logger::LoggerBackendBase {
 public:
  struct Options {
    // Common options for all backends
    std::string min_level = "TRACE";
    std::string module_filter = "(.*)";
    std::string pattern = "";  // Empty = use default pattern
  };

  BackendBase() = default;
  ~BackendBase() override = default;

  // ========== LoggerBackendBase Interface ==========

  void Initialize(YAML::Node options_node) override;
  void Start() override {}
  void Shutdown() override { run_flag_.store(false); }
  bool AllowDuplicates() const noexcept override { return true; }

  void Log(const runtime::core::logger::LogDataWrapper& log_data_wrapper) noexcept override;

  std::list<std::pair<std::string, std::string>> GenInitializationReport() const noexcept override;

  // ========== Common Functionality ==========

  /**
   * @brief Register executor getter function
   */
  void RegisterGetExecutorFunc(
      const std::function<aimrt::executor::ExecutorRef(std::string_view)>& get_executor_func) {
    get_executor_func_ = get_executor_func;
  }

  /**
   * @brief Get statistics
   */
  uint64_t GetTotalLogs() const noexcept { return total_logs_.load(); }
  uint64_t GetFilteredLogs() const noexcept { return filtered_logs_.load(); }
  uint64_t GetWrittenLogs() const noexcept { return total_logs_ - filtered_logs_; }

 protected:
  // ========== Subclass Interface ==========

  /**
   * @brief Parse backend-specific options
   * Called during Initialize() before common options are parsed
   */
  virtual void ParseOptions(YAML::Node options_node) = 0;

  /**
   * @brief Initialize backend-specific resources
   * Called during Initialize() after common options are parsed
   */
  virtual void InitializeBackend() = 0;

  /**
   * @brief Write log to the actual destination
   * Called after all filters passed and log is formatted
   * @param formatted_log Formatted log string
   */
  virtual void WriteLog(const std::string& formatted_log) noexcept = 0;

  // ========== Protected Members ==========

  Options options_;
  std::function<aimrt::executor::ExecutorRef(std::string_view)> get_executor_func_;
  std::atomic_bool run_flag_{false};

 private:
  // Common components
  filters::LevelFilter level_filter_;
  filters::ModuleFilter module_filter_;
  runtime::core::logger::LogFormatter formatter_;
  std::string pattern_ = "[%c.%f][%l][%t][%n][%g:%R @%F]%v";

  // Statistics
  std::atomic<uint64_t> total_logs_{0};
  std::atomic<uint64_t> filtered_logs_{0};
};

}  // namespace aimrt::plugins::logcore::base
