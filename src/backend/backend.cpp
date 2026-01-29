/*
 * This file is part of ezSqlite.
 *
 * Copyright (C) 2025 Stephane Cuillerdier (aka aiekick)
 *
 * ezSqlite is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Affero General Public License as published
 * by the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * ezSqlite is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Affero General Public License for more details.
 *
 * You should have received a copy of the GNU Affero General Public License
 * along with ezSqlite.  If not, see <https://www.gnu.org/licenses/>.
 */

#include "backend.h"
#include <ezlibs/ezOS.hpp>

#ifdef __EMSCRIPTEN__
#include <emscripten.h>
#include <GLES3/gl3.h>
#else
#include <glad/glad.h>
#endif

#include <GLFW/glfw3.h>

#if defined(WINDOWS_OS) && !defined(__EMSCRIPTEN__)
#define GLFW_EXPOSE_NATIVE_WIN32
#include <GLFW/glfw3native.h>
#endif

#include <headers/ezSqliteBuild.h>

#define IMGUI_IMPL_API
#include <3rdparty/imgui_docking/backends/imgui_impl_opengl3.h>
#include <3rdparty/imgui_docking/backends/imgui_impl_glfw.h>

#include <cstdio>     // printf, fprintf
#include <chrono>     // timer
#include <cstdlib>    // abort
#include <fstream>    // std::ifstream
#include <iostream>   // std::cout
#include <algorithm>  // std::min, std::max
#include <stdexcept>  // std::exception

#include <LayoutManager.h>

// singletons
#include <backend/helpers/databaseHelper.h>
#include <backend/managers/databaseManager.h>
#include <backend/diagram/diagramManager.h>

#include <frontend/components/schema/databaseSchemaComp.h>

#include <imguipack.h>

#include <ezlibs/ezTools.hpp>
#include <ezlibs/ezLog.hpp>
#include <ezlibs/ezFile.hpp>

#ifdef WINDOWS_OS
#include <ezlibs/ezEmbed.hpp>
#endif

#include <frontend/frontend.h>

#include <frontend/panes/misc/messagePane.h>

// we include the cpp just for embedded fonts
#include <resources/fontIcons.cpp>
#include <resources/Roboto-Medium.h>
#include <resources/DejaVuSansMono-Bold.h>

#define INITIAL_WIDTH 1700
#define INITIAL_HEIGHT 700

//////////////////////////////////////////////////////////////////////////////////
//// STATIC //////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////

static void glfw_error_callback(int error, const char* description) {
    const std::string desc{description};
    if (desc.find("Failed to convert clipboard to string") != std::string::npos) { return; }
    LogVarError("glfw error %i : %s", error, description);
}

static void glfw_window_close_callback(GLFWwindow* window) {
    glfwSetWindowShouldClose(window, GLFW_FALSE);  // block app closing
    Frontend::ref().ActionWindowCloseApp();
}

//////////////////////////////////////////////////////////////////////////////////
//// PUBLIC //////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////

// todo : to refactor ! i dont like that
bool Backend::init(const ez::App& vApp) {
#ifdef _DEBUG
    SetConsoleVisibility(true);
#else
    SetConsoleVisibility(false);
#endif
    m_InitModels();
    if (m_InitWindow() && m_InitImGui()) {
        m_InitSystems();
        m_InitPanes();
        LoadConfigFile("config.xml", "app");
        return DiagramManager::ref().init();
    }
    return false;
}

void Backend::update() {
    DatabaseManager::ref().newFrame();

    // maintain active, prevent user change via imgui dialog
    ImGui::GetIO().ConfigFlags |= ImGuiConfigFlags_DockingEnable;  // Enable Docking
#ifndef __EMSCRIPTEN__
    io.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable;  // Enable Viewport
#endif

    glfwGetFramebufferSize(m_MainWindowPtr, &m_display_w, &m_display_h);

    m_update();  // to do absolutly before imgui rendering

    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();

    ImGuiViewport* viewport = ImGui::GetMainViewport();
    if (viewport) {
        m_viewRect.Min = viewport->WorkPos;
        m_viewRect.Max = m_viewRect.Min + viewport->WorkSize;
    } else {
        m_viewRect.Max = ImVec2((float)m_display_w, (float)m_display_h);
    }

    Frontend::ref().Display(m_CurrentFrame, m_viewRect);

    ImGui::Render();

    glViewport(0, 0, m_display_w, m_display_h);
    glClear(GL_COLOR_BUFFER_BIT);
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

#ifndef __EMSCRIPTEN__
    auto* backup_current_context = glfwGetCurrentContext();

    // Update and Render additional Platform Windows
    // (Platform functions may change the current OpenGL context, so we save/restore it to make it easier to paste
    // this code elsewhere.
    //  For this specific demo app we could also call glfwMakeContextCurrent(window) directly)
    if (ImGui::GetIO().ConfigFlags & ImGuiConfigFlags_ViewportsEnable) {
        ImGui::UpdatePlatformWindows();
        ImGui::RenderPlatformWindowsDefault();
    }
    glfwMakeContextCurrent(backup_current_context);
#endif

    glfwSwapBuffers(m_MainWindowPtr);

    // mainframe post actions
    PostRenderingActions();

    ++m_CurrentFrame;

#ifndef __EMSCRIPTEN__
    // will pause the view until we move the mouse or press keys
    glfwWaitEventsTimeout(0.5);
#endif
}

static void sUpdate(void* vBackend) {
    auto* backend = static_cast<Backend*>(vBackend);
    backend->update();
}

void Backend::run() {
#ifdef __EMSCRIPTEN__
    emscripten_set_main_loop_arg(sUpdate, this, 0, true);
#else
    while (!glfwWindowShouldClose(m_MainWindowPtr)) {
        update();
    }
#endif
}

// todo : to refactor ! i dont like that
void Backend::unit() {
    SaveConfigFile("config.xml", "app", "config");
    DiagramManager::ref().unit();
    m_UnitSystems();
    m_UnitImGui();
    m_UnitWindow();
    m_UnitModels();
}

bool Backend::isThereAnError() const { return false; }

void Backend::NeedToNewDatabase(const std::string& vFilePathName) {
    m_NeedToNewDatabase = true;
    m_DatabaseFileToLoad = vFilePathName;
}

void Backend::NeedToLoadDatabase(const std::string& vFilePathName) {
    m_NeedToLoadDatabase = true;
    m_DatabaseFileToLoad = vFilePathName;
}

void Backend::NeedToCloseDatabase() { m_NeedToCloseDatabase = true; }

// actions to do after rendering
void Backend::PostRenderingActions() {
    if (m_NeedToLoadDatabase) {
        m_NeedToLoadDatabase = false;
        if (DatabaseManager::ref().loadDatabaseFromFile(m_DatabaseFileToLoad)) { setAppTitle(m_DatabaseFileToLoad); }
    }
    if (m_NeedToNewDatabase) {
        m_NeedToNewDatabase = false;
        if (DatabaseManager::ref().newDatabaseFromFile(m_DatabaseFileToLoad)) { setAppTitle(m_DatabaseFileToLoad); }
    }
    DatabaseSchemaComp::ref().doActions();
}

bool Backend::IsNeedToCloseApp() { return m_NeedToCloseApp; }

void Backend::NeedToCloseApp(const bool& vFlag) { m_NeedToCloseApp = vFlag; }

void Backend::CloseApp() {
    // will escape the main loop
    glfwSetWindowShouldClose(m_MainWindowPtr, 1);
}

void Backend::setAppTitle(const std::string& vFilePathName) {
    auto ps = ez::file::parsePathFileName(vFilePathName);
    if (ps.isOk) {
        glfwSetWindowTitle(  //
            m_MainWindowPtr,
            ez::str::toStr("ezSqlite Beta %s - DatabaseDesc : %s", ezSqlite_BuildId, vFilePathName.c_str()).c_str());
    } else {
        glfwSetWindowTitle(  //
            m_MainWindowPtr,
            ez::str::toStr("ezSqlite Beta %s", ezSqlite_BuildId).c_str());
    }
}

ez::dvec2 Backend::GetMousePos() {
    ez::dvec2 mp;
    glfwGetCursorPos(m_MainWindowPtr, &mp.x, &mp.y);
    return mp;
}

int Backend::GetMouseButton(int vButton) { return glfwGetMouseButton(m_MainWindowPtr, vButton); }

////////////////////////////////////////////////////////////////////////////////////////////////////////////
//// CONSOLE ///////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////

void Backend::SetConsoleVisibility(const bool& vFlag) {
    m_ConsoleVisiblity = vFlag;
#ifndef __EMSCRIPTEN__
    if (m_ConsoleVisiblity) {
#ifdef WIN32
        ShowWindow(GetConsoleWindow(), SW_SHOW);
#endif
    } else {
#ifdef WIN32
        ShowWindow(GetConsoleWindow(), SW_HIDE);
#endif
    }
#endif  // __EMSCRIPTEN__
}

void Backend::SwitchConsoleVisibility() {
    m_ConsoleVisiblity = !m_ConsoleVisiblity;
    SetConsoleVisibility(m_ConsoleVisiblity);
}

bool Backend::GetConsoleVisibility() { return m_ConsoleVisiblity; }

///////////////////////////////////////////////////////
//// CONFIGURATION ////////////////////////////////////
///////////////////////////////////////////////////////

ez::xml::Nodes Backend::getXmlNodes(const std::string& vUserDatas) {
    ez::xml::Node node;
    node.addChild("database").setContent(DatabaseManager::ref().getDatabaseFilepathName());
    node.addChilds(DatabaseManager::ref().getXmlNodes(vUserDatas));
    node.addChilds(Frontend::ref().getXmlNodes(vUserDatas));
    return node.getChildren();
}

bool Backend::setFromXmlNodes(const ez::xml::Node& vNode, const ez::xml::Node& vParent, const std::string& vUserDatas) {
    const auto& strName = vNode.getName();
    const auto& strValue = vNode.getContent();
    // const auto& strParentName = vParent.getName();
    if (strName == "database") { NeedToLoadDatabase(strValue); }
    DatabaseManager::ref().setFromXmlNodes(vNode, vParent, vUserDatas);
    Frontend::ref().setFromXmlNodes(vNode, vParent, vUserDatas);
    return true;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////
//// RENDER ////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////

void Backend::m_RenderOffScreen() {}

void Backend::m_update() {}

void Backend::m_IncFrame() { ++m_CurrentFrame; }

//////////////////////////////////////////////////////////////////////////////////
//// PRIVATE /////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////

bool Backend::m_InitWindow() {
    glfwSetErrorCallback(glfw_error_callback);
    if (!glfwInit()) { return false; }

#ifdef __EMSCRIPTEN__
    // GL 3.0 + GLSL 130
    m_GlslVersion = "#version 300 es";
    glfwWindowHint(GLFW_DEPTH_BITS, 24);
    glfwWindowHint(GLFW_STENCIL_BITS, 8);
    glfwWindowHint(GLFW_CLIENT_API, GLFW_OPENGL_ES_API);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 0);
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_FALSE);
#else
    // GL 3.0 + GLSL 130
    m_GlslVersion = "#version 130";
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 0);
    glfwWindowHint(GLFW_SAMPLES, 4);
#endif

    // Create window with graphics context
    m_MainWindowPtr = glfwCreateWindow(1280, 720, "ezSqlite", nullptr, nullptr);
    if (m_MainWindowPtr == nullptr) {
        std::cout << "Fail to create the window" << std::endl;
        return false;
    }
    glfwMakeContextCurrent(m_MainWindowPtr);
    glfwSwapInterval(1);  // Enable vsync

#ifndef __EMSCRIPTEN__
    if (gladLoadGL() == 0) {
        fprintf(stderr, "Failed to initialize OpenGL loader!\n");
        return false;
    }
#endif

    glfwSetWindowCloseCallback(m_MainWindowPtr, glfw_window_close_callback);

#ifdef WINDOWS_OS
    ez::embed::setEmbeddedIconApp(glfwGetWin32Window(m_MainWindowPtr), "IDI_ICON1");
    m_embeddedAppIcon = ez::embed::extractEmbeddedPngToGlTexture("IDB_PNG1", false);
#endif

    return true;
}

void Backend::m_UnitWindow() {
    glfwDestroyWindow(m_MainWindowPtr);
    glfwTerminate();
}

bool Backend::m_InitImGui() {
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImPlot::CreateContext();

    ImGuiIO& io = ImGui::GetIO();
    (void)io;
    io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;  // Enable Docking
#ifndef __EMSCRIPTEN__
    io.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable;  // Enable Viewport
#endif
    // io.FontAllowUserScaling = true;                      // activate zoom feature with ctrl + mousewheel
#ifdef USE_DECORATIONS_FOR_RESIZE_CHILD_WINDOWS
    io.ConfigViewportsNoDecoration = false;  // toujours mettre une frame aux fenetres enfants
#endif

    // When viewports are enabled we tweak WindowRounding/WindowBg so platform windows can look identical to regular ones.
    ImGuiStyle& style = ImGui::GetStyle();
    if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable) {
        style.WindowRounding = 0.0f;
        style.Colors[ImGuiCol_WindowBg].w = 1.0f;
    }

    // fonts
    {
        {  // main font
            if (ImGui::GetIO().Fonts->AddFontFromMemoryCompressedBase85TTF(RM_compressed_data_base85, 15.0f) == nullptr) {
                assert(0);  // failed to load font
            }
        }
        {  // icon font
            static const ImWchar icons_ranges[] = {ICON_MIN_FONT, ICON_MAX_FONT, 0};
            ImFontConfig icons_config;
            icons_config.MergeMode = true;
            icons_config.PixelSnapH = true;
            if (ImGui::GetIO().Fonts->AddFontFromMemoryCompressedBase85TTF(FONT_ICON_BUFFER_NAME_FONT, 15.0f, &icons_config, icons_ranges) == nullptr) {
                assert(0);  // failed to load font
            }
        }
        {  // dev font
            if (ImGui::GetIO().Fonts->AddFontFromMemoryCompressedBase85TTF(DVSMB_compressed_data_base85, 15.0f) == nullptr) {
                assert(0);  // failed to load font
            }
        }
    }

    Frontend::initSingleton();

    // Setup Platform/Renderer bindings
    if (ImGui_ImplGlfw_InitForOpenGL(m_MainWindowPtr, true) &&  //
        ImGui_ImplOpenGL3_Init(m_GlslVersion)) {
        // ui init
        if (Frontend::ref().init()) { return true; }
    }
    return false;
}

void Backend::m_InitModels() {
    DatabaseHelper::initSingleton();
    DiagramManager::initSingleton();
}

void Backend::m_UnitModels() {
    DatabaseHelper::unitSingleton();
    DiagramManager::unitSingleton();
}

void Backend::m_InitSystems() {}

void Backend::m_UnitSystems() {}

void Backend::m_InitPanes() {
    if (LayoutManager::ref().InitPanes()) {
        // a faire apres InitPanes() sinon MessagePane::ref()->paneFlag vaudra 0 et changeras apres InitPanes()
        Messaging::ref().sMessagePaneId = MessagePane::ref()->GetFlag();
    }
}

void Backend::m_UnitPanes() {}

void Backend::m_UnitImGui() {
    Frontend::ref().unit();
    LayoutManager::ref().Unit();
    Frontend::unitSingleton();

    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();

    ImPlot::DestroyContext();
    ImGui::DestroyContext();
}
