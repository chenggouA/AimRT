#pragma once

#include "aimrt_core_plugin_interface/aimrt_core_plugin_base.h"

namespace aimrt::plugins::logcore {

class LogCorePlugin : public AimRTCorePluginBase {
 public:
  struct Options {
  };

 public:
  LogCorePlugin() = default;
  ~LogCorePlugin() override = default;

  std::string_view Name() const noexcept override { return "logcore_plugin"; }

  bool Initialize(runtime::core::AimRTCore* core_ptr) noexcept override;
  void Shutdown() noexcept override;

 private:
  void SetPluginLogger();
  void RegisterFileBackend();

 private:
  runtime::core::AimRTCore* core_ptr_ = nullptr;
  Options options_;
  bool init_flag_ = false;
};

}  // namespace aimrt::plugins::logcore
