#include "MidiControllerMock.h"

MidiControllerMock::MidiControllerMock()
{
}

bool MidiControllerMock::processMidiMessage(const juce::MidiMessage& message)
{
    if (message.isController())
    {
        int cc = message.getControllerNumber();
        int value = message.getControllerValue(); // 0-127

        if (cc == attackSlopeCC)
        {
            // Map 0-127 to 0-255
            descriptor.attackSlope = static_cast<uint8_t>(value * 2);
        }
        else if (cc == spectralCentroidCC)
        {
            // Map 0-127 to 200-15000 Hz
            float norm = value / 127.0f;
            descriptor.spectralCentroid = static_cast<uint16_t>(200.0f + norm * 14800.0f);
        }
        else if (cc == hfEnergyRatioCC)
        {
            // Map 0-127 to 0-255
            descriptor.hfEnergyRatio = static_cast<uint8_t>(value * 2);
        }
        else if (cc == decayShapeCC)
        {
            // Map 0-127 to 0-255
            descriptor.decayShape = static_cast<uint8_t>(value * 2);
        }

        return false; // CC doesn't trigger a strike
    }

    if (message.isNoteOn())
    {
        descriptor.velocity = static_cast<uint8_t>(message.getVelocity());
        descriptor.padId = static_cast<uint8_t>(juce::jlimit(0, 3, message.getNoteNumber() / 32));
        return true; // Strike!
    }

    return false;
}
