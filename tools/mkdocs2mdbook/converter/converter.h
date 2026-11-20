#ifndef __CONVERTER_CONVERTER_H__
#define __CONVERTER_CONVERTER_H__

#include <string>
#include <filesystem>

namespace converter {

struct ConvertOptions {
    bool convert_alerts = false;  // Convert MkDocs admonitions (!!! note) to GitHub-style alerts (> [!NOTE])
    bool expand_snippets = true;  // Expand MkDocs snippet inclusions (--8<-- "file") into target markdonw
    bool convert_emojis = true;   // Convert MkDocs emoji shortcodes (:memo:) to Unicode emoji characters
};

struct ConvertResult {
    bool success = false;
    std::string error_message;
    size_t converted_files = 0;
    size_t converted_chapters = 0;
};

/**
 * @brief Converts an MkDocs documentation project to an mdBook project.
 * 
 * @param indir The root directory of the MkDocs project (containing mkdocs.yml).
 * @param outdir The destination root directory for the generated mdBook project.
 * @param options Conversion options (such as Markdown admonition alert conversion).
 * @return ConvertResult Details of the conversion result.
 */
ConvertResult convert_project(const std::filesystem::path& indir,
                             const std::filesystem::path& outdir,
                             const ConvertOptions& options = {});

} // namespace converter

#endif // __CONVERTER_CONVERTER_H__
