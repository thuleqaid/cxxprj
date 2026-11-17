#include "spdlog/cfg/argv.h"
#include "common/jsonconfig.h"
#ifdef APP_SUBSYSTEM_GUI
#include "imgui/imgui_main.h"
#endif
#include "console_main.h"

static void default_config(json& config);
static void setup_options(cxxopts::Options& options);
static void update_config(const cxxopts::ParseResult& result, json& config);

int main(int argc, char *argv[])
{
    auto config = JsonConfig::instance();
    config->register_default_hook(default_config);
    config->register_setup_hook(setup_options);
    config->register_update_hook(update_config);
    /* Run program: start /wait ./program SPDLOG_LEVEL=trace,LoggerName_1=debug,LoggerName_2=info,... Options_Defined_By_Cxxopts */
    if (config->parse_args(argc, argv)) {
        exit(0);
    }
    spdlog::cfg::load_argv_levels(argc, argv);
    if (config->isConsole()) {
        console_main();
    } else {
#ifdef APP_SUBSYSTEM_GUI
        gui_main();
#endif
    }
    return 0;
}

static void default_config(json& config)
{
    // Todo: set default values of config
    config["common"]["param_a"] = 10;
    config["common"]["param_b"] = 20;
}

static void setup_options(cxxopts::Options& options)
{
    // Todo: add extra options to options
    options.add_options()("a,param_a", "Param_A", cxxopts::value<int>());
    options.add_options()("b,param_b", "Param_B", cxxopts::value<int>());
}

static void update_config(const cxxopts::ParseResult& result, json& config)
{
    // Todo: update config according to result
    if (result.count("param_a"))
        config["common"]["param_a"] = result["param_a"].as<int>();
    if (result.count("param_b"))
        config["common"]["param_b"] = result["param_b"].as<int>();
}
