#include "FieldSlot.h"
#include <backend/diagram/diagramManager.h>

FieldSlot::FieldSlot(const BaseStyle& vParentStyle)  //
    : Parent(vParentStyle, FieldSlotDatas("", "FIELD", ez::SlotDir::INPUT, &DiagramManager::ref())) {}

bool FieldSlot::init() {
    auto ret = Parent::init();
    auto& datas = getDatasRef<BaseSlotDatas>();
    datas.hoveredInfos = "";
    datas.color = ImGui::GetColorU32(ImVec4(0.9f, 0.9f, 0.9f, 1.0f));
    datas.showName = false;
    datas.showSlot = false;
    datas.showWidget = true;
    return ret;
}

bool FieldSlot::draw() {
    auto& datas = getDatasRef<BaseSlotDatas>();
    if (datas.visible) {
        if (isAnInput()) {
            ImGui::BeginHorizontal(this);
            nd::BeginPin(getPinID(), nd::PinKind::Input);
            if (datas.name.empty()) {
                ImGui::Dummy(ImGui::GetTextLineHeight());
            } else {
                ImGui::TextUnformatted(datas.name.c_str());
            }
            nd::EndPin();
            ImGui::Spring(1);
            ImGui::EndHorizontal();
        } else if (isAnOutput()) {
            nd::BeginPin(getPinID(), nd::PinKind::Output);
            ImGui::BeginHorizontal(this);
            ImGui::Spring(1);
            if (datas.name.empty()) {
                ImGui::Dummy(ImGui::GetTextLineHeight());
            } else {
                ImGui::TextUnformatted(datas.name.c_str());
            }
            ImGui::EndHorizontal();
            nd::EndPin();
        }
    }
    return false;
}

void FieldSlot::drawHoveredSlotText() {
    //m_drawHoveredSlotText(m_getPos(), false, 0, 0);
}

void FieldSlot::m_drawInputWidget() {
    const auto& datas = getDatasRef<FieldSlotDatas>();
    if (datas.dir == ez::SlotDir::INPUT) {
        ImGui::Text("%s", datas.column.name.c_str());
    }
}

void FieldSlot::m_drawOutputWidget() {
    const auto& datas = getDatasRef<FieldSlotDatas>();
    if (datas.dir == ez::SlotDir::OUTPUT) {
        ImGui::Text("%s", datas.column.type.c_str());
    }
}

size_t FieldSlot::m_getMaxConnectionCount() const {
    return 1000U;  // a flow not accept many connections
}
