#include "queryResultComp.h"
#include "queryHistoryComp.h"
#include <frontend/panes/query/queryResultsTablePane.h>
#include <frontend/panes/query/queryResultsValuePane.h>

ImU32 QueryResultComp::sGetSqliteValueTypeColor(const std::string& vSqliteValueType) {
    datas::SqliteValueType type{datas::SqliteValueType ::Null};
    if (vSqliteValueType == "INTEGER") {
        type = datas::SqliteValueType::Integer;
    } else if (vSqliteValueType == "REAL") {
        type = datas::SqliteValueType::Real;
    } else if (vSqliteValueType == "BLOB") {
        type = datas::SqliteValueType::Blob;
    } else if (vSqliteValueType == "TEXT") {
        type = datas::SqliteValueType::Text;
    }
    return sGetSqliteValueTypeColor(type);
}

ImU32 QueryResultComp::sGetSqliteValueTypeColor(const datas::SqliteValueType vSqliteValueType) {
    switch (vSqliteValueType) {
        case datas::SqliteValueType::Integer: return IM_COL32(0, 100, 0, 100);  // INTEGER -> bright green
        case datas::SqliteValueType::Real: return IM_COL32(100, 0, 100, 100);   // REAL -> saturated purple
        case datas::SqliteValueType::Blob: return IM_COL32(100, 20, 40, 100);   // BLOB -> dark red/orange
        case datas::SqliteValueType::Null: return IM_COL32(100, 50, 50, 100);   // NULL -> dark pink (HotPink)
        case datas::SqliteValueType::Text:                                      // no color
        default: break;
    }
    return 0;
}

void QueryResultComp::sColorizeTableCell(const ImU32 vColor) {
    auto drawListPtr = ImGui::GetWindowDrawList();
    const auto& cursor = ImGui::GetCursorScreenPos();
    auto* tbl_ptr = GImGui->CurrentTable;
    const auto& table_column = tbl_ptr->Columns[tbl_ptr->CurrentColumn];
    const auto& column_height = ImGui::GetTextLineHeight();
    const auto& spacingX = IM_TRUNC(ImGui::GetStyle().ItemSpacing.x * 0.5f);
    const auto& spacingY = IM_TRUNC(ImGui::GetStyle().ItemSpacing.y * 0.5f);
    const ImVec2 pMin(table_column.MinX + spacingX - 1.0f, cursor.y - spacingY * 0.5f);
    const ImVec2 pMax(table_column.MaxX, cursor.y + column_height + spacingY);
    drawListPtr->AddRectFilled(pMin, pMax, vColor);
}

void QueryResultComp::setResult(const datas::QueryResult& vResult) {
    m_queryResult = vResult;
    LayoutManager::ref().FocusSpecificPane(QueryResultsTablePane::ref()->GetFlag());
}

bool QueryResultComp::drawTable() {
    if (m_queryResult.isValid()) {
        if (m_drawQueryResultTable(m_queryResult, m_selRow, m_selCol, m_cellValue)) {
            return true;
        }
    }
    return false;
}

void QueryResultComp::drawValue() {
    if (m_queryResult.isValid()) {
        if (!m_cellValue.empty()) {
            ImGui::Text(m_cellValue.c_str());
        }
    }
}

ez::xml::Nodes QueryResultComp::getXmlNodes(const std::string& vUserDatas) {
    ez::xml::Node node;
    return node.getChildren();
}

bool QueryResultComp::setFromXmlNodes(const ez::xml::Node& vNode, const ez::xml::Node& vParent, const std::string& vUserDatas) {
    //const auto& strName = vNode.getName();
    //const auto& strValue = vNode.getContent();
    //const auto& strParentName = vParent.getName();
    return false;  // stop here
}

bool QueryResultComp::m_drawQueryResultTable(const datas::QueryResult& vResult, int& ioSelRow, int& ioSelCol, std::string& vOutValue) {
    bool needResizeToFit{false};
    if (ImGui::BeginMenuBar()) {
        //if (ImGui::BeginMenu("Sizing")) {
            if (ImGui::MenuItem("Fit all columns")) {
                needResizeToFit = true;
            }
        //    ImGui::EndMenu();
        //}
        ImGui::EndMenuBar();
    }
    bool selectionChanged = false;
    const int colCount = static_cast<int>(vResult.columns.size());
    const int rowCount = static_cast<int>(vResult.rows.size());
    if (ImGui::BeginTable(                     //
            "##QueryResultTable",              //
            colCount,                          //
            ImGuiTableFlags_Borders            //
                | ImGuiTableFlags_RowBg        //
                | ImGuiTableFlags_ScrollX      //
                | ImGuiTableFlags_ScrollY      //
                | ImGuiTableFlags_Resizable    //
                | ImGuiTableFlags_Reorderable  //
                | ImGuiTableFlags_Hideable)) {
        ImGui::TableSetupScrollFreeze(0, 1);
        for (const auto& col : vResult.columns) {
            ImGui::TableSetupColumn(col.name.c_str(), ImGuiTableColumnFlags_WidthFixed);
        }
        ImGui::TableHeadersRow();
        m_textHeight = ImGui::GetTextLineHeight();
        m_queryResultTableClipper.Begin(rowCount, ImGui::GetTextLineHeightWithSpacing());
        while (m_queryResultTableClipper.Step()) {
            for (int r = m_queryResultTableClipper.DisplayStart; r < m_queryResultTableClipper.DisplayEnd; ++r) {
                if (r < 0) {
                    continue;
                }
                const auto& row = vResult.rows.at(r);
                ImGui::TableNextRow();
                for (int c = 0; c < colCount; ++c) {
                    datas::SqliteValueType columnType{datas::SqliteValueType::Text};
                    ImGui::TableSetColumnIndex(c);
                    const char* label = "";
                    static char buf[256];
                    buf[0] = '\0';
                    if (c < static_cast<int>(row.values.size())) {
                        const auto& cell = row.values.at(c);
                        std::visit(
                            [&](auto&& val) {
                                using T = std::decay_t<decltype(val)>;
                                if constexpr (std::is_same_v<T, int64_t>) {  // type INTEGER
                                    columnType = datas::SqliteValueType::Integer;
                                    snprintf(buf, sizeof(buf), "%lld", static_cast<long long>(val));
                                } else if constexpr (std::is_same_v<T, double>) {  // type REAL
                                    columnType = datas::SqliteValueType::Real;
                                    snprintf(buf, sizeof(buf), "%.6f", val);
                                } else if constexpr (std::is_same_v<T, std::string>) {  // type TEXT
                                    columnType = datas::SqliteValueType::Text;
                                    if (val.size() < sizeof(buf)) {
                                        memcpy(buf, val.c_str(), val.size() + 1);
                                    } else {
                                        snprintf(buf, sizeof(buf), "%.*s…", (int)sizeof(buf) - 2, val.c_str());
                                    }
                                } else if constexpr (std::is_same_v<T, std::vector<uint8_t>>) {  // type blob
                                    columnType = datas::SqliteValueType::Blob;
                                    snprintf(buf, sizeof(buf), "[BLOB] %zu bytes", val.size());
                                } else if constexpr (std::is_same_v<T, std::nullptr_t>) {  // type NULL
                                    columnType = datas::SqliteValueType::Null;
                                    snprintf(buf, sizeof(buf), "NULL");
                                }
                            },
                            cell);
                        label = buf[0] ? buf : "";
                    }
                    ImGui::PushID(r);
                    ImGui::PushID(c);
                    // if (columnType != SqliteValueType::TYPE_TEXT) {
                    sColorizeTableCell(sGetSqliteValueTypeColor(columnType));
                    //}
                    const bool isSelected = (ioSelRow == r && ioSelCol == c);
                    if (ImGui::Selectable(  //
                            label,
                            isSelected,
                            ImGuiSelectableFlags_AllowOverlap,
                            ImVec2(0, m_textHeight))) {
                        ioSelRow = r;
                        ioSelCol = c;
                        vOutValue = label;
                        selectionChanged = true;
                        LayoutManager::ref().FocusSpecificPane(QueryResultsValuePane::ref()->GetFlag());
                    }
                    ImGui::PopID();  // c
                    ImGui::PopID();  // r
                }
            }
        }
        // Resizing
        if (needResizeToFit) {
            ImGui::TableSetColumnWidthAutoAll(ImGui::GetCurrentContext()->CurrentTable);
        }
        ImGui::EndTable();
    }
    return selectionChanged;
}
