#pragma once

#include <JuceHeader.h>

/**
 * Embedded HTTP server for the performance touch UI.
 * Runs in a juce::Thread, serves REST API + static web assets.
 * POST handlers bounce state changes to the message thread via callAsync.
 */
class PerformanceServer : public juce::Thread
{
public:
    struct Listener
    {
        virtual ~Listener() = default;
        virtual juce::StringArray getPresetNames() = 0;
        virtual bool activatePreset(const juce::String& name) = 0;
        virtual juce::StringArray getIRNames() = 0;
        virtual bool activateIR(const juce::String& name) = 0;
        virtual juce::String getCurrentPresetName() = 0;
        virtual juce::String getCurrentIRName() = 0;
    };

    PerformanceServer(Listener& listener, int port = 8080);
    ~PerformanceServer() override;

    void setWebRoot(const juce::File& directory);
    void startServer();
    void stopServer();

private:
    void run() override;

    Listener& listener;
    int port;
    juce::File webRoot;

    // Forward declaration - implementation uses httplib
    struct Impl;
    std::unique_ptr<Impl> impl;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(PerformanceServer)
};
