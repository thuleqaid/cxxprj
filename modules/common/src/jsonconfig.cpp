#include <fstream>
#include <iostream>
#ifdef APP_SUBSYSTEM_GUI
#include <windows.h>
#endif
#include "common/utils.h"
#include "common/jsonconfig.h"

static std::string SanitizeJson(const std::string& input);
static void loadConfig(json& default_config, const std::string& setting_file);
static bool saveConfig(const json& j, const std::string& setting_file);

JsonConfig::JsonConfig(const char* prog_name, const char* prog_desc):options_(prog_name, prog_desc),
    result_(), setting_file_(""), config_(),
    func_default_(nullptr), func_setup_(nullptr), func_update_(nullptr)
{
}
JsonConfig::~JsonConfig()
{
}

JsonConfig* JsonConfig::instance()
{
    static JsonConfig* instance_ = nullptr;
    if (instance_ == nullptr) {
        instance_ = new JsonConfig();
    }
    return instance_;
}

void JsonConfig::register_default_hook(FUNCPTR_DEFAULT* fptr)
{
    func_default_ = fptr;
}

void JsonConfig::register_setup_hook(FUNCPTR_SETUP* fptr)
{
    func_setup_ = fptr;
}

void JsonConfig::register_update_hook(FUNCPTR_UPDATE* fptr)
{
    func_update_ = fptr;
}

bool JsonConfig::parse_args(int argc, const char* const* argv)
{
#ifdef APP_SUBSYSTEM_GUI
    options_.add_options()
        ("c,console", "Console mode", cxxopts::value<bool>());
#endif
    if (func_setup_ != nullptr) func_setup_(options_);
    options_.add_options()("s,setting", "setting file path", cxxopts::value<std::string>());
    options_.add_options()("u,update", "auto update setting file");
    options_.add_options()("h,help", "Print usage");

    result_ = options_.parse(argc, argv);
    setupConsole();
    if (isHelp()) {
        return true;
    } else {
        config_.clear();
        if (func_default_ != nullptr) func_default_(config_);
        if (result_.count("setting")) setting_file_ = result_["setting"].as<std::string>();
        loadConfig(config_, setting_file_);
        if (func_update_ != nullptr) func_update_(result_, config_);
        if (isAutoUpdate()) saveConfig(config_, setting_file_);
        return false;
    }
}

bool JsonConfig::isConsole() const
{
#ifdef APP_SUBSYSTEM_GUI
    return result_["console"].as<bool>();
#else
    return true;
#endif
}

bool JsonConfig::isAutoUpdate() const
{
    if (result_.count("update") && !setting_file_.empty())
        return true;
    else
        return false;
}

json& JsonConfig::get_config()
{
    return config_;
}

void JsonConfig::setupConsole()
{
#ifdef APP_SUBSYSTEM_GUI
    bool console = isConsole();
    bool help = result_.count("help");
    if (console || help) {
        if (AttachConsole(ATTACH_PARENT_PROCESS)) {
            // Attach to parent console
        } else {
            // Create console
            AllocConsole();
        }
        freopen("CONIN$", "r", stdin);
        freopen("CONOUT$", "w", stdout);
        freopen("CONOUT$", "w", stderr);
    }
#endif
}

bool JsonConfig::isHelp() const
{
    if (result_.count("help")) {
        std::cout << options_.help() << std::endl;
        return true;
    } else {
        return false;
    }
}

static void loadConfig(json& default_config, const std::string& setting_file)
{
    auto fullpath = getExecutablePath().parent_path() / setting_file;
    if (isFileExists(fullpath)) {
        std::ifstream ifs(fullpath);
        if (ifs.is_open())
        {
            std::stringstream buffer;
            buffer << ifs.rdbuf();
            ifs.close();
            try {
                std::string sanitized = SanitizeJson(buffer.str());
                json j_file = json::parse(sanitized);
                default_config.update(j_file);
            } catch (const std::exception& e) {
            }
        }
    }
}

static bool saveConfig(const json& j, const std::string& setting_file)
{
    auto fullpath = getExecutablePath().parent_path() / setting_file;
    bool save_file = true;
    if (isFileExists(fullpath) && isFileReadOnly(fullpath)) {
        save_file = false;
    }
    if (save_file) {
        std::ofstream ofs(fullpath);
        if (ofs.is_open())
        {
            ofs << j.dump(4);
            ofs.close();
        }
    }
    return save_file;
}

static std::string SanitizeJson(const std::string& input)
{
    std::string output = input;
    bool in_string = false;
    bool escape = false;

    // First pass: remove comments while preserving string literals
    for (size_t i = 0; i < output.size(); ++i)
    {
        char c = output[i];
        if (in_string)
        {
            if (escape)
            {
                escape = false;
            }
            else if (c == '\\')
            {
                escape = true;
            }
            else if (c == '"')
            {
                in_string = false;
            }
        }
        else
        {
            if (c == '"')
            {
                in_string = true;
            }
            else if (c == '/' && i + 1 < output.size())
            {
                if (output[i + 1] == '/')
                {
                    // Line comment: replace with spaces until newline
                    output[i] = ' ';
                    output[i + 1] = ' ';
                    i += 2;
                    while (i < output.size() && output[i] != '\n' && output[i] != '\r')
                    {
                        output[i] = ' ';
                        i++;
                    }
                }
                else if (output[i + 1] == '*')
                {
                    // Block comment: replace with spaces until */
                    output[i] = ' ';
                    output[i + 1] = ' ';
                    i += 2;
                    while (i + 1 < output.size() && !(output[i] == '*' && output[i + 1] == '/'))
                    {
                        if (output[i] != '\n' && output[i] != '\r')
                            output[i] = ' ';
                        i++;
                    }
                    if (i + 1 < output.size())
                    {
                        output[i] = ' ';
                        output[i + 1] = ' ';
                        i++;
                    }
                }
            }
        }
    }

    // Second pass: remove trailing commas before ] or }
    in_string = false;
    escape = false;
    for (size_t i = 0; i < output.size(); ++i)
    {
        char c = output[i];
        if (in_string)
        {
            if (escape)
                escape = false;
            else if (c == '\\')
                escape = true;
            else if (c == '"')
                in_string = false;
        }
        else
        {
            if (c == '"')
            {
                in_string = true;
            }
            else if (c == ',')
            {
                // Look ahead to next non-whitespace char
                size_t j = i + 1;
                while (j < output.size() && (output[j] == ' ' || output[j] == '\t' || output[j] == '\n' || output[j] == '\r'))
                {
                    j++;
                }
                if (j < output.size() && (output[j] == ']' || output[j] == '}'))
                {
                    output[i] = ' '; // Replace trailing comma with space
                }
            }
        }
    }

    return output;
}
