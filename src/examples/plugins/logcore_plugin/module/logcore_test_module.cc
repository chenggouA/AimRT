#include "logcore_test_module.h"

namespace aimrt::examples::plugins::logcore_plugin {

bool LogCoreTestModule::Initialize(aimrt::CoreRef core) {
  core_ = core;
  logger_ = core_.GetLogger();

  AIMRT_HL_INFO(logger_, "LogCoreTestModule Initialize completed");

  return true;
}

bool LogCoreTestModule::Start() {
  AIMRT_HL_INFO(logger_, "LogCoreTestModule Start - Running log level tests...");

  // Run a simple test to generate logs at different levels
  RunLogTest();

  AIMRT_HL_INFO(logger_, "LogCoreTestModule Start completed");
  return true;
}

void LogCoreTestModule::Shutdown() {
  run_flag_ = false;
  AIMRT_HL_INFO(logger_, "LogCoreTestModule Shutdown");
}

void LogCoreTestModule::RunLogTest() {
  // Generate multiple log entries at each level for testing
  for (int i = 1; i <= 5; i++) {
    AIMRT_HL_TRACE(logger_, "TRACE level test {}/5 - This should only appear in DEBUG level files", i);
    AIMRT_HL_DEBUG(logger_, "DEBUG level test {}/5 - This should appear in DEBUG+ level files", i);
    AIMRT_HL_INFO(logger_, "INFO level test {}/5 - This should appear in INFO+ level files", i);
    AIMRT_HL_WARN(logger_, "WARN level test {}/5 - This should appear in WARN+ level files", i);
    AIMRT_HL_ERROR(logger_, "ERROR level test {}/5 - This should appear in ERROR+ level files", i);
    AIMRT_HL_FATAL(logger_, "FATAL level test {}/5 - This should appear in all files", i);
  }

  AIMRT_HL_INFO(logger_, "========== Log level test completed ==========");
  AIMRT_HL_INFO(logger_, "Check the log files to verify filtering:");
  AIMRT_HL_INFO(logger_, "  - full.log should contain DEBUG and above");
  AIMRT_HL_INFO(logger_, "  - info.log should contain INFO and above");
  AIMRT_HL_INFO(logger_, "  - error.log should contain ERROR and above");
}

}  // namespace aimrt::examples::plugins::logcore_plugin
