#ifdef TEST_MODE
#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "doctest/doctest.h"

#include "mkdocs_config.h"
#include "mkdocs_parser.h"
#include "mdbook_generator.h"
#include "markdown_processor.h"
#include "converter.h"

#include <filesystem>
#include <fstream>

using namespace converter;

TEST_CASE("Title inference and stem cleaning") {
    CHECK(clean_title_from_stem("index") == "Index");
    CHECK(clean_title_from_stem("arbitrary_types") == "Arbitrary Types");
    CHECK(clean_title_from_stem("fast-fourier-transform") == "Fast Fourier Transform");
    CHECK(clean_title_from_stem("") == "Index");
}

TEST_CASE("MkDocs YAML Parsing - Basic and Nav") {
    std::string yaml = R"(
site_name: My Awesome Book
site_author: Jane Doe
site_description: A comprehensive guide
docs_dir: custom_docs
theme:
  language: zh
nav:
  - Home:
      - index.md
      - license.md
  - Guide:
      - Getting Started: guide/getting_started.md
      - Advanced:
          - guide/advanced/deep.md
)";

    MkDocsConfig config;
    std::string err;
    bool ok = parse_mkdocs_string(yaml, config, err);
    CHECK(ok);
    CHECK(err.empty());
    CHECK(config.site_name == "My Awesome Book");
    CHECK(config.site_author == "Jane Doe");
    CHECK(config.site_description == "A comprehensive guide");
    CHECK(config.language == "zh");
    CHECK(config.docs_dir == "custom_docs");
    CHECK(config.has_nav == true);
    REQUIRE(config.nav.size() == 2);

    // Home group optimized with index.md
    CHECK(config.nav[0].title == "Home");
    CHECK(config.nav[0].path == "index.md");
    REQUIRE(config.nav[0].children.size() == 1);
    CHECK(config.nav[0].children[0].path == "license.md");

    // Guide group
    CHECK(config.nav[1].title == "Guide");
    REQUIRE(config.nav[1].children.size() == 2);
    CHECK(config.nav[1].children[0].title == "Getting Started");
    CHECK(config.nav[1].children[0].path == "guide/getting_started.md");
    CHECK(config.nav[1].children[1].title == "Advanced");
    REQUIRE(config.nav[1].children[1].children.size() == 1);
    CHECK(config.nav[1].children[1].children[0].path == "guide/advanced/deep.md");
}

TEST_CASE("Markdown Admonitions Conversion") {
    SUBCASE("Simple note without title") {
        std::string input = 
            "Introduction text.\n"
            "\n"
            "!!! note\n"
            "    Line 1 of note.\n"
            "    Line 2 of note.\n"
            "\n"
            "Ending text.\n";

        std::string expected = 
            "Introduction text.\n"
            "\n"
            "> [!NOTE]\n"
            "> Line 1 of note.\n"
            "> Line 2 of note.\n"
            "\n"
            "Ending text.\n";

        std::string actual = convert_admonitions_to_github_alerts(input);
        CHECK(actual == expected);
    }

    SUBCASE("Warning with custom title and blank lines") {
        std::string input = 
            "!!! warning \"High Voltage\"\n"
            "    Do not touch the terminal.\n"
            "\n"
            "    Risk of electric shock.\n"
            "\n"
            "Outside paragraph.\n";

        std::string expected = 
            "> [!WARNING]\n"
            "> **High Voltage**\n"
            ">\n"
            "> Do not touch the terminal.\n"
            ">\n"
            "> Risk of electric shock.\n"
            "\n"
            "Outside paragraph.\n";

        std::string actual = convert_admonitions_to_github_alerts(input);
        CHECK(actual == expected);
    }

    SUBCASE("Collapsible tip with single quotes") {
        std::string input = 
            "???+ tip 'Pro Tip'\n"
            "    Use cmake --build build.\n";

        std::string expected = 
            "> [!TIP]\n"
            "> **Pro Tip**\n"
            ">\n"
            "> Use cmake --build build.\n";

        std::string actual = convert_admonitions_to_github_alerts(input);
        CHECK(actual == expected);
    }
}

TEST_CASE("Markdown Snippets Expansion") {
    auto temp_dir = std::filesystem::temp_directory_path() / "test_snippets";
    std::filesystem::remove_all(temp_dir);
    std::filesystem::create_directories(temp_dir / "examples");
    std::filesystem::create_directories(temp_dir / "docs");

    // Create a sample source file with license header and sections
    std::filesystem::path code_file = temp_dir / "examples" / "sample.cpp";
    {
        std::ofstream ofs(code_file);
        ofs << "// Line 1: Header\n";
        ofs << "// Line 2: License\n";
        ofs << "#include <iostream>\n";
        ofs << "// --8<-- [start:main_func]\n";
        ofs << "int main() {\n";
        ofs << "    std::cout << \"Hello!\" << std::endl;\n";
        ofs << "    return 0;\n";
        ofs << "}\n";
        ofs << "// --8<-- [end:main_func]\n";
        ofs << "// Line 10: Trailing comment\n";
    }

    SnippetOptions opts;
    opts.base_paths.push_back(temp_dir / "examples");
    opts.docs_root = temp_dir / "docs";
    opts.project_root = temp_dir;
    opts.dedent_subsections = true;

    SUBCASE("Include from line 3 to end (:3)") {
        std::string md = 
            "```cpp\n"
            "--8<-- \"sample.cpp:3\"\n"
            "```\n";
        
        std::string res = expand_markdown_snippets(md, temp_dir / "docs" / "page.md", opts);
        CHECK(res.find("// Line 1: Header") == std::string::npos);
        CHECK(res.find("#include <iostream>") != std::string::npos);
        CHECK(res.find("int main() {") != std::string::npos);
        CHECK(res.find("// Line 10: Trailing comment") != std::string::npos);
    }

    SUBCASE("Include line range with indent preservation") {
        std::string md = 
            "    ```cpp\n"
            "    --8<-- \"sample.cpp:5:8\"\n"
            "    ```\n";

        std::string res = expand_markdown_snippets(md, temp_dir / "docs" / "page.md", opts);
        CHECK(res.find("    int main() {") != std::string::npos);
        CHECK(res.find("        std::cout << \"Hello!\" << std::endl;") != std::string::npos);
        CHECK(res.find("        return 0;") != std::string::npos);
        CHECK(res.find("    }") != std::string::npos);
        CHECK(res.find("#include <iostream>") == std::string::npos);
    }

    SUBCASE("Include section") {
        std::string md = 
            "```cpp\n"
            "--8<-- \"sample.cpp:main_func\"\n"
            "```\n";

        std::string res = expand_markdown_snippets(md, temp_dir / "docs" / "page.md", opts);
        CHECK(res.find("#include <iostream>") == std::string::npos);
        CHECK(res.find("int main() {") != std::string::npos);
        CHECK(res.find("    return 0;") != std::string::npos);
        CHECK(res.find("// Line 10: Trailing comment") == std::string::npos);
        CHECK(res.find("main_func") == std::string::npos); // Section marker is stripped
    }

    SUBCASE("Escaped snippet marker") {
        std::string md = 
            ";--8<-- \"sample.cpp\"\n";

        std::string res = expand_markdown_snippets(md, temp_dir / "docs" / "page.md", opts);
        CHECK(res.find("--8<-- \"sample.cpp\"") != std::string::npos);
        CHECK(res.find("#include") == std::string::npos);
    }

    SUBCASE("Snippet inside admonition with alert conversion") {
        std::string md = 
            "???+ example \"Example\"\n"
            "\n"
            "    ```cpp\n"
            "    --8<-- \"sample.cpp:5:7\"\n"
            "    ```\n";

        std::string expanded = expand_markdown_snippets(md, temp_dir / "docs" / "page.md", opts);
        std::string alert = convert_admonitions_to_github_alerts(expanded);

        CHECK(alert.find("> [!NOTE]") != std::string::npos);
        CHECK(alert.find("> **Example**") != std::string::npos);
        CHECK(alert.find("> ```cpp") != std::string::npos);
        CHECK(alert.find("> int main() {") != std::string::npos);
        CHECK(alert.find(">     std::cout << \"Hello!\" << std::endl;") != std::string::npos);
    }

    std::filesystem::remove_all(temp_dir);
}

TEST_CASE("MkDocs YAML Parsing - pymdownx.snippets") {
    std::string yaml_content = 
        "site_name: Snippet Config Test\n"
        "markdown_extensions:\n"
        "  - admonition\n"
        "  - pymdownx.snippets:\n"
        "      base_path:\n"
        "        - ../examples\n"
        "        - snippets\n"
        "      check_paths: true\n";

    MkDocsConfig config;
    std::string err;
    bool ok = parse_mkdocs_string(yaml_content, config, err);
    CHECK(ok);
    CHECK(config.snippet_base_paths.size() == 2);
    CHECK(config.snippet_base_paths[0] == "../examples");
    CHECK(config.snippet_base_paths[1] == "snippets");
}

TEST_CASE("Markdown Emoji Conversion") {
    SUBCASE("Common emoji shortcodes") {
        std::string md = "### :memo: **Well Documented** and :rocket: fast!";
        std::string res = replace_markdown_emojis(md);
        CHECK(res == "### \xf0\x9f\x93\x9d **Well Documented** and \xf0\x9f\x9a\x80 fast!\n");
    }

    SUBCASE("Modifiers and aliases") {
        std::string md = ":+1: :-1: :sparkles: :boom: :people_holding_hands:";
        std::string res = replace_markdown_emojis(md);
        CHECK(res.find("\xf0\x9f\x91\x8d") != std::string::npos); // +1 👍
        CHECK(res.find("\xf0\x9f\x91\x8e") != std::string::npos); // -1 👎
        CHECK(res.find("\xe2\x9c\xa8") != std::string::npos);     // sparkles ✨
        CHECK(res.find("\xf0\x9f\x92\xa5") != std::string::npos); // boom 💥
    }

    SUBCASE("Inline code protection") {
        std::string md = "Use `:memo:` as the shortcode for :memo: here.";
        std::string res = replace_markdown_emojis(md);
        CHECK(res.find("`:memo:`") != std::string::npos);
        CHECK(res.find("\xf0\x9f\x93\x9d") != std::string::npos);
    }

    SUBCASE("Fenced code block protection") {
        std::string md = 
            "# Title\n\n"
            "```cpp\n"
            "// Do not replace :memo: or :rocket:\n"
            "fkyaml::node_t::SEQUENCE\n"
            "```\n\n"
            ":memo: outside code block.\n";

        std::string res = replace_markdown_emojis(md);
        CHECK(res.find("// Do not replace :memo: or :rocket:") != std::string::npos);
        CHECK(res.find("fkyaml::node_t::SEQUENCE") != std::string::npos);
        CHECK(res.find("\xf0\x9f\x93\x9d outside code block.") != std::string::npos);
    }

    SUBCASE("C++ double colons in normal text") {
        std::string md = "Refer to `fkyaml::node_t::SEQUENCE` and fkyaml::node_t::SEQUENCE.";
        std::string res = replace_markdown_emojis(md);
        CHECK(res == "Refer to `fkyaml::node_t::SEQUENCE` and fkyaml::node_t::SEQUENCE.\n");
    }

    SUBCASE("Non-existent shortcode") {
        std::string md = "This is :not_an_actual_emoji_shortcode: text.";
        std::string res = replace_markdown_emojis(md);
        CHECK(res == "This is :not_an_actual_emoji_shortcode: text.\n");
    }
}

TEST_CASE("End-to-End convert_project") {
    auto temp_dir = std::filesystem::temp_directory_path() / "test_mkdocs2mdbook";
    auto in_dir = temp_dir / "mkdocs_in";
    auto out_dir_verbatim = temp_dir / "mdbook_out_verbatim";
    auto out_dir_alerts = temp_dir / "mdbook_out_alerts";

    std::filesystem::remove_all(temp_dir);
    std::filesystem::create_directories(in_dir / "docs" / "sub");
    std::filesystem::create_directories(in_dir / "examples");

    // Write a snippet file in examples/code.cpp
    std::ofstream code_ofs(in_dir / "examples" / "code.cpp");
    code_ofs << "// Header line\n";
    code_ofs << "const int answer = 42;\n";
    code_ofs.close();

    // Write mkdocs.yml
    std::ofstream yml(in_dir / "mkdocs.yml");
    yml << "site_name: E2E Test Book\n";
    yml << "site_author: Test Author\n";
    yml << "markdown_extensions:\n";
    yml << "  - pymdownx.snippets:\n";
    yml << "      base_path: examples\n";
    yml << "nav:\n";
    yml << "  - Home: index.md\n";
    yml << "  - Sub Chapter:\n";
    yml << "      - Page One: sub/page1.md\n";
    yml.close();

    // Write docs/index.md with a snippet
    std::ofstream idx(in_dir / "docs" / "index.md");
    idx << "# Welcome\n\n```cpp\n--8<-- \"code.cpp:2\"\n```\n";
    idx.close();

    // Write docs/sub/page1.md with an admonition
    std::ofstream p1(in_dir / "docs" / "sub" / "page1.md");
    p1 << "# Page 1\n\n!!! note \"Note Title\"\n    Content here.\n";
    p1.close();

    SUBCASE("Convert verbatim (convert_alerts = false)") {
        ConvertOptions opts;
        opts.convert_alerts = false;
        auto res = convert_project(in_dir, out_dir_verbatim, opts);
        CHECK(res.success);
        CHECK(res.error_message.empty());
        CHECK(res.converted_files == 2);

        // Verify book.toml
        CHECK(std::filesystem::exists(out_dir_verbatim / "book.toml"));

        // Verify SUMMARY.md
        CHECK(std::filesystem::exists(out_dir_verbatim / "src" / "SUMMARY.md"));
        std::ifstream sum_file(out_dir_verbatim / "src" / "SUMMARY.md");
        std::stringstream ss;
        ss << sum_file.rdbuf();
        std::string sum_str = ss.str();
        CHECK(sum_str.find("- [Home](index.md)") != std::string::npos);
        CHECK(sum_str.find("- [Page One](sub/page1.md)") != std::string::npos);

        // Verify snippet expansion in index.md
        std::ifstream idx_res(out_dir_verbatim / "src" / "index.md");
        std::stringstream idx_ss;
        idx_ss << idx_res.rdbuf();
        std::string idx_content = idx_ss.str();
        CHECK(idx_content.find("const int answer = 42;") != std::string::npos);
        CHECK(idx_content.find("// Header line") == std::string::npos);
        CHECK(idx_content.find("--8<--") == std::string::npos);

        // Verify verbatim content preserved for alerts
        std::ifstream p1_res(out_dir_verbatim / "src" / "sub" / "page1.md");
        std::stringstream p1_ss;
        p1_ss << p1_res.rdbuf();
        CHECK(p1_ss.str().find("!!! note \"Note Title\"") != std::string::npos);
    }

    SUBCASE("Convert with GitHub alerts (convert_alerts = true)") {
        ConvertOptions opts;
        opts.convert_alerts = true;
        auto res = convert_project(in_dir, out_dir_alerts, opts);
        CHECK(res.success);

        // Verify alert was converted
        std::ifstream p1_res(out_dir_alerts / "src" / "sub" / "page1.md");
        std::stringstream p1_ss;
        p1_ss << p1_res.rdbuf();
        std::string content = p1_ss.str();
        CHECK(content.find("> [!NOTE]") != std::string::npos);
        CHECK(content.find("> **Note Title**") != std::string::npos);
        CHECK(content.find("> Content here.") != std::string::npos);
    }

    // Cleanup
    std::filesystem::remove_all(temp_dir);
}

#endif
