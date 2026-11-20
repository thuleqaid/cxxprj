#include "converter.h"
#include "mkdocs_parser.h"
#include "mdbook_generator.h"
#include "markdown_processor.h"

namespace converter {

ConvertResult convert_project(const std::filesystem::path& indir,
                             const std::filesystem::path& outdir,
                             const ConvertOptions& options) {
    ConvertResult result;

    if (!std::filesystem::exists(indir) || !std::filesystem::is_directory(indir)) {
        result.error_message = "Input directory does not exist or is not a directory: " + indir.string();
        return result;
    }

    // Locate mkdocs.yml or mkdocs.yaml
    std::filesystem::path yml_path = indir / "mkdocs.yml";
    if (!std::filesystem::exists(yml_path)) {
        yml_path = indir / "mkdocs.yaml";
    }

    if (!std::filesystem::exists(yml_path)) {
        result.error_message = "Cannot find mkdocs.yml or mkdocs.yaml in: " + indir.string();
        return result;
    }

    MkDocsConfig config;
    std::string err;
    if (!parse_mkdocs_file(yml_path, config, err)) {
        result.error_message = err;
        return result;
    }

    std::filesystem::path docs_root = indir / config.docs_dir;
    if (!std::filesystem::exists(docs_root) || !std::filesystem::is_directory(docs_root)) {
        result.error_message = "Docs directory does not exist: " + docs_root.string();
        return result;
    }

    // If nav was not present in mkdocs.yml, auto-discover markdown files
    if (!config.has_nav || config.nav.empty()) {
        discover_docs_directory(docs_root, config);
    }

    // Create target output directory
    std::error_code ec;
    std::filesystem::create_directories(outdir / "src", ec);
    if (ec) {
        result.error_message = "Failed to create output directory: " + ec.message();
        return result;
    }

    // Generate book.toml
    if (!generate_book_toml(config, outdir, err)) {
        result.error_message = err;
        return result;
    }

    // Generate SUMMARY.md
    size_t chapter_count = 0;
    if (!generate_summary_md(config, docs_root, outdir, chapter_count, err)) {
        result.error_message = err;
        return result;
    }
    result.converted_chapters = chapter_count;

    // Prepare snippet options
    SnippetOptions snippet_opts;
    snippet_opts.docs_root = docs_root;
    snippet_opts.project_root = indir;
    snippet_opts.dedent_subsections = true;

    for (const auto& bp : config.snippet_base_paths) {
        std::filesystem::path p(bp);
        if (p.is_relative()) {
            snippet_opts.base_paths.push_back((indir / p).lexically_normal());
        } else {
            snippet_opts.base_paths.push_back(p);
        }
    }
    snippet_opts.base_paths.push_back(docs_root);
    snippet_opts.base_paths.push_back(indir);

    // Migrate content files (and apply snippet expansion / alert conversion if requested)
    size_t file_count = 0;
    if (!migrate_content(docs_root, outdir / "src", options, snippet_opts, file_count, err)) {
        result.error_message = err;
        return result;
    }
    result.converted_files = file_count;

    result.success = true;
    return result;
}

} // namespace converter
