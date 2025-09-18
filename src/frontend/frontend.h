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

#include <imguipack.h>

#include <ezlibs/ezLog.hpp>
#include <ezlibs/ezClass.hpp>
#include <ezlibs/ezTools.hpp>
#include <ezlibs/ezActions.hpp>
#include <ezlibs/ezXmlConfig.hpp>
#include <ezlibs/ezSingleton.hpp>

#include <functional>
#include <string>
#include <vector>
#include <map>

class Frontend : public ez::xml::Config {
    IMPLEMENT_SINGLETON(Frontend)
    DISABLE_CONSTRUCTORS(Frontend)
    DISABLE_DESTRUCTORS(Frontend)
    bool m_showImGui = false;
    bool m_showImPlot = false;
    bool m_showMetric = false;
    bool m_showAboutDialog = false;          // show about dlg
    bool m_saveDialogIfRequired = false;     // open save options dialog (save / save as / continue without saving / cancel)
    bool m_saveDialogActionWasDone = false;  // if action was done by save options dialog
    ImFont* m_toolbarFontPtr = nullptr;
    ImRect m_displayRect = ImRect(ImVec2(0, 0), ImVec2(1280, 720));
    ez::Actions m_actionsSystem;

public:
    bool init();
    void unit();

    bool isValid() const;
    bool isThereAnError() const;

    void Display(const uint32_t& vCurrentFrame, const ImRect& vRect);

    bool DrawWidgets(const uint32_t& vCurrentFrame, ImGuiContext* vContextPtr, void* vUserDatas);
    bool DrawOverlays(const uint32_t& vCurrentFrame, const ImRect& vRect, ImGuiContext* vContextPtr, void* vUserDatas);
    bool DrawDialogsAndPopups(const uint32_t& vCurrentFrame, const ImRect& vRect,  ImGuiContext* vContextPtr, void* vUserDatas);

    void IWantToCloseTheApp();  // user want close app, but we want to ensure its saved
    void JustDropFiles(int count, const char** paths);

    void ActionMenuNewDatabase();
    void ActionMenuOpenDatabase();
    void ActionMenuImportDatas();
    void ActionMenuReOpenDatabase();
    void ActionMenuCloseDatabase();
    void ActionWindowCloseApp();

    ez::xml::Nodes getXmlNodes(const std::string& vUserDatas = "") override;
    bool setFromXmlNodes(const ez::xml::Node& vNode, const ez::xml::Node& vParent, const std::string& vUserDatas) override;

private:  // actions
    void m_actionCancel();
    bool m_displayNewDatabaseDialog();
    bool m_displayOpenDatabaseDialog();
    bool m_build_themes();
    void m_drawMainMenuBar();
    void m_drawMainStatusBar();
    void m_drawAboutDialog();
};
