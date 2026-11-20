#ifndef __CONVERTER_MKDOCS_CONFIG_H__
#define __CONVERTER_MKDOCS_CONFIG_H__

#include <string>
#include <vector>

namespace converter {

struct NavItem {
    std::string title;
    std::string path;
    std::vector<NavItem> children;

    bool is_group() const {
        return !children.empty();
    }
};

struct MkDocsConfig {
    std::string site_name = "My Book";
    std::string site_author;
    std::string site_description;
    std::string language = "en";
    std::string docs_dir = "docs";
    std::vector<std::string> snippet_base_paths; // Base paths configured for pymdownx.snippets
    std::vector<NavItem> nav;
    bool has_nav = false;
};

} // namespace converter

#endif // __CONVERTER_MKDOCS_CONFIG_H__
