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
#include <3rdparty/imgui_imguicolortextedit/TextEditor.h>

#include <algorithm>
#include <functional>
#include <string>
#include <ezlibs/ezClass.hpp>
#include <ezlibs/ezSingleton.hpp>
#include <ezlibs/ezXmlConfig.hpp>

#define FIND_POPUP_TEXT_FIELD_LENGTH 128

class QueryEditorComp : public ez::xml::Config {
    IMPLEMENT_SINGLETON(QueryEditorComp)
    DISABLE_CONSTRUCTORS(QueryEditorComp)
    DISABLE_DESTRUCTORS(QueryEditorComp)
private:

    std::string m_originalText;
    TextEditor m_editor;
    TextDiff m_diff;
    std::string m_filename;
    size_t m_version{};
    bool m_done{false};
    std::string m_errorMessage;
    std::function<void()> m_onConfirmClose;

    struct FontParams {
        ImFont* pFont{};
        float fontSize{17.0f};
    };
    const FontParams m_FontParamsDefault;
    FontParams m_fontParams;

    // editor state
    enum class State {  //
        edit,
        diff,
        newFile,
        confirmClose,
        confirmQuit,
        confirmError
    } m_state = State::edit;

public:
    struct ErrorMarker {
        int32_t line{};
        ImU32 lineNumberColor = IM_COL32(200, 50, 50, 255);
        ImU32 textColor = IM_COL32(200, 200, 200, 255);
        std::string lineNumberTooltip;
        std::string textTooltip;
    };

public:
    void setLanguage(const TextEditor::Language* vLanguage);

    void setFont(ImFont* vpFont);

    // file releated functions
    void newFile();
    void openFile(const std::string& path);
    void saveFile();

    // manage program exit
    void tryToQuit();
    inline bool isDone() const { return m_done; }

    // render the editor
    void render();

    std::string getCode() const;
    void setCode(const std::string& vCode);

    void clearErrorMarkers();
    void addErrorMarker(const ErrorMarker& vErrorMsg);

    ez::xml::Nodes getXmlNodes(const std::string& vUserDatas = "") override;
    bool setFromXmlNodes(const ez::xml::Node& vNode, const ez::xml::Node& vParent, const std::string& vUserDatas) override;

private:
    // private functions
    void renderMenuBar();
    void renderStatusBar();

    void showDiff();
    void showConfirmClose(std::function<void()> callback);
    void showConfirmQuit();
    void showError(const std::string& message);

    void renderDiff();
    void renderConfirmClose();
    void renderConfirmQuit();
    void renderConfirmError();

    bool isDirty() const;
    bool isSavable() const;

    void increaseFontSIze();
    void decreaseFontSIze();
    void resetFontSIze();
};
