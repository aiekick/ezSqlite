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

#include "databaseManager.h"

#include <ezlibs/ezLog.hpp>
#include <ezlibs/ezFile.hpp>
#include <ezlibs/ezSqlite.hpp>

#include <backend/helpers/databaseHelper.h>
#include <backend/diagram/diagramManager.h>

#include <frontend/components/query/queryEditorComp.h>
#include <frontend/components/query/queryResultComp.h>
#include <frontend/components/query/queryHistoryComp.h>
#include <frontend/panes/query/queryResultsTablePane.h>

#include <LayoutManager.h>

#include <resources/fontIcons.h>

#include <filesystem>

namespace fs = std::filesystem;

void DatabaseManager::clear() {
    m_databaseFilePathName.clear();
    m_databaseFileName.clear();
    m_databaseFilePath.clear();
    m_isLoaded = false;
}

void DatabaseManager::newDatabaseFromMemory() {
    clear();
    m_isLoaded = true;
}

bool DatabaseManager::newDatabaseFromFile(const std::string& vFilePathName) {
    clear();
    DatabaseHelper::ref().createDBFile(vFilePathName);
    const auto filePathName = ez::file::simplifyFilePath(vFilePathName);
    auto ps = ez::file::parsePathFileName(filePathName);
    if (ps.isOk) {
        m_databaseFilePathName = filePathName;
        m_databaseFileName = ps.name;
        m_databaseFilePath = ps.path;
        clearAnalyze();
        m_isLoaded = true;
    }
    return m_isLoaded;
}

bool DatabaseManager::loadDatabaseFromFile() {
    return loadDatabaseFromFile(m_databaseFilePathName);
}

// if wanted to not pass the adress for re open case
// else, the clear will set vFilePathName to empty because with re open, target m_databaseFilePathName
bool DatabaseManager::loadDatabaseFromFile(const std::string& vFilePathName) {
    if (!vFilePathName.empty()) {
        clear();
        const auto filePathName = ez::file::simplifyFilePath(vFilePathName);
        if (DatabaseHelper::ref().isFileASqlite3DB(filePathName)) {
            auto ps = ez::file::parsePathFileName(filePathName);
            if (ps.isOk) {
                clearAnalyze();
                if (analyzeDatabase(filePathName)) {
                    m_databaseFilePathName = filePathName;
                    m_databaseFileName = ps.name;
                    m_databaseFilePath = ps.path;
                    m_isLoaded = true;
                }
            }
        }
    }
    return m_isLoaded;
}

bool DatabaseManager::isDatabaseLoaded() const {
    return m_isLoaded;
}

void DatabaseManager::newFrame() {
    if (m_needQueryExecution) {
        m_needQueryExecution = false;
        if (!executeQuery(QueryEditorComp::ref().getCode(), true, true)) {
            LogVarError("Failed to execute query");
        }
        else {
            QueryResultComp::ref().setResult(m_lastQueryResult);
        }
    }
}

std::string DatabaseManager::getDatabaseFilepathName() const {
    return m_databaseFilePathName;
}

void DatabaseManager::clearAnalyze() {
    m_databases.clear();
}

bool DatabaseManager::analyzeDatabase(const std::string& vDatabaseFilePathName) {
    bool ret = false;
    if (fs::exists(vDatabaseFilePathName)) {
        if (DatabaseHelper::ref().openDBFile(vDatabaseFilePathName)) {
            const auto& results = DatabaseHelper::ref().executeQuery("SELECT name FROM sqlite_schema WHERE type='table' AND name NOT LIKE 'sqlite_%';");
            if (results.isValid() && results.columns.size() == 1U) {
                datas::DatabaseDesc database;
                database.name = fs::path(vDatabaseFilePathName).stem().string();
                // tables
                for (const auto& row : results.rows) {
                    if (row.values.size() == 1U) {
                        const auto& table_name = std::get<std::string>(row.values.at(0));
                        const auto& table_info = DatabaseHelper::ref().executeQuery(ez::str::toStr("PRAGMA table_info('%s')", table_name.c_str()));
                        if (table_info.isValid()) {
                            datas::TableDesc tableDatas;
                            for (size_t r = 0; r < table_info.rows.size(); ++r) {
                                const auto& row = table_info.rows.at(r);
                                datas::ColumnDesc colDatas;
                                for (size_t c = 0; c < table_info.columns.size(); ++c) {
                                    const auto& column = table_info.columns.at(c).name;
                                    if (c < row.values.size()) {
                                        const auto& value = row.values.at(c);
                                        if (column == "cid") {
                                            colDatas.cid = static_cast<datas::RowID>(std::get<int64_t>(value));
                                        } else if (column == "name") {
                                            colDatas.name = std::get<std::string>(value);
                                        } else if (column == "type") {
                                            colDatas.type = std::get<std::string>(value);
                                            colDatas.colType = QueryResultComp::sGetSqliteValueTypeColor(colDatas.type);
                                            if (colDatas.type == "INTEGER") {
                                                colDatas.type = "Int";  // for compact table column display
                                            } else {                    // camelCase
                                                for (size_t idx = 1; idx < colDatas.type.size(); ++idx) {
                                                    colDatas.type[idx] = std::tolower(colDatas.type[idx]);
                                                }
                                            }
                                        } else if (column == "notnull") {
                                            if (static_cast<bool>(!!std::get<int64_t>(value))) {
                                                colDatas.constraints |= static_cast<int32_t>(datas::ColumnConstraint::NotNull);
                                            }
                                        } else if (column == "dflt_value") {
                                            if (std::holds_alternative<std::string>(value)) {  // not std::nullptr_t
                                                colDatas.defaultValue = std::get<std::string>(value);
                                            }
                                        } else if (column == "pk") {
                                            if (static_cast<bool>(!!std::get<int64_t>(value))) {
                                                colDatas.constraints |= static_cast<int32_t>(datas::ColumnConstraint::PrimaryKey);
                                            }
                                        }
                                    }
                                }
                                tableDatas.name = table_name;
                                tableDatas.columns.tryAdd(colDatas.name, colDatas);
                            }
                            // uniqueness check
                            const auto& unique_columns = DatabaseHelper::ref().executeQuery(ez::str::toStr(
                                "SELECT il.name AS name, il.'unique' AS un, ii.name AS col FROM pragma_index_list('%s') AS il JOIN pragma_index_info(il.name) AS ii WHERE il.'unique' = 1;",
                                table_name.c_str()));
                            if (unique_columns.isValid(3U)) {
                                for (const auto& c : unique_columns.rows) {
                                    const auto& unique_column_name = std::get<std::string>(c.values.at(2U));
                                    if (tableDatas.columns.exist(unique_column_name)) {
                                        auto& colDatas = tableDatas.columns.value(unique_column_name);
                                        colDatas.constraints |= static_cast<int32_t>(datas::ColumnConstraint::Unique);
                                    }
                                }
                            }
                            database.tables.tryAdd(table_name, tableDatas);
                        }
                    }
                }
                // foreign keys
                for (const auto& row : results.rows) {
                    if (row.values.size() == 1U) {
                        const auto& table_name = std::get<std::string>(row.values.at(0));
                        const auto& foreign_key_list = DatabaseHelper::ref().executeQuery(ez::str::toStr("PRAGMA foreign_key_list('%s')", table_name.c_str()));
                        if (database.tables.exist(table_name) && foreign_key_list.isValid()) {
                            auto& tableDatas = database.tables.value(table_name);
                            for (size_t r = 0; r < foreign_key_list.rows.size(); ++r) {
                                const auto& row = foreign_key_list.rows.at(r);
                                datas::ForeignKeyData fkDatas;
                                for (size_t c = 0; c < foreign_key_list.columns.size(); ++c) {
                                    const auto& column = foreign_key_list.columns.at(c).name;
                                    if (c < row.values.size()) {
                                        const auto& value = row.values.at(c);
                                        if (column == "seq") {
                                            fkDatas.seq = static_cast<uint32_t>(std::get<int64_t>(value));
                                        } else if (column == "table") {
                                            fkDatas.refTable = std::get<std::string>(value);
                                        } else if (column == "from") {
                                            fkDatas.columnName = std::get<std::string>(value);
                                        } else if (column == "to") {
                                            fkDatas.refColumn = std::get<std::string>(value);
                                        } else if (column == "on_update") {
                                            fkDatas.onUpdate = std::get<std::string>(value);
                                        } else if (column == "on_delete") {
                                            fkDatas.onDelete = std::get<std::string>(value);
                                        } else if (column == "match") {
                                            fkDatas.match = std::get<std::string>(value);
                                        }
                                    }
                                }
                                if (tableDatas.columns.exist(fkDatas.columnName)) {
                                    auto& colDatas = tableDatas.columns.value(fkDatas.columnName);
                                    colDatas.fks.push_back(tableDatas.foreignKeys.size());
                                    colDatas.constraints |= static_cast<uint16_t>(datas::ColumnConstraint::ForeignKey);
                                }
                                tableDatas.foreignKeys.push_back(fkDatas);
                            }
                        }
                    }
                }
                if (database.isValid()) {
                    m_databases.databases.tryAdd(database.name, database);
                    ret = true;
                }
            }
            DatabaseHelper::ref().closeDBFile();
        }
    }
    if (!m_databases.databases.empty()) {
        DiagramManager::ref().loadDatabase(m_databases.databases.front());
    }
    return ret;
}

const datas::DatabasesDesc& DatabaseManager::getAnalyzedDatabases() const {
    return m_databases;
}

bool DatabaseManager::drawMenu(float& vOutWidth) {
    bool ret = false;
    float last_cur_pos = ImGui::GetCursorPosX();
    if (ImGui::MenuItem(ICON_FONT_PLAY " Execute query (F9)", "Execute query")) {
        m_needQueryExecution = true;
        ret = true;
    }
    vOutWidth = ImGui::GetCursorPosX() - last_cur_pos + ImGui::GetStyle().FramePadding.x;
    if (ImGui::IsKeyPressed(ImGuiKey_F9)) {
        m_needQueryExecution = true;
        ret = true;
    }
    return ret;
}

bool DatabaseManager::executeQuery(const std::string& vQuery, const bool vSaveQuery, const bool vShowPane) {
    bool ret = false;
    if (!vQuery.empty()) {
        ez::sqlite::Parser parser;
        ez::sqlite::Parser::Report report;
        if (parser.parse(vQuery, report)) {
            if (!report.ok) {
                for (const auto& err : report.errors) {
                    QueryEditorComp::ErrorMarker marker;
                    marker.line = err.pos.line - 1;
                    marker.lineNumberColor = IM_COL32(200, 20, 20, 150);
                    marker.textColor = IM_COL32(200, 20, 20, 150);
                    marker.textTooltip = err.message;
                    QueryEditorComp::ref().clearErrorMarkers();
                    QueryEditorComp::ref().addErrorMarker(marker);
                }
            } else {
                m_lastQueryResult = DatabaseHelper::ref().executeQuery(vQuery);
                if (m_lastQueryResult.isValid()) {
                    ret = true;
                } else {
                    const auto errorMsg = DatabaseHelper::ref().getLastErrorMsg();
                    if (!errorMsg.empty()) {
                        QueryEditorComp::ErrorMarker marker;
                        marker.line = 0;
                        marker.lineNumberColor = IM_COL32(200, 20, 20, 150);
                        marker.textColor = IM_COL32(200, 20, 20, 150);
                        marker.textTooltip = errorMsg;
                        QueryEditorComp::ref().clearErrorMarkers();
                        QueryEditorComp::ref().addErrorMarker(marker);
                    } else {
                        ret = true;
                    }
                }
            }
        }
    }
    if (ret) {
        if (vSaveQuery) {
            QueryHistoryComp::ref().addQueryToHistory(vQuery);
        }
        if (vShowPane) {
            LayoutManager::ref().FocusSpecificPane(QueryResultsTablePane::ref()->GetFlag());
        }
        QueryEditorComp::ref().clearErrorMarkers();
        clearAnalyze();
        analyzeDatabase(DatabaseManager::ref().getDatabaseFilepathName());
    }
    return ret;
}

const datas::QueryResult& DatabaseManager::getLastQueryResult() const {
    return m_lastQueryResult;
}

ez::xml::Nodes DatabaseManager::getXmlNodes(const std::string& vUserDatas) {
    ez::xml::Node node;
    return node.getChildren();
}

bool DatabaseManager::setFromXmlNodes(const ez::xml::Node& vNode, const ez::xml::Node& vParent, const std::string& vUserDatas) {
    const auto& strName = vNode.getName();
    const auto& strValue = vNode.getContent();
    const auto& strParentName = vParent.getName();

    return true;
}
