#ifndef __CONVERTER_MDBOOK_GENERATOR_H__
#define __CONVERTER_MDBOOK_GENERATOR_H__

#include "mkdocs_config.h"
#include <filesystem>
#include <string>

namespace converter {

// Generates book.toml in outdir.
bool generate_book_toml(const MkDocsConfig& config, const std::filesystem::path& outdir, std::string& error_msg);

// Generates SUMMARY.md in outdir / "src".
bool generate_summary_md(const MkDocsConfig& config,
                         const std::filesystem::path& docs_root,
                         const std::filesystem::path& outdir,
                         size_t& chapter_count,
                         std::string& error_msg);

// Generates the raw Markdown string for SUMMARY.md.
std::string build_summary_content(const MkDocsConfig& config,
                                 const std::filesystem::path& docs_root,
                                 size_t& chapter_count);

} // namespace converter

#endif // __CONVERTER_MDBOOK_GENERATOR_H__
