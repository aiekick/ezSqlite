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

#include "DBManager.h"

#include <ezlibs/ezLog.hpp>
#include <ezlibs/ezFile.hpp>

#include <backend/helpers/dbHelper.h>
#include <backend/controller/controller.h>

#include <LayoutManager.h>

void DBManager::clear() {
    m_databaseFilePathName.clear();
    m_databaseFileName.clear();
    m_databaseFilePath.clear();
    m_isLoaded = false;
}

void DBManager::newDatabaseFromMemory() {
    clear();
    m_isLoaded = true;
}

bool DBManager::newDatabaseFromFile(const std::string& vFilePathName) {
    clear();
    DBHelper::ref().createDBFile(vFilePathName);
    const auto filePathName = ez::file::simplifyFilePath(vFilePathName);
    auto ps = ez::file::parsePathFileName(filePathName);
    if (ps.isOk) {
        m_databaseFilePathName = filePathName;
        m_databaseFileName = ps.name;
        m_databaseFilePath = ps.path;
        Controller::ref().clearAnalyze();
        m_isLoaded = true;
    }
    return m_isLoaded;
}

bool DBManager::loadDatabaseFromFile() {
    return loadDatabaseFromFile(m_databaseFilePathName);
}

// if wanted to not pass the adress for re open case
// else, the clear will set vFilePathName to empty because with re open, target m_databaseFilePathName
bool DBManager::loadDatabaseFromFile(const std::string& vFilePathName) {
    if (!vFilePathName.empty()) {
        clear();
        const auto filePathName = ez::file::simplifyFilePath(vFilePathName);
        if (DBHelper::ref().isFileASqlite3DB(filePathName)) {
            auto ps = ez::file::parsePathFileName(filePathName);
            if (ps.isOk) {
                Controller::ref().clearAnalyze();
                if (Controller::ref().analyzeDatabase(filePathName)) {
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

bool DBManager::isDatabaseLoaded() const {
    return m_isLoaded;
}

void DBManager::newFrame() {

}

std::string DBManager::getDatabaseFilepathName() const {
    return m_databaseFilePathName;
}

ez::xml::Nodes DBManager::getXmlNodes(const std::string& vUserDatas) {
    ez::xml::Node node;
    return node.getChildren();
}

bool DBManager::setFromXmlNodes(const ez::xml::Node& vNode, const ez::xml::Node& vParent, const std::string& vUserDatas) {
    const auto& strName = vNode.getName();
    const auto& strValue = vNode.getContent();
    const auto& strParentName = vParent.getName();

    return true;
}
