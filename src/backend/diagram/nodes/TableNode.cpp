#include "TableNode.h"
#include <backend/diagram/slots/FieldSlot.h>
#include <frontend/components/query/queryResultComp.h>

TableNode::TableNode(const BaseStyle& vParentStyle)  //
    : Parent(vParentStyle, BaseNodeDatas("", "", ImGui::GetColorU32(ImVec4(0.2f, 0.5f, 0.2f, 1.0f)))) {}

bool TableNode::init() {
    return Parent::init();
}

bool TableNode::loadShema(const datas::TableDesc& vSchema) {
    bool ret = false;
    if (!m_schema.isValid()) {
        m_schema = vSchema;
        ret = true;
        auto& tableDatas = this->getDatasRef<BaseNodeDatas>();
        tableDatas.name = m_schema.name;
        tableDatas.type = "Table";
        for (const auto& col : m_schema.columns) {
            auto piCol = createChildSlot<FieldSlot>(ez::SlotDir::INPUT).lock();
            if (piCol != nullptr) {
                auto& slotDatas = piCol->getDatasRef<FieldSlot::FieldSlotDatas>();
                slotDatas.column = col;
                slotDatas.showName = true;
                slotDatas.name.clear();
                if (col.isConstraint(datas::ColumnConstraint::ForeignKey)) {
                    slotDatas.name = "FK";
                } else if (col.isConstraint(datas::ColumnConstraint::PrimaryKey)) {
                    slotDatas.name = "PK";
                }
            }
            auto poCol = createChildSlot<FieldSlot>(ez::SlotDir::OUTPUT).lock();
            if (poCol != nullptr) {
                auto& slotDatas = poCol->getDatasRef<FieldSlot::FieldSlotDatas>();
                slotDatas.column = col;
                slotDatas.showName = true;
                slotDatas.name.clear();
                if (col.isConstraint(datas::ColumnConstraint::NotNull)) {
                    slotDatas.name = "NN";
                }
            }
        }
    }
    return ret;
}

bool TableNode::drawWidgets() {
    return false;
}

void TableNode::drawDebugInfos() {
    Parent::drawDebugInfos();
    ImGui::Indent();
    ImGui::Unindent();
}

ez::xml::Nodes TableNode::getXmlNodes(const std::string& vUserDatas) {
    ez::xml::Node xml;
    xml.addChilds(Parent::getXmlNodes(vUserDatas));
    auto& node = xml.getChildren().back();
    m_getXmlModule(node.addChild("module").addAttribute("type", getDatas<BaseNodeDatas>().type));
    return xml.getChildren();
}

// return true for continue xml parsing of childs in this node or false for interrupt the child exploration (if we want explore child ourselves)
bool TableNode::setFromXmlNodes(const ez::xml::Node& vNode, const ez::xml::Node& vParent, const std::string& vUserDatas) {
    const auto& strName = vNode.getName();
    if (strName == "node") {
        Parent::setFromXmlNodes(vNode, vParent, vUserDatas);
    } else if (strName == "slot") {
        Parent::setFromXmlNodes(vNode, vParent, vUserDatas);
    } else if (strName == "module") {
        if (vNode.getAttribute("type") == getDatas<BaseNodeDatas>().type) {
            m_setXmlModule(vNode, vParent);
        }
    }
    return true;
}

bool TableNode::m_drawHeader() {
    ImGui::BeginHorizontal("header");
    ImGui::Spring(1, 5.0f);
    const auto& datas = getDatas<BaseNodeDatas>();
    const bool pushed = ImGui::PushStyleColorWithContrast4(datas.color, ImGuiCol_Text, ImGui::CustomStyle::puContrastedTextColor, ImGui::CustomStyle::puContrastRatio);
    ImGui::TextUnformatted(getDatas<BaseNodeDatas>().name.c_str());
    if (pushed) {
        ImGui::PopStyleColor();
    }
    ImGui::Spring(1, 5.0f);
    ImGui::EndHorizontal();
    return false;
}

bool TableNode::m_drawHints() {
    return Parent::m_drawHints();
}

bool TableNode::m_drawContent() {
    bool change = false;
    ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(ImGui::GetStyle().ItemSpacing.x, 0));
    ImGui::BeginHorizontal("content");
    ImGui::BeginVertical("ins", ImVec2(0, 0), 0.0f);
    for (auto& slot : m_getInputSlotsRef()) {  // slots
        auto base_slot_ptr = std::static_pointer_cast<BaseSlot>(slot.lock());
        change |= base_slot_ptr->draw();
    }
    ImGui::EndVertical();
    ImGui::BeginVertical("uniques", ImVec2(0, 0), 0.0f);
    for (size_t i = 0; i < m_schema.columns.size(); ++i) {
        const auto& c = m_schema.columns.at(i);
        ImGui::BeginHorizontal(&c);
        ImGui::Text("%s", c.isConstraint(datas::ColumnConstraint::Unique) ? "UN" : "");
        ImGui::EndHorizontal();
    }
    ImGui::EndVertical();
    ImGui::BeginVertical("names", ImVec2(0, 0), 0.0f);
    for (size_t i = 0; i < m_schema.columns.size(); ++i) {
        const auto& c = m_schema.columns.at(i);
        ImGui::BeginHorizontal(&c);
        ImGui::Text("%s", c.name.c_str());
        ImGui::EndHorizontal();
    }
    ImGui::EndVertical();
    ImGui::BeginVertical("types", ImVec2(0, 0), 0.0f);
    for (size_t i = 0; i < m_schema.columns.size(); ++i) {
        const auto& c = m_schema.columns.at(i);
        ImGui::BeginHorizontal(&c);
        ImGui::Spring(1);
        ImGui::Text("%s", c.type.c_str());
        ImGui::EndHorizontal();
    }
    ImGui::EndVertical();                       
    ImGui::BeginVertical("outs", ImVec2(0, 0), 1.0f);
    for (auto& slot : m_getOutputSlotsRef()) {
        auto base_slot_ptr = std::static_pointer_cast<BaseSlot>(slot.lock());
        change |= base_slot_ptr->draw();
    }
    ImGui::EndVertical();
    ImGui::EndHorizontal();
    ImGui::PopStyleVar();
    return change;
}

void TableNode::m_getXmlModule(ez::xml::Node& vInOutNode) {}

void TableNode::m_setXmlModule(const ez::xml::Node& vNode, const ez::xml::Node& vParent) {}
