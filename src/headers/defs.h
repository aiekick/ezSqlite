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

#include <set>
#include <map>
#include <array>
#include <vector>
#include <string>
#include <variant>
#include <cstdint>
#include <ezlibs/ezCnt.hpp>

namespace datas {
typedef std::string DBFile;

typedef uint32_t RowID;
typedef int64_t DateEpoch;

// SQLite-compatible value types
enum class SqliteValueType : uint8_t {  //
    Null = 0,
    Integer,
    Real,
    Text,
    Blob
};

// Bitmask-style constraints for columns
enum class ColumnConstraint : uint8_t {  //
    None = 0,
    PrimaryKey = 1 << 0,
    NotNull = 1 << 1,
    Unique = 1 << 2,
    AutoIncrement = 1 << 3,
    ForeignKey = 1 << 4
};

// Represents a foreign key constraint
struct ForeignKeyData {
    uint32_t seq{};
    std::string columnName;
    std::string refTable;
    std::string refColumn;
    std::string onUpdate;
    std::string onDelete;
    std::string match;
    void clear() { *this = ForeignKeyData(); }
    bool isValid() const { return (!columnName.empty()) && (!refTable.empty()) && (!refColumn.empty()); }
};

struct ColumnDesc {
    RowID cid{0};
    std::string name;
    std::string type;
    uint32_t colType{};  // ImU32
    SqliteValueType nativeType{SqliteValueType::Null};
    uint16_t constraints{static_cast<uint8_t>(ColumnConstraint::None)};
    std::string defaultValue;
    std::vector<size_t> fks;  // foerign key index of the TableDesc.foreignKeys vector
    bool isConstraint(ColumnConstraint vColumnConstraint) const { return !!(constraints & static_cast<uint8_t>(vColumnConstraint)); }
    void clear() { *this = ColumnDesc(); }
    bool isValid() const { return (cid != 0) && (!name.empty()) && (!type.empty()); }
};

struct TableDesc {
    std::string name;
    ez::cnt::DicoVector<std::string, ColumnDesc> columns;  // key is column name
    std::vector<ForeignKeyData> foreignKeys;
    void clear() { *this = TableDesc(); }
    bool isValid() const { return (!name.empty()) && (!columns.empty()); }
};

struct DatabaseDesc {
    std::string name;
    ez::cnt::DicoVector<std::string, TableDesc> tables;
    void clear() { *this = DatabaseDesc(); }
    bool isValid() const { return !tables.empty(); }
};

struct DatabasesDesc {
    ez::cnt::DicoVector<std::string, DatabaseDesc> databases;
    void clear() { *this = DatabasesDesc(); }
    bool isValid() const { return !databases.empty(); }
};

struct Query {
    std::string query;
    void clear() { *this = Query(); }
    bool isValid() const { return !query.empty(); }
    Query() = default;
    Query(const std::string& vQuery) : query(vQuery) {}
};

struct History {
    std::vector<Query> queries;
    std::set<std::string> uniqueQuery;
    void clear() { *this = History(); }
    bool isValid() const { return !queries.empty(); }
};

// Resultat generique de requete
struct ColumnInfo {
    std::string name;
    std::string declType;  // Type declare dans la table (peut etre vide)
};

struct Row {
    std::vector<std::variant<int64_t, double, std::string, std::vector<uint8_t>, std::nullptr_t>> values;
};

struct QueryResult {
    std::vector<ColumnInfo> columns;
    std::vector<Row> rows;
    bool isValid() const { return (!columns.empty()); }
    bool isValid(size_t vExpectedCountColumns) const { return (columns.size() == vExpectedCountColumns); }
    void clear() { *this = QueryResult(); }
};

}  // namespace datas
