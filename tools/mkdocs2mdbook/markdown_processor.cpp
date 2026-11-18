#include "markdown_processor.h"
#include <fstream>
#include <sstream>
#include <regex>
#include <vector>
#include <algorithm>

namespace converter {

namespace {

std::string map_admonition_type(const std::string& type_str) {
    std::string t = type_str;
    std::transform(t.begin(), t.end(), t.begin(), [](unsigned char c){ return std::tolower(c); });
    if (t == "note" || t == "seealso" || t == "info" || t == "todo") return "NOTE";
    if (t == "tip" || t == "hint" || t == "success" || t == "check" || t == "done") return "TIP";
    if (t == "important") return "IMPORTANT";
    if (t == "warning" || t == "attention") return "WARNING";
    if (t == "danger" || t == "error" || t == "bug" || t == "failure" || t == "fail" || t == "missing" || t == "caution") return "CAUTION";
    return "NOTE";
}

bool is_all_whitespace(const std::string& s) {
    return s.find_first_not_of(" \t\r\n") == std::string::npos;
}

} // anonymous namespace

std::string convert_admonitions_to_github_alerts(const std::string& markdown_content) {
    std::istringstream stream(markdown_content);
    std::string raw_line;
    std::vector<std::string> lines;
    while (std::getline(stream, raw_line)) {
        if (!raw_line.empty() && raw_line.back() == '\r') {
            raw_line.pop_back();
        }
        lines.push_back(raw_line);
    }

    // Pattern to match admonition header:
    // e.g. !!! note "Title" or ???+ warning 'Title' or !!! tip
    // Group 1: base indentation
    // Group 2: marker (!!!, ???, ???+)
    // Group 3: type (note, tip, etc.)
    // Group 4: double-quoted title
    // Group 5: single-quoted title
    // Group 6: unquoted title
    std::regex admon_header_regex(R"re(^([ \t]*)(!{3}|\?{3}\+|\?{3})[ \t]+([a-zA-Z0-9_-]+)(?:[ \t]+(?:"([^"]*)"|'([^']*)'|(.*?)))?[ \t]*$)re");

    std::ostringstream out;
    size_t i = 0;
    while (i < lines.size()) {
        const std::string& line = lines[i];
        std::smatch match;

        if (std::regex_match(line, match, admon_header_regex)) {
            std::string base_indent = match[1].str();
            std::string marker_type = match[3].str();
            std::string title;
            if (match[4].matched) title = match[4].str();
            else if (match[5].matched) title = match[5].str();
            else if (match[6].matched) title = match[6].str();

            // Trim title
            auto t_start = title.find_first_not_of(" \t\r\n\"'");
            if (t_start != std::string::npos) {
                auto t_end = title.find_last_not_of(" \t\r\n\"'");
                title = title.substr(t_start, t_end - t_start + 1);
            } else {
                title.clear();
            }

            std::string alert_type = map_admonition_type(marker_type);

            // Output alert header
            out << base_indent << "> [!" << alert_type << "]\n";
            if (!title.empty()) {
                out << base_indent << "> **" << title << "**\n";
                out << base_indent << ">\n";
            }

            // Expected body indentation is base_indent + 4 spaces or base_indent + 1 tab
            size_t req_indent_len = base_indent.length() + 4;

            ++i;
            std::vector<std::string> pending_empty_lines;

            while (i < lines.size()) {
                const std::string& body_line = lines[i];

                if (is_all_whitespace(body_line)) {
                    pending_empty_lines.push_back(body_line);
                    ++i;
                    continue;
                }

                // Check indentation of this non-empty line
                size_t line_indent = 0;
                while (line_indent < body_line.length() && (body_line[line_indent] == ' ' || body_line[line_indent] == '\t')) {
                    if (body_line[line_indent] == '\t') line_indent += 4;
                    else line_indent += 1;
                }

                if (line_indent >= req_indent_len) {
                    // Flush any pending empty lines within this admonition
                    for (size_t p = 0; p < pending_empty_lines.size(); ++p) {
                        out << base_indent << ">\n";
                    }
                    pending_empty_lines.clear();

                    // Strip the required indent
                    size_t stripped = 0;
                    size_t idx = 0;
                    while (idx < body_line.length() && stripped < req_indent_len) {
                        if (body_line[idx] == '\t') stripped += 4;
                        else stripped += 1;
                        ++idx;
                    }

                    std::string content = (idx <= body_line.length()) ? body_line.substr(idx) : "";
                    out << base_indent << "> " << content << "\n";
                    ++i;
                } else {
                    // Admonition finished; do not flush pending empty lines inside admonition
                    break;
                }
            }

            // Flush pending empty lines outside the admonition
            for (const auto& el : pending_empty_lines) {
                out << el << "\n";
            }
        } else {
            out << line << "\n";
            ++i;
        }
    }

    return out.str();
}

bool migrate_content(const std::filesystem::path& src_docs,
                     const std::filesystem::path& dst_src,
                     bool convert_alerts,
                     size_t& file_count,
                     std::string& error_msg) {
    file_count = 0;
    if (!std::filesystem::exists(src_docs)) {
        error_msg = "Source documentation directory does not exist: " + src_docs.string();
        return false;
    }

    std::error_code ec;
    std::filesystem::create_directories(dst_src, ec);
    if (ec) {
        error_msg = "Failed to create destination directory: " + ec.message();
        return false;
    }

    for (const auto& entry : std::filesystem::recursive_directory_iterator(src_docs)) {
        auto rel = std::filesystem::relative(entry.path(), src_docs);
        auto target = dst_src / rel;

        if (entry.is_directory()) {
            std::filesystem::create_directories(target, ec);
            if (ec) {
                error_msg = "Failed to create directory " + target.string() + ": " + ec.message();
                return false;
            }
        } else if (entry.is_regular_file()) {
            // Ensure parent directory exists
            std::filesystem::create_directories(target.parent_path(), ec);

            auto ext = entry.path().extension().string();
            std::transform(ext.begin(), ext.end(), ext.begin(), [](unsigned char c){ return std::tolower(c); });

            if (convert_alerts && (ext == ".md" || ext == ".markdown")) {
                std::ifstream ifs(entry.path(), std::ios::binary);
                if (!ifs.is_open()) {
                    error_msg = "Failed to open file: " + entry.path().string();
                    return false;
                }
                std::stringstream buffer;
                buffer << ifs.rdbuf();
                std::string processed = convert_admonitions_to_github_alerts(buffer.str());

                std::ofstream ofs(target, std::ios::binary);
                if (!ofs.is_open()) {
                    error_msg = "Failed to write file: " + target.string();
                    return false;
                }
                ofs << processed;
            } else {
                std::filesystem::copy_file(entry.path(), target, std::filesystem::copy_options::overwrite_existing, ec);
                if (ec) {
                    error_msg = "Failed to copy file " + entry.path().string() + ": " + ec.message();
                    return false;
                }
            }
            ++file_count;
        }
    }

    return true;
}

} // namespace converter
