#include "FilterWindow.h"

// CORE CONCEPT: Constructor initializing TPT filter controls (cutoff, Q, drive, mode) and Filter ADSR envelope modulation depth.
FilterWindowComponent::FilterWindowComponent(AudioEngine &engine, int layerIndex)
    : audioEngine(engine), layerIdx(layerIndex) {

  titleLabel.setText("LAYER " + juce::String(layerIdx) + " FILTER SETTINGS", juce::dontSendNotification);
  titleLabel.setFont(juce::FontOptions(16.0f, juce::Font::bold));
  titleLabel.setColour(juce::Label::textColourId, juce::Colour(0xff2acc7a)); // Green accent
  addAndMakeVisible(titleLabel);

  addAndMakeVisible(filterGroup);

  addAndMakeVisible(typeLabel);
  addAndMakeVisible(typeComboBox);
  typeComboBox.addItem("Lowpass (LP)", 1);
  typeComboBox.addItem("Bandpass (BP)", 2);
  typeComboBox.addItem("Highpass (HP)", 3);
  typeComboBox.setSelectedId(1);

  auto &layerFilter = audioEngine.getSynth().getLayerFilter(layerIdx);

  typeComboBox.onChange = [this]() {
    int selected = typeComboBox.getSelectedId() - 1;
    audioEngine.getSynth().getLayerFilter(layerIdx).setFilterType(selected);
  };

  setupSlider(cutoffSlider, cutoffLabel, 20.0f, 20000.0f, layerFilter.getCutoffFrequency(), " Hz", true);
  cutoffSlider.onValueChange = [this]() {
    audioEngine.getSynth().getLayerFilter(layerIdx).setCutoffFrequency((float)cutoffSlider.getValue());
  };

  setupSlider(resonanceSlider, resonanceLabel, 0.1f, 20.0f, layerFilter.getResonance(), "");
  resonanceSlider.onValueChange = [this]() {
    audioEngine.getSynth().getLayerFilter(layerIdx).setResonance((float)resonanceSlider.getValue());
  };

  setupSlider(driveSlider, driveLabel, 0.0f, 1.0f, layerFilter.getDrive(), "");
  driveSlider.onValueChange = [this]() {
    audioEngine.getSynth().getLayerFilter(layerIdx).setDrive((float)driveSlider.getValue());
  };

  addAndMakeVisible(fenvGroup);
  setupSlider(attackSlider, attackLabel, 1.0f, 5000.0f, 10.0f, " ms");
  setupSlider(decaySlider, decayLabel, 1.0f, 5000.0f, 300.0f, " ms");
  setupSlider(sustainSlider, sustainLabel, 0.0f, 1.0f, 0.5f, "");
  setupSlider(releaseSlider, releaseLabel, 1.0f, 5000.0f, 400.0f, " ms");
  setupSlider(depthSlider, depthLabel, -1.0f, 1.0f, 0.0f, "");
}

void FilterWindowComponent::setupSlider(juce::Slider &slider, juce::Label &label, float minVal, float maxVal, float defaultVal, const juce::String &suffix, bool isLog) {
  slider.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
  slider.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 60, 18);
  if (isLog) {
    slider.setSkewFactorFromMidPoint(1000.0f);
  }
  slider.setRange(minVal, maxVal, isLog ? 1.0f : 0.01f);
  slider.setValue(defaultVal);
  slider.setTextValueSuffix(suffix);

  label.setJustificationType(juce::Justification::centred);
  label.setFont(juce::FontOptions(11.0f));

  addAndMakeVisible(slider);
  addAndMakeVisible(label);
}

void FilterWindowComponent::paint(juce::Graphics &g) {
  g.fillAll(juce::Colour(0xff21252b));
}

void FilterWindowComponent::resized() {
  auto bounds = getLocalBounds().reduced(12);

  titleLabel.setBounds(bounds.removeFromTop(24));
  bounds.removeFromTop(8);

  // Filter main group
  auto fBounds = bounds.removeFromTop(150);
  filterGroup.setBounds(fBounds);

  auto fInner = fBounds.reduced(10, 20);
  auto typeRow = fInner.removeFromTop(28);
  typeLabel.setBounds(typeRow.removeFromLeft(80));
  typeComboBox.setBounds(typeRow);

  fInner.removeFromTop(10);
  int colWidth = fInner.getWidth() / 3;

  auto c0 = fInner.removeFromLeft(colWidth);
  cutoffLabel.setBounds(c0.removeFromTop(14));
  cutoffSlider.setBounds(c0);

  auto c1 = fInner.removeFromLeft(colWidth);
  resonanceLabel.setBounds(c1.removeFromTop(14));
  resonanceSlider.setBounds(c1);

  auto c2 = fInner;
  driveLabel.setBounds(c2.removeFromTop(14));
  driveSlider.setBounds(c2);

  bounds.removeFromTop(12);

  // Filter envelope group
  auto feBounds = bounds.removeFromTop(140);
  fenvGroup.setBounds(feBounds);

  auto feInner = feBounds.reduced(10, 20);
  int feColWidth = feInner.getWidth() / 5;

  auto e0 = feInner.removeFromLeft(feColWidth);
  attackLabel.setBounds(e0.removeFromTop(14));
  attackSlider.setBounds(e0);

  auto e1 = feInner.removeFromLeft(feColWidth);
  decayLabel.setBounds(e1.removeFromTop(14));
  decaySlider.setBounds(e1);

  auto e2 = feInner.removeFromLeft(feColWidth);
  sustainLabel.setBounds(e2.removeFromTop(14));
  sustainSlider.setBounds(e2);

  auto e3 = feInner.removeFromLeft(feColWidth);
  releaseLabel.setBounds(e3.removeFromTop(14));
  releaseSlider.setBounds(e3);

  auto e4 = feInner;
  depthLabel.setBounds(e4.removeFromTop(14));
  depthSlider.setBounds(e4);
}

// ==============================================================================
// FilterWindow implementation
// ==============================================================================

FilterWindow::FilterWindow(AudioEngine &engine, int layerIndex)
    : DocumentWindow("Layer " + juce::String(layerIndex) + " Filter Settings",
                     juce::Colour(0xff21252b),
                     DocumentWindow::closeButton) {
  setContentOwned(new FilterWindowComponent(engine, layerIndex), true);
  setUsingNativeTitleBar(true);
  setResizable(false, false);
  centreWithSize(480, 360);
  setVisible(true);
}

void FilterWindow::closeButtonPressed() {
  delete this;
}
