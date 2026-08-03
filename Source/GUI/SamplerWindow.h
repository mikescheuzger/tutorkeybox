#pragma once
#include "../Audio/AudioEngine.h"
#include "../Core/LibraryManager.h"
#include "VelocityCurveEditor.h"
#include <juce_gui_basics/juce_gui_basics.h>

// CORE CONCEPT: Floating editor component for inspecting sampler layer settings, dropping sample containers,
// selecting mic variants from the library, editing velocity curves, and tweaking ADSR amp envelopes.
class SamplerWindowComponent : public juce::Component, public juce::FileDragAndDropTarget {
public:
  SamplerWindowComponent(AudioEngine &engine, LibraryManager &library, int layerIndex);
  ~SamplerWindowComponent() override = default;

  void paint(juce::Graphics &g) override;
  void resized() override;

  bool isInterestedInFileDrag(const juce::StringArray &files) override;
  void filesDropped(const juce::StringArray &files, int x, int y) override;

  // CORE CONCEPT: Updates UI controls to match current layer state.
  void refreshFromEngine();

private:
  AudioEngine &audioEngine;
  LibraryManager &libraryManager;
  int layerIdx{0};

  juce::Label titleLabel;
  juce::Label micSelectLabel{"micLabel", "Mic Variant:"};
  juce::ComboBox micComboBox;

  juce::GroupComponent ampGroup{"ampGroup", "AMP ENVELOPE (ADSR)"};
  juce::Slider attackSlider;
  juce::Slider decaySlider;
  juce::Slider sustainSlider;
  juce::Slider releaseSlider;

  juce::Label attackLabel{"aLbl", "Att (ms)"};
  juce::Label decayLabel{"dLbl", "Dec (ms)"};
  juce::Label sustainLabel{"sLbl", "Sus"};
  juce::Label releaseLabel{"rLbl", "Rel (ms)"};

  juce::GroupComponent velGroup{"velGroup", "VELOCITY DYNAMICS"};
  VelocityCurveEditor velCurveEditor;

  juce::Label dropZoneLabel;

  void setupSlider(juce::Slider &slider, juce::Label &label, float minVal, float maxVal, float defaultVal, const juce::String &suffix);

  JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(SamplerWindowComponent)
};

// CORE CONCEPT: Floating window container for SamplerWindowComponent.
class SamplerWindow : public juce::DocumentWindow {
public:
  SamplerWindow(AudioEngine &engine, LibraryManager &library, int layerIndex);
  ~SamplerWindow() override = default;

  void closeButtonPressed() override;

private:
  JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(SamplerWindow)
};
