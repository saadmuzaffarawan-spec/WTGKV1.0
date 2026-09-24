#pragma once

#include <raylib.h>
#include <vector>
#include <string>
#include "core/game_context.h"

class MainMenuSystem {
public:
    MainMenuSystem();
    ~MainMenuSystem();

    void Update(float dt);
    void Draw(int screenW, int screenH, float timeVal);

    bool IsShowingAnyModal() const {
        return m_showCaseFilesModal || m_showSurvivalModal || m_showSettingsModal || m_showDossierModal;
    }

    // Modal Controls
    void ShowCaseFilesModal(bool show) { m_showCaseFilesModal = show; }
    void ShowSurvivalModal(bool show) { m_showSurvivalModal = show; }
    void ShowSettingsModal(bool show) { m_showSettingsModal = show; }
    void ShowDossierModal(bool show) { m_showDossierModal = show; }

    bool IsSettingsModalShowing() const { return m_showSettingsModal; }
    bool IsCaseFilesModalShowing() const { return m_showCaseFilesModal; }
    bool IsSurvivalModalShowing() const { return m_showSurvivalModal; }
    bool IsDossierModalShowing() const { return m_showDossierModal; }

    bool IsStartingGame() const { return m_isStartingGame; }
    bool ShouldTransitionToGame() const { return m_isStartingGame && (m_playTransitionTimer / 0.70f >= 1.0f); }

private:
    void DrawCaseFilesModal(int screenW, int screenH, Vector2 mPos);
    void DrawSurvivalModal(int screenW, int screenH, Vector2 mPos);
    void DrawSettingsModal(int screenW, int screenH, Vector2 mPos);
    void DrawDossierModal(int screenW, int screenH, Vector2 mPos);

    // State
    float m_awakeIntensity = 1.0f;
    float m_idleTimer = 0.0f;
    Vector2 m_lightPos = { 640.0f, 360.0f };
    float m_lurkerEyeFlee = 0.0f;

    bool m_showCaseFilesModal = false;
    bool m_showSurvivalModal = false;
    bool m_showSettingsModal = false;
    bool m_showDossierModal = false;
    int m_caseFileSelected = 0;
    int m_dossierFileSelected = 0;

    bool m_isStartingGame = false;
    float m_playTransitionTimer = 0.0f;

    // Menu Animation & Selection
    float m_menuOptionHover[5] = { 0.0f, 0.0f, 0.0f, 0.0f, 0.0f };
    float m_skullGazeFlare = 0.0f;
    int m_menuSelection = 0;
    int m_prevMenuSelection = 0;

    // Lightning System
    float m_menuLightningTimer = 14.0f;
    float m_menuFlashAlpha = 0.0f;
};
