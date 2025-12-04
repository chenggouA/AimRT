#include "logcore_plugin/core/base/backend_base.h"

namespace YAML {
template <>
struct convert<aimrt::plugins::logcore::base::BackendBase::Options> {
  using Options = aimrt::plugins::logcore::base::BackendBase::Options;

  static Node encode(const Options& rhs) {
    Node node;
    node["min_level"] = rhs.min_level;
    node["module_filter"] = rhs.module_filter;
    node["pattern"] = rhs.pattern;
    return node;
  }

  static bool decode(const Node& node, Options& rhs) {
    if (!node.IsMap()) return false;

    if (node["min_level"])
      rhs.min_level = node["min_level"].as<std::string>();
    if (node["module_filter"])
      rhs.module_filter = node["module_filter"].as<std::string>();
    if (node["pattern"])
      rhs.pattern = node["pattern"].as<std::string>();

    return true;
  }
};
}  // namespace YAML

namespace aimrt::plugins::logcore::base {

void BackendBase::Initialize(YAML::Node options_node) {
  // 1. Parse backend-specific options first
  ParseOptions(options_node);

  // 2. Parse common options
  if (options_node && !options_node.IsNull()) {
    options_ = options_node.as<Options>();
  }

  // 3. Initialize common components
  auto min_level = filters::LevelFilter::ParseLevel(options_.min_level);
  level_filter_.SetMinLevel(min_level);

  module_filter_.SetPattern(options_.module_filter);

  if (!options_.pattern.empty()) {
    pattern_ = options_.pattern;
  }
  formatter_.SetPattern(pattern_);

  // 4. Initialize backend-specific resources
  InitializeBackend();

  run_flag_.store(true);
}

void BackendBase::Log(
    const runtime::core::logger::LogDataWrapper& log_data_wrapper) noexcept {
  try {
    // Why: mark running status with run_flags?
    if (!run_flag_.load(std::memory_order_relaxed)) {
      return;
    }

    total_logs_.fetch_add(1, std::memory_order_relaxed);

    // Filter 1: Level filter
    if (!level_filter_.ShouldLog(log_data_wrapper.lvl)) {
      filtered_logs_.fetch_add(1, std::memory_order_relaxed);
      return;
    }

    // Filter 2: Module filter
    if (!module_filter_.ShouldLog(log_data_wrapper)) {
      filtered_logs_.fetch_add(1, std::memory_order_relaxed);
      return;
    }

    // Format the log
    std::string formatted_log = formatter_.Format(log_data_wrapper);

    // Write to destination (implemented by subclass)
    WriteLog(formatted_log);

  } catch (const std::exception& e) {
    fprintf(stderr, "BackendBase::Log exception: %s\n", e.what());
  }
}

std::list<std::pair<std::string, std::string>>
BackendBase::GenInitializationReport() const noexcept {
  // Return empty report - backend configuration is already shown in
  // the "AimRT Core Option" section of the initialization report
  return {};
}

}  // namespace aimrt::plugins::logcore::base
