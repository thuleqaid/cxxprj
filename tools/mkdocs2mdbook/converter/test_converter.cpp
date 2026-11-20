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

TEST_CASE("End-to-End convert_project") {
    auto temp_dir = std::filesystem::temp_directory_path() / "test_mkdocs2mdbook";
    auto in_dir = temp_dir / "mkdocs_in";
    auto out_dir_verbatim = temp_dir / "mdbook_out_verbatim";
    auto out_dir_alerts = temp_dir / "mdbook_out_alerts";

    std::filesystem::remove_all(temp_dir);
    std::filesystem::create_directories(in_dir / "docs" / "sub");

    // Write mkdocs.yml
    std::ofstream yml(in_dir / "mkdocs.yml");
    yml << "site_name: E2E Test Book\n";
    yml << "site_author: Test Author\n";
    yml << "nav:\n";
    yml << "  - Home: index.md\n";
    yml << "  - Sub Chapter:\n";
    yml << "      - Page One: sub/page1.md\n";
    yml.close();

    // Write docs/index.md
    std::ofstream idx(in_dir / "docs" / "index.md");
    idx << "# Welcome\n\nHello world!\n";
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

        // Verify verbatim content preserved
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
