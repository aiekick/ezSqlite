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

#include "app.h"

#include <headers/ezSqliteBuild.h>

#include <ezlibs/ezFile.hpp>
#include <ezlibs/ezLog.hpp>

#include <backend/backend.h>
#include <frontend/frontend.h>

#include <backend/managers/dbManager.h>

#include <imguipack.h>
#include <frontend/panes/MessagePane.h>
#include <frontend/helpers/locationHelper.h>

// messaging
#define MESSAGING_CODE_INFOS 0
#define MESSAGING_LABEL_INFOS "Infos"
#define MESSAGING_CODE_WARNINGS 1
#define MESSAGING_LABEL_WARNINGS "Warnings"
#define MESSAGING_CODE_ERRORS 2
#define MESSAGING_CODE_DEBUG 3
#define MESSAGING_LABEL_ERRORS "Errors"
#define MESSAGING_LABEL_DEBUG "Debug"

//////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////

bool App::init(int argc, char** argv) {
    mp_app = std::make_unique<ez::App>(argc, argv);
    printf("-----------\n");
    printf("[[ %s Beta %s ]]\n", ezSqlite_Label, ezSqlite_BuildId);
#ifdef _DEBUG
    ez::file::createDirectoryIfNotExist("sqlite3");
#endif
    auto loc = std::setlocale(LC_ALL, ".UTF8");
    if (!loc) {
        printf("setlocale fail to apply with this compiler. it seems the unicode will be NOK\n");
    }
    bool ret = true;
    ret &= (Backend::initSingleton() != nullptr);
    ret &= (LayoutManager::initSingleton() != nullptr);
    ret &= (Messaging::initSingleton() != nullptr);
    ret &= Backend::ref().init(*mp_app.get());
    m_InitMessaging();
    return true;
}

void App::unit() {
    Backend::ref().unit();
    Messaging::unitSingleton();
    LayoutManager::unitSingleton();
    Backend::unitSingleton();
}

void App::run() {
    Backend::ref().run();
}

void App::m_InitMessaging() {
    Messaging::ref().AddCategory(MESSAGING_CODE_INFOS, "Infos(s)", MESSAGING_LABEL_INFOS, ImVec4(0.0f, 0.8f, 0.0f, 1.0f));
    Messaging::ref().AddCategory(MESSAGING_CODE_WARNINGS, "Warnings(s)", MESSAGING_LABEL_WARNINGS, ImVec4(0.8f, 0.8f, 0.0f, 1.0f));
    Messaging::ref().AddCategory(MESSAGING_CODE_ERRORS, "Errors(s)", MESSAGING_LABEL_ERRORS, ImVec4(0.8f, 0.0f, 0.0f, 1.0f));
    Messaging::ref().AddCategory(MESSAGING_CODE_DEBUG, "Debug(s)", MESSAGING_LABEL_DEBUG, ImVec4(0.8f, 0.8f, 0.0f, 1.0f));
    Messaging::ref().SetLayoutManager(&LayoutManager::ref());
    ez::Log::ref().setStandardLogMessageFunctor([](const int& vType, const std::string& vMessage) {
        MessageData msg_datas;
        const auto& type = vType;
        Messaging::ref().AddMessage(vMessage, type, false, msg_datas, {});
    });
}