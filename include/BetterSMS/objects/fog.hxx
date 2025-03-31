#include <Dolphin/GX.h>
#include <Dolphin/types.h>

#include <JSystem/JAudio/JAISound.hxx>
#include <JSystem/JDrama/JDRActor.hxx>
#include <JSystem/JUtility/JUTColor.hxx>

#include "module.hxx"

class TSimpleFog : public JDrama::TActor {
public:
    BETTER_SMS_FOR_CALLBACK static JDrama::TNameRef *instantiate() {
        return new TSimpleFog("TSimpleFog");
    }

    TSimpleFog(const char *name)
        : TActor(name), mType(1), mColor(), mStartZ(0.0f), mEndZ(20000.0f), mNearZ(10.0f),
          mFarZ(300000.0f) {
        mColor = {255, 255, 255, 255};
    }
    virtual ~TSimpleFog() override {}

    void load(JSUMemoryInputStream &in) override;
    void perform(u32 flags, JDrama::TGraphics *) override;

private:
    u32 mType;
    GXColor mColor;
    f32 mStartZ;
    f32 mEndZ;
    f32 mNearZ;
    f32 mFarZ;
};
