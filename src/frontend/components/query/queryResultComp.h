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

class QueryResultComp : public ez::xml::Config {
    IMPLEMENT_SINGLETON(QueryResultComp)
    DISABLE_CONSTRUCTORS(QueryResultComp)
    DISABLE_DESTRUCTORS(QueryResultComp)
public:
    static ImU32 sGetSqliteValueTypeColor(const std::string& vSqliteValueType);
    static ImU32 sGetSqliteValueTypeColor(const datas::SqliteValueType vSqliteValueType);
    static void sColorizeTableCell(const ImU32 vColor);

private:
    ImGuiListClipper m_queryResultTableClipper;
    float m_textHeight{0.0f};
    datas::QueryResult m_queryResult;
    std::string m_cellValue;
    int32_t m_selRow{-1};
    int32_t m_selCol{-1};

public:
    void setResult(const datas::QueryResult& vResult);

    bool drawTable();
    void drawValue();

    ez::xml::Nodes getXmlNodes(const std::string& vUserDatas = "") override;
    bool setFromXmlNodes(const ez::xml::Node& vNode, const ez::xml::Node& vParent, const std::string& vUserDatas) override;

private:
    bool m_drawQueryResultTable(const datas::QueryResult& vResult, int& ioSelRow, int& ioSelCol, std::string& vOutValue);
};
