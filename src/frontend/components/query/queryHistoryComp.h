#pragma once

#include <imguipack.h>
#include <ezlibs/ezClass.hpp>
#include <ezlibs/ezSingleton.hpp>
#include <ezlibs/ezXmlConfig.hpp>

#include <headers/defs.h>

#include <vector>
#include <string>
#include <set>


class QueryHistoryComp : public ez::xml::Config {
    IMPLEMENT_SINGLETON(QueryHistoryComp)
    DISABLE_CONSTRUCTORS(QueryHistoryComp)
    DISABLE_DESTRUCTORS(QueryHistoryComp)

private:
    datas::History m_history;

public:
    void drawHistory();

    void addQueryToHistory(const std::string& vQuery);

    ez::xml::Nodes getXmlNodes(const std::string& vUserDatas = "") override;
    bool setFromXmlNodes(const ez::xml::Node& vNode, const ez::xml::Node& vParent, const std::string& vUserDatas) override;
};
