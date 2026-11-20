#include "markdown_processor.h"
#include "emoji_data.h"
#include <fstream>
#include <sstream>
#include <regex>
#include <vector>
#include <set>
#include <algorithm>
#include <iostream>

namespace converter {

namespace {

std::string trim_string(const std::string& s) {
    auto start = s.find_first_not_of(" \t\r\n\"'");
    if (start == std::string::npos) return "";
    auto end = s.find_last_not_of(" \t\r\n\"'");
    return s.substr(start, end - start + 1);
}

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

struct SnippetFileSpec {
    std::string file_path;
    bool has_lines = false;
    struct LineRange {
        int start = -1; // -1 means omitted
        int end = -1;   // -1 means omitted
    };
    std::vector<LineRange> line_ranges;
    bool has_section = false;
    std::string section_name;
};

bool parse_snippet_spec(const std::string& raw_spec, SnippetFileSpec& spec) {
    std::string trimmed = raw_spec;
    auto start_idx = trimmed.find_first_not_of(" \t\r\n");
    if (start_idx == std::string::npos) return false;
    auto end_idx = trimmed.find_last_not_of(" \t\r\n");
    trimmed = trimmed.substr(start_idx, end_idx - start_idx + 1);

    if (trimmed.empty()) return false;

    // Pattern: ^(.*?)(?:((?::-?\d*){1,2}(?:,-?\d*(?::-?\d*)?)*)|:([a-zA-Z][-_a-zA-Z0-9]*))?$
    // Group 1: file path
    // Group 2: line ranges (:9, :9:20, ::20, :9:, :1:5,10:15)
    // Group 3: section name (:section_name)
    std::regex spec_regex(R"re(^(.*?)(?:((?::-?\d*){1,2}(?:,-?\d*(?::-?\d*)?)*)|:([a-zA-Z][-_a-zA-Z0-9]*))?$)re");
    std::smatch m;
    if (!std::regex_match(trimmed, m, spec_regex)) {
        spec.file_path = trimmed;
        spec.has_lines = false;
        spec.has_section = false;
        spec.line_ranges.clear();
        spec.section_name.clear();
        return true;
    }

    spec.file_path = m[1].str();
    spec.has_lines = false;
    spec.has_section = false;
    spec.line_ranges.clear();
    spec.section_name.clear();

    if (m[3].matched && !m[3].str().empty()) {
        spec.has_section = true;
        spec.section_name = m[3].str();
        return true;
    }

    if (m[2].matched && !m[2].str().empty()) {
        std::string lines_str = m[2].str();
        if (lines_str.front() == ':') {
            lines_str = lines_str.substr(1);
        }

        spec.has_lines = true;
        std::stringstream ss(lines_str);
        std::string range_tok;
        while (std::getline(ss, range_tok, ',')) {
            if (range_tok.empty()) continue;
            SnippetFileSpec::LineRange lr;
            auto colon_idx = range_tok.find(':');
            if (colon_idx == std::string::npos) {
                // e.g. "9" -> start from line 9 through end of file
                try {
                    lr.start = std::stoi(range_tok);
                } catch (...) {
                    lr.start = -1;
                }
                lr.end = -1;
            } else {
                std::string s_str = range_tok.substr(0, colon_idx);
                std::string e_str = range_tok.substr(colon_idx + 1);
                if (!s_str.empty()) {
                    try {
                        lr.start = std::stoi(s_str);
                    } catch (...) {
                        lr.start = -1;
                    }
                } else {
                    lr.start = -1;
                }
                if (!e_str.empty()) {
                    try {
                        lr.end = std::stoi(e_str);
                    } catch (...) {
                        lr.end = -1;
                    }
                } else {
                    lr.end = -1;
                }
            }
            spec.line_ranges.push_back(lr);
        }
        return true;
    }

    return true;
}

std::filesystem::path resolve_snippet_path(
    const std::string& raw_path,
    const std::filesystem::path& current_file_path,
    const SnippetOptions& options) {
    if (raw_path.empty()) return {};

    std::filesystem::path p(raw_path);
    if (p.is_absolute() && std::filesystem::exists(p) && std::filesystem::is_regular_file(p)) {
        return p;
    }

    // 1. Try base_paths from config
    for (const auto& base : options.base_paths) {
        auto candidate = (base / p).lexically_normal();
        if (std::filesystem::exists(candidate) && std::filesystem::is_regular_file(candidate)) {
            return candidate;
        }
    }

    // 2. Try relative to current markdown file directory
    if (!current_file_path.empty()) {
        auto cur_dir = current_file_path.has_parent_path() ? current_file_path.parent_path() : std::filesystem::current_path();
        auto candidate = (cur_dir / p).lexically_normal();
        if (std::filesystem::exists(candidate) && std::filesystem::is_regular_file(candidate)) {
            return candidate;
        }
    }

    // 3. Try relative to docs_root
    if (!options.docs_root.empty()) {
        auto candidate = (options.docs_root / p).lexically_normal();
        if (std::filesystem::exists(candidate) && std::filesystem::is_regular_file(candidate)) {
            return candidate;
        }
    }

    // 4. Try relative to project_root
    if (!options.project_root.empty()) {
        auto candidate = (options.project_root / p).lexically_normal();
        if (std::filesystem::exists(candidate) && std::filesystem::is_regular_file(candidate)) {
            return candidate;
        }
    }

    return {};
}

bool read_file_lines(const std::filesystem::path& file_path, std::vector<std::string>& lines) {
    std::ifstream ifs(file_path, std::ios::binary);
    if (!ifs.is_open()) return false;
    std::string line;
    bool first = true;
    while (std::getline(ifs, line)) {
        if (!line.empty() && line.back() == '\r') {
            line.pop_back();
        }
        // Strip UTF-8 BOM if present on first line
        if (first) {
            first = false;
            if (line.size() >= 3 &&
                static_cast<unsigned char>(line[0]) == 0xEF &&
                static_cast<unsigned char>(line[1]) == 0xBB &&
                static_cast<unsigned char>(line[2]) == 0xBF) {
                line = line.substr(3);
            }
        }
        lines.push_back(line);
    }
    return true;
}

std::vector<std::string> extract_section(const std::vector<std::string>& lines, const std::string& section_name) {
    std::regex section_marker_regex(R"re(^(.*?)(?:;*)-{1,}8<-{1,}[ \t]+\[[ \t]*(start|end)[ \t]*:[ \t]*([a-zA-Z][-_0-9a-zA-Z]*)[ \t]*\])re");
    std::vector<std::string> result;
    bool capturing = false;
    for (const auto& line : lines) {
        std::smatch m;
        if (std::regex_search(line, m, section_marker_regex)) {
            std::string type = m[2].str();
            std::string name = m[3].str();
            if (name == section_name) {
                if (type == "start") {
                    capturing = true;
                    continue;
                } else if (type == "end") {
                    capturing = false;
                    break;
                }
            }
            continue;
        }
        if (capturing) {
            result.push_back(line);
        }
    }
    return result;
}

std::vector<std::string> strip_section_markers(const std::vector<std::string>& lines) {
    std::regex section_marker_regex(R"re(^(.*?)(?:;*)-{1,}8<-{1,}[ \t]+\[[ \t]*(start|end)[ \t]*:[ \t]*([a-zA-Z][-_0-9a-zA-Z]*)[ \t]*\])re");
    std::vector<std::string> result;
    for (const auto& line : lines) {
        if (!std::regex_search(line, section_marker_regex)) {
            result.push_back(line);
        }
    }
    return result;
}

std::vector<std::string> extract_line_ranges(
    const std::vector<std::string>& lines,
    const std::vector<SnippetFileSpec::LineRange>& ranges) {
    std::vector<std::string> result;
    int total = static_cast<int>(lines.size());
    if (total == 0) return result;

    for (const auto& r : ranges) {
        int s_start = 0;
        int s_end = total;

        if (r.start != -1) {
            if (r.start < 0) {
                s_start = std::max(0, std::min(total, total + r.start));
            } else {
                s_start = std::max(0, std::min(total, r.start - 1));
            }
        }

        if (r.end != -1) {
            if (r.end < 0) {
                s_end = std::max(0, std::min(total, total + 1 + r.end));
            } else {
                s_end = std::max(0, std::min(total, r.end));
            }
        }

        for (int i = s_start; i < s_end && i < total; ++i) {
            result.push_back(lines[i]);
        }
    }
    return result;
}

void dedent_lines(std::vector<std::string>& lines) {
    size_t min_indent = std::string::npos;
    for (const auto& line : lines) {
        if (line.find_first_not_of(" \t\r\n") == std::string::npos) {
            continue;
        }
        size_t indent = 0;
        for (char c : line) {
            if (c == ' ') indent += 1;
            else if (c == '\t') indent += 4;
            else break;
        }
        if (indent < min_indent) {
            min_indent = indent;
        }
    }

    if (min_indent == 0 || min_indent == std::string::npos) {
        return;
    }

    for (auto& line : lines) {
        if (line.find_first_not_of(" \t\r\n") == std::string::npos) {
            line.clear();
            continue;
        }
        size_t stripped = 0;
        size_t idx = 0;
        while (idx < line.length() && stripped < min_indent) {
            if (line[idx] == ' ') stripped += 1;
            else if (line[idx] == '\t') stripped += 4;
            else break;
            ++idx;
        }
        line = line.substr(idx);
    }
}

std::vector<std::string> expand_snippets_internal(
    const std::vector<std::string>& input_lines,
    const std::filesystem::path& current_file_path,
    const SnippetOptions& options,
    std::set<std::filesystem::path>& seen_files,
    int depth) {
    if (depth > 20) {
        return input_lines;
    }

    std::vector<std::string> result;
    // Regex for inline snippet:
    // Group 1: space/indent
    // Group 2: escape semicolons
    // Group 3: double-quoted snippet path
    // Group 4: single-quoted snippet path
    std::regex inline_snippet_regex(R"re(^([ \t]*)(;*)-{1,}8<-{1,}[ \t]+(?:"([^"\r\n\\]*(?:\\.[^"\r\n\\]*)*)"|'([^'\r\n\\]*(?:\\.[^'\r\n\\]*)*)')[ \t]*$)re");

    // Regex for block snippet marker:
    std::regex block_snippet_marker_regex(R"re(^([ \t]*)(;*)-{1,}8<-{1,}[ \t]*$)re");

    bool in_block = false;
    std::string block_base_indent;

    auto expand_target = [&](const std::string& indent, const std::string& raw_spec) {
        SnippetFileSpec spec;
        if (!parse_snippet_spec(raw_spec, spec)) {
            return false;
        }

        auto target_path = resolve_snippet_path(spec.file_path, current_file_path, options);
        if (target_path.empty() || !std::filesystem::exists(target_path)) {
            std::cerr << "Warning: Snippet file not found: " << spec.file_path 
                      << " (referenced in " << current_file_path.string() << ")" << std::endl;
            return false;
        }

        std::error_code ec;
        auto norm_target = std::filesystem::weakly_canonical(target_path, ec);
        if (!ec && seen_files.count(norm_target) > 0) {
            std::cerr << "Warning: Circular snippet inclusion detected: " << norm_target.string() << std::endl;
            return true; // handled by skipping circular inclusion
        }

        std::vector<std::string> file_lines;
        if (!read_file_lines(target_path, file_lines)) {
            std::cerr << "Warning: Failed to read snippet file: " << target_path.string() << std::endl;
            return false;
        }

        std::vector<std::string> snippet_content;
        if (spec.has_section) {
            snippet_content = extract_section(file_lines, spec.section_name);
            if (options.dedent_subsections) {
                dedent_lines(snippet_content);
            }
        } else if (spec.has_lines) {
            snippet_content = extract_line_ranges(file_lines, spec.line_ranges);
            if (options.dedent_subsections) {
                dedent_lines(snippet_content);
            }
        } else {
            snippet_content = strip_section_markers(file_lines);
        }

        // Recursively expand any snippets inside the snippet file
        if (!ec) seen_files.insert(norm_target);
        auto expanded = expand_snippets_internal(snippet_content, target_path, options, seen_files, depth + 1);
        if (!ec) seen_files.erase(norm_target);

        for (const auto& sl : expanded) {
            if (sl.empty()) {
                result.push_back("");
            } else {
                result.push_back(indent + sl);
            }
        }
        return true;
    };

    for (size_t i = 0; i < input_lines.size(); ++i) {
        const std::string& line = input_lines[i];

        // Check for block snippet marker
        std::smatch block_m;
        if (std::regex_match(line, block_m, block_snippet_marker_regex)) {
            std::string escape = block_m[2].str();
            if (!escape.empty()) {
                // Escaped: strip first semicolon and keep as plain line
                auto semi_pos = line.find(';');
                result.push_back(line.substr(0, semi_pos) + line.substr(semi_pos + 1));
                continue;
            }
            in_block = !in_block;
            if (in_block) {
                block_base_indent = block_m[1].str();
            }
            continue;
        }

        if (in_block) {
            if (line.find_first_not_of(" \t\r\n") == std::string::npos) {
                result.push_back("");
                continue;
            }
            auto trimmed = trim_string(line);
            if (!trimmed.empty() && trimmed[0] == ';') {
                continue;
            }
            if (!expand_target(block_base_indent, trimmed)) {
                result.push_back(line);
            }
            continue;
        }

        // Check for inline snippet
        std::smatch inline_m;
        if (std::regex_match(line, inline_m, inline_snippet_regex)) {
            std::string indent = inline_m[1].str();
            std::string escape = inline_m[2].str();
            if (!escape.empty()) {
                // Escaped: strip first semicolon
                auto semi_pos = line.find(';');
                result.push_back(line.substr(0, semi_pos) + line.substr(semi_pos + 1));
                continue;
            }

            std::string raw_path = inline_m[3].matched ? inline_m[3].str() : inline_m[4].str();
            if (!expand_target(indent, raw_path)) {
                result.push_back(line);
            }
        } else {
            result.push_back(line);
        }
    }

    return result;
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

std::string expand_markdown_snippets(const std::string& markdown_content,
                                    const std::filesystem::path& current_file_path,
                                    const SnippetOptions& snippet_options) {
    std::istringstream stream(markdown_content);
    std::string raw_line;
    std::vector<std::string> lines;
    while (std::getline(stream, raw_line)) {
        if (!raw_line.empty() && raw_line.back() == '\r') {
            raw_line.pop_back();
        }
        lines.push_back(raw_line);
    }

    std::set<std::filesystem::path> seen_files;
    if (!current_file_path.empty()) {
        std::error_code ec;
        auto norm = std::filesystem::weakly_canonical(current_file_path, ec);
        if (!ec) {
            seen_files.insert(norm);
        }
    }

    auto expanded_lines = expand_snippets_internal(lines, current_file_path, snippet_options, seen_files, 0);

    std::ostringstream out;
    for (size_t i = 0; i < expanded_lines.size(); ++i) {
        out << expanded_lines[i] << "\n";
    }
    return out.str();
}

std::string replace_markdown_emojis(const std::string& markdown_content) {
    std::istringstream stream(markdown_content);
    std::string raw_line;
    std::ostringstream out;
    bool in_fence = false;
    char fence_char = '\0';
    size_t fence_len = 0;

    while (std::getline(stream, raw_line)) {
        if (!raw_line.empty() && raw_line.back() == '\r') {
            raw_line.pop_back();
        }

        // Check for fenced code block delimiters: ``` or ~~~
        // Can be indented by whitespace and/or '>' blockquote markers
        size_t indent = 0;
        while (indent < raw_line.length() && (raw_line[indent] == ' ' || raw_line[indent] == '\t' || raw_line[indent] == '>')) {
            ++indent;
        }

        if (indent < raw_line.length() && (raw_line[indent] == '`' || raw_line[indent] == '~')) {
            char ch = raw_line[indent];
            size_t count = 0;
            while (indent + count < raw_line.length() && raw_line[indent + count] == ch) {
                ++count;
            }

            if (count >= 3) {
                if (!in_fence) {
                    in_fence = true;
                    fence_char = ch;
                    fence_len = count;
                    out << raw_line << "\n";
                    continue;
                } else if (in_fence && ch == fence_char && count >= fence_len) {
                    // Check if closing fence has only trailing whitespace
                    size_t rest = indent + count;
                    while (rest < raw_line.length() && (raw_line[rest] == ' ' || raw_line[rest] == '\t')) {
                        ++rest;
                    }
                    if (rest == raw_line.length()) {
                        in_fence = false;
                        fence_char = '\0';
                        fence_len = 0;
                        out << raw_line << "\n";
                        continue;
                    }
                }
            }
        }

        if (in_fence) {
            out << raw_line << "\n";
            continue;
        }

        // Outside fenced code block: process line while respecting inline backtick code spans
        std::string processed_line;
        processed_line.reserve(raw_line.length() + 16);

        size_t idx = 0;
        size_t inline_code_ticks = 0;

        while (idx < raw_line.length()) {
            char c = raw_line[idx];

            if (c == '`') {
                size_t tick_count = 0;
                while (idx + tick_count < raw_line.length() && raw_line[idx + tick_count] == '`') {
                    ++tick_count;
                }

                if (inline_code_ticks == 0) {
                    inline_code_ticks = tick_count;
                } else if (inline_code_ticks == tick_count) {
                    inline_code_ticks = 0;
                }

                processed_line.append(tick_count, '`');
                idx += tick_count;
                continue;
            }

            if (inline_code_ticks > 0) {
                processed_line.push_back(c);
                ++idx;
                continue;
            }

            // Outside inline code: check for emoji shortcode :name:
            if (c == ':') {
                size_t next_colon = raw_line.find(':', idx + 1);
                if (next_colon != std::string::npos && next_colon > idx + 1) {
                    std::string_view candidate(raw_line.data() + idx + 1, next_colon - idx - 1);
                    bool valid_shortcode_chars = true;
                    for (char sc : candidate) {
                        if (!((sc >= 'a' && sc <= 'z') || (sc >= 'A' && sc <= 'Z') ||
                              (sc >= '0' && sc <= '9') || sc == '_' || sc == '-' || sc == '+')) {
                            valid_shortcode_chars = false;
                            break;
                        }
                    }

                    if (valid_shortcode_chars) {
                        const char* emoji_utf8 = find_emoji(candidate);
                        if (emoji_utf8 != nullptr) {
                            processed_line.append(emoji_utf8);
                            idx = next_colon + 1;
                            continue;
                        }
                    }
                }
            }

            processed_line.push_back(c);
            ++idx;
        }

        out << processed_line << "\n";
    }

    return out.str();
}

bool migrate_content(const std::filesystem::path& src_docs,
                     const std::filesystem::path& dst_src,
                     const ConvertOptions& options,
                     const SnippetOptions& snippet_options,
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

            bool is_md = (ext == ".md" || ext == ".markdown");

            if (is_md && (options.expand_snippets || options.convert_emojis || options.convert_alerts)) {
                std::ifstream ifs(entry.path(), std::ios::binary);
                if (!ifs.is_open()) {
                    error_msg = "Failed to open file: " + entry.path().string();
                    return false;
                }
                std::stringstream buffer;
                buffer << ifs.rdbuf();
                std::string processed = buffer.str();

                if (options.expand_snippets) {
                    processed = expand_markdown_snippets(processed, entry.path(), snippet_options);
                }

                if (options.convert_emojis) {
                    processed = replace_markdown_emojis(processed);
                }

                if (options.convert_alerts) {
                    processed = convert_admonitions_to_github_alerts(processed);
                }

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
