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

#pragma once

#ifdef __EMSCRIPTEN__
#include <emscripten.h>
#include <GLES3/gl3.h>
#else
#include <glad/glad.h>
#endif

#include <imguipack.h>

#include <headers/defs.h>

#include <ezlibs/ezApp.hpp>
#include <ezlibs/ezClass.hpp>
#include <ezlibs/ezTools.hpp>
#include <ezlibs/ezXmlConfig.hpp>
#include <ezlibs/ezSingleton.hpp>

#include <map>
#include <memory>
#include <array>
#include <string>
#include <vector>
#include <functional>
#include <unordered_map>

struct GLFWwindow;
class Backend : public ez::xml::Config {
    IMPLEMENT_SINGLETON(Backend)
    DISABLE_CONSTRUCTORS(Backend)
    DISABLE_DESTRUCTORS(Backend)
private:
    GLFWwindow* m_MainWindowPtr = nullptr;
    const char* m_GlslVersion = "";
    ez::ivec2 m_DisplayPos;
    ez::ivec2 m_DisplaySize;

    // mouse
    ez::fvec4 m_MouseFrameSize;
    ez::fvec2 m_MousePos;
    ez::fvec2 m_LastNormalizedMousePos;
    ez::fvec2 m_NormalizedMousePos;

    bool m_ConsoleVisiblity = false;
    uint32_t m_CurrentFrame = 0U;

    bool m_NeedToCloseApp = false;  // when app closing app is required

    bool m_NeedToNewDatabase = false;
    bool m_NeedToLoadDatabase = false;
    bool m_NeedToCloseDatabase = false;
    std::string m_DatabaseFileToLoad;

    std::function<void(std::set<std::string>)> m_ChangeFunc;
    std::set<std::string> m_PathsToTrack;

    GLuint m_embeddedAppIcon{};

    int32_t m_display_w{};
    int32_t m_display_h{};
    ImRect m_viewRect;

public:  // getters
    ImVec2 GetDisplayPos() { return ImVec2((float)m_DisplayPos.x, (float)m_DisplayPos.y); }
    ImVec2 GetDisplaySize() { return ImVec2((float)m_DisplaySize.x, (float)m_DisplaySize.y); }

public:
    bool init(const ez::App& vApp);
    void run();
    void unit();

    void update();

    bool isThereAnError() const;

    void NeedToNewDatabase(const std::string& vFilePathName);
    void NeedToLoadDatabase(const std::string& vFilePathName);
    void NeedToCloseDatabase();

    void PostRenderingActions();

    bool IsNeedToCloseApp();
    void NeedToCloseApp(const bool& vFlag = true);
    void CloseApp();

    void setAppTitle(const std::string& vFilePathName = {});

    ez::dvec2 GetMousePos();
    int GetMouseButton(int vButton);

    ez::xml::Nodes getXmlNodes(const std::string& vUserDatas = "") override;
    bool setFromXmlNodes(const ez::xml::Node& vNode, const ez::xml::Node& vParent, const std::string& vUserDatas) override;

    void SetConsoleVisibility(const bool& vFlag);
    void SwitchConsoleVisibility();
    bool GetConsoleVisibility();

    GLuint getembeddedAppIcon() { return m_embeddedAppIcon; }

private:
    void m_RenderOffScreen();

    bool m_InitWindow();
    bool m_InitImGui();
    void m_InitModels();
    void m_InitSystems();
    void m_InitPanes();

    void m_UnitWindow();
    void m_UnitModels();
    void m_UnitImGui();
    void m_UnitSystems();
    void m_UnitPanes();

    void m_update();
    void m_IncFrame();
};
