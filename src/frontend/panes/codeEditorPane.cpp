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

#include <frontend/panes/codeEditorPane.h>
#include <backend/managers/dbManager.h>
#include <frontend/components/codeEditor.h>

bool CodeEditorPane::Init() {
    if (ImGui::GetIO().Fonts->Fonts.size() > 1U) {
        CodeEditor::ref().setFont(ImGui::GetIO().Fonts->Fonts[1]);
    }
    CodeEditor::ref().setLanguage(TextEditor::Language::Sql());
    return true;
}

void CodeEditorPane::Unit() {
}


///////////////////////////////////////////////////////////////////////////////////
//// IMGUI PANE ///////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////

bool CodeEditorPane::DrawPanes(const uint32_t& vCurrentFrame, bool* vOpened, ImGuiContext* vContextPtr, void* /*vUserDatas*/) {
    ImGui::SetCurrentContext(vContextPtr);
    bool change = false;
    if (vOpened != nullptr && *vOpened) {
        static ImGuiWindowFlags flags = ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_MenuBar;
        if (ImGui::Begin(GetName().c_str(), vOpened, flags)) {
#ifdef USE_DECORATIONS_FOR_RESIZE_CHILD_WINDOWS
            auto win = ImGui::GetCurrentWindowRead();
            if (win->Viewport->Idx != 0)
                flags |= ImGuiWindowFlags_NoResize;  // | ImGuiWindowFlags_NoTitleBar;
            else
                flags = ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_MenuBar;
#endif
            CodeEditor::ref().render();
        }
        ImGui::End();
    }
    return change;
}

bool CodeEditorPane::DrawOverlays(const uint32_t& /*vCurrentFrame*/, const ImRect& /*vRect*/, ImGuiContext* vContextPtr, void* /*vUserDatas*/) {
    ImGui::SetCurrentContext(vContextPtr);
    return false;
}

bool CodeEditorPane::DrawDialogsAndPopups(const uint32_t& /*vCurrentFrame*/, const ImRect& /*vRect*/, ImGuiContext* vContextPtr, void* /*vUserDatas*/) {
    ImGui::SetCurrentContext(vContextPtr);    
    return false;
}

bool CodeEditorPane::DrawWidgets(const uint32_t& /*vCurrentFrame*/, ImGuiContext* vContextPtr, void* /*vUserDatas*/) {
    ImGui::SetCurrentContext(vContextPtr);
    return false;
}
