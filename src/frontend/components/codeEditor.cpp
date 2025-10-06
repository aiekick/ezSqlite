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

#include "CodeEditor.h"

#include <ezlibs/ezTools.hpp>

#include <filesystem>
#include <fstream>
#include <codecvt>

#include <cstdio>
#include <exception>

#if __APPLE__
#define SHORTCUT "Cmd-"
#else
#define SHORTCUT "Ctrl-"
#endif

void CodeEditor::setLanguage(const TextEditor::Language* vLanguage) {
    if (vLanguage != nullptr) {
        m_editor.SetLanguage(vLanguage);
    }
}

void CodeEditor::setFont(ImFont* vpFont) {
    m_fontParams.pFont = vpFont;
}

void CodeEditor::newFile() {
    if (isDirty()) {
        showConfirmClose([this]() {
            m_originalText.clear();
            m_editor.SetText("");
            m_version = m_editor.GetUndoIndex();
            m_filename = "untitled";
        });

    } else {
        m_originalText.clear();
        m_editor.SetText("");
        m_version = m_editor.GetUndoIndex();
        m_filename = "untitled";
    }
}

void CodeEditor::openFile(const std::string& path) {
    try {
        std::ifstream stream(path.c_str());
        std::string text;

        stream.seekg(0, std::ios::end);
        text.reserve(stream.tellg());
        stream.seekg(0, std::ios::beg);

        text.assign((std::istreambuf_iterator<char>(stream)), std::istreambuf_iterator<char>());
        stream.close();

        m_originalText = text;
        m_editor.SetText(text);
        m_version = m_editor.GetUndoIndex();
        m_filename = path;

    } catch (std::exception& e) {
        showError(e.what());
    }
}

void CodeEditor::saveFile() {
    try {
        m_editor.StripTrailingWhitespaces();
        std::ofstream stream(m_filename.c_str());
        stream << m_editor.GetText();
        stream.close();
        m_version = m_editor.GetUndoIndex();

    } catch (std::exception& e) {
        showError(e.what());
    }
}

void CodeEditor::render() {
    // add a menubar
    renderMenuBar();

    ImGui::PushFont(m_fontParams.pFont, m_fontParams.fontSize);
    m_editor.Render("TextEditor");
    ImGui::PopFont();

    if (m_state == State::confirmClose) {
        renderConfirmClose();
    } else if (m_state == State::confirmQuit) {
        renderConfirmQuit();
    } else if (m_state == State::confirmError) {
        renderConfirmError();
    }
}

std::string CodeEditor::getCode() const {
    return m_editor.GetText();
}

void CodeEditor::setCode(const std::string& vCode) {
    m_editor.SetText(vCode);
}


void CodeEditor::tryToQuit() {
    if (isDirty()) {
        showConfirmQuit();
    } else {
        m_done = true;
    }
}

void CodeEditor::renderMenuBar() {
    // create menubar
    if (ImGui::BeginMenuBar()) {
        if (ImGui::BeginMenu("Edit")) {
            if (ImGui::MenuItem("Undo", " " SHORTCUT "Z", nullptr, m_editor.CanUndo())) {
                m_editor.Undo();
            }
#if __APPLE__
            if (ImGui::MenuItem("Redo", "^" SHORTCUT "Z", nullptr, m_editor.CanRedo())) {
                m_editor.Redo();
            }
#else
            if (ImGui::MenuItem("Redo", " " SHORTCUT "Y", nullptr, m_editor.CanRedo())) {
                m_editor.Redo();
            }
#endif

            ImGui::Separator();
            if (ImGui::MenuItem("Cut", " " SHORTCUT "X", nullptr, m_editor.AnyCursorHasSelection())) {
                m_editor.Cut();
            }
            if (ImGui::MenuItem("Copy", " " SHORTCUT "C", nullptr, m_editor.AnyCursorHasSelection())) {
                m_editor.Copy();
            }
            if (ImGui::MenuItem("Paste", " " SHORTCUT "V", nullptr, ImGui::GetClipboardText() != nullptr)) {
                m_editor.Paste();
            }

            ImGui::Separator();
            if (ImGui::MenuItem("Tabs To Spaces")) {
                m_editor.TabsToSpaces();
            }
            if (ImGui::MenuItem("Spaces To Tabs")) {
                m_editor.SpacesToTabs();
            }
            if (ImGui::MenuItem("Strip Trailing Whitespaces")) {
                m_editor.StripTrailingWhitespaces();
            }

            ImGui::EndMenu();
        }

        if (ImGui::BeginMenu("Selection")) {
            if (ImGui::MenuItem("Select All", " " SHORTCUT "A", nullptr, !m_editor.IsEmpty())) {
                m_editor.SelectAll();
            }
            ImGui::Separator();

            if (ImGui::MenuItem("Indent Line(s)", " " SHORTCUT "]", nullptr, !m_editor.IsEmpty())) {
                m_editor.IndentLines();
            }
            if (ImGui::MenuItem("Deindent Line(s)", " " SHORTCUT "[", nullptr, !m_editor.IsEmpty())) {
                m_editor.DeindentLines();
            }
            if (ImGui::MenuItem("Move Line(s) Up", nullptr, nullptr, !m_editor.IsEmpty())) {
                m_editor.MoveUpLines();
            }
            if (ImGui::MenuItem("Move Line(s) Down", nullptr, nullptr, !m_editor.IsEmpty())) {
                m_editor.MoveDownLines();
            }
            if (ImGui::MenuItem("Toggle Comments", " " SHORTCUT "/", nullptr, m_editor.HasLanguage())) {
                m_editor.ToggleComments();
            }
            ImGui::Separator();

            if (ImGui::MenuItem("To Uppercase", nullptr, nullptr, m_editor.AnyCursorHasSelection())) {
                m_editor.SelectionToUpperCase();
            }
            if (ImGui::MenuItem("To Lowercase", nullptr, nullptr, m_editor.AnyCursorHasSelection())) {
                m_editor.SelectionToLowerCase();
            }
            ImGui::Separator();

            if (ImGui::MenuItem("Add Next Occurrence", " " SHORTCUT "D", nullptr, m_editor.CurrentCursorHasSelection())) {
                m_editor.AddNextOccurrence();
            }
            if (ImGui::MenuItem("Select All Occurrences", "^" SHORTCUT "D", nullptr, m_editor.CurrentCursorHasSelection())) {
                m_editor.SelectAllOccurrences();
            }

            ImGui::EndMenu();
        }

        if (ImGui::BeginMenu("View")) {
            if (ImGui::MenuItem("Zoom In", " " SHORTCUT "+")) {
                increaseFontSIze();
            }
            if (ImGui::MenuItem("Zoom Out", " " SHORTCUT "-")) {
                decreaseFontSIze();
            }
            if (ImGui::MenuItem("Reset zoom", " " SHORTCUT "backspace", false, m_FontParamsDefault.fontSize != m_fontParams.fontSize)) {
                resetFontSIze();
            }
            
            ImGui::Separator();

            bool flag;
            flag = m_editor.IsShowWhitespacesEnabled();
            if (ImGui::MenuItem("Show Whitespaces", nullptr, &flag)) {
                m_editor.SetShowWhitespacesEnabled(flag);
            };
            flag = m_editor.IsShowLineNumbersEnabled();
            if (ImGui::MenuItem("Show Line Numbers", nullptr, &flag)) {
                m_editor.SetShowLineNumbersEnabled(flag);
            };
            flag = m_editor.IsShowingMatchingBrackets();
            if (ImGui::MenuItem("Show Matching Brackets", nullptr, &flag)) {
                m_editor.SetShowMatchingBrackets(flag);
            };
            flag = m_editor.IsCompletingPairedGlyphs();
            if (ImGui::MenuItem("Complete Matching Glyphs", nullptr, &flag)) {
                m_editor.SetCompletePairedGlyphs(flag);
            };
            flag = m_editor.IsShowPanScrollIndicatorEnabled();
            if (ImGui::MenuItem("Show Pan/Scroll Indicator", nullptr, &flag)) {
                m_editor.SetShowPanScrollIndicatorEnabled(flag);
            };
            flag = m_editor.IsMiddleMousePanMode();
            if (ImGui::MenuItem("Middle Mouse Pan Mode", nullptr, &flag)) {
                if (flag) {
                    m_editor.SetMiddleMousePanMode();
                } else {
                    m_editor.SetMiddleMouseScrollMode();
                }
            };

            ImGui::EndMenu();
        }

        if (ImGui::BeginMenu("Find")) {
            if (ImGui::MenuItem("Find", " " SHORTCUT "F")) {
                m_editor.OpenFindReplaceWindow();
            }
            if (ImGui::MenuItem("Find Next", " " SHORTCUT "G", nullptr, m_editor.HasFindString())) {
                m_editor.FindNext();
            }
            if (ImGui::MenuItem("Find All", "^" SHORTCUT "G", nullptr, m_editor.HasFindString())) {
                m_editor.FindAll();
            }
            ImGui::Separator();
            ImGui::EndMenu();
        }

        ImGui::EndMenuBar();
    }

    // handle keyboard shortcuts
    if (ImGui::IsWindowFocused(ImGuiFocusedFlags_RootAndChildWindows) && !ImGui::GetIO().WantCaptureKeyboard) {
        if (ImGui::IsKeyDown(ImGuiMod_Ctrl)) {
            if (ImGui::IsKeyPressed(ImGuiKey_N)) {
                newFile();
            } else if (ImGui::IsKeyPressed(ImGuiKey_S)) {
                saveFile();
            } else if (ImGui::IsKeyPressed(ImGuiKey_I)) {
                showDiff();
            } else if (ImGui::IsKeyPressed(ImGuiKey_Equal) || ImGui::IsKeyPressed(ImGuiKey_KeypadAdd)) {
                increaseFontSIze();
            } else if (ImGui::IsKeyPressed(ImGuiKey_Minus) || ImGui::IsKeyPressed(ImGuiKey_KeypadSubtract)) {
                decreaseFontSIze();
            } else if (ImGui::IsKeyPressed(ImGuiKey_Backspace)) {
                resetFontSIze();
            }
            if (ImGui::GetIO().MouseWheel > 0) {
                increaseFontSIze();
            } else if (ImGui::GetIO().MouseWheel < 0) {
                decreaseFontSIze();
            }
        }
    }
}

void CodeEditor::renderStatusBar() {
    // language support
    static const char* languages[] = {"C", "C++", "Cs", "AngelScript", "Lua", "Python", "GLSL", "HLSL", "JSON", "Markdown", "SQL"};

    static const TextEditor::Language* definitions[] = {
        TextEditor::Language::C(),
        TextEditor::Language::Cpp(),
        TextEditor::Language::Cs(),
        TextEditor::Language::AngelScript(),
        TextEditor::Language::Lua(),
        TextEditor::Language::Python(),
        TextEditor::Language::Glsl(),
        TextEditor::Language::Hlsl(),
        TextEditor::Language::Json(),
        TextEditor::Language::Markdown(),
        TextEditor::Language::Sql()};

    std::string language = m_editor.GetLanguageName();

    // create a statusbar window
    ImGui::PushStyleColor(ImGuiCol_ChildBg, ImVec4(0.15f, 0.15f, 0.15f, 1.0f));
    ImGui::BeginChild("StatusBar", ImVec2(0.0f, 0.0f), ImGuiChildFlags_Borders);
    ImGui::SetNextItemWidth(120.0f);

    // allow user to select language for colorizing
    if (ImGui::BeginCombo("##LanguageSelector", language.c_str())) {
        for (int n = 0; n < IM_ARRAYSIZE(languages); n++) {
            bool selected = (language == languages[n]);

            if (ImGui::Selectable(languages[n], selected)) {
                m_editor.SetLanguage(definitions[n]);
            }

            if (selected) {
                ImGui::SetItemDefaultFocus();
            }
        }

        ImGui::EndCombo();
    }

    // determine horizontal gap so the rest is right aligned
    ImGui::SameLine(0.0f, 0.0f);
    ImGui::AlignTextToFramePadding();

    int line;
    int column;
    int tabSize = m_editor.GetTabSize();
    float glyphWidth = ImGui::CalcTextSize("#").x;
    m_editor.GetCurrentCursor(line, column);

    // determine status message
    char status[256];

    auto statusSize = std::snprintf(status, sizeof(status), "Ln %d, Col %d  Tab Size: %d  File: %s", line + 1, column + 1, tabSize, m_filename.c_str());

    auto size = glyphWidth * (statusSize + 3);
    auto width = ImGui::GetContentRegionAvail().x;

    ImGui::SameLine(0.0f, width - size);
    ImGui::TextUnformatted(status);

    // render "text dirty" indicator
    ImGui::SameLine(0.0f, glyphWidth * 1.0f);
    auto drawlist = ImGui::GetWindowDrawList();
    auto pos = ImGui::GetCursorScreenPos();
    auto offset = ImGui::GetFrameHeight() * 0.5f;
    auto radius = offset * 0.6f;
    auto color = isDirty() ? IM_COL32(164, 0, 0, 255) : IM_COL32(164, 164, 164, 255);
    drawlist->AddCircleFilled(ImVec2(pos.x + offset, pos.y + offset), radius, color);

    ImGui::EndChild();
    ImGui::PopStyleColor();
}

void CodeEditor::showDiff() {
    m_diff.SetLanguage(m_editor.GetLanguage());
    m_diff.SetText(m_originalText, m_editor.GetText());
    m_state = State::diff;
}

void CodeEditor::showConfirmClose(std::function<void()> callback) {
    m_onConfirmClose = callback;
    m_state = State::confirmClose;
}

void CodeEditor::showConfirmQuit() {
    m_state = State::confirmQuit;
}

void CodeEditor::showError(const std::string& message) {
    m_errorMessage = message;
    m_state = State::confirmError;
}

void CodeEditor::renderDiff() {
    ImGui::OpenPopup("Changes since Opening File##m_diff");
    auto viewport = ImGui::GetMainViewport();
    ImVec2 center = viewport->GetCenter();
    ImGui::SetNextWindowPos(center, ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));

    if (ImGui::BeginPopupModal("Changes since Opening File##m_diff", NULL, ImGuiWindowFlags_AlwaysAutoResize)) {
        m_diff.Render("m_diff", viewport->Size * 0.8f, true);

        ImGui::Separator();
        static constexpr float buttonWidth = 80.0f;
        auto buttonOffset = ImGui::GetContentRegionAvail().x - buttonWidth;
        bool sideBySide = m_diff.GetSideBySideMode();

        if (ImGui::Checkbox("Show side-by-side", &sideBySide)) {
            m_diff.SetSideBySideMode(sideBySide);
        }

        ImGui::SameLine();
        ImGui::Indent(buttonOffset);

        if (ImGui::Button("OK", ImVec2(buttonWidth, 0.0f)) || ImGui::IsKeyPressed(ImGuiKey_Escape, false)) {
            m_state = State::edit;
            ImGui::CloseCurrentPopup();
        }

        ImGui::EndPopup();
    }
}

void CodeEditor::renderConfirmClose() {
    ImGui::OpenPopup("Confirm Close");
    ImVec2 center = ImGui::GetMainViewport()->GetCenter();
    ImGui::SetNextWindowPos(center, ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));

    if (ImGui::BeginPopupModal("Confirm Close", NULL, ImGuiWindowFlags_AlwaysAutoResize)) {
        ImGui::Text("This file has changed!\nDo you really want to delete it?\n\n");
        ImGui::Separator();

        static constexpr float buttonWidth = 80.0f;
        ImGui::Indent(ImGui::GetContentRegionAvail().x - buttonWidth * 2.0f - ImGui::GetStyle().ItemSpacing.x);

        if (ImGui::Button("OK", ImVec2(buttonWidth, 0.0f))) {
            m_state = State::edit;
            m_onConfirmClose();
            ImGui::CloseCurrentPopup();
        }

        ImGui::SameLine();

        if (ImGui::Button("Cancel", ImVec2(buttonWidth, 0.0f)) || ImGui::IsKeyPressed(ImGuiKey_Escape, false)) {
            m_state = State::edit;
            ImGui::CloseCurrentPopup();
        }

        ImGui::EndPopup();
    }
}

void CodeEditor::renderConfirmQuit() {
    ImGui::OpenPopup("Quit CodeEditor?");
    ImVec2 center = ImGui::GetMainViewport()->GetCenter();
    ImGui::SetNextWindowPos(center, ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));

    if (ImGui::BeginPopupModal("Quit CodeEditor?", NULL, ImGuiWindowFlags_AlwaysAutoResize)) {
        ImGui::Text("Your text has changed and is not saved!\nDo you really want to quit?\n\n");
        ImGui::Separator();

        static constexpr float buttonWidth = 80.0f;
        ImGui::Indent(ImGui::GetContentRegionAvail().x - buttonWidth * 2.0f - ImGui::GetStyle().ItemSpacing.x);

        if (ImGui::Button("OK", ImVec2(buttonWidth, 0.0f))) {
            m_done = true;
            m_state = State::edit;
            ImGui::CloseCurrentPopup();
        }

        ImGui::SameLine();

        if (ImGui::Button("Cancel", ImVec2(buttonWidth, 0.0f)) || ImGui::IsKeyPressed(ImGuiKey_Escape, false)) {
            m_state = State::edit;
            ImGui::CloseCurrentPopup();
        }

        ImGui::EndPopup();
    }
}

void CodeEditor::renderConfirmError() {
    ImGui::OpenPopup("Error");
    ImVec2 center = ImGui::GetMainViewport()->GetCenter();
    ImGui::SetNextWindowPos(center, ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));

    if (ImGui::BeginPopupModal("Error", NULL, ImGuiWindowFlags_AlwaysAutoResize)) {
        ImGui::Text("%s\n", m_errorMessage.c_str());
        ImGui::Separator();

        static constexpr float buttonWidth = 80.0f;
        ImGui::Indent(ImGui::GetContentRegionAvail().x - buttonWidth);

        if (ImGui::Button("OK", ImVec2(buttonWidth, 0.0f)) || ImGui::IsKeyPressed(ImGuiKey_Escape, false)) {
            m_errorMessage.clear();
            m_state = State::edit;
            ImGui::CloseCurrentPopup();
        }

        ImGui::EndPopup();
    }
}

bool CodeEditor::isDirty() const {
    return m_editor.GetUndoIndex() != m_version;
}

bool CodeEditor::isSavable() const {
    return isDirty() && m_filename != "untitled";
}

void CodeEditor::increaseFontSIze() {
    m_fontParams.fontSize = std::clamp(m_fontParams.fontSize + 1.0f, 8.0f, 100.0f);
}

void CodeEditor::decreaseFontSIze() {
    m_fontParams.fontSize = std::clamp(m_fontParams.fontSize - 1.0f, 8.0f, 100.0f);
}

void CodeEditor::resetFontSIze() {
    m_fontParams.fontSize = m_FontParamsDefault.fontSize;
}

void CodeEditor::clearErrorMarkers() {
    m_editor.ClearMarkers();
}

void CodeEditor::addErrorMarker(const ErrorMarker& vErrorMsg) {
    m_editor.AddMarker(vErrorMsg.line, vErrorMsg.lineNumberColor, vErrorMsg.textColor, vErrorMsg.lineNumberTooltip, vErrorMsg.textTooltip);
}
