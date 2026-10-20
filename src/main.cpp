#include <Geode/Geode.hpp>
#include <Geode/modify/PlayLayer.hpp>
#include <Geode/modify/GJBaseGameLayer.hpp>

using namespace geode::prelude;

namespace {
    constexpr int FPS_LIST[] = {30, 60, 120, 240, 360};
    constexpr int FPS_COUNT = 5;
    // Toleranz in Sekunden beim Vergleich (360 vs 240 FPS liegen ~0.0014 s auseinander)
    constexpr double TOLERANCE = 0.0004;

    int g_counts[FPS_COUNT] = {0, 0, 0, 0, 0};
    double g_lastTime[2] = {-1.0, -1.0}; // pro Spieler (P1 = 0, P2 = 1)
    int g_lastDown[2] = {-1, -1};

    void clearLastInputs() {
        for (int p = 0; p < 2; p++) {
            g_lastTime[p] = -1.0;
            g_lastDown[p] = -1;
        }
    }

    void resetCounts() {
        for (int i = 0; i < FPS_COUNT; i++) g_counts[i] = 0;
        clearLastInputs();
    }

    void updateLabel(PlayLayer* pl) {
        if (!pl || !pl->m_uiLayer) return;
        auto label = static_cast<CCLabelBMFont*>(pl->m_uiLayer->getChildByID("fp-counter"_spr));
        if (!label) return;

        std::string text;
        for (int i = 0; i < FPS_COUNT; i++) {
            text += fmt::format("{}fps: {}", FPS_LIST[i], g_counts[i]);
            if (i + 1 < FPS_COUNT) text += "\n";
        }
        label->setString(text.c_str());
    }
}

class $modify(FPPlayLayer, PlayLayer) {
    void setupHasCompleted() {
        PlayLayer::setupHasCompleted();
        resetCounts();

        auto label = CCLabelBMFont::create("", "bigFont.fnt");
        label->setID("fp-counter"_spr);
        label->setAnchorPoint({0.f, 1.f});
        label->setScale(0.4f);
        label->setOpacity(190);
        auto winSize = CCDirector::get()->getWinSize();
        label->setPosition({5.f, winSize.height - 5.f});
        m_uiLayer->addChild(label, 100);

        updateLabel(this);
    }

    void resetLevel() {
        PlayLayer::resetLevel();
        if (Mod::get()->getSettingValue<bool>("reset-on-death")) {
            resetCounts();
        } else {
            clearLastInputs();
        }
        updateLabel(this);
    }
};

class $modify(FPBaseGameLayer, GJBaseGameLayer) {
    void handleButton(bool down, int button, bool isPlayer1) {
        GJBaseGameLayer::handleButton(down, button, isPlayer1);

        // Nur im Level spielen, nicht im Editor, und nur der Jump-Button
        auto pl = PlayLayer::get();
        if (!pl || static_cast<GJBaseGameLayer*>(pl) != this) return;
        if (button != 1) return;

        int p = isPlayer1 ? 0 : 1;
        int d = down ? 1 : 0;
        if (g_lastDown[p] == d) return; // doppelte Events ignorieren

        double now = m_gameState.m_levelTime;

        if (g_lastTime[p] >= 0.0) {
            double dt = now - g_lastTime[p];
            // Genau 1 Frame Abstand (Halten oder Pause) bei diesem FPS-Wert?
            for (int i = 0; i < FPS_COUNT; i++) {
                if (std::abs(dt - 1.0 / FPS_LIST[i]) <= TOLERANCE) {
                    g_counts[i]++;
                    break;
                }
            }
        }

        g_lastTime[p] = now;
        g_lastDown[p] = d;
        updateLabel(pl);
    }
};
