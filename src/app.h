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

#include <memory>

#include <ezlibs/ezApp.hpp>
#include <ezlibs/ezClass.hpp>

class App {
    DISABLE_CONSTRUCTORS(App)
    DISABLE_DESTRUCTORS(App)
private:
    std::unique_ptr<ez::App> mp_app;

public:
    bool init(int argc, char** argv);
    void run();
    void unit();

private:
    void m_InitMessaging();
};
