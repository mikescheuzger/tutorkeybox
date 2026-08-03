#include "VelocityCurveEditor.h"

// CORE CONCEPT: Constructor setting up default linear Bezier control points.
VelocityCurveEditor::VelocityCurveEditor() {}

void VelocityCurveEditor::resized() {}

// CORE CONCEPT: Evaluates cubic Bezier curve (p0, p1, p2, p3) at parameter t (0..1).
static float evalCubicBezier(float p0, float p1, float p2, float p3, float t) {
  float u = 1.0f - t;
  float tt = t * t;
  float uu = u * u;
  float uuu = uu * u;
  float ttt = tt * t;

  return uuu * p0 + 3.0f * uu * t * p1 + 3.0f * u * tt * p2 + ttt * p3;
}

// CORE CONCEPT: Evaluates mapped output velocity for a given input velocity (0.0 to 1.0) along the Bezier curve.
float VelocityCurveEditor::getMappedVelocity(float inputVel) const {
  float x = juce::jlimit(0.0f, 1.0f, inputVel);
  // Binary search for t parameter corresponding to x coordinate
  float tLow = 0.0f, tHigh = 1.0f;
  for (int iter = 0; iter < 8; ++iter) {
    float tMid = (tLow + tHigh) * 0.5f;
    float xVal = evalCubicBezier(controlPoints[0], controlPoints[2], controlPoints[4], controlPoints[6], tMid);
    if (xVal < x)
      tLow = tMid;
    else
      tHigh = tMid;
  }
  float tFinal = (tLow + tHigh) * 0.5f;
  float yVal = evalCubicBezier(controlPoints[1], controlPoints[3], controlPoints[5], controlPoints[7], tFinal);
  return juce::jlimit(0.0f, 1.0f, yVal);
}

// CORE CONCEPT: Sets 4-point control array and repaints widget.
void VelocityCurveEditor::setControlPoints(const std::array<float, 8> &points) {
  controlPoints = points;
  repaint();
}

// CORE CONCEPT: Renders dark mode background grid, accent Bezier curve, and interactive handle nodes.
void VelocityCurveEditor::paint(juce::Graphics &g) {
  auto bounds = getLocalBounds().toFloat().reduced(4.0f);

  // Background
  g.setColour(juce::Colour(0xff181a1f));
  g.fillRoundedRectangle(bounds, 6.0f);
  g.setColour(juce::Colour(0xff2d3139));
  g.drawRoundedRectangle(bounds, 6.0f, 1.5f);

  // Grid lines
  g.setColour(juce::Colour(0x22ffffff));
  for (int i = 1; i < 4; ++i) {
    float x = bounds.getX() + bounds.getWidth() * (i / 4.0f);
    float y = bounds.getY() + bounds.getHeight() * (i / 4.0f);
    g.drawVerticalLine(juce::roundToInt(x), bounds.getY(), bounds.getBottom());
    g.drawHorizontalLine(juce::roundToInt(y), bounds.getX(), bounds.getRight());
  }

  // Draw Bezier Curve Path
  juce::Path curvePath;
  float startX = bounds.getX() + controlPoints[0] * bounds.getWidth();
  float startY = bounds.getBottom() - controlPoints[1] * bounds.getHeight();
  curvePath.startNewSubPath(startX, startY);

  for (int step = 1; step <= 50; ++step) {
    float t = step / 50.0f;
    float cx = evalCubicBezier(controlPoints[0], controlPoints[2], controlPoints[4], controlPoints[6], t);
    float cy = evalCubicBezier(controlPoints[1], controlPoints[3], controlPoints[5], controlPoints[7], t);
    float px = bounds.getX() + cx * bounds.getWidth();
    float py = bounds.getBottom() - cy * bounds.getHeight();
    curvePath.lineTo(px, py);
  }

  g.setColour(juce::Colour(0xff4a90e2)); // Cyan/blue accent
  g.strokePath(curvePath, juce::PathStrokeType(2.5f));

  // Draw interior control handles (P1 and P2)
  for (int p = 1; p <= 2; ++p) {
    float px = bounds.getX() + controlPoints[p * 2] * bounds.getWidth();
    float py = bounds.getBottom() - controlPoints[p * 2 + 1] * bounds.getHeight();

    g.setColour(juce::Colour(0xffff9500)); // Orange control handle
    g.fillEllipse(px - 4.0f, py - 4.0f, 8.0f, 8.0f);
    g.setColour(juce::Colours::white);
    g.drawEllipse(px - 4.0f, py - 4.0f, 8.0f, 8.0f, 1.0f);
  }
}

void VelocityCurveEditor::mouseDown(const juce::MouseEvent &event) {
  auto bounds = getLocalBounds().toFloat().reduced(4.0f);
  juce::Point<float> mousePos = event.position;

  draggingPointIndex = -1;
  float minDist = 15.0f;

  for (int p = 1; p <= 2; ++p) {
    float px = bounds.getX() + controlPoints[p * 2] * bounds.getWidth();
    float py = bounds.getBottom() - controlPoints[p * 2 + 1] * bounds.getHeight();
    float dist = mousePos.getDistanceFrom({px, py});
    if (dist < minDist) {
      minDist = dist;
      draggingPointIndex = p;
    }
  }
}

void VelocityCurveEditor::mouseDrag(const juce::MouseEvent &event) {
  if (draggingPointIndex < 1 || draggingPointIndex > 2)
    return;

  auto bounds = getLocalBounds().toFloat().reduced(4.0f);
  float normX = juce::jlimit(0.0f, 1.0f, (event.position.x - bounds.getX()) / bounds.getWidth());
  float normY = juce::jlimit(0.0f, 1.0f, (bounds.getBottom() - event.position.y) / bounds.getHeight());

  controlPoints[draggingPointIndex * 2]     = normX;
  controlPoints[draggingPointIndex * 2 + 1] = normY;

  repaint();
  if (onCurveChanged != nullptr)
    onCurveChanged();
}
