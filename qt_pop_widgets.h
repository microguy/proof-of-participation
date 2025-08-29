// Copyright (c) 2025 MicroGuy / Goldcoin Developers
// Proof of Participation Qt 6.9 Widgets - Modern C++23
// GUI components for PoP consensus system

#pragma once

#include <QtWidgets/QWidget>
#include <QtWidgets/QLabel>
#include <QtWidgets/QProgressBar>
#include <QtWidgets/QVBoxLayout>
#include <QtWidgets/QHBoxLayout>
#include <QtWidgets/QGridLayout>
#include <QtWidgets/QGroupBox>
#include <QtWidgets/QPushButton>
#include <QtWidgets/QTextEdit>
#include <QtWidgets/QTimer>
#include <QtWidgets/QFrame>
#include <QtCore/QDateTime>
#include <QtCharts/QChart>
#include <QtCharts/QLineSeries>
#include <QtCharts/QChartView>
#include <memory>

#include "participation_modern.h"
#include "hybridfee_modern.h"

QT_BEGIN_NAMESPACE
class QVBoxLayout;
class QHBoxLayout;
class QLabel;
class QProgressBar;
QT_END_NAMESPACE

namespace goldcoin::qt {

// Main PoP Overview Widget - shows participation status
class ParticipationOverviewWidget : public QWidget {
    Q_OBJECT

public:
    explicit ParticipationOverviewWidget(QWidget *parent = nullptr);
    ~ParticipationOverviewWidget() override = default;

public slots:
    void updateParticipationStatus();
    void updateNetworkStats();

private slots:
    void onCheckEligibilityClicked();
    void onStartParticipationClicked();
    void onStopParticipationClicked();

private:
    void setupUI();
    void createParticipationSection();
    void createStatsSection();
    void createControlSection();
    
    // UI Components
    std::unique_ptr<QVBoxLayout> m_mainLayout;
    
    // Participation Status
    QGroupBox *m_participationGroup;
    QLabel *m_stakeLevelLabel;
    QLabel *m_coinAgeLabel;
    QLabel *m_eligibilityLabel;
    QProgressBar *m_maturityProgress;
    
    // Network Stats
    QGroupBox *m_networkGroup;
    QLabel *m_participantsLabel;
    QLabel *m_blockTimeLabel;
    QLabel *m_decentralizationLabel;
    
    // Controls
    QGroupBox *m_controlGroup;
    QPushButton *m_checkEligibilityBtn;
    QPushButton *m_startParticipationBtn;
    QPushButton *m_stopParticipationBtn;
    
    // Update timer
    std::unique_ptr<QTimer> m_updateTimer;
    
    // Data
    goldcoin::pop::ProofOfParticipation::Stats m_networkStats;
    goldcoin::pop::ParticipationValidator::WalletMetrics m_walletMetrics;
};

// Zero-Fee Assistant Widget - shows fee eligibility and estimates
class ZeroFeeAssistantWidget : public QWidget {
    Q_OBJECT

public:
    explicit ZeroFeeAssistantWidget(QWidget *parent = nullptr);
    ~ZeroFeeAssistantWidget() override = default;

public slots:
    void updateFeeMarket();
    void checkTransactionFee(qint64 amount, const QString& address);

signals:
    void feeEstimateReady(qint64 estimatedFee, bool likelyFree);

private slots:
    void onCheckFeeClicked();
    void onRefreshMarketClicked();

private:
    void setupUI();
    void createFeeChecker();
    void createMarketStats();
    void createPriorityCalculator();
    
    // UI Components
    std::unique_ptr<QVBoxLayout> m_mainLayout;
    
    // Fee Checker
    QGroupBox *m_checkerGroup;
    QLabel *m_priorityScoreLabel;
    QLabel *m_freeEligibleLabel;
    QLabel *m_estimatedFeeLabel;
    QPushButton *m_checkFeeBtn;
    
    // Market Statistics
    QGroupBox *m_marketGroup;
    QLabel *m_freeZonePressureLabel;
    QLabel *m_medianFeeLabel;
    QLabel *m_congestionLabel;
    QProgressBar *m_congestionProgress;
    QPushButton *m_refreshMarketBtn;
    
    // Priority Calculator Info
    QGroupBox *m_priorityGroup;
    QLabel *m_coinAgeInfoLabel;
    QLabel *m_txHistoryInfoLabel;
    QLabel *m_thresholdInfoLabel;
    
    // Update timer
    std::unique_ptr<QTimer> m_marketTimer;
    
    // Data
    goldcoin::fees::BlockSpaceManager::FeeMarketStats m_marketStats;
};

// AI Autonomy Monitor Widget - shows network AI evolution
class AIAutonomyWidget : public QWidget {
    Q_OBJECT

public:
    explicit AIAutonomyWidget(QWidget *parent = nullptr);
    ~AIAutonomyWidget() override = default;

public slots:
    void updateAutonomyStatus();
    void updateDecisionHistory();

private slots:
    void onViewDecisionHistoryClicked();
    void onRefreshStatusClicked();

private:
    void setupUI();
    void createCurrentPhase();
    void createRoadmap();
    void createDecisionLog();
    
    // UI Components  
    std::unique_ptr<QVBoxLayout> m_mainLayout;
    
    // Current AI Phase
    QGroupBox *m_currentPhaseGroup;
    QLabel *m_currentYearLabel;
    QLabel *m_autonomyLevelLabel;
    QProgressBar *m_phaseProgress;
    QLabel *m_statusLabel;
    
    // AI Roadmap
    QGroupBox *m_roadmapGroup;
    QLabel *m_phase1Label;
    QLabel *m_phase2Label;
    QLabel *m_phase3Label;
    
    // Recent Decisions
    QGroupBox *m_decisionsGroup;
    QTextEdit *m_decisionLog;
    QPushButton *m_viewHistoryBtn;
    QPushButton *m_refreshBtn;
    
    // Update timer
    std::unique_ptr<QTimer> m_statusTimer;
    
    // Timeline labels for visual roadmap
    std::vector<QLabel*> m_timelineLabels;
};

// Network Health Monitor - shows PoP network statistics
class NetworkHealthWidget : public QWidget {
    Q_OBJECT

public:
    explicit NetworkHealthWidget(QWidget *parent = nullptr);
    ~NetworkHealthWidget() override = default;

public slots:
    void updateNetworkHealth();
    void updateParticipantStats();

private slots:
    void onRefreshClicked();
    void onExportStatsClicked();

private:
    void setupUI();
    void createHealthMetrics();
    void createParticipantStats();
    void createSecurityMonitor();
    void createPerformanceChart();
    
    // UI Components
    std::unique_ptr<QVBoxLayout> m_mainLayout;
    
    // Health Metrics
    QGroupBox *m_healthGroup;
    QLabel *m_totalParticipantsLabel;
    QLabel *m_eligibleParticipantsLabel;
    QLabel *m_decentralizationIndexLabel;
    QProgressBar *m_healthProgress;
    
    // Participant Statistics
    QGroupBox *m_participantGroup;
    QLabel *m_avgStakeLabel;
    QLabel *m_avgBlockTimeLabel;
    QLabel *m_networkUptimeLabel;
    
    // Security Monitoring
    QGroupBox *m_securityGroup;
    QLabel *m_ipClustersLabel;
    QLabel *m_suspiciousActivityLabel;
    QLabel *m_lastBlockProducerLabel;
    
    // Performance Chart
    QGroupBox *m_chartGroup;
    QtCharts::QChartView *m_blockTimeChart;
    QtCharts::QLineSeries *m_blockTimeSeries;
    
    // Controls
    QPushButton *m_refreshBtn;
    QPushButton *m_exportBtn;
    
    // Update timer
    std::unique_ptr<QTimer> m_healthTimer;
    
    // Chart data
    static constexpr int MAX_CHART_POINTS = 100;
    QList<QPointF> m_blockTimeHistory;
};

// PoP Settings Dialog - configure participation preferences
class PopSettingsDialog : public QWidget {
    Q_OBJECT

public:
    explicit PopSettingsDialog(QWidget *parent = nullptr);
    ~PopSettingsDialog() override = default;

public slots:
    void loadSettings();
    void saveSettings();

signals:
    void settingsChanged();

private slots:
    void onSaveClicked();
    void onCancelClicked();
    void onRestoreDefaultsClicked();

private:
    void setupUI();
    void createParticipationSettings();
    void createSecuritySettings();
    void createDisplaySettings();
    
    // UI Components
    std::unique_ptr<QVBoxLayout> m_mainLayout;
    
    // Participation Settings
    QGroupBox *m_participationGroup;
    // Add specific settings controls here
    
    // Security Settings  
    QGroupBox *m_securityGroup;
    // Add security option controls here
    
    // Display Settings
    QGroupBox *m_displayGroup;
    // Add display preference controls here
    
    // Dialog buttons
    QPushButton *m_saveBtn;
    QPushButton *m_cancelBtn;
    QPushButton *m_defaultsBtn;
};

// Transaction Priority Indicator - shows free transaction eligibility
class TransactionPriorityIndicator : public QFrame {
    Q_OBJECT

public:
    enum class PriorityLevel {
        Free,
        LowFee,
        StandardFee,
        HighFee
    };

    explicit TransactionPriorityIndicator(QWidget *parent = nullptr);
    ~TransactionPriorityIndicator() override = default;

public slots:
    void setPriorityLevel(PriorityLevel level);
    void updatePriorityScore(double score);

private:
    void setupUI();
    void updateDisplay();
    
    PriorityLevel m_currentLevel{PriorityLevel::StandardFee};
    double m_priorityScore{0.0};
    
    QLabel *m_statusIcon;
    QLabel *m_statusText;
    QLabel *m_scoreLabel;
};

} // namespace goldcoin::qt