#include "GUI/MainComponent.h"
#include <juce_gui_basics/juce_gui_basics.h>

// ==============================================================================
/**
 * The main desktop application wrapper managing the app lifecycle and window.
 */
class TutorKeyboxApplication : public juce::JUCEApplication {
public:
  TutorKeyboxApplication() {}

  const juce::String getApplicationName() override { return "TutorKeybox"; }
  const juce::String getApplicationVersion() override { return "1.0.0"; }
  bool moreThanOneInstanceAllowed() override { return false; }

  void anotherInstanceStarted(const juce::String & /*commandLine*/) override {
    // Bring existing window to front if launched again
    if (mainWindow != nullptr) {
      mainWindow->toFront(true);
    }
  }

  // Called when the application starts
  void initialise(const juce::String & /*commandLine*/) override {
    mainWindow = std::make_unique<MainWindow>(getApplicationName());
  }

  // Called when the application quits
  void shutdown() override {
    mainWindow = nullptr; // Closes and deletes the window
  }

  void systemRequestedQuit() override { quit(); }

  // ==============================================================================
  /**
   * Desktop window frame class holding our MainComponent.
   */
  class MainWindow : public juce::DocumentWindow {
  public:
    MainWindow(juce::String name)
        : DocumentWindow(
              name,
              juce::Desktop::getInstance().getDefaultLookAndFeel().findColour(
                  juce::ResizableWindow::backgroundColourId),
              DocumentWindow::allButtons) {
      setUsingNativeTitleBar(true);
      setContentOwned(new MainComponent(), true);

#if JUCE_IOS || JUCE_ANDROID
      setFullScreen(true);
#else
      setResizable(true, true);
      setResizeLimits(400, 200, 1920, 1080);
      setSize(1000, 600);
      centreWithSize(1000, 600);
#endif

      setVisible(true);
    }

    void closeButtonPressed() override {
      JUCEApplication::getInstance()->systemRequestedQuit();
    }

  private:
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(MainWindow)
  };

private:
  std::unique_ptr<MainWindow> mainWindow;
};

// ==============================================================================
// This macro generates the main() function that launches the JUCE app
START_JUCE_APPLICATION(TutorKeyboxApplication)
