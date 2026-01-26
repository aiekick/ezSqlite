#pragma once

#include <imguipack/imguipack.h>

#include <grapher/grapher.h>
#include <grapher/interfaces/SlotColorBankInterface.h>

#include <ezlibs/ezClass.hpp>
#include <ezlibs/ezSingleton.hpp>
#include <ezlibs/ezXmlConfig.hpp>

#include <headers/defs.h>

#include <memory>
#include <functional>

class DiagramManager : public SlotColorBankInterface, public IDrawDebugInfos, public ez::xml::Config {
    IMPLEMENT_SINGLETON(DiagramManager)
    DISABLE_CONSTRUCTORS(DiagramManager)
    DISABLE_DESTRUCTORS(DiagramManager)

private:
    BaseStyle m_graphStyle;
    BaseGraph::BaseGraphDatas m_graphConfig;
    BaseGraphPtr m_graphPtr = nullptr;
    std::map<std::string, ImVec4> m_ColorSlots;
    BaseLayout m_baseLayout;

    BaseLibrary m_diagramLibrary;
    // Library to show, can be filtered from m_nodesLibrary or not
    BaseLibrary m_libraryToShow;

    // used to create a node from this slot and connect
    // the input slot af the newx node to this slot
    // empty if not createFromSlot mode
    BaseSlotWeak m_createNodeFromSlot;

    BaseNodeWeak m_selectedNode;
    
    datas::DatabaseDesc m_databaseDesc;

public:
    bool init();
    void unit();
    void clear();

    bool loadDatabase(const datas::DatabaseDesc& vDatabaseDesc);

    bool drawDiagram();
    bool drawControl();
    BaseGraphWeak getGraph() const;
    bool getSlotColor(const std::string& vBaseSlotType, ImVec4& vOutColor) const override;
    bool getSlotColor(const std::string& vBaseSlotType, ImU32& vOutColor) const override;
    void addSlotColor(const std::string& vBaseSlotType, const ImVec4& vSlotColor) override;
    void drawDebugInfos() override;
    ez::xml::Nodes getXmlNodes(const std::string& vUserDatas) override;
    bool setFromXmlNodes(const ez::xml::Node& vNode, const ez::xml::Node& vParent, const std::string& vUserDatas) override;
    BaseNodeWeak createChildNodeInGraph(const BaseLibrary::NodeType& vNodeType, const BaseGraphWeak& vGraph);
    void beforeXmlLoading();
    void afterXmlLoading();

private:
    void m_selectNode(const BaseGraphWeak& vGraph, const BaseNodeWeak& vNode);
    bool m_loadNodeFromXml(const BaseGraphWeak& vGraph, const ez::xml::Node& vNode, const ez::xml::Node& vParent);
    bool m_filterLibraryForInputSlotType(const BaseLibrary::SlotType& vSlotType);
    void m_showLibrary();
};
