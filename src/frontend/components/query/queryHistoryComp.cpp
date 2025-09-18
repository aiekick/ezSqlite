#include "queryHistoryComp.h"
#include "queryEditorComp.h"

void QueryHistoryComp::drawHistory() {
    static ImGuiTreeNodeFlags tflags = ImGuiTreeNodeFlags_SpanFullWidth | ImGuiTreeNodeFlags_DefaultOpen;
    if (m_history.isValid()) {
        std::vector<datas::Query>::reverse_iterator it_to_erase = m_history.queries.rend();
        ImGui::PushID("history");
        if (ImGui::TreeNodeEx("##recentsQueries", tflags, "Recents")) {
            ImGui::Indent();
            for (auto it = m_history.queries.rbegin(); it != m_history.queries.rend(); ++it) {
                if (ImGui::SmallContrastedButton("X")) {
                    it_to_erase = it;
                }
                ImGui::SameLine();
                if (ImGui::Selectable(it->query.c_str(), false, ImGuiSelectableFlags_AllowDoubleClick, ImVec2(0, ImGui::GetTextLineHeight()))) {
                    if (ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left)) {
                        QueryEditorComp::ref().setCode(it->query);
                    }
                }
                if (ImGui::IsItemHovered()) {
                    ImGui::SetTooltip(it->query.c_str());
                }
            }
            ImGui::Unindent();
            ImGui::TreePop();
        }
        ImGui::PopID();
        if (it_to_erase != m_history.queries.rend()) {
            if (m_history.uniqueQuery.find(it_to_erase->query) != m_history.uniqueQuery.end()) {
                m_history.uniqueQuery.erase(it_to_erase->query);
            }
            m_history.queries.erase(std::next(it_to_erase).base());
        }
    }
}

void QueryHistoryComp::addQueryToHistory(const std::string& vQuery) {
    if (m_history.uniqueQuery.find(vQuery) == m_history.uniqueQuery.end()) {
        m_history.uniqueQuery.emplace(vQuery);
        m_history.queries.emplace_back(vQuery);
    }
}

ez::xml::Nodes QueryHistoryComp::getXmlNodes(const std::string& vUserDatas) {
    ez::xml::Node node;
    auto& nodeHistory = node.addChild("history");
    for (const auto& h : m_history.queries) {
        nodeHistory.addChild("query").setContent(ez::xml::Node::escapeXml(h.query));
    }
    return node.getChildren();
}

bool QueryHistoryComp::setFromXmlNodes(const ez::xml::Node& vNode, const ez::xml::Node& vParent, const std::string& vUserDatas) {
    const auto& strName = vNode.getName();
    const auto& strValue = vNode.getContent();
    const auto& strParentName = vParent.getName();
    if (strName == "history") {
        return true;  // go on childs
    }
    if (strName == "query" && strParentName == "history") {
        addQueryToHistory(strValue);
    }
    return false;  // stop here
}
