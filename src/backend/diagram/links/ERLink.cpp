#include "ERLink.h"
#include <grapher/baseSlot.h>
#include <cmath>
#include <algorithm>

ERLinkPtr ERLink::create(const BaseStyle& vParentStyle, const BaseSlotWeak& vStart, const BaseSlotWeak& vEnd) {
    auto link_ptr = std::make_shared<ERLink>(vParentStyle);
    if (!link_ptr->init(vStart, vEnd)) {
        link_ptr.reset();
    }
    return link_ptr;
}

ERLink::ERLink(const BaseStyle& vParentStyle) : Parent(vParentStyle) {}

bool ERLink::draw() {
    // Get the slots using public interface
    const auto& inSlot = getInSlot();
    const auto& outSlot = getOutSlot();

    auto inPtr = inSlot.lock();
    auto outPtr = outSlot.lock();

    if (inPtr == nullptr || outPtr == nullptr) {
        return false;
    }

    // Get the slot data to access color and other properties
    const auto& inDatas = inPtr->getDatas<BaseSlot::BaseSlotDatas>();
    const auto& outDatas = outPtr->getDatas<BaseSlot::BaseSlotDatas>();

    ImU32 linkColor = inDatas.color;
    float linkThick = 2.0f;

    // Draw the orthogonal link
    m_drawOrthogonalLink(inSlot, outSlot, linkColor, linkThick);

    // Update hover state for interaction
    m_updateHoverState();

    return true;
}

void ERLink::calculateOrthogonalPath(const ImVec2& vStart, const ImVec2& vEnd) {
    m_pathPoints.clear();

    // Simple 3-segment orthogonal path: horizontal -> vertical -> horizontal
    // or vertical -> horizontal -> vertical depending on relative positions

    float dx = vEnd.x - vStart.x;
    float dy = vEnd.y - vStart.y;

    m_pathPoints.push_back(vStart);

    // Choose routing strategy based on relative position
    if (std::abs(dx) > std::abs(dy)) {
        // Horizontal routing preferred
        float midX = vStart.x + dx * 0.5f;
        m_pathPoints.push_back(ImVec2(midX, vStart.y));
        m_pathPoints.push_back(ImVec2(midX, vEnd.y));
    } else {
        // Vertical routing preferred
        float midY = vStart.y + dy * 0.5f;
        m_pathPoints.push_back(ImVec2(vStart.x, midY));
        m_pathPoints.push_back(ImVec2(vEnd.x, midY));
    }

    m_pathPoints.push_back(vEnd);
}

void ERLink::m_drawOrthogonalLink(const BaseSlotWeak& vIn, const BaseSlotWeak& vOut, ImU32 vColor, float vThick) {
    auto inPtr = vIn.lock();
    auto outPtr = vOut.lock();

    if (inPtr == nullptr || outPtr == nullptr) {
        return;
    }

    // Get pin IDs for the connection
    auto startPinId = inPtr->getPinID();
    auto endPinId = outPtr->getPinID();

    // In imgui-node-editor, we need to get the pin positions
    // The node editor tracks pin positions internally during BeginPin/EndPin
    // We'll calculate positions based on the pins after they're drawn

    // For now, use a simpler approach: let imgui-node-editor position the pins
    // and we'll draw on top using ImDrawList

    // First, we still need to tell the node editor about the link so it tracks it
    // But we won't use its rendering
    nd::Link(getUuid(), startPinId, endPinId, IM_COL32(0, 0, 0, 0), 0.0f);  // Invisible link

    // Now get actual pin positions using node editor functions
    // Note: Pins must be drawn in the current frame for this to work
    // We'll get positions from the canvas

    // Get draw list for custom rendering
    ImDrawList* drawList = nd::GetHintBackgroundDrawList();
    if (drawList == nullptr) {
        drawList = ImGui::GetWindowDrawList();
    }

    if (drawList == nullptr) {
        return;
    }

    // Get pin screen positions
    // The slots store their positions which we can use
    ImVec2 startPos = nd::CanvasToScreen(inPtr->m_getPos());
    ImVec2 endPos = nd::CanvasToScreen(outPtr->m_getPos());

    // Calculate orthogonal path
    calculateOrthogonalPath(startPos, endPos);

    if (m_pathPoints.size() < 2) {
        return;
    }

    // Determine color based on state
    ImU32 linkColor = vColor;
    float thickness = vThick;

    if (m_isSelected) {
        linkColor = IM_COL32(255, 176, 50, 255);  // Orange for selection
        thickness = vThick + 1.0f;
    } else if (m_isHovered) {
        linkColor = IM_COL32(50, 176, 255, 255);  // Blue for hover
        thickness = vThick + 0.5f;
    }

    // Draw the polyline segments
    for (size_t i = 0; i < m_pathPoints.size() - 1; ++i) {
        drawList->AddLine(
            m_pathPoints[i],
            m_pathPoints[i + 1],
            linkColor,
            thickness
        );
    }
}

void ERLink::m_updateHoverState() {
    ImVec2 mousePos = ImGui::GetMousePos();
    m_isHovered = isPointNearLink(mousePos, m_hoverDistance);
}

bool ERLink::isPointNearLink(const ImVec2& vPoint, float vThreshold) const {
    if (m_pathPoints.size() < 2) {
        return false;
    }

    // Check distance to each segment
    for (size_t i = 0; i < m_pathPoints.size() - 1; ++i) {
        float dist = pointToSegmentDistance(vPoint, m_pathPoints[i], m_pathPoints[i + 1]);
        if (dist <= vThreshold) {
            return true;
        }
    }

    return false;
}

float ERLink::pointToSegmentDistance(const ImVec2& vPoint, const ImVec2& vSegStart, const ImVec2& vSegEnd) {
    // Vector from segment start to point
    float px = vPoint.x - vSegStart.x;
    float py = vPoint.y - vSegStart.y;

    // Vector from segment start to segment end
    float sx = vSegEnd.x - vSegStart.x;
    float sy = vSegEnd.y - vSegStart.y;

    // Squared length of segment
    float segLengthSq = sx * sx + sy * sy;

    if (segLengthSq < 0.0001f) {
        // Segment is essentially a point
        return std::sqrt(px * px + py * py);
    }

    // Project point onto line, clamped to segment
    float t = std::max(0.0f, std::min(1.0f, (px * sx + py * sy) / segLengthSq));

    // Closest point on segment
    float closestX = vSegStart.x + t * sx;
    float closestY = vSegStart.y + t * sy;

    // Distance from point to closest point
    float dx = vPoint.x - closestX;
    float dy = vPoint.y - closestY;

    return std::sqrt(dx * dx + dy * dy);
}
