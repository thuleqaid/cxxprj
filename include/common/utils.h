#ifndef __COMMON_UTILS_H__
#define __COMMON_UTILS_H__

#include <filesystem>

enum PEArchitecture
{
    PE_ARCH_ERROR = -1,
    PE_ARCH_UNKNOWN = 0,
    PE_ARCH_X86 = 1,    // 32-bit (0x014c)
    PE_ARCH_X64 = 2     // 64-bit (0x8664)
};

std::filesystem::path getExecutablePath();
std::filesystem::path getSystemPath();
bool isFileExists(const std::filesystem::path &path);
bool isFileReadOnly(const std::filesystem::path &path);
PEArchitecture GetPEArchitecture(const char* filePath);
bool RunProcessSilent(const std::string& cmd, std::string& output);

#endif /* __COMMON_UTILS_H__ */