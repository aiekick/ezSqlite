#pragma once

#include <grapher/grapher.h>
#include <headers/defs.h>

class TableNode;
typedef std::shared_ptr<TableNode> TableNodePtr;
typedef std::weak_ptr<TableNode> TableNodeWeak;

class TableNode : public BaseNode {
    typedef BaseNode Parent;

private:
    datas::TableDesc m_schema;

public:
    explicit TableNode(const BaseStyle& vParentStyle);
    ENABLE_CLONE(TableNode);
    bool init() override;
    bool loadShema(const datas::TableDesc& vSchema);
    bool drawWidgets() override;
    void drawDebugInfos() override;
    ez::xml::Nodes getXmlNodes(const std::string& vUserDatas) override;
    bool setFromXmlNodes(const ez::xml::Node& vNode, const ez::xml::Node& vParent, const std::string& vUserDatas) override;

protected:
    bool m_drawHeader() override;
    bool m_drawHints() override;
    bool m_drawContent() override;
    virtual void m_getXmlModule(ez::xml::Node& vInOutNode);
    virtual void m_setXmlModule(const ez::xml::Node& vNode, const ez::xml::Node& vParent);
};
