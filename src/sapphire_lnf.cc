#include "sapphire_lnf.h"

using juce::degreesToRadians;

namespace sapphire
{

LookAndFeel::LookAndFeel(std::unique_ptr<juce::Drawable> knob,
                         std::unique_ptr<juce::Drawable> marker)
    : knob_(std::move(knob)), knob_marker_(std::move(marker)), rotary_scale_factor_(0)
{
}

void LookAndFeel::drawRotarySlider(juce::Graphics &g, int x, int y, int width, int height,
                                   float sliderPos, float rotaryStartAngle, float rotaryEndAngle,
                                   juce::Slider &slider)
{
    const int sf = static_cast<int>(
        std::ceil(juce::Component::getApproximateScaleFactorForComponent(&slider)));
    if (sf != rotary_scale_factor_)
    {
        // Must recreate and recache the SVG image.
        knob_cache_ =
            std::make_unique<juce::Image>(juce::Image::ARGB, width * sf, height * sf, true);
        juce::Graphics cg(*knob_cache_);
        knob_->drawWithin(cg, juce::Rectangle{0, 0, width * sf, height * sf}.toFloat(),
                          juce::RectanglePlacement(), 1.f);
        rotary_scale_factor_ = sf;
        std::cout << "Scale factor: " << sf << std::endl;
    }

    // g.drawImageAt(*knob_cache_, x, y);
    g.drawImage(*knob_cache_, x, y, width, height, 0, 0, width * sf, height * sf);
    // Maybe do this instead, and set to a buffered image at the component level.
#if 0
    knob_->drawWithin(g, juce::Rectangle{x, y, width, height}.toFloat(),
                      juce::RectanglePlacement(), 1.f);
#endif
}

} // namespace sapphire
