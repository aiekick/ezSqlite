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

#include "controller.h"
#include <resources/fontIcons.h>
#include <frontend/components/codeEditor.h>
#include <backend/managers/dbManager.h>
#include <ezlibs/ezSqlite.hpp>
#include <ezlibs/ezFile.hpp>
#include <ezlibs/ezLog.hpp>
#include <filesystem>

namespace fs = std::filesystem;

bool Controller::init() {
        return true;
}

void Controller::unit() {}

void Controller::clearAnalyze() {
     m_databases.clear();
}

bool Controller::drawMenu(float& vOutWidth) {
    bool needQueryExecution = false;
    float last_cur_pos = ImGui::GetCursorPosX();
    if (ImGui::MenuItem(ICON_FONT_PLAY " Execute query (F9)", "Execute query")) {
        needQueryExecution = true;
    }
    vOutWidth = ImGui::GetCursorPosX() - last_cur_pos + ImGui::GetStyle().FramePadding.x;
    if (ImGui::IsKeyPressed(ImGuiKey_F9)) {
        needQueryExecution = true;
    }
    bool ret = false;
    if (needQueryExecution) {
        ret = true;
        if (!executeQuery(CodeEditor::ref().getCode(), true)) {
            LogVarError("Failed to execute query");
        }
    }
    return ret;
}

bool Controller::analyzeDatabase(const std::string& vDatabaseFilePathName) {
    bool ret = false;
    if (fs::exists(vDatabaseFilePathName)) {
        if (DBHelper::ref().openDBFile(vDatabaseFilePathName)) {
            const auto& results = DBHelper::ref().executeQuery("SELECT name FROM sqlite_schema WHERE type='table' AND name NOT LIKE 'sqlite_%';");
            if (results.isValid() && results.columns.size() == 1U) {
                Database database;
                database.name = fs::path(vDatabaseFilePathName).stem().string();
                for (const auto& row : results.rows) {
                    if (row.values.size() == 1U) {
                        const auto& table_name = std::get<std::string>(row.values.at(0));
                        const auto& table_datas = DBHelper::ref().executeQuery(ez::str::toStr("PRAGMA table_info(%s)", table_name.c_str()));
                        if (table_datas.isValid()) {
                            TableDatas tblDatas;
                            for (size_t r = 0; r < table_datas.rows.size(); ++r) {
                                const auto& row = table_datas.rows.at(r);
                                TableFieldDatas fldDatas;
                                for (size_t c = 0; c < table_datas.columns.size(); ++c) {
                                    const auto& column = table_datas.columns.at(c).name;
                                    if (c < row.values.size()) {
                                        const auto& value = row.values.at(c);
                                        if (column == "cid") {
                                            fldDatas.cid = static_cast<RowID>(std::get<int64_t>(value));
                                        } else if (column == "name") {
                                            fldDatas.name = std::get<std::string>(value);
                                        } else if (column == "type") {
                                            fldDatas.type = std::get<std::string>(value);
                                            if (fldDatas.type == "INTEGER") {
                                                fldDatas.type = "INT"; // fro compact table column display
                                            }
                                        } else if (column == "notnull") {
                                            fldDatas.notNull = static_cast<bool>(!!std::get<int64_t>(value));
                                        } else if (column == "dflt_value") {
                                            if (std::holds_alternative<std::string>(value)) {  // not std::nullptr_t
                                                fldDatas.defaultValue = std::get<std::string>(value);
                                            }
                                        } else if (column == "pk") {
                                            fldDatas.primaryKey = static_cast<bool>(!!std::get<int64_t>(value));
                                        }
                                    }
                                }
                                tblDatas.name = table_name;
                                tblDatas.fields.push_back(fldDatas);
                            }
                            database.tables.tryAdd(table_name, tblDatas);
                        }
                    }
                }
                if (database.isValid()) {
                    m_databases.databases.tryAdd(database.name, database);
                    ret = true;
                }
            }
            DBHelper::ref().closeDBFile();
        }
    }
    return ret;
}

bool Controller::executeQuery(const std::string& vQuery, const bool vSaveQuery) {
    bool ret = false;
    if (vQuery.empty()) {
        ret = true;
    } else {
        ez::sqlite::Parser parser;
        ez::sqlite::Parser::Report report;
        if (parser.parse(vQuery, report)) {
            if (!report.ok) {
                for (const auto& err : report.errors) {
                    CodeEditor::ErrorMarker marker;
                    marker.line = err.pos.line-1;
                    marker.lineNumberColor = IM_COL32(200, 20, 20, 150);
                    marker.textColor = IM_COL32(200, 20, 20, 150);
                    marker.textTooltip = err.message;
                    CodeEditor::ref().clearErrorMarkers();
                    CodeEditor::ref().addErrorMarker(marker);
                }
            } else {
                m_queryResult = DBHelper::ref().executeQuery(vQuery);
                if (m_queryResult.isValid()) {
                    ret = true;
                } else {
                    const auto errorMsg = DBHelper::ref().getLastErrorMsg();
                    if (!errorMsg.empty()) {
                        CodeEditor::ErrorMarker marker;
                        marker.line = 0;
                        marker.lineNumberColor = IM_COL32(200, 20, 20, 150);
                        marker.textColor = IM_COL32(200, 20, 20, 150);
                        marker.textTooltip = errorMsg;
                        CodeEditor::ref().clearErrorMarkers();
                        CodeEditor::ref().addErrorMarker(marker);
                    } else {
                        ret = true;
                    }
                }
            }
        }
    }
    if (ret) {
        if (vSaveQuery) {
            m_addQueryToHistory(vQuery);
        }
        CodeEditor::ref().clearErrorMarkers();
        clearAnalyze();
        analyzeDatabase(DBManager::ref().getDatabaseFilepathName());
    }
    return ret;
}

void Controller::doActions() {
    m_actions.runImmediateActions();
}

void Controller::drawQueryResultTable() {
    if (m_queryResult.isValid()) {
        if (m_drawQueryResultTable(m_queryResult, m_selRow, m_selCol, m_cellValue)) {
            
        }
    }
}

void Controller::drawQueryResultValue() {
    if (m_queryResult.isValid()) {
        if (!m_cellValue.empty()) {
            ImGui::Text(m_cellValue.c_str());
        }
    }
}

void Controller::drawQueryHistory() {
    static ImGuiTreeNodeFlags tflags = ImGuiTreeNodeFlags_SpanFullWidth | ImGuiTreeNodeFlags_DefaultOpen;
    if (m_history.isValid()) {
        std::vector<Query>::reverse_iterator it_to_erase = m_history.queries.rend();
        ImGui::PushID("history");
        if (ImGui::TreeNodeEx("##recentsQueries", tflags, "Recents")) {
            ImGui::Indent();
            for (auto it = m_history.queries.rbegin(); it != m_history.queries.rend(); ++it) {
                if (ImGui::SmallContrastedButton("X")) {
                    it_to_erase = it;
                }
                ImGui::SameLine();
                if (ImGui::Selectable(it->query.c_str(), false, ImGuiSelectableFlags_AllowDoubleClick, ImVec2(0, ImGui::GetTextLineHeight()))) {
                    if (ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left)) {
                        CodeEditor::ref().setCode(it->query);
                    }
                }
                if (ImGui::IsItemHovered()) {
                    ImGui::SetTooltip(it->query.c_str());
                }
            }
            ImGui::Unindent();
            ImGui::TreePop();
        }
        ImGui::PopID();
        if (it_to_erase != m_history.queries.rend()) {
            if (m_history.uniqueQuery.find(it_to_erase->query) != m_history.uniqueQuery.end()) {
                m_history.uniqueQuery.erase(it_to_erase->query);
            }
            m_history.queries.erase(std::next(it_to_erase).base());
        }
    }
}

void Controller::drawDatabaseStructure() {
    static ImGuiTableFlags tf =        //
        ImGuiTableFlags_Borders        //
        | ImGuiTableFlags_RowBg        //
        | ImGuiTableFlags_ScrollX      //
        | ImGuiTableFlags_ScrollY      //
        | ImGuiTableFlags_Resizable    //
        | ImGuiTableFlags_Reorderable  //
        | ImGuiTableFlags_Hideable;
    static ImGuiTreeNodeFlags leaf = ImGuiTreeNodeFlags_Leaf | ImGuiTreeNodeFlags_NoTreePushOnOpen;
    static ImGuiTreeNodeFlags tflags = ImGuiTreeNodeFlags_OpenOnArrow;
    std::string query_to_execute;
    if (ImGui::BeginTable("DBTreeTable", 5, tf)) {
        ImGui::TableSetupScrollFreeze(0, 1);
        ImGui::TableSetupColumn("Name", ImGuiTableColumnFlags_WidthFixed);
        ImGui::TableSetupColumn("Type", ImGuiTableColumnFlags_WidthFixed);
        ImGui::TableSetupColumn("NN", ImGuiTableColumnFlags_WidthFixed);
        ImGui::TableSetupColumn("PK", ImGuiTableColumnFlags_WidthFixed);
        ImGui::TableSetupColumn("Default", ImGuiTableColumnFlags_WidthFixed);
        ImGui::TableHeadersRow();
        if (m_databases.isValid()) {
            for (const auto& database : m_databases.databases) {
                ImGui::TableNextRow();
                ImGui::TableSetColumnIndex(0);
                ImGui::PushID(database.name.c_str());
                if (ImGui::TreeNodeEx("##database", tflags | ImGuiTreeNodeFlags_DefaultOpen, "%s (%zu)", database.name.c_str(), database.tables.size())) {
                    ImGui::Indent();
                    for (const auto& kv : database.tables) {
                        ImGui::TableNextRow();
                        ImGui::TableSetColumnIndex(0);
                        ImGui::PushID(kv.name.c_str());
                        bool tableOpened = ImGui::TreeNodeEx("##table", tflags);
                        ImGui::SameLine();
                        ImGui::Selectable(ez::str::toStr("%s (%zu)", kv.name.c_str(), kv.fields.size()).c_str(), false);
                        if (query_to_execute.empty() && ImGui::IsItemHovered() && ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left)) {
                            query_to_execute = "SELECT * FROM " + kv.name + ";";
                        }
                        if (ImGui::BeginPopupContextItem(               //
                                NULL,                                   //
                                ImGuiPopupFlags_NoOpenOverItems |       //
                                    ImGuiPopupFlags_MouseButtonRight |  //
                                    ImGuiPopupFlags_NoOpenOverExistingPopup)) {
                            m_drawTableContextMenu(kv);
                            ImGui::EndPopup();
                        }
                        if (tableOpened) {
                            ImGui::Indent();
                            for (size_t i = 0; i < kv.fields.size(); ++i) {
                                const auto& c = kv.fields.at(i);
                                ImGui::TableNextRow();
                                ImGui::TableSetColumnIndex(0);
                                ImGui::TreeNodeEx((void*)(intptr_t)i, leaf, "%s", c.name.c_str());
                                ImGui::TableSetColumnIndex(1);
                                ImGui::TextUnformatted(c.type.c_str());
                                ImGui::TableSetColumnIndex(2);
                                ImGui::PushStyleColor(ImGuiCol_Text, c.notNull ? ImGui::CustomStyle::GoodColor : ImGui::CustomStyle::BadColor);
                                ImGui::TextUnformatted(c.notNull ? "YES" : "NO");
                                ImGui::PopStyleColor();
                                ImGui::TableSetColumnIndex(3);
                                ImGui::PushStyleColor(ImGuiCol_Text, c.primaryKey ? ImGui::CustomStyle::GoodColor : ImGui::CustomStyle::BadColor);
                                ImGui::TextUnformatted(c.primaryKey ? "YES" : "NO");
                                ImGui::PopStyleColor();
                                ImGui::TableSetColumnIndex(4);
                                if (!c.defaultValue.empty()) {
                                    ImGui::TextUnformatted(c.defaultValue.c_str());
                                } else {
                                    ImGui::TextDisabled("NULL");
                                }
                            }
                            ImGui::Unindent();
                            ImGui::TreePop();
                        }
                        ImGui::PopID();
                    }
                    ImGui::Unindent();
                    ImGui::TreePop();
                }
                ImGui::PopID();
            }
        }
        ImGui::EndTable();
    }
    if (!query_to_execute.empty()) {
        executeQuery(query_to_execute, false);
    }
}

ez::xml::Nodes Controller::getXmlNodes(const std::string& vUserDatas) {
    ez::xml::Node node;
    auto& controller = node.addChild("controller");
    auto& nodeHistory = controller.addChild("history");
    for (const auto& h : m_history.queries) {
        nodeHistory.addChild("query").setContent(ez::xml::Node::escapeXml(h.query));
    }
    return node.getChildren();
}

bool Controller::setFromXmlNodes(const ez::xml::Node& vNode, const ez::xml::Node& vParent, const std::string& vUserDatas) {
    const auto& strName = vNode.getName();
    const auto& strValue = vNode.getContent();
    const auto& strParentName = vParent.getName();
    if (strName == "controller" || strName == "history") {
        return true; // go on childs
    }
    if (strName == "query" && strParentName == "history") {
        m_addQueryToHistory(strValue);
    }
    return false; // stop here
}

ImU32 Controller::m_getSqliteTypeColor(const SqliteType vSqliteType) {
    switch (vSqliteType) {
        case SqliteType::TYPE_INTEGER: return IM_COL32(0, 100, 0, 100);  // INTEGER -> vert vif
        case SqliteType::TYPE_REAL: return IM_COL32(100, 0, 100, 100);   // REAL -> violet saturé
        case SqliteType::TYPE_BLOB: return IM_COL32(100, 20, 40, 100);   // BLOB -> rouge foncé/orangé
        case SqliteType::TYPE_NULL: return IM_COL32(100, 50, 50, 100);   // NULL -> rose foncé (HotPink)
        case SqliteType::TYPE_TEXT:                                     // no color
        default: break;
    }
    return 0;
}

void Controller::m_colorizeTableCell(const ImU32 vColor) {
    auto drawListPtr = ImGui::GetWindowDrawList();
    const auto& cursor = ImGui::GetCursorScreenPos();
    auto* tbl_ptr = GImGui->CurrentTable;
    const auto& table_column = tbl_ptr->Columns[tbl_ptr->CurrentColumn];
    const auto& column_height = ImGui::GetTextLineHeight();
    const auto& spacingX = IM_TRUNC(ImGui::GetStyle().ItemSpacing.x * 0.5f);
    const auto& spacingY = IM_TRUNC(ImGui::GetStyle().ItemSpacing.y * 0.5f);
    const ImVec2 pMin(table_column.MinX + spacingX - 1.0f, cursor.y - spacingY * 0.5f);
    const ImVec2 pMax(table_column.MaxX, cursor.y + column_height + spacingY);
    drawListPtr->AddRectFilled(pMin, pMax, vColor);
}

bool Controller::m_drawQueryResultTable(const QueryResult& vResult, int& ioSelRow, int& ioSelCol, std::string& vOutValue) {
    bool needResizeToFit{false};
    if (ImGui::BeginMenuBar()) {
        if (ImGui::BeginMenu("Sizing")) {
            if (ImGui::MenuItem("Size all columns to fit")) {
                needResizeToFit = true;
            }
            ImGui::EndMenu();
        }
        ImGui::EndMenuBar();
    }
    bool selectionChanged = false;
    const int colCount = static_cast<int>(vResult.columns.size());
    const int rowCount = static_cast<int>(vResult.rows.size());
    if (ImGui::BeginTable(                     //
            "##QueryResultTable",              //
            colCount,                          //
            ImGuiTableFlags_Borders            //
                | ImGuiTableFlags_RowBg        //
                | ImGuiTableFlags_ScrollX      //
                | ImGuiTableFlags_ScrollY      //
                | ImGuiTableFlags_Resizable    //
                | ImGuiTableFlags_Reorderable  //
                | ImGuiTableFlags_Hideable)) {
        ImGui::TableSetupScrollFreeze(0, 1);
        for (const auto& col : vResult.columns) {
            ImGui::TableSetupColumn(col.name.c_str(), ImGuiTableColumnFlags_WidthFixed);
        }
        ImGui::TableHeadersRow();
        m_textHeight = ImGui::GetTextLineHeight();
        m_queryResultTableClipper.Begin(rowCount, ImGui::GetTextLineHeightWithSpacing());
        while (m_queryResultTableClipper.Step()) {
            for (int r = m_queryResultTableClipper.DisplayStart; r < m_queryResultTableClipper.DisplayEnd; ++r) {
                if (r < 0) {
                    continue;
                }
                const auto& row = vResult.rows.at(r);
                ImGui::TableNextRow();
                for (int c = 0; c < colCount; ++c) {
                    SqliteType columnType{SqliteType::TYPE_TEXT};
                    ImGui::TableSetColumnIndex(c);
                    const char* label = "";
                    static char buf[256];
                    buf[0] = '\0';
                    if (c < static_cast<int>(row.values.size())) {
                        const auto& cell = row.values.at(c);
                        std::visit(
                            [&](auto&& val) {
                                using T = std::decay_t<decltype(val)>;
                                if constexpr (std::is_same_v<T, int64_t>) {  // type INTEGER
                                    columnType = SqliteType::TYPE_INTEGER;
                                    snprintf(buf, sizeof(buf), "%lld", static_cast<long long>(val));
                                } else if constexpr (std::is_same_v<T, double>) {  // type REAL
                                    columnType = SqliteType::TYPE_REAL;
                                    snprintf(buf, sizeof(buf), "%.6f", val);
                                } else if constexpr (std::is_same_v<T, std::string>) {  // type TEXT
                                    columnType = SqliteType::TYPE_TEXT;
                                    if (val.size() < sizeof(buf)) {
                                        memcpy(buf, val.c_str(), val.size() + 1);
                                    } else {
                                        snprintf(buf, sizeof(buf), "%.*s…", (int)sizeof(buf) - 2, val.c_str());
                                    }
                                } else if constexpr (std::is_same_v<T, std::vector<uint8_t>>) {  // type blob
                                    columnType = SqliteType::TYPE_BLOB;
                                    snprintf(buf, sizeof(buf), "[BLOB] %zu bytes", val.size());
                                } else if constexpr (std::is_same_v<T, std::nullptr_t>) {  // type NULL
                                    columnType = SqliteType::TYPE_NULL;
                                    snprintf(buf, sizeof(buf), "NULL");
                                }
                            },
                            cell);
                        label = buf[0] ? buf : "";
                    }
                    ImGui::PushID(r);
                    ImGui::PushID(c);
                    //if (columnType != SqliteType::TYPE_TEXT) {
                        m_colorizeTableCell(m_getSqliteTypeColor(columnType));
                    //}
                    const bool isSelected = (ioSelRow == r && ioSelCol == c);
                    if (ImGui::Selectable(  //
                            label,
                            isSelected,
                            ImGuiSelectableFlags_AllowOverlap,
                            ImVec2(0, m_textHeight))) {
                        ioSelRow = r;
                        ioSelCol = c;
                        vOutValue = label;
                        selectionChanged = true;
                    }
                    ImGui::PopID();  // c
                    ImGui::PopID();  // r
                }
            }
        }
        // Resizing
        if (needResizeToFit) {
            ImGui::TableSetColumnWidthAutoAll(ImGui::GetCurrentContext()->CurrentTable);
        }        
        ImGui::EndTable();
    }
    return selectionChanged;
}

void Controller::m_addQueryToHistory(const std::string& vQuery) {
    if (m_history.uniqueQuery.find(vQuery) == m_history.uniqueQuery.end()) {
        m_history.uniqueQuery.emplace(vQuery);
        m_history.queries.emplace_back(vQuery);
    }
}

void Controller::m_drawTableContextMenu(const TableDatas& vTableDatas) {
    if (ImGui::MenuItem("Show SELECT statement")) {
        CodeEditor::ref().setCode("SELECT * FROM " + vTableDatas.name + ";");
    }
    if (ImGui::MenuItem("Show CREATE statement")) {
        m_actions.pushBackImmediateAction([this, &vTableDatas]() {
            if (executeQuery("SELECT sql FROM sqlite_schema WHERE name = '" + vTableDatas.name + "';", false)) {
                if (m_queryResult.isValid()) {
                    CodeEditor::ref().setCode(  //
                        std::get<std::string>(  //
                            m_queryResult.rows.front().values.front()));
                    m_queryResult.clear();
                }
            }
        });
    }
    ImGui::Separator();
    if (ImGui::MenuItem("Show DROP TABLE statement")) {
        CodeEditor::ref().setCode("DROP TABLE " + vTableDatas.name + ";");
    }
}
