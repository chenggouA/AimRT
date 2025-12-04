#pragma once

#include <atomic>
#include "aimrt_module_cpp_interface/module_base.h"

namespace aimrt::examples::plugins::logcore_plugin {

class LogCoreTestModule : public aimrt::ModuleBase {
 public:
  LogCoreTestModule() = default;
  ~LogCoreTestModule() override = default;

  ModuleInfo Info() const override {
    return ModuleInfo{.name = "LogCoreTestModule"};
  }

  bool Initialize(aimrt::CoreRef core) override;

  bool Start() override;

  void Shutdown() override;

 private:
  void RunLogTest();

 private:
  aimrt::CoreRef core_;
  aimrt::logger::LoggerRef logger_;
  std::atomic<bool> run_flag_{true};
};

}  // namespace aimrt::examples::plugins::logcore_plugin
