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

#include <App.h>
#include <string>
#include <iostream>

#include <ezlibs/ezLog.hpp>
#include <ezlibs/ezTools.hpp>

//#define ENABLE_MEM_CHECK

int main(int argc, char** argv) {
	int res = EXIT_SUCCESS;
	
#ifdef _MSC_VER
#ifdef _DEBUG
#ifdef ENABLE_MEM_CHECK
    // active memory leak detector
    // https://stackoverflow.com/questions/4790564/finding-memory-leaks-in-a-c-application-with-visual-studio
    _CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF | _CRTDBG_CHECK_CRT_DF);
    _CrtMemState sOld;
    _CrtMemCheckpoint(&sOld);  // take a snapshot
#endif
#endif
#endif

    {
        try {
            App app;
            if (app.init(argc, argv)) {
                app.run();
                app.unit();
            }
        } catch (const std::exception& e) {
            LogVarLightInfo("Exception %s", e.what());
            res = EXIT_FAILURE;
            EZ_TOOLS_DEBUG_BREAK;
        }

        ez::Log::ref().close();
    }

#ifdef _MSC_VER
#ifdef _DEBUG
#ifdef ENABLE_MEM_CHECK
    _CrtMemState sNew;
    _CrtMemCheckpoint(&sNew);  // take a snapshot
    _CrtMemState sDiff;
    if (_CrtMemDifference(&sDiff, &sOld, &sNew))  // if there is a difference
    {
        std::cout << "-----------_CrtMemDumpStatistics ---------" << std::endl;
        _CrtMemDumpStatistics(&sDiff);
        std::cout << "-----------_CrtMemDumpAllObjectsSince ---------" << std::endl;
        _CrtMemDumpAllObjectsSince(&sOld);
        std::cout << "-----------_CrtDumpMemoryLeaks ---------" << std::endl;
        _CrtDumpMemoryLeaks();
    }
#endif
#endif
#endif

    return res;
}
