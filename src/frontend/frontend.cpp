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

#include "frontend.h"

#include <imguipack.h>

#include <ezlibs.hpp>
#include <GLFW/glfw3.h>
#include <freetype/freetype.h>

#include <backend/backend.h>
#include <headers/ezSqliteBuild.h>

#include <backend/managers/databaseManager.h>

#include <frontend/panes/misc/messagePane.h>
#include <frontend/panes/query/queryEditorPane.h>
#include <frontend/panes/schema/schemaPane.h>
#include <frontend/panes/query/queryHistoryPane.h>
#include <frontend/panes/query/queryResultsTablePane.h>
#include <frontend/panes/query/queryResultsValuePane.h>

#include <frontend/components/schema/databaseSchemaComp.h>
#include <frontend/components/query/queryHistoryComp.h>
#include <frontend/components/query/queryEditorComp.h>
#include <frontend/components/query/queryResultComp.h>

#include <frontend/helpers/locationHelper.h>

#include <sqlite3/sqlite3.hpp>

// panes
#define DEBUG_PANE_ICON ICON_SDFM_BUG
#define SCENE_PANE_ICON ICON_SDFM_FORMAT_LIST_BULLETED_TYPE
#define TUNING_PANE_ICON ICON_SDFM_TUNE
#define CONSOLE_PANE_ICON ICON_SDFMT_COMMENT_TEXT_MULTIPLE

// features
#define GRID_ICON ICON_SDFMT_GRID
#define MOUSE_ICON ICON_SDFMT_MOUSE
#define CAMERA_ICON ICON_SDFMT_CAMCORDER
#define GIZMO_ICON ICON_SDFMT_AXIS_ARROW

#define STR_HELPER(x) #x
#define STR(x) STR_HELPER(x)

using namespace std::placeholders;

//////////////////////////////////////////////////////////////////////////////////
//// PUBLIC //////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////

// clang-format off
bool Frontend::init() {
    m_build_themes();

    ImGuiThemeHelper::initSingleton();
    ImGuiFileDialog::initSingleton();
    LocationHelper::initSingleton();

    QueryEditorPane::initSingleton();
    DatabaseInfosPane::initSingleton();
    QueryHistoryPane::initSingleton();
    QueryResultsTablePane::initSingleton();
    QueryResultsValuePane::initSingleton();
    MessagePane::initSingleton();
        
    DatabaseSchemaComp::initSingleton();
    QueryHistoryComp::initSingleton();
    QueryEditorComp::initSingleton();
    QueryResultComp::initSingleton();

    LocationHelper::ref().init();
    LayoutManager::ref().Init("Panes", "Default Layout");

    // Views
    LayoutManager::ref().AddPane(  
        LayoutPaneInfos(QueryResultsTablePane::ref(),"Query results")
            .setMenu("results", "Query").setDisposalCentral().setDefaultOpened(true));
    LayoutManager::ref().AddPane(  
        LayoutPaneInfos(MessagePane::ref(),"Console")
            .setMenu("Console").setDisposalSide("BOTTOM", 0.25f));
    LayoutManager::ref().AddPane(  
        LayoutPaneInfos(QueryEditorPane::ref(),"Query editor")
            .setMenu("editor", "Query").setDisposalSide("TOP", 0.25f).setDefaultOpened(true));
    LayoutManager::ref().AddPane(  
        LayoutPaneInfos(DatabaseInfosPane::ref(),"DatabaseDesc structure")
            .setMenu("structure", "DatabaseDesc").setDisposalSide("LEFT", 0.25f).setDefaultOpened(true));
    LayoutManager::ref().AddPane(  
        LayoutPaneInfos(QueryHistoryPane::ref(),"Queries history")
            .setMenu("history", "Query").setDisposalSide("LEFT/BOTTOM", 0.4f).setDefaultOpened(true));
    LayoutManager::ref().AddPane(  
        LayoutPaneInfos(QueryResultsValuePane::ref(),"Query field value")
            .setMenu("field value", "Query").setDisposalSide("BOTTOM", 0.25f).setDefaultOpened(true));

    // InitPanes is done in m_InitPanes, because a specific order is needed

    return true;
}
// clang-format on

void Frontend::unit() {
    LocationHelper::ref().unit();

    LayoutManager::ref().UnitPanes();

    ImGuiThemeHelper::unitSingleton();
    ImGuiFileDialog::unitSingleton();
    LocationHelper::unitSingleton();

    DatabaseSchemaComp::unitSingleton();
    QueryHistoryComp::unitSingleton();
    QueryEditorComp::unitSingleton();
    QueryResultComp::unitSingleton();

    QueryEditorPane::unitSingleton();
    DatabaseInfosPane::unitSingleton();
    QueryHistoryPane::unitSingleton();
    QueryResultsValuePane::unitSingleton();
    QueryResultsTablePane::unitSingleton();
    MessagePane::unitSingleton();
}

bool Frontend::isValid() const { return false; }

bool Frontend::isThereAnError() const { return false; }

void Frontend::Display(const uint32_t& vCurrentFrame, const ImRect& vRect) {
    const auto context_ptr = ImGui::GetCurrentContext();
    if (context_ptr != nullptr) {
        const auto& io = ImGui::GetIO();

        m_displayRect = vRect;

        ImGui::CustomStyle::ResetCustomId();

        m_drawMainMenuBar();
        m_drawMainStatusBar();

        if (LayoutManager::ref().BeginDockSpace(ImGuiDockNodeFlags_PassthruCentralNode)) {
            LayoutManager::ref().EndDockSpace();
        }

        if (LayoutManager::ref().DrawPanes(vCurrentFrame, context_ptr, {})) {}

        DrawDialogsAndPopups(vCurrentFrame, m_displayRect, context_ptr, {});

        ImGuiThemeHelper::ref().Draw();
        LayoutManager::ref().InitAfterFirstDisplay(io.DisplaySize);
    }
}

bool Frontend::DrawWidgets(const uint32_t& vCurrentFrame, ImGuiContext* vContextPtr, void* vUserDatas) {
    bool res = false;
    return res;
}

bool Frontend::DrawOverlays(const uint32_t& vCurrentFrame, const ImRect& vRect, ImGuiContext* vContextPtr, void* vUserDatas) {
    bool res = false;
    return res;
}

bool Frontend::DrawDialogsAndPopups(const uint32_t& vCurrentFrame, const ImRect& vRect, ImGuiContext* vContextPtr, void* vUserDatas) {
    m_actionsSystem.executeFirstConditionalAction();
    m_actionsSystem.runImmediateActions();
    LayoutManager::ref().DrawDialogsAndPopups(vCurrentFrame, vRect, vContextPtr, vUserDatas);
    if (m_showImGui) { ImGui::ShowDemoWindow(&m_showImGui); }
    if (m_showImPlot) { ImPlot::ShowDemoWindow(&m_showImPlot); }
    if (m_showMetric) { ImGui::ShowMetricsWindow(&m_showMetric); }
    if (m_showAboutDialog) { m_drawAboutDialog(); }
    return false;
}

void Frontend::m_drawAboutDialog() {
    static auto paragraphColor = ImVec4(1.0f, 1.0f, 0.0f, 1.0f);
    ImGui::OpenPopup("About");
    ImGui::SetNextWindowPos(m_displayRect.GetCenter(), ImGuiCond_Always, ImVec2(0.5f, 0.5f));
    if (ImGui::BeginPopupModal(
            "About",                                 //
            (bool*)nullptr,                          //
            ImGuiWindowFlags_NoResize |              //
                ImGuiWindowFlags_AlwaysAutoResize |  //
                ImGuiWindowFlags_NoDocking)) {
        ImGui::BeginVertical("VLayout", ImVec2(0, 0), 0.5f);
        ImGui::Spring(0.0f);
        ImTextureRef ref;
        ref._TexID = Backend::ref().getembeddedAppIcon();
        ImGui::Image(ref, ImVec2(100, 100));
        ImGui::Spring(0.0f);
        ImGui::TextColored(paragraphColor, "Version : %s x64 Beta v%s", ezSqlite_Label, ezSqlite_BuildId);
        ImGui::Spring(0.0f);
        ImGui::ClickableTextUrl("Github repository : ", "https://github.com/aiekick/ezSqlite/releases");
        ImGui::Spring(0.0f);
        ImGui::Separator();
        ImGui::Spring(0.0f);
        ImGui::TextColored(paragraphColor, "License : GNU Affero General Public License");
        ImGui::Spring(0.0f);
        ImGui::Text("Copyright (C) 2025 Stephane Cuillerdier (aka aiekick)");
        ImGui::Spring(0.0f);
        ImGui::Text("ezSqlite is free software: you can redistribute it and/or modify");
        ImGui::Spring(0.0f);
        ImGui::Text("it under the terms of the GNU Affero General Public License as published");
        ImGui::Spring(0.0f);
        ImGui::Text("by the Free Software Foundation, either version 3 of the License, or");
        ImGui::Spring(0.0f);
        ImGui::Text("(at your option) any later version.");
        ImGui::Spring(0.0f);
        ImGui::Text("ezSqlite is distributed in the hope that it will be useful,");
        ImGui::Spring(0.0f);
        ImGui::Text("but WITHOUT ANY WARRANTY; without even the implied warranty of");
        ImGui::Spring(0.0f);
        ImGui::Text("MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the");
        ImGui::Spring(0.0f);
        ImGui::Text("GNU Affero General Public License for more details.");
        ImGui::Spring(0.0f);
        ImGui::Text("You should have received a copy of the GNU Affero General Public License");
        ImGui::Spring(0.0f);
        ImGui::Text("along with ezSqlite. If not, see <https://www.gnu.org/licenses/>.");
        ImGui::Spring(0.0f);
        ImGui::Separator();
        ImGui::Spring(0.0f);
        ImGui::TextColored(paragraphColor, "used libraries :");
        ImGui::Spring(0.0f);
        ImGui::ClickableTextUrl("ezlibs v" ezLibs_BuildId " (MIT)", "https://github.com/aiekick/ezLibs");
        ImGui::Spring(0.0f);
        ImGui::ClickableTextUrl("Freetype v" STR(FREETYPE_MAJOR) "." STR(FREETYPE_MINOR) "." STR(FREETYPE_PATCH) " (ZLIB)", "https://github.com/freetype/freetype");
        ImGui::Spring(0.0f);
        ImGui::ClickableTextUrl("Glad v2.0.8 (MIT)", "https://github.com/Dav1dde/glad");
        ImGui::Spring(0.0f);
        ImGui::ClickableTextUrl("Glfw v" STR(GLFW_VERSION_MAJOR) "." STR(GLFW_VERSION_MINOR) "." STR(GLFW_VERSION_REVISION) " (ZLIB)", "https://github.com/glfw/glfw");
        ImGui::Spring(0.0f);
        ImGui::ClickableTextUrl("ImGui docking + stack layout v" IMGUI_VERSION " (MIT)", "https://github.com/ocornut/imgui");
        ImGui::Spring(0.0f);
        ImGui::ClickableTextUrl("ImGuiFileDialog " IGFD_VERSION " (MIT)", "https://github.com/aiekick/ImGuiFileDialog");
        ImGui::Spring(0.0f);
        ImGui::ClickableTextUrl("Sqlite3 v" SQLITE_VERSION " (Unlicense / BSD)", "https://github.com/sqlite/sqlite");
        ImGui::Spring(0.0f);
        ImGui::Separator();
        ImGui::Spring(0.0f);
        if (ImGui::ContrastedButton("Close")) { m_showAboutDialog = false; }
        ImGui::Spring(0.0f);
        ImGui::EndVertical();

        ImGui::EndPopup();
    }
}

void Frontend::m_drawMainMenuBar() {
    static float s_controller_menu_size = 0.0f;
    // static float s_translation_menu_size = 0.0f;
    if (ImGui::BeginMainMenuBar()) {
        float full_width = ImGui::GetContentRegionAvail().x;
        if (ImGui::BeginMenu(" Database")) {
            if (ImGui::MenuItem(" New database")) { ActionMenuNewDatabase(); }
            if (ImGui::MenuItem(" Open database")) { ActionMenuOpenDatabase(); }
            if (DatabaseManager::ref().isDatabaseLoaded()) {
                ImGui::Separator();
                if (ImGui::MenuItem(" Reopen database")) { ActionMenuReOpenDatabase(); }
                ImGui::Separator();
                if (ImGui::MenuItem(" Close database")) { ActionMenuCloseDatabase(); }
            }
            ImGui::EndMenu();
        }

        ImGui::Spacing();

        const auto& io = ImGui::GetIO();
        LayoutManager::ref().DisplayMenu(io.DisplaySize);

        ImGui::Spacing();

        if (ImGui::BeginMenu("Help")) {
            if (ImGui::MenuItem(" About")) { m_showAboutDialog = true; }
            ImGui::EndMenu();
        }

#if _DEBUG
        if (ImGui::BeginMenu("[ImGui]")) {
            ImGui::Separator();
            ImGui::MenuItem("Show ImGui", "", &m_showImGui);
            ImGui::MenuItem("Show ImGui Metric/Debug", "", &m_showMetric);
            ImGui::MenuItem("Show ImPlot", "", &m_showImPlot);
            ImGui::EndMenu();
        }
#endif

        ImGui::SpacingFromStart((full_width - s_controller_menu_size) * 0.5f);
        DatabaseManager::ref().drawMenu(s_controller_menu_size);

#ifdef _DEBUG
        const auto label = ez::str::toStr("Dear ImGui %s (Docking)", ImGui::GetVersion());
        const auto size = ImGui::CalcTextSize(label.c_str());
        ImGui::Spacing(ImGui::GetContentRegionAvail().x - size.x - ImGui::GetStyle().FramePadding.x * 2.0f);
        ImGui::Text("%s", label.c_str());
#else
        const auto label = ez::str::toStr("Sqlite v%s", SQLITE_VERSION);
        const auto size = ImGui::CalcTextSize(label.c_str());
        ImGui::Spacing(ImGui::GetContentRegionAvail().x - size.x - ImGui::GetStyle().FramePadding.x * 2.0f);
        ImGui::Text("%s", label.c_str());
        // ImGui::SpacingFromStart(full_width - s_translation_menu_size);
#endif
        //  s_translation_menu_size = LocationHelper::ref().drawMenu();

        ImGui::EndMainMenuBar();
    }
}

void Frontend::m_drawMainStatusBar() {
    if (ImGui::BeginMainStatusBar()) {
        Messaging::ref().DrawStatusBar();

#ifdef _DEBUG
        const auto& io = ImGui::GetIO();
        const auto fps = ez::str::toStr("%.1f ms/frame (%.1f fps)", 1000.0f / io.Framerate, io.Framerate);
        const auto size = ImGui::CalcTextSize(fps.c_str());
        ImGui::Spacing(ImGui::GetContentRegionAvail().x - size.x - ImGui::GetStyle().FramePadding.x * 2.0f);
        ImGui::Text("%s", fps.c_str());
#endif

        ImGui::EndMainStatusBar();
    }
}

///////////////////////////////////////////////////////
//// ACTIONS //////////////////////////////////////////
///////////////////////////////////////////////////////

void Frontend::ActionMenuNewDatabase() {
    m_actionsSystem.clear();
    m_actionsSystem.pushBackConditonalAction([this]() {
        IGFD::FileDialogConfig config;
        config.countSelectionMax = 1;
        config.flags = ImGuiFileDialogFlags_Modal;
        ImGuiFileDialog::ref().OpenDialog("NewDatabaseDlg", "New DatabaseDesc File", "Any files{((.*))}", config);
        return true;
    });
    m_actionsSystem.pushBackConditonalAction([this]() { return m_displayNewDatabaseDialog(); });
}

void Frontend::ActionMenuOpenDatabase() {
    m_actionsSystem.clear();
    m_actionsSystem.pushBackConditonalAction([this]() {
        IGFD::FileDialogConfig config;
        config.countSelectionMax = 1;
        config.flags = ImGuiFileDialogFlags_Modal;
        ImGuiFileDialog::ref().OpenDialog("OpenDatabaseDlg", "Open DatabaseDesc File", "Any files{((.*))}", config);
        return true;
    });
    m_actionsSystem.pushBackConditonalAction([this]() { return m_displayOpenDatabaseDialog(); });
}

void Frontend::ActionMenuImportDatas() {
    m_actionsSystem.clear();
    m_actionsSystem.pushBackConditonalAction([this]() {
        IGFD::FileDialogConfig config;
        config.countSelectionMax = 0;
        config.flags = ImGuiFileDialogFlags_Modal;
        ImGuiFileDialog::ref().OpenDialog("Import Datas", "Import Datas from File", ".csv", config);
        return true;
    });
    m_actionsSystem.pushBackConditonalAction([this]() { return m_displayOpenDatabaseDialog(); });
}

void Frontend::ActionMenuReOpenDatabase() {
    m_actionsSystem.clear();
    m_actionsSystem.pushBackConditonalAction([]() {
        Backend::ref().NeedToLoadDatabase(DatabaseManager::ref().getDatabaseFilepathName());
        return true;
    });
}

void Frontend::ActionMenuCloseDatabase() {
    m_actionsSystem.clear();
    m_actionsSystem.pushBackConditonalAction([]() {
        Backend::ref().NeedToCloseDatabase();
        return true;
    });
}

void Frontend::ActionWindowCloseApp() {
    if (Backend::ref().IsNeedToCloseApp()) { return; }
    m_actionsSystem.clear();
    m_actionsSystem.pushBackConditonalAction([]() {
        Backend::ref().CloseApp();
        return true;
    });
}

void Frontend::m_actionCancel() {

    m_actionsSystem.clear();
    Backend::ref().NeedToCloseApp(false);
}

///////////////////////////////////////////////////////
//// DIALOG FUNCS /////////////////////////////////////
///////////////////////////////////////////////////////

bool Frontend::m_displayNewDatabaseDialog() {
    // need to return false to continue to be displayed next frame

    ImVec2 max = m_displayRect.GetSize();
    ImVec2 min = max * 0.5f;

    if (ImGuiFileDialog::ref().Display("NewDatabaseDlg", ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoDocking, min, max)) {
        if (ImGuiFileDialog::ref().IsOk()) {
            auto file = ImGuiFileDialog::ref().GetFilePathName();
            Backend::ref().NeedToNewDatabase(file);
        } else {               // cancel
            m_actionCancel();  // we interrupts all actions
        }

        ImGuiFileDialog::ref().Close();

        return true;
    }

    return false;
}

bool Frontend::m_displayOpenDatabaseDialog() {
    // need to return false to continue to be displayed next frame

    ImVec2 max = m_displayRect.GetSize();
    ImVec2 min = max * 0.5f;

    if (ImGuiFileDialog::ref().Display("OpenDatabaseDlg", ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoDocking, min, max)) {
        if (ImGuiFileDialog::ref().IsOk()) {
            Backend::ref().NeedToLoadDatabase(ImGuiFileDialog::ref().GetFilePathName());
        } else {               // cancel
            m_actionCancel();  // we interrupts all actions
        }

        ImGuiFileDialog::ref().Close();

        return true;
    }

    return false;
}

///////////////////////////////////////////////////////
//// APP CLOSING //////////////////////////////////////
///////////////////////////////////////////////////////

void Frontend::IWantToCloseTheApp() { ActionWindowCloseApp(); }

///////////////////////////////////////////////////////
//// DROP /////////////////////////////////////////////
///////////////////////////////////////////////////////

void Frontend::JustDropFiles(int count, const char** paths) { assert(0); }

///////////////////////////////////////////////////////
//// CONFIGURATION ////////////////////////////////////
///////////////////////////////////////////////////////

ez::xml::Nodes Frontend::getXmlNodes(const std::string& vUserDatas) {
    ez::xml::Node node;
    node.addChilds(ImGuiThemeHelper::ref().getXmlNodes("app"));
    node.addChilds(LayoutManager::ref().getXmlNodes("app"));
    node.addChilds(DatabaseSchemaComp::ref().getXmlNodes("app"));
    node.addChilds(QueryResultComp::ref().getXmlNodes("app"));
    node.addChilds(QueryEditorComp::ref().getXmlNodes("app"));
    node.addChilds(QueryHistoryComp::ref().getXmlNodes("app"));
    node.addChild("places").setContent(ImGuiFileDialog::ref().SerializePlaces());
#ifdef _DEBUG
    node.addChild("showaboutdialog").setContent(m_showAboutDialog);
    node.addChild("showimgui").setContent(m_showImGui);
    node.addChild("showmetric").setContent(m_showMetric);
#endif
    return node.getChildren();
}

bool Frontend::setFromXmlNodes(const ez::xml::Node& vNode, const ez::xml::Node& vParent, const std::string& vUserDatas) {
    const auto& strName = vNode.getName();
    const auto& strValue = vNode.getContent();
    // const auto& strParentName = vParent.getName();

    if (strName == "places") {
        ImGuiFileDialog::ref().DeserializePlaces(strValue);
    }
#ifdef _DEBUG
    else if (strName == "showaboutdialog") {
        m_showAboutDialog = ez::ivariant(strValue).GetB();
    } else if (strName == "showimgui") {
        m_showImGui = ez::ivariant(strValue).GetB();
    } else if (strName == "showmetric") {
        m_showMetric = ez::ivariant(strValue).GetB();
    }
#endif
    ImGuiThemeHelper::ref().setFromXmlNodes(vNode, vParent, "app");
    LayoutManager::ref().setFromXmlNodes(vNode, vParent, "app");
    DatabaseSchemaComp::ref().setFromXmlNodes(vNode, vParent, "app");
    QueryResultComp::ref().setFromXmlNodes(vNode, vParent, "app");
    QueryEditorComp::ref().setFromXmlNodes(vNode, vParent, "app");
    QueryHistoryComp::ref().setFromXmlNodes(vNode, vParent, "app");
    return true;
}
