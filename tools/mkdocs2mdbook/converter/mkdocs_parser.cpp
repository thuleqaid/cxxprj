#include "mkdocs_parser.h"
#include <fkYAML/node.hpp>
#include <fstream>
#include <sstream>
#include <algorithm>
#include <cctype>
#include <regex>
#include <set>
#include <map>

namespace converter {

namespace {

std::string trim_string(const std::string& s) {
    auto start = s.find_first_not_of(" \t\r\n\"'");
    if (start == std::string::npos) return "";
    auto end = s.find_last_not_of(" \t\r\n\"'");
    return s.substr(start, end - start + 1);
}

bool ends_with_ci(const std::string& str, const std::string& suffix) {
    if (str.length() < suffix.length()) return false;
    auto str_suffix = str.substr(str.length() - suffix.length());
    std::string s1 = str_suffix, s2 = suffix;
    std::transform(s1.begin(), s1.end(), s1.begin(), [](unsigned char c){ return std::tolower(c); });
    std::transform(s2.begin(), s2.end(), s2.begin(), [](unsigned char c){ return std::tolower(c); });
    return s1 == s2;
}

void parse_nav_element(const fkyaml::node& node, std::vector<NavItem>& out_items, const std::string& key_hint = "") {
    if (node.is_string()) {
        NavItem item;
        item.title = key_hint;
        item.path = node.get_value<std::string>();
        out_items.push_back(item);
    } else if (node.is_sequence()) {
        for (const auto& elem : node) {
            parse_nav_element(elem, out_items, "");
        }
    } else if (node.is_mapping()) {
        for (auto it = node.begin(); it != node.end(); ++it) {
            std::string key = it.key().get_value<std::string>();
            const auto& val = it.value();
            if (val.is_string()) {
                NavItem item;
                item.title = key;
                item.path = val.get_value<std::string>();
                out_items.push_back(item);
            } else if (val.is_sequence() || val.is_mapping()) {
                NavItem group;
                group.title = key;
                parse_nav_element(val, group.children, "");
                out_items.push_back(group);
            }
        }
    }
}

void optimize_nav_tree(std::vector<NavItem>& items) {
    for (auto& item : items) {
        if (!item.children.empty()) {
            optimize_nav_tree(item.children);

            // If this item has no direct path, check if the first child is an index/README
            if (item.path.empty() && !item.children.empty()) {
                const auto& first_child = item.children[0];
                if (first_child.children.empty() && !first_child.path.empty()) {
                    std::string p = first_child.path;
                    if (ends_with_ci(p, "index.md") || ends_with_ci(p, "index.markdown") ||
                        ends_with_ci(p, "README.md") || ends_with_ci(p, "readme.md") ||
                        (!first_child.title.empty() && first_child.title == item.title)) {
                        item.path = first_child.path;
                        item.children.erase(item.children.begin());
                    }
                }
            }
        }
    }
}

} // anonymous namespace

std::string clean_title_from_stem(const std::string& stem) {
    if (stem.empty()) return "Index";
    std::string result;
    bool capitalize_next = true;
    for (size_t i = 0; i < stem.size(); ++i) {
        char c = stem[i];
        if (c == '_' || c == '-') {
            result += ' ';
            capitalize_next = true;
        } else {
            if (capitalize_next) {
                result += static_cast<char>(std::toupper(static_cast<unsigned char>(c)));
                capitalize_next = false;
            } else {
                result += c;
            }
        }
    }
    return result;
}

std::string infer_markdown_title(const std::filesystem::path& file_path, const std::string& fallback_stem) {
    if (std::filesystem::exists(file_path)) {
        std::ifstream file(file_path, std::ios::binary);
        if (file.is_open()) {
            std::string line;
            bool in_frontmatter = false;
            int line_count = 0;
            while (std::getline(file, line) && line_count < 100) {
                ++line_count;
                // Normalize line endings
                if (!line.empty() && line.back() == '\r') line.pop_back();

                std::string trimmed = trim_string(line);

                if (line_count == 1 && trimmed == "---") {
                    in_frontmatter = true;
                    continue;
                }

                if (in_frontmatter) {
                    if (trimmed == "---") {
                        in_frontmatter = false;
                        continue;
                    }
                    if (trimmed.rfind("title:", 0) == 0) {
                        std::string t = trim_string(trimmed.substr(6));
                        if (!t.empty()) {
                            return t;
                        }
                    }
                    continue;
                }

                // Check for H1 markdown header
                if (line.rfind("# ", 0) == 0) {
                    std::string t = trim_string(line.substr(2));
                    if (!t.empty()) {
                        return t;
                    }
                }
            }
        }
    }

    std::string stem = fallback_stem;
    if (stem.empty()) {
        stem = file_path.stem().string();
    }
    return clean_title_from_stem(stem);
}

bool parse_mkdocs_string(const std::string& yaml_content, MkDocsConfig& config, std::string& error_msg) {
    try {
        auto root = fkyaml::node::deserialize(yaml_content);
        if (!root.is_mapping()) {
            error_msg = "Root YAML node is not a mapping.";
            return false;
        }

        if (root.contains("site_name") && root["site_name"].is_string()) {
            config.site_name = root["site_name"].get_value<std::string>();
        }
        if (root.contains("site_author") && root["site_author"].is_string()) {
            config.site_author = root["site_author"].get_value<std::string>();
        }
        if (root.contains("site_description") && root["site_description"].is_string()) {
            config.site_description = root["site_description"].get_value<std::string>();
        }
        if (root.contains("docs_dir") && root["docs_dir"].is_string()) {
            config.docs_dir = root["docs_dir"].get_value<std::string>();
        }

        if (root.contains("theme") && root["theme"].is_mapping()) {
            const auto& theme = root["theme"];
            if (theme.contains("language") && theme["language"].is_string()) {
                config.language = theme["language"].get_value<std::string>();
            }
        }

        if (root.contains("markdown_extensions")) {
            const auto& exts = root["markdown_extensions"];
            auto parse_snippet_ext = [&](const fkyaml::node& ext_node) {
                if (ext_node.is_mapping()) {
                    for (auto it = ext_node.begin(); it != ext_node.end(); ++it) {
                        std::string name = it.key().get_value<std::string>();
                        if (name.find("snippets") != std::string::npos) {
                            const auto& val = it.value();
                            if (val.is_mapping() && val.contains("base_path")) {
                                const auto& bp = val["base_path"];
                                if (bp.is_string()) {
                                    config.snippet_base_paths.push_back(bp.get_value<std::string>());
                                } else if (bp.is_sequence()) {
                                    for (const auto& item : bp) {
                                        if (item.is_string()) {
                                            config.snippet_base_paths.push_back(item.get_value<std::string>());
                                        }
                                    }
                                }
                            }
                        }
                    }
                }
            };

            if (exts.is_sequence()) {
                for (const auto& item : exts) {
                    parse_snippet_ext(item);
                }
            } else if (exts.is_mapping()) {
                parse_snippet_ext(exts);
            }
        }

        if (root.contains("nav") && root["nav"].is_sequence()) {
            config.has_nav = true;
            config.nav.clear();
            parse_nav_element(root["nav"], config.nav, "");
            optimize_nav_tree(config.nav);
        } else {
            config.has_nav = false;
        }

        return true;
    } catch (const std::exception& e) {
        error_msg = std::string("YAML parse error: ") + e.what();
        return false;
    }
}

bool parse_mkdocs_file(const std::filesystem::path& yml_path, MkDocsConfig& config, std::string& error_msg) {
    if (!std::filesystem::exists(yml_path)) {
        error_msg = "Configuration file does not exist: " + yml_path.string();
        return false;
    }

    std::ifstream ifs(yml_path, std::ios::binary);
    if (!ifs.is_open()) {
        error_msg = "Failed to open configuration file: " + yml_path.string();
        return false;
    }

    std::stringstream buffer;
    buffer << ifs.rdbuf();
    return parse_mkdocs_string(buffer.str(), config, error_msg);
}

void discover_docs_directory(const std::filesystem::path& docs_root, MkDocsConfig& config) {
    config.nav.clear();
    config.has_nav = true;

    if (!std::filesystem::exists(docs_root) || !std::filesystem::is_directory(docs_root)) {
        return;
    }

    // Collect all relative paths of markdown files
    std::vector<std::string> rel_files;
    for (const auto& entry : std::filesystem::recursive_directory_iterator(docs_root)) {
        if (entry.is_regular_file()) {
            auto ext = entry.path().extension().string();
            std::transform(ext.begin(), ext.end(), ext.begin(), [](unsigned char c){ return std::tolower(c); });
            if (ext == ".md" || ext == ".markdown") {
                auto rel = std::filesystem::relative(entry.path(), docs_root);
                std::string rel_str = rel.generic_string();
                rel_files.push_back(rel_str);
            }
        }
    }

    std::sort(rel_files.begin(), rel_files.end());

    // If index.md exists at root, ensure it is first
    auto it_index = std::find(rel_files.begin(), rel_files.end(), "index.md");
    if (it_index != rel_files.end() && it_index != rel_files.begin()) {
        std::string idx = *it_index;
        rel_files.erase(it_index);
        rel_files.insert(rel_files.begin(), idx);
    }

    // Build directory hierarchy
    struct DirNode {
        std::string index_path;
        std::vector<std::pair<std::string, std::string>> files; // title, rel_path
        std::map<std::string, DirNode> subdirs;
    };

    DirNode root_node;
    for (const auto& rel_path : rel_files) {
        std::filesystem::path p(rel_path);
        std::string filename = p.filename().string();
        auto parent = p.parent_path().generic_string();

        std::string title = infer_markdown_title(docs_root / rel_path, p.stem().string());

        if (parent.empty()) {
            if (filename == "index.md" || filename == "README.md") {
                root_node.index_path = rel_path;
            } else {
                root_node.files.push_back({title, rel_path});
            }
        } else {
            // Find or create in subdirs
            DirNode* current = &root_node;
            std::filesystem::path cur_p;
            for (const auto& part : p.parent_path()) {
                current = &(current->subdirs[part.generic_string()]);
            }
            if (filename == "index.md" || filename == "README.md") {
                current->index_path = rel_path;
            } else {
                current->files.push_back({title, rel_path});
            }
        }
    }

    // Helper to convert DirNode into NavItem list
    auto convert_dir_node = [](auto& self, const std::string& dir_name, const DirNode& d) -> NavItem {
        NavItem group;
        group.title = clean_title_from_stem(dir_name);
        group.path = d.index_path;

        for (const auto& f : d.files) {
            NavItem child;
            child.title = f.first;
            child.path = f.second;
            group.children.push_back(child);
        }
        for (const auto& sub : d.subdirs) {
            group.children.push_back(self(self, sub.first, sub.second));
        }
        return group;
    };

    if (!root_node.index_path.empty()) {
        NavItem root_idx;
        root_idx.title = infer_markdown_title(docs_root / root_node.index_path, "Home");
        root_idx.path = root_node.index_path;
        config.nav.push_back(root_idx);
    }

    for (const auto& f : root_node.files) {
        NavItem item;
        item.title = f.first;
        item.path = f.second;
        config.nav.push_back(item);
    }

    for (const auto& sub : root_node.subdirs) {
        config.nav.push_back(convert_dir_node(convert_dir_node, sub.first, sub.second));
    }
}

} // namespace converter
