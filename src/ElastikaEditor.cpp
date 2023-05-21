#include "ElastikaProcessor.h"
#include "ElastikaEditor.h"
#include "ElastikaBinary.h"
#include "led_vu.h"
#include "sapphire_lnf.h"

ElastikaEditor::ElastikaEditor(ElastikaAudioProcessor &p)
    : juce::AudioProcessorEditor(&p), processor(p)
{
    // Make sure that before the constructor has finished, you've set the
    // editor's size to whatever you need it to be.
    auto knob_xml = juce::XmlDocument::parse(ElastikaBinary::knob_svg);
    auto marker_xml = juce::XmlDocument::parse(ElastikaBinary::knobmarker_svg);
    auto small_knob_xml = juce::XmlDocument::parse(ElastikaBinary::knobsmall_svg);
    auto small_marker_xml = juce::XmlDocument::parse(ElastikaBinary::knobmarkersmall_svg);
    lnf = std::make_unique<sapphire::LookAndFeel>(juce::Drawable::createFromSVG(*knob_xml),
                                                  juce::Drawable::createFromSVG(*marker_xml));
    small_lnf =
        std::make_unique<sapphire::LookAndFeel>(juce::Drawable::createFromSVG(*small_knob_xml),
                                                juce::Drawable::createFromSVG(*small_marker_xml));
    setLookAndFeel(lnf.get());

    setSize(300, 600);
    setResizable(true, true);

    std::unique_ptr<juce::XmlElement> xml = juce::XmlDocument::parse(ElastikaBinary::elastika_svg);
    background = juce::Drawable::createFromSVG(*xml);
    background->setInterceptsMouseClicks(false, true);
    addAndMakeVisible(*background);

    //  Find knobs.
    std::cout << "Top tag name: " << xml->getTagName() << std::endl;
    auto controls = xml->getChildByAttribute("id", "ControlLayer");
    if (!controls)
    {
        std::cout << "Big failure!" << std::endl;
        return;
    }
    for (const auto *control : controls->getChildWithTagNameIterator("circle"))
    {
        std::cout << "id: " << control->getStringAttribute("id")
                  << "; x: " << control->getStringAttribute("cx")
                  << "; y: " << control->getStringAttribute("cy") << std::endl;
        const juce::String &id = control->getStringAttribute("id");
        // Offsets from declared controls in the SVG. Determined through extensive trial and error.
        float dx = 0.5f; // was 1.05f when sliders were knobs by mistake
        float dy = 0.5f; // Seems to be based on the knob size (11; compare knob size 6)
        float cx = control->getStringAttribute("cx").getFloatValue();
        float cy = control->getStringAttribute("cy").getFloatValue();
        if (id.endsWith("knob") || id.endsWith("atten"))
        {
            auto kn = std::make_unique<juce::Slider>();
            kn->setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
            kn->setTextBoxStyle(juce::Slider::NoTextBox, true, 0, 0);
            kn->setPopupMenuEnabled(true);
            background->addAndMakeVisible(*kn);
            if (id.endsWith("knob"))
            {
                kn->setSize(11, 11);
                dx = 0.5f;
                dy = 0.5f;
            }
            else if (id.endsWith("atten"))
            {
                kn->setLookAndFeel(small_lnf.get());
                kn->setSize(6, 6);
                dx = 0.9166f;
                dy = 0.9166f;
            }
            elements.push_back(std::move(kn));
        }
        else if (id.endsWith("slider"))
        {
            auto sl = std::make_unique<juce::Slider>();
            sl->setSliderStyle(juce::Slider::LinearVertical);
            sl->setTextBoxStyle(juce::Slider::NoTextBox, true, 0, 0);
            background->addAndMakeVisible(*sl);
            sl->setSize(8, 28);
            sl->setMouseDragSensitivity(28);
            sl->setRange(0, 1);
            sl->setValue(0.5);
            sl->setDoubleClickReturnValue(true, 0.5);
            // TODO: Get the "snap to mouse position" to work with the scaling we have where we only
            // use 90% of the track (the remaining 10% is for the bottom part of the thumb; the
            // thumb's "position" is the very top pixel of the thumb). Until then, it doesn't work
            // right throughout the whole track, so we set this to false.
            sl->setSliderSnapsToMousePosition(false);
            dx = 0.6875f;
            dy = 0.6875f;
            elements.push_back(std::move(sl));
        }
        else if (id.endsWith("cv") || id.endsWith("input") || id.endsWith("output"))
        {
            auto led = std::make_unique<sapphire::LedVu>();
            background->addAndMakeVisible(*led);
            led->setSize(3, 3);
            dx = 0.5f;
            dy = 0.5f;
            elements.push_back(std::move(led));
        }
        else
        {
            continue;
        }
        juce::Point<float> real{cx + dx, cy + dy};
        juce::Point<int> rounded = real.toInt();
        elements.back()->setCentrePosition(rounded);
        elements.back()->setTransform(juce::AffineTransform::translation(
            real.getX() - rounded.getX(), real.getY() - rounded.getY()));
    }

#if 0
      auto addKnob = [this](const std::string label, auto &paramRef) {
        auto kn = std::make_unique<juce::Slider>();
        kn->setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
        kn->setTextBoxStyle(juce::Slider::NoTextBox, true, 0, 0);
        addAndMakeVisible(*kn);

        auto spa = std::make_unique<juce::SliderParameterAttachment>(paramRef, *kn);
        attachments.push_back(std::move(spa));
        knobs.push_back(std::move(kn));
        auto lb = std::make_unique<juce::Label>(label, label);
        lb->setColour(juce::Label::textColourId, juce::Colours::black);
        lb->setJustificationType(juce::Justification::centred);
        addAndMakeVisible(*lb);
        labels.push_back(std::move(lb));
    };
    addKnob("Friction", *(processor.friction));
    addKnob("Span", *(processor.span));
    addKnob("Curl", *(processor.curl));
    addKnob("Mass", *(processor.mass));
    addKnob("Drive", *(processor.drive));
    addKnob("Gain", *(processor.gain));
    addKnob("InTilt", *(processor.inputTilt));
    addKnob("OutTilt", *(processor.outputTilt));
#endif

    resized();
}

ElastikaEditor::~ElastikaEditor() {}

void ElastikaEditor::resized()
{
    if (background)
    {
        background->setTransformToFit(getLocalBounds().toFloat(), juce::RectanglePlacement());
    }
#if 0
    auto slsz = getWidth() / 4 * 0.8;
    auto slmg = getWidth() / 4 * 0.1;
    auto p = getLocalBounds().withTrimmedTop(80).withWidth(slsz).withHeight(slsz).translated(slmg, 0);

    for (int i=0; i<knobs.size(); ++i)
    {
        knobs[i]->setBounds(p);
        auto l = p.translated(0, slsz + 2).withHeight(20);
        labels[i]->setBounds(l);;

        p = p.translated(slsz + 2 * slmg, 0);


        if (i == 3)
            p = getLocalBounds().withTrimmedTop(80 + slsz + 30).withWidth(slsz).withHeight(slsz).translated(slmg, 0);
    }
#endif
}
