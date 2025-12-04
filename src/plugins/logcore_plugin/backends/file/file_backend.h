#pragma once
#include <fstream>
#include <string>
#include <variant>
#include "logcore_plugin/core/base/backend_base.h"
#include "logcore_plugin/global.h"

namespace aimrt::plugins::logcore::file {

class FileBackend : public base::BackendBase {
 public:
  struct SizeRotation {
    std::string max_size = "1MB";
    uint32_t max_files = 10;
    long long max_size_bytes_;
  };

  struct DailyRotation {
    uint32_t keep_days = 30;
  };
  using RotationParams = std::variant<std::monostate, SizeRotation, DailyRotation>;

  struct Options : public base::BackendBase::Options {
    // File-specific options
    std::string path = "./log";
    std::string filename = "aimrt.log";
    RotationParams rotation_params;
  };

  FileBackend() = default;
  ~FileBackend() override = default;

  std::string_view Type() const noexcept override {
    return "logcore_file";
  }

 protected:
  void ParseOptions(YAML::Node options_node) override;
  void InitializeBackend() override;
  void WriteLog(const std::string& formatted_log) noexcept override;
  bool ShouldRotate(const std::string& formatted_log);
  void Rotate();
  uint32_t GetNextIndex();

 private:
  Options options_;
  std::ofstream ofs_;
  std::string base_filename_;
};

}  // namespace aimrt::plugins::logcore::file
