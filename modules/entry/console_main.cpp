#include "console_main.h"
#include "spdlog/spdlog.h"
#include "common/jsonconfig.h"
#include <iostream>

void console_main()
{
    spdlog::info("Welcome to SpdLog!");
    spdlog::debug("Welcome to SpdLog!");
    auto config = JsonConfig::instance()->get_config();
    std::cout << config << std::endl;
}
