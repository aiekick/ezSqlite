#include "databaseSchemaComp.h"

#include <frontend/components/query/queryEditorComp.h>
#include <frontend/components/query/queryResultComp.h>

#include <backend/managers/databaseManager.h>
#include <backend/diagram/diagramManager.h>

#include <frontend/panes/query/queryResultsTablePane.h>

void DatabaseSchemaComp::doActions() {
    m_actions.runImmediateActions();
}

void DatabaseSchemaComp::drawSchema() {
    static auto labelYES{"Yes"};
    static auto labelNO{"No"};
    static auto labelNull{"Null"};
    static ImGuiTableFlags tf =        //
        ImGuiTableFlags_Borders        //
        | ImGuiTableFlags_RowBg        //
        | ImGuiTableFlags_ScrollX      //
        | ImGuiTableFlags_ScrollY      //
        | ImGuiTableFlags_Reorderable  //
        | ImGuiTableFlags_Hideable;
    static ImGuiTreeNodeFlags leaf = ImGuiTreeNodeFlags_Leaf | ImGuiTreeNodeFlags_NoTreePushOnOpen;
    static ImGuiTreeNodeFlags tflags = ImGuiTreeNodeFlags_OpenOnArrow;
    std::string query_to_execute;
    if (ImGui::BeginTable("DBTreeTable", 8, tf)) {
        ImGui::TableSetupScrollFreeze(0, 1);
        ImGui::TableSetupColumn("Name", ImGuiTableColumnFlags_WidthFixed);
        ImGui::TableSetupColumn("Type", ImGuiTableColumnFlags_WidthFixed);
        ImGui::TableSetupColumn("AI", ImGuiTableColumnFlags_WidthFixed);
        ImGui::TableSetupColumn("NN", ImGuiTableColumnFlags_WidthFixed);
        ImGui::TableSetupColumn("UN", ImGuiTableColumnFlags_WidthFixed);
        ImGui::TableSetupColumn("PK", ImGuiTableColumnFlags_WidthFixed);
        ImGui::TableSetupColumn("FK", ImGuiTableColumnFlags_WidthFixed);
        ImGui::TableSetupColumn("Def", ImGuiTableColumnFlags_WidthFixed);
        ImGui::TableHeadersRow();
        const auto& dbs = DatabaseManager::ref().getAnalyzedDatabases();
        if (dbs.isValid()) {
            for (const auto& database : dbs.databases) {
                ImGui::TableNextRow();
                ImGui::TableSetColumnIndex(0);
                ImGui::PushID(database.name.c_str());
                if (ImGui::TreeNodeEx("##database", tflags | ImGuiTreeNodeFlags_DefaultOpen, "Database : %s (%zu)", database.name.c_str(), database.tables.size())) {
                    if (query_to_execute.empty() && ImGui::IsItemHovered() && ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left)) {
                        DiagramManager::ref().loadDatabase(database);
                    }
                    ImGui::Indent();
                    for (const auto& kv : database.tables) {
                        ImGui::TableNextRow();
                        ImGui::TableSetColumnIndex(0);
                        ImGui::PushID(kv.name.c_str());
                        bool tableOpened = ImGui::TreeNodeEx("##table", tflags);
                        ImGui::SameLine();
                        ImGui::Selectable(ez::str::toStr("%s (%zu)", kv.name.c_str(), kv.columns.size()).c_str(), false);
                        if (query_to_execute.empty() && ImGui::IsItemHovered() && ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left)) {
                            query_to_execute = "SELECT * FROM " + kv.name + ";";
                        }
                        if (ImGui::BeginPopupContextItem(               //
                                NULL,                                   //
                                ImGuiPopupFlags_NoOpenOverItems |       //
                                    ImGuiPopupFlags_MouseButtonRight |  //
                                    ImGuiPopupFlags_NoOpenOverExistingPopup)) {
                            m_drawTableContextMenu(kv);
                            ImGui::EndPopup();
                        }
                        if (tableOpened) {
                            ImGui::Indent();
                            for (size_t i = 0; i < kv.columns.size(); ++i) {
                                const auto& c = kv.columns.at(i);
                                ImGui::TableNextRow();
                                ImGui::TableSetColumnIndex(0);
                                ImGui::TreeNodeEx((void*)(intptr_t)i, leaf, "%s", c.name.c_str());
                                ImGui::TableSetColumnIndex(1);
                                QueryResultComp::sColorizeTableCell(c.colType);
                                ImGui::TextUnformatted(c.type.c_str());
                                ImGui::TableSetColumnIndex(2);
                                const bool isAutoInc = c.isConstraint(datas::ColumnConstraint::AutoIncrement);
                                ImGui::PushStyleColor(ImGuiCol_Text, isAutoInc ? ImGui::CustomStyle::GoodColor : ImGui::CustomStyle::BadColor);
                                ImGui::TextUnformatted(isAutoInc ? labelYES : labelNO);
                                ImGui::PopStyleColor();
                                ImGui::TableSetColumnIndex(3);
                                const bool isNotNull = c.isConstraint(datas::ColumnConstraint::NotNull);
                                ImGui::PushStyleColor(ImGuiCol_Text, isNotNull ? ImGui::CustomStyle::GoodColor : ImGui::CustomStyle::BadColor);
                                ImGui::TextUnformatted(isNotNull ? labelYES : labelNO);
                                ImGui::PopStyleColor();
                                ImGui::TableSetColumnIndex(4);
                                const bool isUnique = c.isConstraint(datas::ColumnConstraint::Unique);
                                ImGui::PushStyleColor(ImGuiCol_Text, isUnique ? ImGui::CustomStyle::GoodColor : ImGui::CustomStyle::BadColor);
                                ImGui::TextUnformatted(isUnique ? labelYES : labelNO);
                                ImGui::PopStyleColor();
                                ImGui::TableSetColumnIndex(5);
                                const bool isPrimaryKey = c.isConstraint(datas::ColumnConstraint::PrimaryKey);
                                ImGui::PushStyleColor(ImGuiCol_Text, isPrimaryKey ? ImGui::CustomStyle::GoodColor : ImGui::CustomStyle::BadColor);
                                ImGui::TextUnformatted(isPrimaryKey ? labelYES : labelNO);
                                ImGui::PopStyleColor();
                                ImGui::TableSetColumnIndex(6);
                                const bool isForeignKey = c.isConstraint(datas::ColumnConstraint::ForeignKey);
                                ImGui::PushStyleColor(ImGuiCol_Text, isForeignKey ? ImGui::CustomStyle::GoodColor : ImGui::CustomStyle::BadColor);
                                ImGui::TextUnformatted(isForeignKey ? ez::str::toStr("%i", static_cast<int32_t>(c.fks.size())).c_str() : labelNO);
                                ImGui::PopStyleColor();
                                ImGui::TableSetColumnIndex(7);
                                if (!c.defaultValue.empty()) {
                                    ImGui::TextUnformatted(c.defaultValue.c_str());
                                } else {
                                    ImGui::TextDisabled(labelNull);
                                }
                            }
                            ImGui::Unindent();
                            ImGui::TreePop();
                        }
                        ImGui::PopID();
                    }
                    ImGui::Unindent();
                    ImGui::TreePop();
                }
                ImGui::PopID();
            }
        }
        ImGui::EndTable();
    }
    if (!query_to_execute.empty()) {
        if (DatabaseManager::ref().executeQuery(query_to_execute, false, true)) {
            QueryResultComp::ref().setResult(DatabaseManager::ref().getLastQueryResult());
        }
    }
}

ez::xml::Nodes DatabaseSchemaComp::getXmlNodes(const std::string& vUserDatas) {
    ez::xml::Node node;
    return node.getChildren();
}

bool DatabaseSchemaComp::setFromXmlNodes(const ez::xml::Node& vNode, const ez::xml::Node& vParent, const std::string& vUserDatas) {
    //const auto& strName = vNode.getName();
    //const auto& strValue = vNode.getContent();
    //const auto& strParentName = vParent.getName();
    return false;  // stop here
}

void DatabaseSchemaComp::m_drawTableContextMenu(const datas::TableDesc& vTableDatas) {
    if (ImGui::MenuItem("Show SELECT statement")) {
        QueryEditorComp::ref().setCode("SELECT * FROM " + vTableDatas.name + ";");
    }
    if (ImGui::MenuItem("Show CREATE statement")) {
        m_actions.pushBackImmediateAction([this, &vTableDatas]() {
            if (DatabaseManager::ref().executeQuery("SELECT sql FROM sqlite_schema WHERE name = '" + vTableDatas.name + "';", false,false)) {
                const auto& result = DatabaseManager::ref().getLastQueryResult();
                if (result.isValid()) {
                    QueryEditorComp::ref().setCode(  //
                        std::get<std::string>(       //
                            result.rows.front().values.front()));
                }
            }
        });
    }
    ImGui::Separator();
    if (ImGui::MenuItem("Show DROP TABLE statement")) {
        QueryEditorComp::ref().setCode("DROP TABLE " + vTableDatas.name + ";");
    }
}
