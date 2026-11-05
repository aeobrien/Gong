#include "DiagnosticWindow.h"

// ===== DiagnosticWindow =====

DiagnosticWindow::DiagnosticWindow(DiagnosticState& state)
    : DocumentWindow("Gong Diagnostics",
                     juce::Colour(0xff1e1e1e),
                     DocumentWindow::closeButton | DocumentWindow::minimiseButton)
{
    content = new DiagnosticContentComponent(state);
    setContentOwned(content, true);
    setResizable(true, true);
    centreWithSize(700, 550);
    setVisible(true);
    setAlwaysOnTop(false);
}

DiagnosticWindow::~DiagnosticWindow() = default;

void DiagnosticWindow::closeButtonPressed()
{
    setVisible(false);
}

void DiagnosticWindow::refresh()
{
    if (isVisible() && content)
        content->refresh();
}

// ===== DiagnosticContentComponent =====

DiagnosticContentComponent::DiagnosticContentComponent(DiagnosticState& state)
    : signalTab(state), energyTab(state), modTab(state)
{
    addAndMakeVisible(tabs);
    tabs.addTab("Signal Chain", juce::Colour(0xff2a2a2a), &signalTab, false);
    tabs.addTab("Energy", juce::Colour(0xff2a2a2a), &energyTab, false);
    tabs.addTab("Modulation", juce::Colour(0xff2a2a2a), &modTab, false);
    setSize(700, 550);
}

void DiagnosticContentComponent::resized()
{
    tabs.setBounds(getLocalBounds());
}

void DiagnosticContentComponent::refresh()
{
    signalTab.refresh();
    energyTab.refresh();
    modTab.refresh();
}

// ===== SignalChainTab =====

SignalChainTab::SignalChainTab(DiagnosticState& s) : state(s)
{
    // Test signal type
    addAndMakeVisible(testTypeLabel);
    addAndMakeVisible(testTypeCombo);
    testTypeCombo.addItem("Off", 1);
    testTypeCombo.addItem("Sine", 2);
    testTypeCombo.addItem("Sweep", 3);
    testTypeCombo.addItem("White", 4);
    testTypeCombo.addItem("Pink", 5);
    testTypeCombo.addItem("Impulse", 6);
    testTypeCombo.setSelectedId(1, juce::dontSendNotification);
    testTypeCombo.onChange = [this] {
        state.testSignalType.store(testTypeCombo.getSelectedId() - 1, std::memory_order_relaxed);
    };

    // Test freq
    addAndMakeVisible(testFreqLabel);
    addAndMakeVisible(testFreqSlider);
    testFreqSlider.setRange(20.0, 5000.0, 1.0);
    testFreqSlider.setValue(440.0);
    testFreqSlider.setSkewFactorFromMidPoint(440.0);
    testFreqSlider.setTextValueSuffix(" Hz");
    testFreqSlider.onValueChange = [this] {
        state.testSineFreq.store(static_cast<float>(testFreqSlider.getValue()), std::memory_order_relaxed);
    };

    // Injection point
    addAndMakeVisible(testInjectLabel);
    addAndMakeVisible(testInjectCombo);
    testInjectCombo.addItem("Input", 1);
    testInjectCombo.addItem("After Synth", 2);
    testInjectCombo.addItem("After Conv", 3);
    testInjectCombo.addItem("After Exciter", 4);
    testInjectCombo.setSelectedId(1, juce::dontSendNotification);
    testInjectCombo.onChange = [this] {
        state.testInjectionPoint.store(testInjectCombo.getSelectedId() - 1, std::memory_order_relaxed);
    };

    // Test gain
    addAndMakeVisible(testGainLabel);
    addAndMakeVisible(testGainSlider);
    testGainSlider.setRange(0.0, 1.0, 0.01);
    testGainSlider.setValue(0.5);
    testGainSlider.onValueChange = [this] {
        state.testSignalGain.store(static_cast<float>(testGainSlider.getValue()), std::memory_order_relaxed);
    };

    // Stage buttons
    auto setupBtns = [this](StageButtons& btns, std::atomic<int>& atom) {
        setupStageButtons(btns, atom);
    };
    setupBtns(synthButtons, state.synthState);
    setupBtns(convButtons, state.convState);
    setupBtns(exciterButtons, state.exciterState);
    setupBtns(compButtons, state.compState);
}

void SignalChainTab::setupStageButtons(StageButtons& btns, std::atomic<int>& stateAtom)
{
    auto setupBtn = [this](juce::TextButton& btn, std::atomic<int>& atom, int value) {
        addAndMakeVisible(btn);
        btn.setClickingTogglesState(true);
        btn.setColour(juce::TextButton::buttonOnColourId,
                      value == 2 ? juce::Colours::yellow.darker() :
                      value == 1 ? juce::Colours::red.darker() :
                      juce::Colours::blue.darker());
        btn.onClick = [&btn, &atom, value] {
            atom.store(btn.getToggleState() ? value : 0, std::memory_order_relaxed);
        };
    };
    setupBtn(btns.solo, stateAtom, 2);
    setupBtn(btns.mute, stateAtom, 1);
    setupBtn(btns.bypass, stateAtom, 3);
}

void SignalChainTab::paint(juce::Graphics& g)
{
    g.fillAll(juce::Colour(0xff1e1e1e));

    // Draw meter table
    int tableTop = 80;
    int rowH = 32;
    int nameW = 90;
    int meterW = 80;
    int timeW = 60;
    int margin = 10;

    g.setFont(juce::FontOptions(11.0f).withStyle("Bold"));
    g.setColour(juce::Colours::grey);

    int x = margin;
    g.drawText("Stage", x, tableTop, nameW, rowH, juce::Justification::centredLeft);
    x += nameW;
    g.drawText("RMS L", x, tableTop, meterW, rowH, juce::Justification::centred);
    x += meterW;
    g.drawText("RMS R", x, tableTop, meterW, rowH, juce::Justification::centred);
    x += meterW;
    g.drawText("Peak", x, tableTop, meterW, rowH, juce::Justification::centred);
    x += meterW;
    g.drawText("Time", x, tableTop, timeW, rowH, juce::Justification::centred);

    const char* stageNames[] = { "Input", "Synth", "Convolution", "Exciter", "Compressor", "Output" };
    g.setFont(juce::FontOptions(10.0f));

    for (int row = 0; row < 6; ++row)
    {
        int y = tableTop + (row + 1) * rowH;

        // Alternate row bg
        if (row % 2 == 0)
        {
            g.setColour(juce::Colour(0xff252525));
            g.fillRect(margin, y, getWidth() - margin * 2, rowH);
        }

        g.setColour(juce::Colours::white);
        x = margin;
        g.drawText(stageNames[row], x, y, nameW, rowH, juce::Justification::centredLeft);
        x += nameW;

        auto& m = meterCache[row];

        // RMS L bar
        g.setColour(juce::Colour(0xff333333));
        g.fillRect(x + 4, y + 8, meterW - 8, rowH - 16);
        g.setColour(juce::Colour(0xff00cc66));
        int barW = juce::jlimit(0, meterW - 8, static_cast<int>(juce::jlimit(0.0f, 1.0f, m.rmsL) * (meterW - 8)));
        g.fillRect(x + 4, y + 8, barW, rowH - 16);
        x += meterW;

        // RMS R bar
        g.setColour(juce::Colour(0xff333333));
        g.fillRect(x + 4, y + 8, meterW - 8, rowH - 16);
        g.setColour(juce::Colour(0xff00cc66));
        barW = juce::jlimit(0, meterW - 8, static_cast<int>(juce::jlimit(0.0f, 1.0f, m.rmsR) * (meterW - 8)));
        g.fillRect(x + 4, y + 8, barW, rowH - 16);
        x += meterW;

        // Peak bar
        g.setColour(juce::Colour(0xff333333));
        g.fillRect(x + 4, y + 8, meterW - 8, rowH - 16);
        g.setColour(m.peakL > 0.95f ? juce::Colours::red : juce::Colour(0xff00cc66));
        barW = juce::jlimit(0, meterW - 8, static_cast<int>(juce::jlimit(0.0f, 1.0f, m.peakL) * (meterW - 8)));
        g.fillRect(x + 4, y + 8, barW, rowH - 16);
        x += meterW;

        // Time text
        if (row >= 1 && row <= 4) // synth through compressor
        {
            g.setColour(juce::Colours::white);
            juce::String timeStr;
            if (m.timeUs >= 1000.0f)
                timeStr = juce::String(m.timeUs / 1000.0f, 1) + "ms";
            else
                timeStr = juce::String(static_cast<int>(m.timeUs)) + "us";
            g.drawText(timeStr, x, y, timeW, rowH, juce::Justification::centred);
        }
    }

    // Total time
    float totalUs = state.totalTimeUs.load(std::memory_order_relaxed);
    float budget = state.budgetUs.load(std::memory_order_relaxed);
    int bottomY = tableTop + 7 * rowH + 4;
    g.setColour(juce::Colours::white);
    g.setFont(juce::FontOptions(11.0f));
    juce::String totalStr = "Total: " + juce::String(totalUs / 1000.0f, 1) + "ms / "
        + juce::String(budget / 1000.0f, 1) + "ms budget ("
        + juce::String(static_cast<int>(budget > 0 ? (totalUs / budget * 100.0f) : 0)) + "%)";
    g.drawText(totalStr, margin, bottomY, getWidth() - margin * 2, 20, juce::Justification::centredLeft);
}

void SignalChainTab::resized()
{
    auto area = getLocalBounds().reduced(10);
    auto topRow = area.removeFromTop(28);

    testTypeLabel.setBounds(topRow.removeFromLeft(45));
    testTypeCombo.setBounds(topRow.removeFromLeft(80));
    topRow.removeFromLeft(8);
    testFreqLabel.setBounds(topRow.removeFromLeft(35));
    testFreqSlider.setBounds(topRow.removeFromLeft(100));
    topRow.removeFromLeft(8);
    testInjectLabel.setBounds(topRow.removeFromLeft(40));
    testInjectCombo.setBounds(topRow.removeFromLeft(100));
    topRow.removeFromLeft(8);
    testGainLabel.setBounds(topRow.removeFromLeft(35));
    testGainSlider.setBounds(topRow.removeFromLeft(80));

    // Position S/M/B buttons
    int tableTop = 80;
    int rowH = 32;
    int btnX = getWidth() - 110;
    int btnW = 28;
    int btnGap = 4;

    auto placeBtns = [&](StageButtons& btns, int row) {
        int y = tableTop + (row + 1) * rowH + 4;
        btns.solo.setBounds(btnX, y, btnW, rowH - 8);
        btns.mute.setBounds(btnX + btnW + btnGap, y, btnW, rowH - 8);
        btns.bypass.setBounds(btnX + 2 * (btnW + btnGap), y, btnW, rowH - 8);
    };

    placeBtns(synthButtons, 1);
    placeBtns(convButtons, 2);
    placeBtns(exciterButtons, 3);
    placeBtns(compButtons, 4);
}

void SignalChainTab::refresh()
{
    auto readMeter = [](const StageMeter& m, float timeUs) -> MeterValues {
        return {
            m.rmsL.load(std::memory_order_relaxed),
            m.rmsR.load(std::memory_order_relaxed),
            m.peakL.load(std::memory_order_relaxed),
            timeUs
        };
    };

    meterCache[0] = readMeter(state.input, 0);
    meterCache[1] = readMeter(state.afterSynth, state.synthTime.lastUs.load(std::memory_order_relaxed));
    meterCache[2] = readMeter(state.afterConv, state.convTime.lastUs.load(std::memory_order_relaxed));
    meterCache[3] = readMeter(state.afterExciter, state.exciterTime.lastUs.load(std::memory_order_relaxed));
    meterCache[4] = readMeter(state.afterComp, state.compTime.lastUs.load(std::memory_order_relaxed));
    meterCache[5] = readMeter(state.output, 0);

    repaint();
}

// ===== EnergyMonitorTab =====

EnergyMonitorTab::EnergyMonitorTab(DiagnosticState& s) : state(s) {}

void EnergyMonitorTab::paint(juce::Graphics& g)
{
    g.fillAll(juce::Colour(0xff1e1e1e));

    int margin = 20;
    int barH = 28;
    int barGap = 12;
    int y = 20;
    int barMaxW = getWidth() - margin * 2 - 140;

    auto drawBar = [&](const juce::String& label, float value, juce::Colour colour) {
        g.setColour(juce::Colours::white);
        g.setFont(juce::FontOptions(12.0f));
        g.drawText(label, margin, y, 100, barH, juce::Justification::centredRight);

        int barX = margin + 110;
        g.setColour(juce::Colour(0xff333333));
        g.fillRect(barX, y + 4, barMaxW, barH - 8);
        g.setColour(colour);
        int w = juce::jlimit(0, barMaxW, static_cast<int>(juce::jlimit(0.0f, 1.0f, value) * barMaxW));
        g.fillRect(barX, y + 4, w, barH - 8);

        g.setColour(juce::Colours::white);
        g.drawText(juce::String(static_cast<int>(value * 100)) + "%",
                   barX + barMaxW + 5, y, 40, barH, juce::Justification::centredLeft);

        y += barH + barGap;
    };

    drawBar("Global", globalEnergy, juce::Colour(0xffff8800));

    y += 10;
    g.setColour(juce::Colours::grey);
    g.setFont(juce::FontOptions(11.0f).withStyle("Bold"));
    g.drawText("Per-Band Energy", margin, y, 200, 20, juce::Justification::centredLeft);
    y += 24;

    juce::Colour bandColours[] = {
        juce::Colour(0xff4488ff), juce::Colour(0xff44cc88),
        juce::Colour(0xffcc8844), juce::Colour(0xffcc4488)
    };

    for (int i = 0; i < 4; ++i)
        drawBar("Band " + juce::String(i), bandEnergies[i], bandColours[i]);
}

void EnergyMonitorTab::refresh()
{
    globalEnergy = state.globalEnergy.load(std::memory_order_relaxed);
    for (int i = 0; i < 4; ++i)
        bandEnergies[i] = state.bandEnergy[i].load(std::memory_order_relaxed);
    repaint();
}

// ===== ModulationMatrixTab =====

ModulationMatrixTab::ModulationMatrixTab(DiagnosticState& s) : state(s)
{
    addAndMakeVisible(addButton);
    addButton.onClick = [this] { addRouteRow(); resized(); };

    addAndMakeVisible(viewport);
    viewport.setViewedComponent(&routeContainer, false);
    viewport.setScrollBarsShown(true, false);

    // LFO controls
    addAndMakeVisible(lfo1RateLabel);
    addAndMakeVisible(lfo1RateSlider);
    lfo1RateSlider.setRange(0.01, 20.0, 0.01);
    lfo1RateSlider.setValue(0.5);
    lfo1RateSlider.setTextValueSuffix(" Hz");
    lfo1RateSlider.setSkewFactorFromMidPoint(2.0);
    lfo1RateSlider.onValueChange = [this] {
        state.lfo1Rate.store(static_cast<float>(lfo1RateSlider.getValue()), std::memory_order_relaxed);
    };

    addAndMakeVisible(lfo1ShapeCombo);
    lfo1ShapeCombo.addItem("Sine", 1);
    lfo1ShapeCombo.addItem("Triangle", 2);
    lfo1ShapeCombo.addItem("Saw", 3);
    lfo1ShapeCombo.addItem("Square", 4);
    lfo1ShapeCombo.setSelectedId(1, juce::dontSendNotification);
    lfo1ShapeCombo.onChange = [this] {
        state.lfo1Shape.store(lfo1ShapeCombo.getSelectedId() - 1, std::memory_order_relaxed);
    };

    addAndMakeVisible(lfo2RateLabel);
    addAndMakeVisible(lfo2RateSlider);
    lfo2RateSlider.setRange(0.01, 20.0, 0.01);
    lfo2RateSlider.setValue(2.0);
    lfo2RateSlider.setTextValueSuffix(" Hz");
    lfo2RateSlider.setSkewFactorFromMidPoint(2.0);
    lfo2RateSlider.onValueChange = [this] {
        state.lfo2Rate.store(static_cast<float>(lfo2RateSlider.getValue()), std::memory_order_relaxed);
    };

    addAndMakeVisible(lfo2ShapeCombo);
    lfo2ShapeCombo.addItem("Sine", 1);
    lfo2ShapeCombo.addItem("Triangle", 2);
    lfo2ShapeCombo.addItem("Saw", 3);
    lfo2ShapeCombo.addItem("Square", 4);
    lfo2ShapeCombo.setSelectedId(2, juce::dontSendNotification);
    lfo2ShapeCombo.onChange = [this] {
        state.lfo2Shape.store(lfo2ShapeCombo.getSelectedId() - 1, std::memory_order_relaxed);
    };
}

void ModulationMatrixTab::addRouteRow(const ModRoute& route)
{
    if (rows.size() >= DiagnosticState::kMaxRoutes) return;

    auto* row = rows.add(new RouteRow());

    // Source combo
    for (int i = 0; i < static_cast<int>(ModSource::Count); ++i)
        row->source.addItem(getSourceName(static_cast<ModSource>(i)), i + 1);
    row->source.setSelectedId(static_cast<int>(route.source) + 1, juce::dontSendNotification);
    row->source.onChange = [this] { pushRoutesToState(); };
    routeContainer.addAndMakeVisible(row->source);

    // Target combo
    for (int i = 0; i < static_cast<int>(ModTarget::Count); ++i)
        row->target.addItem(getTargetName(static_cast<ModTarget>(i)), i + 1);
    row->target.setSelectedId(static_cast<int>(route.target) + 1, juce::dontSendNotification);
    row->target.onChange = [this] { pushRoutesToState(); };
    routeContainer.addAndMakeVisible(row->target);

    // Amount slider
    row->amount.setRange(-1.0, 1.0, 0.01);
    row->amount.setValue(route.amount);
    row->amount.setSliderStyle(juce::Slider::LinearHorizontal);
    row->amount.setTextBoxStyle(juce::Slider::TextBoxRight, false, 40, 20);
    row->amount.onValueChange = [this] { pushRoutesToState(); };
    routeContainer.addAndMakeVisible(row->amount);

    // Curve combo
    row->curve.addItem("Linear", 1);
    row->curve.addItem("Exponential", 2);
    row->curve.addItem("S-Curve", 3);
    row->curve.addItem("Logarithmic", 4);
    row->curve.setSelectedId(static_cast<int>(route.curve) + 1, juce::dontSendNotification);
    row->curve.onChange = [this] { pushRoutesToState(); };
    routeContainer.addAndMakeVisible(row->curve);

    // Enabled toggle
    row->enabled.setToggleState(route.enabled, juce::dontSendNotification);
    row->enabled.onClick = [this] { pushRoutesToState(); };
    routeContainer.addAndMakeVisible(row->enabled);

    // Delete button
    row->deleteBtn.onClick = [this, row] {
        int idx = rows.indexOf(row);
        if (idx >= 0) removeRouteRow(idx);
    };
    routeContainer.addAndMakeVisible(row->deleteBtn);

    pushRoutesToState();
}

void ModulationMatrixTab::removeRouteRow(int index)
{
    rows.remove(index);
    pushRoutesToState();
    resized();
}

void ModulationMatrixTab::pushRoutesToState()
{
    std::vector<ModRoute> routes;
    for (auto* row : rows)
    {
        ModRoute r;
        r.source = static_cast<ModSource>(row->source.getSelectedId() - 1);
        r.target = static_cast<ModTarget>(row->target.getSelectedId() - 1);
        r.amount = static_cast<float>(row->amount.getValue());
        r.curve = static_cast<CurveType>(row->curve.getSelectedId() - 1);
        r.enabled = row->enabled.getToggleState();
        routes.push_back(r);
    }
    state.setRoutes(routes.data(), static_cast<int>(routes.size()));
}

std::vector<ModRoute> ModulationMatrixTab::getRoutes() const
{
    std::vector<ModRoute> routes;
    for (auto* row : rows)
    {
        ModRoute r;
        r.source = static_cast<ModSource>(row->source.getSelectedId() - 1);
        r.target = static_cast<ModTarget>(row->target.getSelectedId() - 1);
        r.amount = static_cast<float>(row->amount.getValue());
        r.curve = static_cast<CurveType>(row->curve.getSelectedId() - 1);
        r.enabled = row->enabled.getToggleState();
        routes.push_back(r);
    }
    return routes;
}

void ModulationMatrixTab::setRoutes(const std::vector<ModRoute>& routes)
{
    rows.clear();
    for (auto& r : routes)
        addRouteRow(r);
    resized();
}

float ModulationMatrixTab::getLfo1Rate() const { return static_cast<float>(lfo1RateSlider.getValue()); }
float ModulationMatrixTab::getLfo2Rate() const { return static_cast<float>(lfo2RateSlider.getValue()); }
int ModulationMatrixTab::getLfo1Shape() const { return lfo1ShapeCombo.getSelectedId() - 1; }
int ModulationMatrixTab::getLfo2Shape() const { return lfo2ShapeCombo.getSelectedId() - 1; }

void ModulationMatrixTab::setLfoConfig(float rate1, int shape1, float rate2, int shape2)
{
    lfo1RateSlider.setValue(rate1, juce::dontSendNotification);
    lfo1ShapeCombo.setSelectedId(shape1 + 1, juce::dontSendNotification);
    lfo2RateSlider.setValue(rate2, juce::dontSendNotification);
    lfo2ShapeCombo.setSelectedId(shape2 + 1, juce::dontSendNotification);
    state.lfo1Rate.store(rate1, std::memory_order_relaxed);
    state.lfo1Shape.store(shape1, std::memory_order_relaxed);
    state.lfo2Rate.store(rate2, std::memory_order_relaxed);
    state.lfo2Shape.store(shape2, std::memory_order_relaxed);
}

void ModulationMatrixTab::resized()
{
    auto area = getLocalBounds().reduced(8);

    // LFO row
    auto lfoRow = area.removeFromTop(28);
    lfo1RateLabel.setBounds(lfoRow.removeFromLeft(70));
    lfo1RateSlider.setBounds(lfoRow.removeFromLeft(100));
    lfoRow.removeFromLeft(4);
    lfo1ShapeCombo.setBounds(lfoRow.removeFromLeft(80));
    lfoRow.removeFromLeft(20);
    lfo2RateLabel.setBounds(lfoRow.removeFromLeft(70));
    lfo2RateSlider.setBounds(lfoRow.removeFromLeft(100));
    lfoRow.removeFromLeft(4);
    lfo2ShapeCombo.setBounds(lfoRow.removeFromLeft(80));

    area.removeFromTop(4);

    // Add button
    auto btnRow = area.removeFromTop(26);
    addButton.setBounds(btnRow.removeFromLeft(90));

    area.removeFromTop(4);

    // Route rows in viewport
    viewport.setBounds(area);

    int rowH = 30;
    int containerH = rows.size() * rowH + 4;
    routeContainer.setBounds(0, 0, area.getWidth() - 12, juce::jmax(containerH, area.getHeight()));

    for (int i = 0; i < rows.size(); ++i)
    {
        auto* row = rows[i];
        int y = i * rowH;
        int x = 0;

        row->enabled.setBounds(x, y, 24, rowH);
        x += 26;
        row->source.setBounds(x, y + 2, 110, rowH - 4);
        x += 114;
        row->target.setBounds(x, y + 2, 110, rowH - 4);
        x += 114;
        row->amount.setBounds(x, y + 2, 130, rowH - 4);
        x += 134;
        row->curve.setBounds(x, y + 2, 90, rowH - 4);
        x += 94;
        row->deleteBtn.setBounds(x, y + 2, 26, rowH - 4);
    }
}

void ModulationMatrixTab::refresh()
{
    // Modulation matrix tab is UI-driven, nothing to refresh from audio thread
}
