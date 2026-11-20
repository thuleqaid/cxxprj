#include <iostream>
#include "cxxopts/cxxopts.hpp"
#include "converter/converter.h"

static void convert(std::string indir, std::string outdir, const converter::ConvertOptions& options);

int main(int argc, char *argv[])
{
    cxxopts::Options options("MyProgram", "Convert MkDocs documentation project to mdBook project");
    options.add_options()
        ("i,input", "mkdocs dir", cxxopts::value<std::string>())
        ("o,output", "mdbook dir", cxxopts::value<std::string>())
        ("a,alerts", "Convert mkdocs admonitions to GitHub-style alerts", cxxopts::value<bool>()->default_value("false"))
        ("h,help", "Print usage")
        ;
    auto result = options.parse(argc, argv);

    if (result.count("help"))
    {
        std::cout << options.help() << std::endl;
        exit(0);
    } else {
        if ((result.count("input")) && (result.count("output")))
        {
            converter::ConvertOptions opts;
            opts.convert_alerts = result.count("alerts") ? result["alerts"].as<bool>() : false;
            convert(result["input"].as<std::string>(), result["output"].as<std::string>(), opts);
        } else {
            std::cout << options.help() << std::endl;
        }
    }
    return 0;
}

static void convert(std::string indir, std::string outdir, const converter::ConvertOptions& options)
{
    std::cout << "Starting mkdocs to mdbook conversion..." << std::endl;
    std::cout << "Input directory : " << indir << std::endl;
    std::cout << "Output directory: " << outdir << std::endl;
    std::cout << "Convert alerts  : " << (options.convert_alerts ? "enabled" : "disabled") << std::endl;

    auto result = converter::convert_project(indir, outdir, options);
    if (!result.success) {
        std::cerr << "Conversion failed: " << result.error_message << std::endl;
        return;
    }

    std::cout << "Conversion completed successfully!" << std::endl;
    std::cout << "Total chapters indexed: " << result.converted_chapters << std::endl;
    std::cout << "Total files migrated  : " << result.converted_files << std::endl;
}
