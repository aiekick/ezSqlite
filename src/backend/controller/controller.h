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
#include <headers/defs.h>
#include <ezlibs/ezCnt.hpp>
#include <ezlibs/ezClass.hpp>
#include <ezlibs/ezXmlConfig.hpp>
#include <ezlibs/ezSingleton.hpp>
#include <ezlibs/ezActions.hpp>
#include <backend/helpers/dbHelper.h>

#include <string>
#include <vector>
#include <map>
#include <set>

struct TableFieldDatas {
    RowID cid{};
    std::string name;
    std::string type;
    bool notNull{};
    std::string defaultValue;
    bool primaryKey{};
    void clear() { *this = TableFieldDatas(); }
    bool isValid() { return (cid != 0) && (!name.empty()) && (!type.empty()); }
};

struct TableDatas {
    std::string name;
    std::vector<TableFieldDatas> fields;
    void clear() { *this = TableDatas(); }
    bool isValid() { return (!name.empty()) && (!fields.empty()); }
};

struct Database {
    std::string name;
    ez::cnt::DicoVector<std::string, TableDatas> tables;
    void clear() { *this = Database(); }
    bool isValid() { return !tables.empty(); }
};

struct Databases {
    ez::cnt::DicoVector<std::string, Database> databases;
    void clear() { *this = Databases(); }
    bool isValid() { return !databases.empty(); }
};

struct Query {
    std::string query;
    void clear() { *this = Query(); }
    bool isValid() { return !query.empty(); }
    Query() = default;
    Query(const std::string& vQuery) : query(vQuery) {}
};

struct History {
    std::vector<Query> queries;
    std::set<std::string> uniqueQuery;
    void clear() { *this = History(); }
    bool isValid() { return !queries.empty(); }
};

class Controller : public ez::xml::Config {
    IMPLEMENT_SINGLETON(Controller)
    DISABLE_CONSTRUCTORS(Controller)
    DISABLE_DESTRUCTORS(Controller)
private:
    History m_history;
    Databases m_databases;
    ImGuiListClipper m_queryResultTableClipper;
    float m_textHeight{0.0f};
    QueryResult m_queryResult;
    std::string m_cellValue;
    int32_t m_selRow{-1};
    int32_t m_selCol{-1};
    ez::Actions m_actions;

public:
    bool init();
    void unit();

    void clearAnalyze();

    bool analyzeDatabase(const std::string& vDatabaseFilePathName);
    bool executeQuery(const std::string& vQuery, const bool vSaveQuery);

    void doActions();

    // IMGUI

    bool drawMenu(float& vOutWidth);
    void drawQueryResultTable();
    void drawQueryResultValue();
    void drawQueryHistory();
    void drawDatabaseStructure();

    ez::xml::Nodes getXmlNodes(const std::string& vUserDatas = "") override;
    bool setFromXmlNodes(const ez::xml::Node& vNode, const ez::xml::Node& vParent, const std::string& vUserDatas) override;

private:
    ImU32 m_getSqliteTypeColor(const SqliteType vSqliteType);
    void m_colorizeTableCell(const ImU32 vColor);
    bool m_drawQueryResultTable(const QueryResult& vResult, int& ioSelRow, int& ioSelCol, std::string& vOutValue);
    void m_addQueryToHistory(const std::string& vQuery);
    void m_drawTableContextMenu(const TableDatas& vTableDatas);
};
