#include "PatchCableOverlay.h"

PatchCableOverlay::PatchCableOverlay()
{
    setInterceptsMouseClicks(false, false);
    setOpaque(false);
}

void PatchCableOverlay::clearCables()
{
    cables.clear();
    repaint();
}

void PatchCableOverlay::addCable(ModulePanel* source, ModulePanel* dest)
{
    cables.push_back({ source, dest, 0.0f });
}

void PatchCableOverlay::setSignalLevel(int cableIndex, float level)
{
    if (cableIndex >= 0 && cableIndex < static_cast<int>(cables.size()))
    {
        cables[static_cast<size_t>(cableIndex)].signalLevel = level;
        repaint();
    }
}

void PatchCableOverlay::paint(juce::Graphics& g)
{
    float r = 8.0f;

    for (auto& cable : cables)
    {
        if (cable.source == nullptr || cable.dest == nullptr)
            continue;

        auto srcPos = getLocalPoint(cable.source->getParentComponent(),
                                     cable.source->getOutputPortPosition()).toFloat();
        auto dstPos = getLocalPoint(cable.dest->getParentComponent(),
                                     cable.dest->getInputPortPosition()).toFloat();

        auto colour = cable.source->getModuleColour();
        float alpha = 0.25f + cable.signalLevel * 0.6f;
        float thickness = 1.5f + cable.signalLevel * 1.0f;

        juce::Path path;

        float vertDist = std::abs(dstPos.y - srcPos.y);
        bool crossRow = vertDist > 60.0f;

        if (crossRow && crossRowRouteY > 0.0f)
        {
            // Cross-row cable:
            // src -> right a bit -> down -> across at crossRowRouteY -> left past dst -> down -> right into dst

            float exitX = srcPos.x + 10.0f;
            float overshootX = dstPos.x - 22.0f;

            path.startNewSubPath(srcPos);
            path.lineTo(exitX, srcPos.y);

            // Turn down
            path.quadraticTo(exitX + r, srcPos.y, exitX + r, srcPos.y + r);
            path.lineTo(exitX + r, crossRowRouteY - r);

            // Turn left
            path.quadraticTo(exitX + r, crossRowRouteY, exitX, crossRowRouteY);
            path.lineTo(overshootX + r, crossRowRouteY);

            // Turn down
            path.quadraticTo(overshootX, crossRowRouteY, overshootX, crossRowRouteY + r);
            path.lineTo(overshootX, dstPos.y - r);

            // Turn right into destination
            path.quadraticTo(overshootX, dstPos.y, overshootX + r, dstPos.y);
            path.lineTo(dstPos.x, dstPos.y);
        }
        else if (std::abs(dstPos.y - srcPos.y) > 15.0f && dstPos.x > srcPos.x - 10.0f)
        {
            // Short vertical hop (e.g., resonator output splitting to combo/crash stacked above/below)
            // Simple right-angle: horizontal then vertical then horizontal
            float midX = srcPos.x + (dstPos.x - srcPos.x) * 0.5f;

            path.startNewSubPath(srcPos);
            path.lineTo(midX - r, srcPos.y);
            path.quadraticTo(midX, srcPos.y, midX, srcPos.y + (dstPos.y > srcPos.y ? r : -r));
            path.lineTo(midX, dstPos.y - (dstPos.y > srcPos.y ? r : -r));
            path.quadraticTo(midX, dstPos.y, midX + r, dstPos.y);
            path.lineTo(dstPos.x, dstPos.y);
        }
        else
        {
            // Same-row, same height: gentle horizontal curve
            float midX = (srcPos.x + dstPos.x) * 0.5f;
            path.startNewSubPath(srcPos);
            path.cubicTo(midX, srcPos.y, midX, dstPos.y, dstPos.x, dstPos.y);
        }

        g.setColour(colour.withAlpha(alpha));
        g.strokePath(path, juce::PathStrokeType(thickness, juce::PathStrokeType::curved,
                                                  juce::PathStrokeType::rounded));

        if (cable.signalLevel > 0.3f)
        {
            g.setColour(colour.withAlpha(alpha * 0.25f));
            g.strokePath(path, juce::PathStrokeType(thickness + 3.0f, juce::PathStrokeType::curved,
                                                      juce::PathStrokeType::rounded));
        }
    }
}
