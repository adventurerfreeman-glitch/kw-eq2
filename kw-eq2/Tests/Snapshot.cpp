// Developer tool: renders the plugin editor to PNG files (run under xvfb on Linux).
#include <juce_audio_utils/juce_audio_utils.h>
#include "../Source/PluginProcessor.h"
#include "../Source/PluginEditor.h"
using namespace juce;

static void shot (KwProcessor& p, const char* name)
{
    std::unique_ptr<AudioProcessorEditor> ed (p.createEditor());
    ed->setBounds (0, 0, 1040, 690);
    auto img = ed->createComponentSnapshot (ed->getLocalBounds(), true, 1.0f);
    File f (File::getCurrentWorkingDirectory().getChildFile (name));
    f.deleteFile(); FileOutputStream os (f); PNGImageFormat().writeImageToStream (img, os);
}
static void set (KwProcessor& p, const char* id, float v)
{ auto* q = p.apvts.getParameter (id); q->setValueNotifyingHost (q->convertTo0to1 (v)); }

int main()
{
    ScopedJuceInitialiser_GUI init;
    KwProcessor p;
    p.prepareToPlay (48000, 512);
    shot (p, "snap_default.png");
    set (p, "tilt", 1.0f); set (p, "mids", 1.0f); set (p, "lean", 1); set (p, "contour", 0.4f); set (p, "punch", 0.8f); set (p, "blade", 0.7f); set (p, "pivot", 1); set (p, "width", 1); set (p, "cut", 0.4f); set (p, "push", 0.8f); set (p, "pushHz", 120);
    shot (p, "snap_all.png");
    set (p, "onLean", 0); set (p, "width", 0);
    shot (p, "snap_lean_off_focused.png");
    return 0;
}
