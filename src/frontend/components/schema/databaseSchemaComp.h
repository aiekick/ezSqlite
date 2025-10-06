#pragma once

#include <imguipack.h>
#include <ezlibs/ezClass.hpp>
#include <ezlibs/ezActions.hpp>
#include <ezlibs/ezSingleton.hpp>
#include <ezlibs/ezXmlConfig.hpp>

#include <headers/defs.h>

#include <vector>
#include <string>
#include <set>

class DatabaseSchemaComp : public ez::xml::Config {
    IMPLEMENT_SINGLETON(DatabaseSchemaComp)
    DISABLE_CONSTRUCTORS(DatabaseSchemaComp)
    DISABLE_DESTRUCTORS(DatabaseSchemaComp)
private:
    ez::Actions m_actions;

public:
    void drawSchema();

    void doActions();

    ez::xml::Nodes getXmlNodes(const std::string& vUserDatas = "") override;
    bool setFromXmlNodes(const ez::xml::Node& vNode, const ez::xml::Node& vParent, const std::string& vUserDatas) override;

private:
    void m_drawTableContextMenu(const datas::TableDesc& vTableDatas);
};
