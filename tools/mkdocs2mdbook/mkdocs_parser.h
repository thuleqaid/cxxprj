#ifndef __CONVERTER_MKDOCS_PARSER_H__
#define __CONVERTER_MKDOCS_PARSER_H__

#include "mkdocs_config.h"
#include <filesystem>
#include <string>

namespace converter {

// Parses an mkdocs.yml file into MkDocsConfig.
bool parse_mkdocs_file(const std::filesystem::path& yml_path, MkDocsConfig& config, std::string& error_msg);

// Parses raw YAML content into MkDocsConfig.
bool parse_mkdocs_string(const std::string& yaml_content, MkDocsConfig& config, std::string& error_msg);

// Discovers documentation files recursively in docs_root when nav is not specified.
void discover_docs_directory(const std::filesystem::path& docs_root, MkDocsConfig& config);

// Infers the title from frontmatter title: or first # Heading, or falls back to cleaned stem.
std::string infer_markdown_title(const std::filesystem::path& file_path, const std::string& fallback_stem = "");

// Converts snake_case or kebab-case stem into Title Case.
std::string clean_title_from_stem(const std::string& stem);

} // namespace converter

#endif // __CONVERTER_MKDOCS_PARSER_H__
