#ifndef __COMMON_JSON_CONFIG_H__
#define __COMMON_JSON_CONFIG_H__
#include "nlohmann/json.hpp"
using json = nlohmann::json;
#include "cxxopts/cxxopts.hpp"

#ifndef PROG_NAME
    #define PROG_NAME "MyProgram"
#endif
#ifndef PROG_DESCRIPTION
    #define PROG_DESCRIPTION "A brief description"
#endif

class JsonConfig {
    public:
        using FUNCPTR_DEFAULT=void (json&);
        using FUNCPTR_SETUP=void (cxxopts::Options&);
        using FUNCPTR_UPDATE=void (const cxxopts::ParseResult&, json&);
        static JsonConfig* instance();
        void register_default_hook(FUNCPTR_DEFAULT* fptr);
        void register_setup_hook(FUNCPTR_SETUP* fptr);
        void register_update_hook(FUNCPTR_UPDATE* fptr);
        bool parse_args(int argc, const char* const* argv);
        bool isConsole() const;
        bool isAutoUpdate() const;
        json& get_config();

    private:
        JsonConfig(const char* prog_name=PROG_NAME, const char* prog_desc=PROG_DESCRIPTION);
        JsonConfig(const JsonConfig&) = delete;
        virtual ~JsonConfig();
        JsonConfig& operator=(const JsonConfig&) = delete;

    private:
        cxxopts::Options options_;
        cxxopts::ParseResult result_;
        std::string setting_file_;
        json config_;
        FUNCPTR_DEFAULT *func_default_;
        FUNCPTR_SETUP *func_setup_;
        FUNCPTR_UPDATE *func_update_;
        void setupConsole();
        bool isHelp() const;
};

#endif /* __COMMON_JSON_CONFIG_H__ */