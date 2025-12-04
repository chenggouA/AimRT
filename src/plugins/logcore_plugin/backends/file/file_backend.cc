#include "logcore_plugin/backends/file/file_backend.h"
#include <cctype>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <filesystem>
#include <fstream>
#include <ios>
#include <ranges>
#include <stdexcept>
#include <string>
#include <type_traits>
#include <variant>
#include "log_util.h"

namespace YAML {
template <>
struct convert<aimrt::plugins::logcore::file::FileBackend::RotationParams> {
  static Node encode(const aimrt::plugins::logcore::file::FileBackend::RotationParams& rhs) {
    Node node;

    std::visit([&node](auto&& arg) {
      using T = std::decay_t<decltype(arg)>;
      if constexpr (std::is_same_v<T, aimrt::plugins::logcore::file::FileBackend::SizeRotation>) {
        node["type"] = "size";
        node["max_size"] = arg.max_size;
        node["max_files"] = arg.max_files;
      } else if constexpr (std::is_same_v<T, aimrt::plugins::logcore::file::FileBackend::DailyRotation>) {
        node["type"] = "daily";
        node["key_days"] = arg.keep_days;
      }
    },
               rhs);
    return node;
  }

  static bool decode(const Node& node, aimrt::plugins::logcore::file::FileBackend::RotationParams& rhs) {
    try {
      std::string type = node["type"].as<std::string>();
      if (type == "size") {
        auto max_size_debug = node["max_size"].as<std::string>();
        rhs = aimrt::plugins::logcore::file::FileBackend::SizeRotation{node["max_size"].as<std::string>(), node["max_files"].as<uint32_t>()};
      } else if (type == "daily") {
        rhs = aimrt::plugins::logcore::file::FileBackend::DailyRotation(node["keep_days"].as<uint32_t>());
      } else
        return false;
      return true;
    } catch (const std::invalid_argument&) {
      return false;
    }
  }
};

template <>
struct convert<aimrt::plugins::logcore::file::FileBackend::Options> {
  using Options = aimrt::plugins::logcore::file::FileBackend::Options;

  static Node encode(const Options& rhs) {
    Node node;
    // Common options
    node["min_level"] = rhs.min_level;
    node["module_filter"] = rhs.module_filter;
    node["pattern"] = rhs.pattern;

    // File-specific options
    node["path"] = rhs.path;
    node["rotation"] = rhs.rotation_params;
    node["filename"] = rhs.filename;

    return node;
  }

  static bool decode(const Node& node, Options& rhs) {
    if (!node.IsMap()) return false;

    // Common options
    if (node["min_level"])
      rhs.min_level = node["min_level"].as<std::string>();
    if (node["module_filter"])
      rhs.module_filter = node["module_filter"].as<std::string>();
    if (node["pattern"])
      rhs.pattern = node["pattern"].as<std::string>();

    // File-specific options
    if (node["path"])
      rhs.path = node["path"].as<std::string>();
    if (node["rotation"])
      rhs.rotation_params = node["rotation"].as<aimrt::plugins::logcore::file::FileBackend::RotationParams>();
    if (node["filename"])
      rhs.filename = node["filename"].as<std::string>();
    return true;
  }
};
}  // namespace YAML

namespace aimrt::plugins::logcore::file {

long long stringToIntegerBytes(std::string max_size) {
  std::string processed;
  for (auto c : max_size | std::views::filter([](char c) {
                  return !std::isspace(c);
                }) |
                    std::views::transform([](char c) {
                      return ::toupper(c);
                    })) {
    processed += c;
  }

  AIMRT_CHECK_ERROR_THROW(!processed.empty(), "max_size is empty");

  size_t unit_pos = processed.find_first_not_of("0123456789");

  if (unit_pos == std::string::npos) return std::stoi(processed);

  std::string number_part = processed.substr(0, unit_pos);
  std::string unit_part = processed.substr(unit_pos);

  AIMRT_CHECK_ERROR(!number_part.empty(), "No numeric value found ");

  long long value = std::stoi(number_part);
  long long multiplier;

  if (unit_part == "B") {
    multiplier = 1;
  } else if (unit_part == "KB") {
    multiplier = 1024;
  } else if (unit_part == "MB") {
    multiplier = 1024 * 1024;
  } else
    AIMRT_ERROR_THROW("Invalid unit: {}. Expected B, KB, or MB", unit_part);

  return value * multiplier;
};

void FileBackend::ParseOptions(YAML::Node options_node) {
  if (options_node && !options_node.IsNull()) {
    options_ = options_node.as<Options>();

    std::visit([](auto&& arg) {
      using T = std::decay_t<decltype(arg)>;
      if constexpr (std::is_same_v<T, FileBackend::SizeRotation>) {
        arg.max_size_bytes_ = stringToIntegerBytes(arg.max_size);
      }
    },
               options_.rotation_params);
  }
}

void FileBackend::InitializeBackend() {
  std::filesystem::path log_path(options_.path);
  base_filename_ = (log_path / options_.filename).string();

  if (!(std::filesystem::exists(log_path) && std::filesystem::is_directory(log_path))) {
    std::filesystem::create_directories(log_path);
  }
  ofs_.open(base_filename_, std::ios_base::app | std::ios_base::out);
  bool is_open = ofs_.is_open();
  AIMRT_CHECK_ERROR_THROW(is_open, "Failed to open log file.");
}

bool FileBackend::ShouldRotate(const std::string& formatted_log) {
  bool should_rotate = false;
  std::visit([this, &should_rotate, &formatted_log](auto&& params) {
    using T = std::decay_t<decltype(params)>;
    if constexpr (std::is_same_v<T, FileBackend::SizeRotation>) {
      std::error_code ec;
      long long size = std::filesystem::file_size(base_filename_, ec);
      if (ec)
        size = 0;  // Treat missing file as 0 bytes.

      if (size > 0 && size + (long long)formatted_log.size() > params.max_size_bytes_)  // Use size > 0 to prevent empty rotations.
        should_rotate = true;
    } else if constexpr (std::is_same_v<T, FileBackend::DailyRotation>) {
    }
  },
             options_.rotation_params);
  return should_rotate;
}
// NOTE: This implementation is adapted from RotateFileLoggerBackend::GetNextIndex()
uint32_t FileBackend::GetNextIndex() {
  uint32_t idx = 1;
  std::filesystem::path log_dir =
      std::filesystem::path(base_filename_).parent_path();

  const std::filesystem::directory_iterator end_itr;
  for (std::filesystem::directory_iterator itr(log_dir); itr != end_itr;
       ++itr) {
    const std::string& cur_log_file_name = itr->path().string();
    if (cur_log_file_name.size() <= base_filename_.size() + 1) continue;
    if (cur_log_file_name.substr(0, base_filename_.size() + 1) !=
        (base_filename_ + "_"))
      continue;

    const std::string& cur_log_file_name_suffix =
        cur_log_file_name.substr(base_filename_.size() + 1);
    if (!aimrt::common::util::IsDigitStr(cur_log_file_name_suffix)) continue;
    uint32_t cur_idx = atoi(cur_log_file_name_suffix.c_str());
    if (cur_idx >= idx) idx = cur_idx + 1;
  }

  return idx;
}

void FileBackend::Rotate() {
  //
  if (ofs_.is_open()) {
    ofs_.flush();
    ofs_.clear();
    ofs_.close();
  }

  std::visit([this](auto&& params) {
    using T = std::decay_t<decltype(params)>;
    if constexpr (std::is_same_v<T, FileBackend::SizeRotation>) {
      if (std::filesystem::status(base_filename_).type() == std::filesystem::file_type::regular) {
        std::filesystem::rename(base_filename_, base_filename_ + "_" + std::to_string(FileBackend::GetNextIndex()));
      }

      ofs_.open(base_filename_, std::ios_base::app | std::ios_base::out);
      AIMRT_CHECK_ERROR_THROW(ofs_.is_open(), "Failed to open log file.");
    }
  },
             options_.rotation_params);
}

void FileBackend::WriteLog(const std::string& formatted_log) noexcept {
  if (ShouldRotate(formatted_log))
    Rotate();

  if (!ofs_.is_open()) {
    AIMRT_ERROR_INTERVAL(1000, "Faild to open {} file ", base_filename_);
  }
  ofs_.write(formatted_log.data(), formatted_log.size());
  ofs_.flush();
  // TODO: write log
}

}  // namespace aimrt::plugins::logcore::file
