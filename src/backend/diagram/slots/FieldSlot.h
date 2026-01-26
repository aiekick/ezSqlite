#pragma once

#include <grapher/grapher.h>
 #include <headers/defs.h>

class FieldSlot : public BaseSlot {
    typedef BaseSlot Parent;

public:
    struct FieldSlotDatas : BaseSlotDatas {
        datas::ColumnDesc column;
        FieldSlotDatas() = default;
        FieldSlotDatas(
            const std::string& vName,
            const std::string& vType,
            const ez::SlotDir vSlotDir,
            const SlotColorBankInterface* vSlotColorBankPtr = nullptr,
            ez::UserDatas vUserDatas = nullptr,
            const size_t& vMaxConnectionCount = 1U)  // cant be changed after definition from the
            : BaseSlotDatas(vName, vType, vSlotDir, vSlotColorBankPtr, vUserDatas, vMaxConnectionCount) {}
    };

public:
    explicit FieldSlot(const BaseStyle& vParentStyle);
    bool init() override;

    bool draw() override;
    void drawHoveredSlotText();

protected:
    void m_drawInputWidget() override;
    void m_drawOutputWidget() override;
    size_t m_getMaxConnectionCount() const final;
};

typedef std::shared_ptr<FieldSlot> FieldSlotPtr;
typedef std::weak_ptr<FieldSlot> FieldSlotWeak;
