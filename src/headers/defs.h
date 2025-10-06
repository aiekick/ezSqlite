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
#include <cstdint>

typedef std::string DBFile;

typedef uint32_t RowID;
typedef int64_t DateEpoch;

enum class SqliteType {  //
    TYPE_INTEGER = 0,
    TYPE_REAL,
    TYPE_TEXT,
    TYPE_BLOB,
    TYPE_NULL
};