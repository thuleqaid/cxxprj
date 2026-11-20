#ifndef __CONVERTER_MARKDOWN_PROCESSOR_H__
#define __CONVERTER_MARKDOWN_PROCESSOR_H__

#include <filesystem>
#include <string>

namespace converter {

// Converts MkDocs admonition blocks (!!! type, ??? type) to GitHub-style alerts (> [!TYPE]).
std::string convert_admonitions_to_github_alerts(const std::string& markdown_content);

// Migrates all files from src_docs to dst_src.
// If convert_alerts is true, transforms Markdown admonitions.
// Otherwise, copies files verbatim.
bool migrate_content(const std::filesystem::path& src_docs,
                     const std::filesystem::path& dst_src,
                     bool convert_alerts,
                     size_t& file_count,
                     std::string& error_msg);

} // namespace converter

#endif // __CONVERTER_MARKDOWN_PROCESSOR_H__
