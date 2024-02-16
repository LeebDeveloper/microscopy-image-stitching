#include <iostream>
#include <vector>
#include <filesystem>
#include "Image_Stitching.h"


#define EXECUTABLE "mimgstch"

void print_usage() {
    std::cout << "Microscopic image stitcher" << std::endl
              << "Usage:" << std::endl
              << "  " EXECUTABLE " FILE1 FILE2..." << std::endl << std::endl
              << "Options:" << std::endl
              << "  -h, --help         Show this help" << std::endl
              << "  -o, --output FILE  Save result to FILE" << std::endl;
}

int main(int argc, char* argv[]) {
    const std::vector<std::string_view> args(argv + 1, argv + argc);

    std::vector<std::filesystem::path> input_files;
    std::filesystem::path output_file = {"out.bmp"};

    bool next_arg_is_output = false;
    for(const auto& arg: args) {

        if(arg == "-h" || arg == "--help") {
            print_usage();
            return 0;
        }

        if(arg == "-o" || arg == "--output") {
            next_arg_is_output = true;
            continue;
        }
        if(not next_arg_is_output) {
            input_files.emplace_back(arg);
        } else {
            output_file = arg;
            next_arg_is_output = false;
        }
    }

    if(next_arg_is_output) {
        std::cout << "Output file was not specified. See " EXECUTABLE " --help" << std::endl;
        return 1;
    }

    if(input_files.size() < 2) {
        std::cout << "At least 2 files needed to be stitched" << std::endl;
        return 1;
    }

    stitch_images(input_files, output_file);

    return 0;
}
