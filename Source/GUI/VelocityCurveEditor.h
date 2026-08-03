#pragma once
#include <juce_gui_basics/juce_gui_basics.h>
#include <array>

// CORE CONCEPT: Interactive 4-point cubic Bezier curve editor widget for customizing velocity response dynamics.
class VelocityCurveEditor : public juce::Component {
public:
  VelocityCurveEditor();
  ~VelocityCurveEditor() override = default;

  // CORE CONCEPT: Renders dark mode background grid and cubic Bezier velocity curve.
  void paint(juce::Graphics &g) override;

  // CORE CONCEPT: Handles layout bounds for control handles.
  void resized() override;

  // CORE CONCEPT: Evaluates mapped velocity output (0.0 to 1.0) for a given raw velocity input (0.0 to 1.0).
  float getMappedVelocity(float inputVel) const;

  // CORE CONCEPT: Returns active 4-point Bezier control coordinates [x0, y0, x1, y1, x2, y2, x3, y3].
  std::array<float, 8> getControlPoints() const { return controlPoints; }

  // CORE CONCEPT: Sets 4-point Bezier control coordinates and repaints widget.
  void setControlPoints(const std::array<float, 8> &points);

  // Callback when curve control points are modified by user
  std::function<void()> onCurveChanged;

private:
  void mouseDown(const juce::MouseEvent &event) override;
  void mouseDrag(const juce::MouseEvent &event) override;

  int draggingPointIndex{-1};
  std::array<float, 8> controlPoints{0.0f, 0.0f, 0.33f, 0.33f, 0.66f, 0.66f, 1.0f, 1.0f};

  JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(VelocityCurveEditor)
};
