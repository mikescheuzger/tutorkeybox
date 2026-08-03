#include "ReverbWindow.h"

// CORE CONCEPT: Constructor initializing IR sample dropdown, mix, pre-delay, and damping sliders.
ReverbWindowComponent::ReverbWindowComponent(AudioEngine &engine)
    : audioEngine(engine) {

  titleLabel.setText("CONVOLUTION REVERB SETTINGS", juce::dontSendNotification);
  titleLabel.setFont(juce::FontOptions(16.0f, juce::Font::bold));
  titleLabel.setColour(juce::Label::textColourId, juce::Colour(0xffff9500)); // Orange accent
  addAndMakeVisible(titleLabel);

  addAndMakeVisible(irLabel);
  addAndMakeVisible(irComboBox);

  populateIrDropdown();

  addAndMakeVisible(paramsGroup);
  auto &rev = audioEngine.getReverbEngine();

  setupSlider(mixSlider, mixLabel, 0.0f, 1.0f, rev.getMix(), "");
  mixSlider.onValueChange = [this]() {
    audioEngine.getReverbEngine().setMix((float)mixSlider.getValue());
  };

  setupSlider(preDelaySlider, preDelayLabel, 0.0f, 100.0f, rev.getPreDelayMs(), " ms");
  preDelaySlider.onValueChange = [this]() {
    audioEngine.getReverbEngine().setPreDelayMs((float)preDelaySlider.getValue());
  };

  setupSlider(dampingSlider, dampingLabel, 500.0f, 20000.0f, rev.getDampingHz(), " Hz");
  dampingSlider.onValueChange = [this]() {
    audioEngine.getReverbEngine().setDampingHz((float)dampingSlider.getValue());
  };
}

void ReverbWindowComponent::populateIrDropdown() {
  irComboBox.addItem("-- Select Impulse Response --", 1);

  juce::File irFolder = juce::File::getCurrentWorkingDirectory().getChildFile("Upright Samples_unbearbeitet/IR Samples");
  if (!irFolder.exists()) {
    irFolder = juce::File::getSpecialLocation(juce::File::userDocumentsDirectory).getChildFile("TKBLibrary/IR Samples");
  }

  juce::Array<juce::File> irFiles;
  if (irFolder.exists()) {
    irFiles = irFolder.findChildFiles(juce::File::findFiles, false, "*.wav");
  }

  for (int i = 0; i < irFiles.size(); ++i) {
    irComboBox.addItem(irFiles[i].getFileNameWithoutExtension(), i + 2);
  }

  irComboBox.onChange = [this, irFiles]() {
    int selectedId = irComboBox.getSelectedId();
    if (selectedId >= 2) {
      int idx = selectedId - 2;
      if (idx < irFiles.size()) {
        audioEngine.getReverbEngine().loadImpulseResponse(irFiles[idx]);
      }
    }
  };
}

void ReverbWindowComponent::setupSlider(juce::Slider &slider, juce::Label &label, float minVal, float maxVal, float defaultVal, const juce::String &suffix) {
  slider.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
  slider.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 60, 18);
  slider.setRange(minVal, maxVal, 0.01f);
  slider.setValue(defaultVal);
  slider.setTextValueSuffix(suffix);

  label.setJustificationType(juce::Justification::centred);
  label.setFont(juce::FontOptions(11.0f));

  addAndMakeVisible(slider);
  addAndMakeVisible(label);
}

void ReverbWindowComponent::paint(juce::Graphics &g) {
  g.fillAll(juce::Colour(0xff21252b));
}

void ReverbWindowComponent::resized() {
  auto bounds = getLocalBounds().reduced(12);

  titleLabel.setBounds(bounds.removeFromTop(24));
  bounds.removeFromTop(8);

  auto irRow = bounds.removeFromTop(28);
  irLabel.setBounds(irRow.removeFromLeft(120));
  irComboBox.setBounds(irRow);

  bounds.removeFromTop(16);

  auto pBounds = bounds.removeFromTop(150);
  paramsGroup.setBounds(pBounds);

  auto sliderArea = pBounds.reduced(10, 24);
  int colWidth = sliderArea.getWidth() / 3;

  auto c0 = sliderArea.removeFromLeft(colWidth);
  mixLabel.setBounds(c0.removeFromTop(14));
  mixSlider.setBounds(c0);

  auto c1 = sliderArea.removeFromLeft(colWidth);
  preDelayLabel.setBounds(c1.removeFromTop(14));
  preDelaySlider.setBounds(c1);

  auto c2 = sliderArea;
  dampingLabel.setBounds(c2.removeFromTop(14));
  dampingSlider.setBounds(c2);
}

// ==============================================================================
// ReverbWindow implementation
// ==============================================================================

ReverbWindow::ReverbWindow(AudioEngine &engine)
    : DocumentWindow("Convolution Reverb",
                     juce::Colour(0xff21252b),
                     DocumentWindow::closeButton) {
  setContentOwned(new ReverbWindowComponent(engine), true);
  setUsingNativeTitleBar(true);
  setResizable(false, false);
  centreWithSize(420, 260);
  setVisible(true);
}

void ReverbWindow::closeButtonPressed() {
  delete this;
}
