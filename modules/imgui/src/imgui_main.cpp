// dear imgui
// Minimal Application demonstrating integrating the Dear ImGui Test Engine

// You can use this sample to understand the minimum code needed to add Test Engine into your own application.
// This sample implement the instructions provided at https://github.com/ocornut/imgui_test_engine/wiki/Setting-Up.
// You can also quickly build and run this if you'd like to experiment, but there's not much value in running this.

// This particular example use a backend-wrapper helper called 'imgui_app' which is also used by imgui_test_suite.
// We do this in order to make this application supports multiple backends to simplify our own testing.
// Note that imgui_app does not support all backends.
// However, you are not meant and should not need to use imgui_app yourself.
// This is just a helper for app_minimal and imgui_test_suite (applications). imgui_test_engine (library) does not need it.

// You can add Test Engine support to any of the imgui/examples/ applications by copying the Test Engine specific lines.

#include "imgui/imgui_main.h"
// Visual Studio warnings
#ifdef _MSC_VER
#pragma warning (disable: 4996)     // 'This function or variable may be unsafe': strcpy, strdup, sprintf, vsnprintf, sscanf, fopen
#endif

#define IMGUI_DEFINE_MATH_OPERATORS
#include "imgui.h"
#include "imgui_internal.h"
#ifdef IMGUI_ENABLE_FREETYPE
#include "misc/freetype/imgui_freetype.cpp"
#endif

// imgui_app is a helper to wrap multiple Dear ImGui platform/renderer backends
#include "imgui_app.h"

// Test Engine
#include "imgui_te_engine.h"
#include "imgui_te_ui.h"
#include "imgui_te_utils.h"       // ImOsIsDebuggerPresent()
#include "imgui_te_exporters.h"   // ImGuiTestEngineExportFormat when use is uncommented.
#include "imgui_user.h"

#include "common/utils.h"
#include <filesystem>
#include <vector>
static std::vector<std::filesystem::path> getFontDirs()
{
    auto path1 = getExecutablePath().parent_path() / "fonts";
    auto path2 = getSystemPath() / "Fonts";
    return std::vector{path1, path2};
}

static std::string get1stAvailableFont(const std::vector<std::string> &fontlist)
{
    const std::vector<std::filesystem::path> basedirs = getFontDirs();
    for (auto basedir = basedirs.cbegin(); basedir != basedirs.cend(); basedir++) {
        for (auto fontfile = fontlist.cbegin(); fontfile != fontlist.cend(); fontfile++) {
            std::filesystem::path path = (*basedir) / (*fontfile);
            std::error_code ec;
            if (std::filesystem::exists(path, ec) && std::filesystem::is_regular_file(path, ec)) {
                return path.string();
            }
        }
    }
    return "";
}

static std::string getChineseFont()
{
    /* https://fontmeme.com/fonts/noto-sans-sc-font/ */
    const std::vector<std::string> fontlist = {"msyh.ttc", "msyhl.ttc", "msyh.ttf", "simhei.ttf", "simfun.ttc", "NotoSansSC-Regular.otf"};
    return get1stAvailableFont(fontlist);
}
static std::string getEmojiFont()
{
    /* https://fontmeme.com/fonts/noto-emoji-font/ */
    const std::vector<std::string> fontlist = {"seguiemj.ttf", "NotoEmoji-Regular.ttf"};
    return get1stAvailableFont(fontlist);
}

void gui_main()
{
    // Setup application backend
    ImGuiApp* app = ImGuiApp_ImplDefault_Create();

    // Setup Dear ImGui context
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;     // Enable Keyboard Controls
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;      // Enable Gamepad Controls
#ifdef IMGUI_HAS_DOCK
    io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;
#endif
#ifdef IMGUI_HAS_VIEWPORT
    io.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable;
#endif
#if IMGUI_VERSION_NUM >= 19004
#ifdef IMGUI_MAIN_ENABLE_TEST_ENGINE
    io.ConfigDebugIsDebuggerPresent = ImOsIsDebuggerPresent();
#else
    io.ConfigDebugIsDebuggerPresent = false;
#endif
#endif

    // Setup application
    app->DpiAware = true;
    app->SrgbFramebuffer = false;
    app->ClearColor = ImVec4(0.120f, 0.120f, 0.120f, 1.000f);
    app->InitCreateWindow(app, IMGUI_MAIN_WINDOW_TITLE, ImVec2(IMGUI_MAIN_DEFAULT_WIDTH, IMGUI_MAIN_DEFAULT_HEIGHT));
    app->InitBackends(app);

    io.Fonts->AddFontFromFileTTF(getChineseFont().c_str(), 16.0f);
    ImFontConfig cfg;
    cfg.MergeMode = true;
#ifdef IMGUI_ENABLE_FREETYPE
    cfg.FontLoaderFlags |= ImGuiFreeTypeLoaderFlags_LoadColor;
#endif
    /* https://online-fonts.com/fonts/segoe-ui-emoji */
    io.Fonts->AddFontFromFileTTF(getEmojiFont().c_str(), 16.0f, &cfg);

#ifdef IMGUI_MAIN_ENABLE_TEST_ENGINE
    // Setup test engine
    engine = ImGuiTestEngine_CreateContext();
    ImGuiTestEngineIO& test_io = ImGuiTestEngine_GetIO(engine);
    test_io.ConfigVerboseLevel = ImGuiTestVerboseLevel_Info;
    test_io.ConfigVerboseLevelOnError = ImGuiTestVerboseLevel_Debug;
    test_io.ConfigRunSpeed = ImGuiTestRunSpeed_Cinematic; // Default to slowest mode in this demo
    test_io.ScreenCaptureFunc = ImGuiApp_ScreenCaptureFunc;
    test_io.ScreenCaptureUserData = (void*)app;

    // Optional: save test output in junit-compatible XML format.
    //test_io.ExportResultsFilename = "./results.xml";
    //test_io.ExportResultsFormat = ImGuiTestEngineExportFormat_JUnitXml;
#endif /* IMGUI_MAIN_ENABLE_TEST_ENGINE */

    // Setup scaling
    float main_scale = app->DpiScale;
    ImGuiStyle& style = ImGui::GetStyle();
    style.ScaleAllSizes(main_scale);
    style.FontScaleDpi = main_scale;

#ifdef IMGUI_MAIN_ENABLE_TEST_ENGINE
    // Start test engine
    ImGuiTestEngine_Start(engine, ImGui::GetCurrentContext());
    ImGuiTestEngine_InstallDefaultCrashHandler();

    // Register tests
    RegisterAppMinimalTests(engine);
#endif /* IMGUI_MAIN_ENABLE_TEST_ENGINE */

    hook_before_loop();
    // Main loop
    bool aborted = false;
    while (!aborted)
    {
        if (!aborted && !app->NewFrame(app))
            aborted = true;
        if (app->Quit)
            aborted = true;

#ifdef IMGUI_MAIN_ENABLE_TEST_ENGINE
        if (aborted && ImGuiTestEngine_TryAbortEngine(engine))
            break;
#else
        if (aborted)
            break;
#endif /* IMGUI_MAIN_ENABLE_TEST_ENGINE */

        ImGui::NewFrame();

        // Show windows
        construct_ui();
#ifdef IMGUI_MAIN_ENABLE_TEST_ENGINE
        ImGuiTestEngine_ShowTestEngineWindows(engine, nullptr);
#endif /* IMGUI_MAIN_ENABLE_TEST_ENGINE */

        // Render and swap
#ifdef IMGUI_MAIN_ENABLE_TEST_ENGINE
        app->Vsync = test_io.IsRequestingMaxAppSpeed ? false : true;
#else
        app->Vsync = false;
#endif /* IMGUI_MAIN_ENABLE_TEST_ENGINE */
        ImGui::Render();
        app->Render(app);

#ifdef IMGUI_MAIN_ENABLE_TEST_ENGINE
        // Post-swap handler is REQUIRED in order to support screen capture
        ImGuiTestEngine_PreSwap(engine);
        app->Present(app);
        ImGuiTestEngine_PostSwap(engine);
#else
        app->Present(app);
#endif /* IMGUI_MAIN_ENABLE_TEST_ENGINE */

        hook_in_loop();
    }

    // Shutdown
#ifdef IMGUI_MAIN_ENABLE_TEST_ENGINE
    ImGuiTestEngine_Stop(engine);
#endif /* IMGUI_MAIN_ENABLE_TEST_ENGINE */
    app->ShutdownBackends(app);
    app->ShutdownCloseWindow(app);
    ImGui::DestroyContext();

#ifdef IMGUI_MAIN_ENABLE_TEST_ENGINE
    // IMPORTANT: we need to destroy the Dear ImGui context BEFORE the test engine context, so .ini data may be saved.
    ImGuiTestEngine_DestroyContext(engine);
#endif /* IMGUI_MAIN_ENABLE_TEST_ENGINE */

    app->Destroy(app);
}
