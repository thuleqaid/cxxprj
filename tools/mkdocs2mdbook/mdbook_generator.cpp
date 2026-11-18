#include "mdbook_generator.h"
#include "mkdocs_parser.h"
#include <fstream>
#include <sstream>

namespace converter {

namespace {

std::string escape_toml_string(const std::string& input) {
    std::string out;
    out.reserve(input.size() + 8);
    for (char c : input) {
        if (c == '\\') {
            out += "\\\\";
        } else if (c == '"') {
            out += "\\\"";
        } else if (c == '\n') {
            out += "\\n";
        } else if (c == '\r') {
            out += "\\r";
        } else if (c == '\t') {
            out += "\\t";
        } else {
            out += c;
        }
    }
    return out;
}

void render_nav_item(const NavItem& item,
                     const std::filesystem::path& docs_root,
                     int indent_level,
                     std::stringstream& ss,
                     size_t& chapter_count) {
    ++chapter_count;
    std::string indent(indent_level * 2, ' ');

    std::string title = item.title;
    if (title.empty() && !item.path.empty()) {
        std::filesystem::path p(item.path);
        title = infer_markdown_title(docs_root / item.path, p.stem().string());
    }
    if (title.empty()) {
        title = "Untitled";
    }

    if (!item.path.empty()) {
        // Convert any Windows backslashes in relative path to forward slashes for mdBook
        std::string rel_link = item.path;
        for (char& c : rel_link) {
            if (c == '\\') c = '/';
        }
        ss << indent << "- [" << title << "](" << rel_link << ")\n";
    } else {
        // Draft chapter / unlinked section header in mdBook
        ss << indent << "- [" << title << "]()\n";
    }

    for (const auto& child : item.children) {
        render_nav_item(child, docs_root, indent_level + 1, ss, chapter_count);
    }
}

} // anonymous namespace

bool generate_book_toml(const MkDocsConfig& config, const std::filesystem::path& outdir, std::string& error_msg) {
    std::filesystem::path toml_path = outdir / "book.toml";
    std::ofstream ofs(toml_path, std::ios::binary);
    if (!ofs.is_open()) {
        error_msg = "Failed to create book.toml at: " + toml_path.string();
        return false;
    }

    ofs << "[book]\n";
    ofs << "title = \"" << escape_toml_string(config.site_name) << "\"\n";

    if (!config.site_author.empty()) {
        ofs << "authors = [\"" << escape_toml_string(config.site_author) << "\"]\n";
    } else {
        ofs << "authors = []\n";
    }

    if (!config.site_description.empty()) {
        ofs << "description = \"" << escape_toml_string(config.site_description) << "\"\n";
    }

    ofs << "language = \"" << escape_toml_string(config.language) << "\"\n";
    ofs << "src = \"src\"\n\n";

    ofs << "[build]\n";
    ofs << "build-dir = \"book\"\n";
    ofs << "create-missing = false\n";

    return true;
}

std::string build_summary_content(const MkDocsConfig& config,
                                 const std::filesystem::path& docs_root,
                                 size_t& chapter_count) {
    std::stringstream ss;
    chapter_count = 0;

    ss << "# Summary\n\n";

    for (const auto& item : config.nav) {
        render_nav_item(item, docs_root, 0, ss, chapter_count);
    }

    return ss.str();
}

bool generate_summary_md(const MkDocsConfig& config,
                         const std::filesystem::path& docs_root,
                         const std::filesystem::path& outdir,
                         size_t& chapter_count,
                         std::string& error_msg) {
    std::filesystem::path src_dir = outdir / "src";
    if (!std::filesystem::exists(src_dir)) {
        std::error_code ec;
        std::filesystem::create_directories(src_dir, ec);
        if (ec) {
            error_msg = "Failed to create directory " + src_dir.string() + ": " + ec.message();
            return false;
        }
    }

    std::filesystem::path summary_path = src_dir / "SUMMARY.md";
    std::ofstream ofs(summary_path, std::ios::binary);
    if (!ofs.is_open()) {
        error_msg = "Failed to create SUMMARY.md at: " + summary_path.string();
        return false;
    }

    std::string content = build_summary_content(config, docs_root, chapter_count);
    ofs << content;
    return true;
}

} // namespace converter
