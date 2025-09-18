/*
Copyright 2021-2023 Stephane Cuillerdier (aka aiekick)

Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at

http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License.
*/

#include "locationHelper.h"

#include <ezlibs/ezTools.hpp>
#include <imguipack/imguipack.h>


///////////////////////////////////////////////////////
//// STATIC ///////////////////////////////////////////
///////////////////////////////////////////////////////

LanguageEnum LocationHelper::s_HelpLanguage = LanguageEnum::FR;

const char* LocationHelper::layout_menu_name = nullptr;
const char* LocationHelper::layout_menu_help = nullptr;

const char* LocationHelper::mainframe_menubar_project = nullptr;
const char* LocationHelper::mainframe_menubar_project_open = nullptr;
const char* LocationHelper::mainframe_menubar_project_reload = nullptr;
const char* LocationHelper::mainframe_menubar_project_close = nullptr;
const char* LocationHelper::mainframe_menubar_settings = nullptr;

///////////////////////////////////////////////////////
//// CTOR /////////////////////////////////////////////
///////////////////////////////////////////////////////

bool LocationHelper::init() {
    defineLanguage(LanguageEnum::EN, true);  // Default
    return true;
}

void LocationHelper::unit() {

}

///////////////////////////////////////////////////////
//// CHANGE LANGUAGE //////////////////////////////////
///////////////////////////////////////////////////////

void LocationHelper::defineLanguage(LanguageEnum vLanguage, bool vForce) {
    if (vLanguage != LocationHelper::s_HelpLanguage || vForce) {
        LocationHelper::s_HelpLanguage = vLanguage;

        switch (vLanguage) {
            case LanguageEnum::EN: m_defineLanguageEN(); break;
            case LanguageEnum::FR: m_defineLanguageFR(); break;
            default: break;
        }
    }
}

///////////////////////////////////////////////////////
//// CHANGE IMGUI MENU ////////////////////////////////
///////////////////////////////////////////////////////

float LocationHelper::drawMenu() {
    float last_cur_pos = ImGui::GetCursorPosX();

    if (ImGui::MenuItem("EN", nullptr, LocationHelper::s_HelpLanguage == LanguageEnum::EN)) {
        defineLanguage(LanguageEnum::EN);
    }

    if (ImGui::IsItemHovered()) {
        ImGui::SetTooltip(R"(Change the translation to the English.
But you need to restart the app
And dont forgot reset the layout to the default after the restart
If you have a shity layout
)");
    }

    if (ImGui::MenuItem("FR", nullptr, LocationHelper::s_HelpLanguage == LanguageEnum::FR)) {
        defineLanguage(LanguageEnum::FR);
    }

    if (ImGui::IsItemHovered()) {
        ImGui::SetTooltip(R"(Change la traduction pour le Francais
Mais vous devez redemarrer l'application et ne pas oublier 
de reinitialier la disposition par defaut apres le redemarrage
si vous avez une disposition a la con
)");
    }

    return ImGui::GetCursorPosX() - last_cur_pos + ImGui::GetStyle().FramePadding.x;
}

///////////////////////////////////////////////////////
//// CHANGE LANGUAGE : PRIVATE ////////////////////////
///////////////////////////////////////////////////////

void LocationHelper::m_defineLanguageEN() {
    LocationHelper::layout_menu_name = " Layouts";
    LocationHelper::layout_menu_help = "Default Layout";

    LocationHelper::mainframe_menubar_project = "DatabaseDesc";
    LocationHelper::mainframe_menubar_project_open = " Open";
    LocationHelper::mainframe_menubar_project_reload = " Reload";
    LocationHelper::mainframe_menubar_project_close = " Close";
    LocationHelper::mainframe_menubar_settings = " Settings";
}

void LocationHelper::m_defineLanguageFR() {
    LocationHelper::layout_menu_name = " Dispositions";
    LocationHelper::layout_menu_help = "Disposition par defaut";

    LocationHelper::mainframe_menubar_project = "DatabaseDesc";
    LocationHelper::mainframe_menubar_project_open = " Ouvrir";
    LocationHelper::mainframe_menubar_project_reload = " Recharger";
    LocationHelper::mainframe_menubar_project_close = " Fermer";
    LocationHelper::mainframe_menubar_settings = " Reglages";
}

///////////////////////////////////////////////////////
//// CONFIGURATION ////////////////////////////////////
///////////////////////////////////////////////////////

ez::xml::Nodes LocationHelper::getXmlNodes(const std::string& vUserDatas) {
    ez::xml::Node node;
    node.addChild("help_lang").setContent(ez::str::toStr((int)LocationHelper::s_HelpLanguage));
    return node.getChildren();
}

bool LocationHelper::setFromXmlNodes(const ez::xml::Node& vNode, const ez::xml::Node& vParent, const std::string& vUserDatas) {
    const auto& strName = vNode.getName();
    const auto& strValue = vNode.getContent();
    const auto& strParentName = vParent.getName();

    if (strName == "help_lang") {
        defineLanguage((LanguageEnum)ez::ivariant(strValue).GetI());
    }
    
    return true;  // continue for explore childs. need to return false if we want explore child ourselves
}