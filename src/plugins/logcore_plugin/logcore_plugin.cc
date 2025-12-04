#include "logcore_plugin.h"

#include "core/aimrt_core.h"
#include "logcore_plugin/backends/file/file_backend.h"
#include "logcore_plugin/global.h"

namespace YAML {
template <>
struct convert<aimrt::plugins::logcore::LogCorePlugin::Options> {
  using Options = aimrt::plugins::logcore::LogCorePlugin::Options;

  static Node encode(const Options& rhs) {
    Node node;
    return node;
  }

  static bool decode(const Node& node, Options& rhs) {
    if (!node.IsMap()) return false;
    return true;
  }
};
}  // namespace YAML

namespace aimrt::plugins::logcore {

bool LogCorePlugin::Initialize(runtime::core::AimRTCore* core_ptr) noexcept {
  try {
    core_ptr_ = core_ptr;

    YAML::Node plugin_options_node = core_ptr_->GetPluginManager().GetPluginOptionsNode(Name());

    if (plugin_options_node && !plugin_options_node.IsNull()) {
      options_ = plugin_options_node.as<Options>();
    }

    init_flag_ = true;

    // Register file backend before logger manager initializes
    core_ptr_->RegisterHookFunc(
        runtime::core::AimRTCore::State::kPreInitLog,
        [this] { RegisterFileBackend(); });

    // Set plugin logger after logger manager initializes
    core_ptr_->RegisterHookFunc(
        runtime::core::AimRTCore::State::kPostInitLog,
        [this] { SetPluginLogger(); });

    plugin_options_node = options_;
    core_ptr_->GetPluginManager().UpdatePluginOptionsNode(Name(), plugin_options_node);

    return true;
  } catch (const std::exception& e) {
    AIMRT_ERROR("LogCorePlugin initialize failed: {}", e.what());
  }

  return false;
}

void LogCorePlugin::Shutdown() noexcept {
  try {
    if (!init_flag_) return;

  } catch (const std::exception& e) {
    AIMRT_ERROR("LogCorePlugin shutdown failed: {}", e.what());
  }
}

void LogCorePlugin::SetPluginLogger() {
  SetLogger(aimrt::logger::LoggerRef(
      core_ptr_->GetLoggerManager().GetLoggerProxy().NativeHandle()));
}

void LogCorePlugin::RegisterFileBackend() {
  core_ptr_->GetLoggerManager()
      .RegisterLoggerBackendGenFunc(
          "logcore_file",  // Backend type identifier
          [this]() -> std::unique_ptr<runtime::core::logger::LoggerBackendBase> {
            auto backend_ptr = std::make_unique<file::FileBackend>();

            // Register executor getter function
            backend_ptr->RegisterGetExecutorFunc(
                [this](std::string_view executor_name) -> aimrt::executor::ExecutorRef {
                  if (executor_name.empty()) {
                    return aimrt::executor::ExecutorRef(
                        core_ptr_->GetGuardThreadExecutor().NativeHandle());
                  }
                  return core_ptr_->GetExecutorManager().GetExecutor(executor_name);
                });

            return backend_ptr;
          });
}

}  // namespace aimrt::plugins::logcore
