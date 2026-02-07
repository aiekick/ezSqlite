#include "diagramManager.h"
#include <ezlibs/ezImGui.hpp>
#include <grapher/baseNode.h>
#include <grapher/baseSlot.h>
#include <backend/diagram/diagramLibrary.h>
#include <backend/diagram/links/ERLink.h>
#include <LayoutManager.h>
#include <frontend/panes/diagram/diagramViewPane.h>


bool DiagramManager::init() {
    m_graphStyle.style.NodeRounding = 1.0f;
    m_graphStyle.style.NodeBorderWidth = 0.5f;
    m_graphStyle.style.altDragSnapping = 5.0f;
    m_graphStyle.style.PinRounding = 1.0f;
    m_graphStyle.style.LinkStrength = 0.0f;  // Straight links instead of splines
    m_graphConfig.showFlow = true;
    m_graphConfig.flowType = "FLOW";
    m_graphConfig.showFlowKey = ImGuiKey_Backspace;
    m_graphPtr = BaseGraph::create(m_graphStyle, m_graphConfig);
    m_graphPtr->setLoadNodeFromXmlFunctor(                                                                                                    //
        [this](const BaseGraphWeak& vGraph, const ez::xml::Node& vNode, const ez::xml::Node& vParent, BaseGraph::UserDatas /*vUserDatas*/) {  //
            return m_loadNodeFromXml(vGraph, vNode, vParent);
        });
    m_graphPtr->setBgRightClickActionFunctor(                                           //
        [this](const BaseGraphWeak& /*vGraph*/, BaseGraph::UserDatas /*vUserDatas*/) {  //
            m_showLibrary();
        });
    m_graphPtr->setPrepareForCreateNodeFromSlotActionFunctor(                                                              //
        [this](const BaseGraphWeak& /*vGraph*/, const BaseSlotWeak& vSlot, BaseGraph::UserDatas /*vUserDatas*/) -> bool {  //
            m_createNodeFromSlot = vSlot;
            BaseLibrary::SlotType slot_type;
            if (!m_createNodeFromSlot.expired()) {
                slot_type = m_createNodeFromSlot.lock()->getDatas<BaseSlot::BaseSlotDatas>().type;
            }
            return m_filterLibraryForInputSlotType(slot_type);
        });
    m_graphPtr->setSelectNodeActionFunctor(                                                                    //
        [this](const BaseGraphWeak& vGraph, const BaseNodeWeak& vNode, BaseGraph::UserDatas /*vUserDatas*/) {  //
            m_selectNode(vGraph, vNode);
        });
    m_graphPtr->setCreateLinkFunctor(                                                                 //
        [](const BaseStyle& vParentStyle, const BaseSlotWeak& vStart, const BaseSlotWeak& vEnd) {  //
            return ERLink::create(vParentStyle, vStart, vEnd);
        });
    addSlotColor("NONE", ImVec4(0.5f, 0.5f, 0.5f, 1.0f));
    addSlotColor("FLOW", ImVec4(0.9f, 0.9f, 0.9f, 1.0f));
    addSlotColor("FILE", ImVec4(0.5f, 0.5f, 0.9f, 1.0f));
    addSlotColor("STRING", ImVec4(0.9f, 0.9f, 0.1f, 1.0f));
    addSlotColor("TEXT", ImVec4(0.9f, 0.5f, 0.1f, 1.0f));

    m_diagramLibrary = DiagramLibrary::getLibrary();

    return true;
}

void DiagramManager::unit() {
    m_graphPtr.reset();
}

void DiagramManager::clear() {
    m_graphPtr->clear();
}

bool DiagramManager::loadDatabase(const datas::DatabaseDesc& vDatabaseDesc) {
    bool ret = false;
    if (!m_databaseDesc.isValid()) {
        m_databaseDesc = vDatabaseDesc;
        ret = true;
        // Create all table nodes first
        std::map<std::string, TableNodeWeak> tableNodes;
        for (const auto& tbl : m_databaseDesc.tables) {
            auto pTblNode = m_graphPtr->createChildNode<TableNode>().lock();
            if (pTblNode != nullptr) {
                ret &= pTblNode->loadShema(tbl);
                tableNodes[tbl.name] = pTblNode;
            }
        }
        // Create FK connections
        for (const auto& tbl : m_databaseDesc.tables) {
            if (tableNodes.find(tbl.name) != tableNodes.end()) {
                auto srcTableNode = tableNodes[tbl.name].lock();
                if (srcTableNode != nullptr) {
                    for (const auto& fk : tbl.foreignKeys) {
                        if (tableNodes.find(fk.refTable) != tableNodes.end()) {
                            auto dstTableNode = tableNodes[fk.refTable].lock();
                            if (dstTableNode != nullptr) {
                                // Find source column output slot and target column input slot
                                auto srcSlot = srcTableNode->findOutputSlotByColumnName(fk.columnName);
                                auto dstSlot = dstTableNode->findInputSlotByColumnName(fk.refColumn);
                                if (!srcSlot.expired() && !dstSlot.expired()) {
                                    m_graphPtr->connectSlots(srcSlot, dstSlot);
                                }
                            }
                        }
                    }
                }
            }
        }
        // Apply automatic layout after loading all nodes and connections
        m_baseLayout.applyLayout(m_graphPtr);
    }
    LayoutManager::ref().FocusSpecificPane(DiagramViewPane::ref()->GetFlag());
    return ret;
}

bool DiagramManager::drawDiagram() {
    if (ImGui::BeginMenuBar()) {
        if (ImGui::BeginMenu("Settings")) {
            ImGui::MenuItem("Debug", nullptr, &m_graphStyle.debugMode);
            ImGui::EndMenu();
        }

        if (ImGui::MenuItem("Auto layout")) {
            m_baseLayout.applyLayout(m_graphPtr);
        }

        m_graphPtr->setCurrentEditor();

        if (nd::GetSelectedObjectCount()) {
            if (ImGui::BeginMenu("Selection")) {
                if (ImGui::MenuItem("Zoom on Selection")) {
                    m_graphPtr->zoomToSelection();
                }
                if (ImGui::MenuItem("Center on Selection")) {
                    m_graphPtr->navigateToSelection();
                }
                ImGui::EndMenu();
            }
        }

        if (ImGui::BeginMenu("Content")) {
            if (ImGui::MenuItem("Zoom on Content")) {
                m_graphPtr->zoomToContent();
            }

            if (ImGui::MenuItem("Center on Content")) {
                m_graphPtr->navigateToContent();
            }

            ImGui::EndMenu();
        }

        if (ImGui::BeginMenu("Style")) {
            m_baseLayout.drawSettings();
            ImGui::EndMenu();
        }

        ImGui::EndMenuBar();
    }

    m_showCreateTableDialog();

    if (m_graphPtr->isGraphChanged()) {
        // ProjectFile::Instance()->SetProjectChange();
        m_graphPtr->setGraphChanged(false);
    }

    return m_graphPtr->drawGraph();
}

bool DiagramManager::drawControl() {
    auto node_ptr = m_selectedNode.lock();
    if (node_ptr != nullptr) {
        return node_ptr->drawWidgets();
    }
    return false;
}

BaseGraphWeak DiagramManager::getGraph() const {
    return m_graphPtr;
}

bool DiagramManager::getSlotColor(const std::string& vBaseSlotType, ImVec4& vOutColor) const {
    if (m_ColorSlots.find(vBaseSlotType) != m_ColorSlots.end()) {
        vOutColor = m_ColorSlots.at(vBaseSlotType);
        return true;
    }
    return false;
}

bool DiagramManager::getSlotColor(const std::string& vBaseSlotType, ImU32& vOutColor) const {
    if (m_ColorSlots.find(vBaseSlotType) != m_ColorSlots.end()) {
        vOutColor = ImGui::GetColorU32(m_ColorSlots.at(vBaseSlotType));
        return true;
    }
    return false;
}

void DiagramManager::addSlotColor(const std::string& vBaseSlotType, const ImVec4& vSlotColor) {
    m_ColorSlots[vBaseSlotType] = vSlotColor;
}

void DiagramManager::drawDebugInfos() {
    m_graphPtr->drawDebugInfos();
}

ez::xml::Nodes DiagramManager::getXmlNodes(const std::string& vUserDatas) {
    ez::xml::Node xml;
    if (vUserDatas == "app") {
    } else if (vUserDatas == "project") {
        xml.addChilds(m_graphPtr->getXmlNodes());
    }
    return xml.getChildren();
}

bool DiagramManager::setFromXmlNodes(const ez::xml::Node& vNode, const ez::xml::Node& vParent, const std::string& vUserDatas) {
    const auto& strName = vNode.getName();
    const auto& strValue = vNode.getContent();
    const auto& strParentName = vParent.getName();
    if (vUserDatas == "app") {
    } else if (vUserDatas == "project") {
        m_graphPtr->setFromXmlNodes(vNode, vParent, vUserDatas);
    }
    return false;  // prevent xml node childs exploring
}

BaseNodeWeak DiagramManager::createChildNodeInGraph(const BaseLibrary::NodeType& vNodeType, const BaseGraphWeak& vGraph) {
    return m_diagramLibrary.createChildNodeInGraph(vNodeType, vGraph);
}

void DiagramManager::beforeXmlLoading() {
    m_graphPtr->beforeXmlLoading();
}

void DiagramManager::afterXmlLoading() {
    m_graphPtr->afterXmlLoading();
}

void DiagramManager::m_selectNode(const BaseGraphWeak& vGraph, const BaseNodeWeak& vNode) {
    m_selectedNode = vNode;
}

bool DiagramManager::m_loadNodeFromXml(const BaseGraphWeak& vGraph, const ez::xml::Node& vNode, const ez::xml::Node& vParent) {
    auto graph_ptr = vGraph.lock();
    if (graph_ptr != nullptr) {
        auto node_type = vNode.getAttribute("type");
        auto node_ptr = createChildNodeInGraph(node_type, vGraph).lock();
        if (node_ptr != nullptr) {
            node_ptr->beforeXmlLoading();
            node_ptr->setFromXmlNodes(vNode, vParent, {});
        }
    }
    return false;
}

bool DiagramManager::m_filterLibraryForInputSlotType(const BaseLibrary::SlotType& vInputSlotType) {
    m_libraryToShow = m_diagramLibrary;
    if (!vInputSlotType.empty()) {
        return m_libraryToShow.filterNodesForSomeInputSlotTypes({vInputSlotType});
    }
    return false;
}

void DiagramManager::m_showLibrary() {
    // First show our custom "Create Table" menu item
    if (ImGui::BeginPopup("##DiagramLibraryMenu")) {
        if (ImGui::MenuItem("Create Table...")) {
            m_showCreateTableDialogPopup = true;
        }
        ImGui::Separator();
        ImGui::EndPopup();
    }

    // Then show the regular library menu
    BaseLibrary::LibraryEntry entryToCreate;
    if (m_libraryToShow.showMenu(entryToCreate)) {
        BaseNodeWeak new_node = m_libraryToShow.createChildNodeInGraph(entryToCreate, m_graphPtr);
        // new node just created
        if (!new_node.expired()) {
            // if created node from slot mode
            // we will connect the slot to the first input slot
            // of the corresponding type in the new node
            if (!m_createNodeFromSlot.expired()) {
                auto slot_ptr = m_createNodeFromSlot.lock();
                auto new_node_ptr = new_node.lock();
                auto wanted_slot_type = slot_ptr->getDatas<BaseSlot::BaseSlotDatas>().type;
                auto wanted_slot_name = slot_ptr->getDatas<BaseSlot::BaseSlotDatas>().name;
                auto found_slot = new_node_ptr->findSlotByTypeAndOptionalName(ez::SlotDir::INPUT, wanted_slot_type, wanted_slot_name);
                // a slot of the good type was found
                // we will connect it
                if (!found_slot.expired()) {
                    m_graphPtr->connectSlots(m_createNodeFromSlot, found_slot);
                } else {
                    // we have filtered the list for this slot
                    // so if we not have it, its not normal
                    // and we must check what happen
                    LogVarDebugError("Fail to found a slot of type [%s] for node of type [%s]", wanted_slot_type.c_str(), entryToCreate.nodeType.c_str());
                }
            }
        }
    }
}

void DiagramManager::m_showCreateTableDialog() {
    if (m_showCreateTableDialogPopup) {
        ImGui::OpenPopup("Create Table");
        m_showCreateTableDialogPopup = false;
    }

    if (ImGui::BeginPopupModal("Create Table", nullptr, ImGuiWindowFlags_AlwaysAutoResize)) {
        ImGui::Text("Enter table name:");
        ImGui::SetNextItemWidth(300.0f);
        if (ImGui::InputText("##TableName", m_newTableNameBuffer, sizeof(m_newTableNameBuffer), ImGuiInputTextFlags_EnterReturnsTrue)) {
            if (m_newTableNameBuffer[0] != '\0') {
                m_createEmptyTable(m_newTableNameBuffer);
                m_newTableNameBuffer[0] = '\0';
                ImGui::CloseCurrentPopup();
            }
        }
        ImGui::Separator();
        if (ImGui::Button("Create", ImVec2(120, 0))) {
            if (m_newTableNameBuffer[0] != '\0') {
                m_createEmptyTable(m_newTableNameBuffer);
                m_newTableNameBuffer[0] = '\0';
                ImGui::CloseCurrentPopup();
            }
        }
        ImGui::SameLine();
        if (ImGui::Button("Cancel", ImVec2(120, 0))) {
            m_newTableNameBuffer[0] = '\0';
            ImGui::CloseCurrentPopup();
        }
        ImGui::SetItemDefaultFocus();
        ImGui::EndPopup();
    }
}

TableNodeWeak DiagramManager::m_createEmptyTable(const std::string& vTableName) {
    datas::TableDesc tableDesc;
    tableDesc.name = vTableName;
    auto pTblNode = m_graphPtr->createChildNode<TableNode>().lock();
    if (pTblNode != nullptr) {
        pTblNode->loadShema(tableDesc);
    }
    return pTblNode;
}
