
#include "common/utils.h"
#include <windows.h>
#include <vector>

std::filesystem::path getExecutablePath()
{
    wchar_t szExe[MAX_PATH] = { 0 };
    if (GetModuleFileNameW(NULL, szExe, MAX_PATH)) {
        return std::filesystem::path(szExe);
    } else {
        return std::filesystem::path();
    }
}

std::filesystem::path getSystemPath()
{
    char winDir[MAX_PATH] = { 0 };
    if (GetWindowsDirectoryA(winDir, MAX_PATH)) {
        return std::filesystem::path(winDir);
    } else {
        return std::filesystem::path();
    }
}

bool isFileExists(const std::filesystem::path &path)
{
    std::error_code ec;
    if (std::filesystem::exists(path, ec) && std::filesystem::is_regular_file(path, ec)) {
        return true;
    } else {
        return false;
    }
}

bool isFileReadOnly(const std::filesystem::path &path)
{
    DWORD attributes;
    if (path.is_relative()) {
        auto fullpath = getExecutablePath().parent_path() / path;
        attributes = GetFileAttributesW(fullpath.wstring().c_str());
    } else {
        attributes = GetFileAttributesW(path.wstring().c_str());
    }

    // 检查是否出错
    if (attributes == INVALID_FILE_ATTRIBUTES) {
        // 获取错误代码
        DWORD errorCode = GetLastError();
        
        // 文件不存在
        if (errorCode == ERROR_FILE_NOT_FOUND) {
            return false;
        }
        // 路径未找到
        if (errorCode == ERROR_PATH_NOT_FOUND) {
            return false;
        }
        // 访问被拒绝
        if (errorCode == ERROR_ACCESS_DENIED) {
            return false;
        }
        // 其他错误，返回 false
        return false;
    }
    
    // 检查是否设置了 FILE_ATTRIBUTE_READONLY 标志
    return (attributes & FILE_ATTRIBUTE_READONLY) != 0;
}

PEArchitecture GetPEArchitecture(const char* filePath)
{
    if (!filePath || filePath[0] == '\0')
        return PE_ARCH_ERROR;
    FILE* fp = fopen(filePath, "rb");
    if (!fp)
    {
        std::filesystem::path p(filePath);
        if (p.is_relative())
        {
            std::filesystem::path fullpath = getExecutablePath().parent_path() / p;
            fp = _wfopen(fullpath.c_str(), L"rb");
        }
    }
    if (!fp)
        return PE_ARCH_ERROR;

    IMAGE_DOS_HEADER dosHeader;
    if (fread(&dosHeader, 1, sizeof(dosHeader), fp) != sizeof(dosHeader))
    {
        fclose(fp);
        return PE_ARCH_ERROR;
    }
    if (dosHeader.e_magic != IMAGE_DOS_SIGNATURE)
    {
        fclose(fp);
        return PE_ARCH_ERROR;
    }
    if (fseek(fp, dosHeader.e_lfanew, SEEK_SET) != 0)
    {
        fclose(fp);
        return PE_ARCH_ERROR;
    }
    DWORD peSignature = 0;
    if (fread(&peSignature, 1, sizeof(peSignature), fp) != sizeof(peSignature) || peSignature != IMAGE_NT_SIGNATURE)
    {
        fclose(fp);
        return PE_ARCH_ERROR;
    }
    IMAGE_FILE_HEADER fileHeader;
    if (fread(&fileHeader, 1, sizeof(fileHeader), fp) != sizeof(fileHeader))
    {
        fclose(fp);
        return PE_ARCH_ERROR;
    }
    fclose(fp);

    if (fileHeader.Machine == IMAGE_FILE_MACHINE_I386)
        return PE_ARCH_X86;
    if (fileHeader.Machine == IMAGE_FILE_MACHINE_AMD64)
        return PE_ARCH_X64;
    return PE_ARCH_UNKNOWN;
}

bool RunProcessSilent(const std::string& cmd, std::string& output)
{
    HANDLE hReadPipe = NULL;
    HANDLE hWritePipe = NULL;
    SECURITY_ATTRIBUTES sa = { sizeof(SECURITY_ATTRIBUTES), NULL, TRUE };
    if (!CreatePipe(&hReadPipe, &hWritePipe, &sa, 0))
        return false;
    SetHandleInformation(hReadPipe, HANDLE_FLAG_INHERIT, 0);

    STARTUPINFOA si = { sizeof(STARTUPINFOA) };
    si.dwFlags = STARTF_USESTDHANDLES | STARTF_USESHOWWINDOW;
    si.hStdOutput = hWritePipe;
    si.hStdError = hWritePipe;
    si.wShowWindow = SW_HIDE;

    PROCESS_INFORMATION pi = { 0 };
    std::vector<char> cmdVec(cmd.begin(), cmd.end());
    cmdVec.push_back('\0');

    if (!CreateProcessA(NULL, cmdVec.data(), NULL, NULL, TRUE, CREATE_NO_WINDOW, NULL, NULL, &si, &pi))
    {
        CloseHandle(hWritePipe);
        CloseHandle(hReadPipe);
        return false;
    }

    CloseHandle(hWritePipe);

    char buffer[512];
    DWORD bytesRead = 0;
    output.clear();
    while (ReadFile(hReadPipe, buffer, sizeof(buffer) - 1, &bytesRead, NULL) && bytesRead > 0)
    {
        buffer[bytesRead] = '\0';
        output += buffer;
    }

    WaitForSingleObject(pi.hProcess, 5000);
    DWORD exitCode = 0;
    GetExitCodeProcess(pi.hProcess, &exitCode);

    CloseHandle(pi.hProcess);
    CloseHandle(pi.hThread);
    CloseHandle(hReadPipe);

    return (exitCode == 0);
}
