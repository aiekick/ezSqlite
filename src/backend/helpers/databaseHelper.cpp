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

#include "databaseHelper.h"

#include <cstring>
#include <fstream>
#include <vector>
#include <new>

#include <sqlite3/sqlite3.hpp>
#include <ezlibs/ezFile.hpp>

void SqliteDbDeleter::operator()(sqlite3* vDb) const noexcept {
    if (vDb != nullptr) {
        sqlite3_close_v2(vDb);
    }
}

const int32_t DatabaseHelper::m_maxInsertAttempts = 50;

bool DatabaseHelper::init(const std::string& vDBFilePathName) noexcept {
    unit();
    m_dataBaseFilePathName = vDBFilePathName;
    return true;
}

void DatabaseHelper::unit() noexcept {
    m_closeDB();
    m_dataBaseFilePathName.clear();
    m_lastErrorMsg.clear();
    m_transactionStarted = false;
}

// DATABASE FILE

bool DatabaseHelper::isFileASqlite3DB(const std::string& vDBFilePathName) const noexcept {
    auto res = false;
    std::ifstream fileStream(vDBFilePathName, std::ios_base::binary);
    if (fileStream.is_open()) {
        char magicHeader[16 + 1] = {};
        fileStream.read(magicHeader, 16);
        // "SQLite format 3\000"
        const char expected[16 + 1] = {'S', 'Q', 'L', 'i', 't', 'e', ' ', 'f', 'o', 'r', 'm', 'a', 't', ' ', '3', '\0', '\0'};
        res = std::memcmp(magicHeader, expected, 16) == 0;
        fileStream.close();
    }
    return res;
}

bool DatabaseHelper::createDBFile(const std::string& vDBFilePathName) noexcept {
    if (vDBFilePathName.empty()) {
        return false;
    }
    m_dataBaseFilePathName = vDBFilePathName;
    ez::file::destroyFile(m_dataBaseFilePathName);
    return m_createDB();
}

bool DatabaseHelper::openDBFile() noexcept {
    return openDBFile(m_dataBaseFilePathName);
}

bool DatabaseHelper::openDBFile(const std::string& vDBFilePathName) noexcept {
    if (m_sqliteDb != nullptr) {
        // already open
        return true;
    }
    m_dataBaseFilePathName = vDBFilePathName;
    return m_openDB();
}

void DatabaseHelper::closeDBFile() noexcept {
    m_closeDB();
}

// TRANSACTIONS

bool DatabaseHelper::beginDBTransaction() noexcept {
    if (!m_openDB()) {
        return false;
    }
    const auto rc = m_debugSqlite3Exec(__FUNCTION__, "BEGIN TRANSACTION;");
    if (rc == SQLITE_OK) {
        m_transactionStarted = true;
        return true;
    }
    return false;
}

void DatabaseHelper::commitDBTransaction() noexcept {
    (void)m_debugSqlite3Exec(__FUNCTION__, "COMMIT;");
    m_transactionStarted = false;
    m_closeDB();
}

void DatabaseHelper::rollbackDBTransaction() noexcept {
    (void)m_debugSqlite3Exec(__FUNCTION__, "ROLLBACK;");
    m_transactionStarted = false;
}

// MISC

std::string DatabaseHelper::getLastErrorMsg() const noexcept {
    return m_lastErrorMsg;
}

datas::QueryResult DatabaseHelper::executeQuery(const std::string& vSql) noexcept {
    datas::QueryResult result{};

    if (!m_openDB()) {
        return result;
    }

    m_lastErrorMsg.clear();
    sqlite3_stmt* stmt = nullptr;
    if (m_debugSqlite3PrepareV2(__FUNCTION__, vSql, -1, &stmt, nullptr) != SQLITE_OK) {
        m_lastErrorMsg = sqlite3_errmsg(m_sqliteDb.get());
        return result;
    }

    const int colCount = sqlite3_column_count(stmt);
    result.columns.reserve(colCount);

    for (int i = 0; i < colCount; ++i) {
        datas::ColumnInfo ci;
        ci.name = sqlite3_column_name(stmt, i);
        const char* decl = sqlite3_column_decltype(stmt, i);
        ci.declType = decl ? decl : "";
        result.columns.push_back(std::move(ci));
    }

    while (true) {
        int stepRes = sqlite3_step(stmt);
        if (stepRes == SQLITE_ROW) {
            datas::Row row;
            row.values.reserve(colCount);
            for (int i = 0; i < colCount; ++i) {
                int type = sqlite3_column_type(stmt, i);
                switch (type) {
                    case SQLITE_INTEGER: row.values.emplace_back((int64_t)sqlite3_column_int64(stmt, i)); break;
                    case SQLITE_FLOAT: row.values.emplace_back(sqlite3_column_double(stmt, i)); break;
                    case SQLITE_TEXT: row.values.emplace_back(std::string(reinterpret_cast<const char*>(sqlite3_column_text(stmt, i)))); break;
                    case SQLITE_BLOB: {
                        const void* data = sqlite3_column_blob(stmt, i);
                        int size = sqlite3_column_bytes(stmt, i);
                        row.values.emplace_back(std::vector<uint8_t>((const uint8_t*)data, (const uint8_t*)data + size));
                        break;
                    }
                    default: row.values.emplace_back(nullptr); break;
                }
            }
            result.rows.push_back(std::move(row));
        } else if (stepRes == SQLITE_DONE) {
            break;
        } else {
            m_lastErrorMsg = sqlite3_errmsg(m_sqliteDb.get());
            break;
        }
    }

    sqlite3_finalize(stmt);

    m_closeDB();
    return result;
}

// PRIVATE

bool DatabaseHelper::m_openDB() noexcept {
    if (m_sqliteDb != nullptr) {
        return true;
    }

    sqlite3* rawHandle = nullptr;
    const auto flags = SQLITE_OPEN_READWRITE;  // open existing
    const auto rc = sqlite3_open_v2(m_dataBaseFilePathName.c_str(), &rawHandle, flags, nullptr);
    if (rc != SQLITE_OK) {
        if (rawHandle != nullptr) {
            m_lastErrorMsg = sqlite3_errmsg(rawHandle);
            sqlite3_close_v2(rawHandle);
        } else {
            m_lastErrorMsg = "sqlite3_open_v2 failed.";
        }
        return false;
    }

    m_sqliteDb.reset(rawHandle);
    (void)m_enableForeignKey();
    return true;
}

bool DatabaseHelper::m_createDB() noexcept {
    m_closeDB();

    sqlite3* rawHandle = nullptr;
    const auto flags = SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE;
    const auto rc = sqlite3_open_v2(m_dataBaseFilePathName.c_str(), &rawHandle, flags, nullptr);
    if (rc != SQLITE_OK) {
        if (rawHandle != nullptr) {
            m_lastErrorMsg = sqlite3_errmsg(rawHandle);
            sqlite3_close_v2(rawHandle);
        } else {
            m_lastErrorMsg = "sqlite3_open_v2 failed.";
        }
        return false;
    }

    m_sqliteDb.reset(rawHandle);
    (void)m_enableForeignKey();
    m_closeDB();  // keep behavior: close after creation
    return true;
}

void DatabaseHelper::m_closeDB() noexcept {
    if (!m_transactionStarted) {
        m_sqliteDb.reset();
    }
}

bool DatabaseHelper::m_enableForeignKey() noexcept {
    if (m_sqliteDb == nullptr) {
        return false;
    }
    const auto rc = m_debugSqlite3Exec(__FUNCTION__, "PRAGMA foreign_keys = ON;");
    if (rc != SQLITE_OK) {
        m_lastErrorMsg = sqlite3_errmsg(m_sqliteDb.get());
        return false;
    }
    return true;
}

int32_t DatabaseHelper::m_debugSqlite3Exec(  //
    const std::string& vDebugLabel,    //
    const std::string& vSqlQuery) noexcept {
    (void)vDebugLabel;  // kept for parity, can be used to log SQL filenames in debug
    char* errMsg = nullptr;
    const auto rc = sqlite3_exec(m_sqliteDb.get(), vSqlQuery.c_str(), nullptr, nullptr, &errMsg);
    if (rc != SQLITE_OK) {
        if (errMsg != nullptr) {
            m_lastErrorMsg.assign(errMsg);
            sqlite3_free(errMsg);
        } else if (m_sqliteDb != nullptr) {
            m_lastErrorMsg = sqlite3_errmsg(m_sqliteDb.get());
        }
    }
    return rc;
}

int32_t DatabaseHelper::m_debugSqlite3PrepareV2(  //
    const std::string& vDebugLabel,         //
    const std::string& vSqlQuery,           //
    int32_t vNBytes,                        //
    sqlite3_stmt** vppStmt,                 //
    const char** vpzTail) noexcept {
    (void)vDebugLabel;
    return sqlite3_prepare_v2(m_sqliteDb.get(), vSqlQuery.c_str(), vNBytes, vppStmt, vpzTail);
}
