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

#include <ezlibs/ezClass.hpp>
#include <ezlibs/ezXmlConfig.hpp>
#include <ezlibs/ezSingleton.hpp>
#include <string>
#include <memory>

class DBManager : public ez::xml::Config {
    IMPLEMENT_SINGLETON(DBManager)
    DISABLE_CONSTRUCTORS(DBManager)
    DISABLE_DESTRUCTORS(DBManager)

private:  // to save
    std::string m_databaseFilePathName;
    std::string m_databaseFileName;
    std::string m_databaseFilePath;

private:  // dont save
    bool m_isLoaded = false;

public:
    void clear();
    void newDatabaseFromMemory();
    bool newDatabaseFromFile(const std::string& vFilePathName);
    bool loadDatabaseFromFile();
    bool loadDatabaseFromFile(const std::string& vFilePathName);
    bool isDatabaseLoaded() const;

    void newFrame();

    std::string getDatabaseFilepathName() const;

    ez::xml::Nodes getXmlNodes(const std::string& vUserDatas = "") override;
    bool setFromXmlNodes(const ez::xml::Node& vNode, const ez::xml::Node& vParent, const std::string& vUserDatas) override;
};
