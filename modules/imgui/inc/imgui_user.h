#ifndef __IMGUI_USER_H__
#define __IMGUI_USER_H__

#include "imgui/imgui_main.h"
#define IMGUI_DEFINE_MATH_OPERATORS
#include "imgui.h"
#ifdef IMGUI_MAIN_ENABLE_TEST_ENGINE
#include <string>
#include <vector>
#include "imgui_internal.h"
#include "imgui_te_context.h"
#include "imgui_capture_tool.h"

struct TestCaseRequest {
    std::string path;
    ImGuiTestItemInfo info;
    ImGuiTest* testcase=nullptr;
};

extern std::vector<TestCaseRequest> g_testdata;
extern ImGuiTestEngine* engine;

void RegisterAppMinimalTests(ImGuiTestEngine* e);

#endif /* IMGUI_MAIN_ENABLE_TEST_ENGINE */

void construct_ui();
void hook_before_loop();
void hook_in_loop();

#endif /* __IMGUI_USER_H__ */