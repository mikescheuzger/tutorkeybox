#include "../Source/Synth/SamplePackager.h"
#include <iostream>
#include <juce_gui_basics/juce_gui_basics.h>

int main(int argc, char *argv[]) {
  juce::ScopedJuceInitialiser_GUI juceInit;

  if (argc < 3) {
    std::cout << "Usage: ./TKBPackager <input_wav_folder> <output_bin_file>"
              << std::endl;
    return 1;
  }

  juce::File inputDir(argv[1]);
  juce::File outputFile(argv[2]);

  if (SamplePackager::createPackage(inputDir, outputFile)) {
    std::cout << "SUCCESS: Created "
              << outputFile.getFullPathName().toStdString() << std::endl;
    return 0;
  }

  std::cout << "ERROR: Packaging failed!" << std::endl;
  return 1;
}
