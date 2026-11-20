#ifndef __CONVERTER_MARKDOWN_PROCESSOR_H__
#define __CONVERTER_MARKDOWN_PROCESSOR_H__

#include "converter.h"
#include <filesystem>
#include <string>
#include <vector>

namespace converter {

struct SnippetOptions {
    std::vector<std::filesystem::path> base_paths;
    std::filesystem::path docs_root;
    std::filesystem::path project_root;
    bool dedent_subsections = true;
};

// Converts MkDocs admonition blocks (!!! type, ??? type) to GitHub-style alerts (> [!TYPE]).
std::string convert_admonitions_to_github_alerts(const std::string& markdown_content);

// Replaces MkDocs emoji shortcodes (e.g. :memo:, :rocket:) with Unicode emoji characters.
// Skips fenced code blocks and inline code spans.
std::string replace_markdown_emojis(const std::string& markdown_content);

// Expands MkDocs snippet inclusions (--8<-- "filename") in markdown content.
std::string expand_markdown_snippets(const std::string& markdown_content,
                                    const std::filesystem::path& current_file_path,
                                    const SnippetOptions& snippet_options);

// Migrates all files from src_docs to dst_src.
// If options.expand_snippets is true, expands MkDocs snippets.
// If options.convert_alerts is true, transforms Markdown admonitions.
// Otherwise, copies files verbatim.
bool migrate_content(const std::filesystem::path& src_docs,
                     const std::filesystem::path& dst_src,
                     const ConvertOptions& options,
                     const SnippetOptions& snippet_options,
                     size_t& file_count,
                     std::string& error_msg);

} // namespace converter

#endif // __CONVERTER_MARKDOWN_PROCESSOR_H__
