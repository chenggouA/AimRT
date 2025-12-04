#include <csignal>
#include <iostream>

#include "core/aimrt_core.h"
#include "module/logcore_test_module.h"

using namespace aimrt::runtime::core;
using namespace aimrt::examples::plugins::logcore_plugin;

AimRTCore* global_core_ptr = nullptr;

void SignalHandler(int sig) {
  if (global_core_ptr && (sig == SIGINT || sig == SIGTERM)) {
    global_core_ptr->Shutdown();
    return;
  }
  raise(sig);
}

int32_t main(int32_t argc, char** argv) {
  signal(SIGINT, SignalHandler);
  signal(SIGTERM, SignalHandler);

  std::cout << "AimRT LogCore Plugin Example - Start" << std::endl;

  try {
    AimRTCore core;
    global_core_ptr = &core;

    // Register module
    LogCoreTestModule logcore_test_module;
    core.GetModuleManager().RegisterModule(logcore_test_module.NativeHandle());

    // Initialize
    AimRTCore::Options options;
    if (argc > 1) options.cfg_file_path = argv[1];
    core.Initialize(options);

    // Start
    core.Start();

    // Shutdown
    core.Shutdown();

    global_core_ptr = nullptr;

  } catch (const std::exception& e) {
    std::cerr << "AimRT run with exception: " << e.what() << std::endl;
    return -1;
  }

  std::cout << "AimRT LogCore Plugin Example - Exit" << std::endl;
  return 0;
}
