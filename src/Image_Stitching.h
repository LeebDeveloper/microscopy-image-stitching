#include <vector>
#include <filesystem>

using path_t = std::filesystem::path;

void stitch_images(const std::vector<path_t>& input_files, const path_t& output_file);
