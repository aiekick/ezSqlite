#pragma once

#include <grapher/grapher.h>
#include <vector>

class ERLink;
typedef std::shared_ptr<ERLink> ERLinkPtr;
typedef std::weak_ptr<ERLink> ERLinkWeak;

// ER-style link with orthogonal routing (right-angle segments)
class ERLink : public BaseLink {
    typedef BaseLink Parent;

private:
    std::vector<ImVec2> m_pathPoints;  // Orthogonal path points
    bool m_isHovered = false;
    bool m_isSelected = false;
    float m_hoverDistance = 5.0f;  // Distance threshold for hover detection

public:
    static ERLinkPtr create(const BaseStyle& vParentStyle, const BaseSlotWeak& vStart, const BaseSlotWeak& vEnd);

    explicit ERLink(const BaseStyle& vParentStyle);
    bool draw() override;

    // Calculate orthogonal path between two points
    void calculateOrthogonalPath(const ImVec2& vStart, const ImVec2& vEnd);

    // Check if a point is near the link (for hover/selection)
    bool isPointNearLink(const ImVec2& vPoint, float vThreshold = 5.0f) const;

    // Distance from point to line segment
    static float pointToSegmentDistance(const ImVec2& vPoint, const ImVec2& vSegStart, const ImVec2& vSegEnd);

private:
    void m_drawOrthogonalLink(const BaseSlotWeak& vIn, const BaseSlotWeak& vOut, ImU32 vColor, float vThick);
    void m_updateHoverState();
};
