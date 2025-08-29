// Copyright (c) 2025 MicroGuy / Goldcoin Developers  
// Proof of Participation Qt 6.9 Widgets Implementation

#include "qt_pop_widgets.h"
#include <QtWidgets/QApplication>
#include <QtWidgets/QMessageBox>
#include <QtCore/QSettings>
#include <QtCore/QStandardPaths>
#include <QtGui/QFont>
#include <QtGui/QIcon>
#include <format>

namespace goldcoin::qt {

// ParticipationOverviewWidget Implementation
ParticipationOverviewWidget::ParticipationOverviewWidget(QWidget *parent)
    : QWidget(parent)
    , m_mainLayout(std::make_unique<QVBoxLayout>(this))
    , m_updateTimer(std::make_unique<QTimer>(this)) {
    
    setupUI();
    
    // Connect timer for regular updates
    connect(m_updateTimer.get(), &QTimer::timeout,
            this, &ParticipationOverviewWidget::updateParticipationStatus);
    m_updateTimer->start(10000); // Update every 10 seconds
    
    // Initial update
    updateParticipationStatus();
}

void ParticipationOverviewWidget::setupUI() {
    setWindowTitle("Proof of Participation - Overview");
    
    createParticipationSection();
    createStatsSection();  
    createControlSection();
    
    setLayout(m_mainLayout.get());
}

void ParticipationOverviewWidget::createParticipationSection() {
    m_participationGroup = new QGroupBox("Your Participation Status", this);
    auto *layout = new QGridLayout(m_participationGroup);
    
    // Stake level
    layout->addWidget(new QLabel("Current Stake:"), 0, 0);
    m_stakeLevelLabel = new QLabel("Calculating...", this);
    m_stakeLevelLabel->setStyleSheet("font-weight: bold; color: #2E8B57;");
    layout->addWidget(m_stakeLevelLabel, 0, 1);
    
    // Coin age
    layout->addWidget(new QLabel("Coin Age:"), 1, 0);
    m_coinAgeLabel = new QLabel("Calculating...", this);
    layout->addWidget(m_coinAgeLabel, 1, 1);
    
    // Eligibility status
    layout->addWidget(new QLabel("Eligibility:"), 2, 0);
    m_eligibilityLabel = new QLabel("Checking...", this);
    layout->addWidget(m_eligibilityLabel, 2, 1);
    
    // Maturity progress
    layout->addWidget(new QLabel("Maturity Progress:"), 3, 0);
    m_maturityProgress = new QProgressBar(this);
    m_maturityProgress->setRange(0, 1440); // 1440 blocks for maturity
    layout->addWidget(m_maturityProgress, 3, 1);
    
    m_mainLayout->addWidget(m_participationGroup);
}

void ParticipationOverviewWidget::createStatsSection() {
    m_networkGroup = new QGroupBox("Network Statistics", this);
    auto *layout = new QGridLayout(m_networkGroup);
    
    // Participants count
    layout->addWidget(new QLabel("Active Participants:"), 0, 0);
    m_participantsLabel = new QLabel("Loading...", this);
    m_participantsLabel->setStyleSheet("font-weight: bold;");
    layout->addWidget(m_participantsLabel, 0, 1);
    
    // Block time
    layout->addWidget(new QLabel("Average Block Time:"), 1, 0);
    m_blockTimeLabel = new QLabel("Loading...", this);
    layout->addWidget(m_blockTimeLabel, 1, 1);
    
    // Decentralization index
    layout->addWidget(new QLabel("Decentralization Index:"), 2, 0);
    m_decentralizationLabel = new QLabel("Loading...", this);
    layout->addWidget(m_decentralizationLabel, 2, 1);
    
    m_mainLayout->addWidget(m_networkGroup);
}

void ParticipationOverviewWidget::createControlSection() {
    m_controlGroup = new QGroupBox("Participation Control", this);
    auto *layout = new QHBoxLayout(m_controlGroup);
    
    // Control buttons
    m_checkEligibilityBtn = new QPushButton("Check Eligibility", this);
    m_startParticipationBtn = new QPushButton("Start Participating", this);
    m_stopParticipationBtn = new QPushButton("Stop Participating", this);
    
    // Style buttons
    m_checkEligibilityBtn->setStyleSheet("QPushButton { background-color: #4CAF50; color: white; padding: 8px; }");
    m_startParticipationBtn->setStyleSheet("QPushButton { background-color: #2196F3; color: white; padding: 8px; }");
    m_stopParticipationBtn->setStyleSheet("QPushButton { background-color: #f44336; color: white; padding: 8px; }");
    
    layout->addWidget(m_checkEligibilityBtn);
    layout->addWidget(m_startParticipationBtn);
    layout->addWidget(m_stopParticipationBtn);
    
    // Connect signals
    connect(m_checkEligibilityBtn, &QPushButton::clicked,
            this, &ParticipationOverviewWidget::onCheckEligibilityClicked);
    connect(m_startParticipationBtn, &QPushButton::clicked,
            this, &ParticipationOverviewWidget::onStartParticipationClicked);
    connect(m_stopParticipationBtn, &QPushButton::clicked,
            this, &ParticipationOverviewWidget::onStopParticipationClicked);
    
    m_mainLayout->addWidget(m_controlGroup);
}

void ParticipationOverviewWidget::updateParticipationStatus() {
    // In production, this would query the actual wallet
    // For now, simulate data
    
    // Update stake level
    qint64 currentStake = 5000 * 100000000; // 5000 GLC in satoshis
    m_stakeLevelLabel->setText(QString("%1 GLC").arg(currentStake / 100000000.0, 0, 'f', 2));
    
    if (currentStake >= 1000 * 100000000) {
        m_stakeLevelLabel->setStyleSheet("font-weight: bold; color: #2E8B57;"); // Green
    } else {
        m_stakeLevelLabel->setStyleSheet("font-weight: bold; color: #DC143C;"); // Red
    }
    
    // Update coin age (simulate)
    int coinAgeBlocks = 800; // Example
    m_coinAgeLabel->setText(QString("%1 blocks (%2 days)").arg(coinAgeBlocks).arg(coinAgeBlocks / 720.0, 0, 'f', 1));
    
    // Update maturity progress
    m_maturityProgress->setValue(coinAgeBlocks);
    
    // Update eligibility
    bool eligible = (currentStake >= 1000 * 100000000) && (coinAgeBlocks >= 1440);
    if (eligible) {
        m_eligibilityLabel->setText("✓ Eligible to Participate");
        m_eligibilityLabel->setStyleSheet("color: #2E8B57; font-weight: bold;");
        m_startParticipationBtn->setEnabled(true);
    } else {
        m_eligibilityLabel->setText("✗ Not Yet Eligible");  
        m_eligibilityLabel->setStyleSheet("color: #DC143C; font-weight: bold;");
        m_startParticipationBtn->setEnabled(false);
    }
}

void ParticipationOverviewWidget::updateNetworkStats() {
    // Simulate network statistics
    m_participantsLabel->setText("1,247 active");
    m_blockTimeLabel->setText("119.3 seconds");
    m_decentralizationLabel->setText("87.3% (Excellent)");
}

void ParticipationOverviewWidget::onCheckEligibilityClicked() {
    QString message;
    qint64 currentStake = 5000 * 100000000; // Simulated
    int coinAge = 800; // Simulated
    
    message += QString("Current Stake: %1 GLC\n").arg(currentStake / 100000000.0, 0, 'f', 2);
    message += QString("Required: 1000 GLC (%1)\n\n").arg(currentStake >= 1000 * 100000000 ? "✓" : "✗");
    
    message += QString("Coin Age: %1 blocks\n").arg(coinAge);
    message += QString("Required: 1440 blocks (%1)\n\n").arg(coinAge >= 1440 ? "✓" : "✗");
    
    if (currentStake >= 1000 * 100000000 && coinAge >= 1440) {
        message += "🎉 You are eligible to participate in Proof of Participation!\n\n";
        message += "Click 'Start Participating' to begin earning block rewards.";
    } else {
        message += "⏳ Not yet eligible. ";
        if (currentStake < 1000 * 100000000) {
            message += QString("Need %1 more GLC. ").arg((1000 * 100000000 - currentStake) / 100000000.0, 0, 'f', 2);
        }
        if (coinAge < 1440) {
            message += QString("Need %1 more blocks (%2 days).").arg(1440 - coinAge).arg((1440 - coinAge) / 720.0, 0, 'f', 1);
        }
    }
    
    QMessageBox::information(this, "Participation Eligibility Check", message);
}

void ParticipationOverviewWidget::onStartParticipationClicked() {
    auto reply = QMessageBox::question(this, "Start Participation",
                                     "Start participating in Proof of Participation consensus?\n\n"
                                     "Your wallet will compete in the block lottery every 2 minutes.\n"
                                     "Rewards will be earned when you win blocks.",
                                     QMessageBox::Yes | QMessageBox::No);
    
    if (reply == QMessageBox::Yes) {
        // In production, this would start the PoP participation
        QMessageBox::information(this, "Participation Started",
                                "🎉 You are now participating in Proof of Participation!\n\n"
                                "Your wallet will automatically compete for block rewards.\n"
                                "Good luck!");
        
        m_startParticipationBtn->setEnabled(false);
        m_stopParticipationBtn->setEnabled(true);
    }
}

void ParticipationOverviewWidget::onStopParticipationClicked() {
    auto reply = QMessageBox::question(this, "Stop Participation",
                                     "Stop participating in Proof of Participation?\n\n"
                                     "You will no longer compete for block rewards.",
                                     QMessageBox::Yes | QMessageBox::No);
    
    if (reply == QMessageBox::Yes) {
        // In production, this would stop PoP participation
        QMessageBox::information(this, "Participation Stopped",
                                "Participation stopped. You can restart anytime.");
        
        m_startParticipationBtn->setEnabled(true);
        m_stopParticipationBtn->setEnabled(false);
    }
}

// ZeroFeeAssistantWidget Implementation  
ZeroFeeAssistantWidget::ZeroFeeAssistantWidget(QWidget *parent)
    : QWidget(parent)
    , m_mainLayout(std::make_unique<QVBoxLayout>(this))
    , m_marketTimer(std::make_unique<QTimer>(this)) {
    
    setupUI();
    
    // Update market stats regularly
    connect(m_marketTimer.get(), &QTimer::timeout,
            this, &ZeroFeeAssistantWidget::updateFeeMarket);
    m_marketTimer->start(30000); // Update every 30 seconds
    
    // Initial update
    updateFeeMarket();
}

void ZeroFeeAssistantWidget::setupUI() {
    setWindowTitle("Zero-Fee Assistant");
    
    createFeeChecker();
    createMarketStats();
    createPriorityCalculator();
    
    setLayout(m_mainLayout.get());
}

void ZeroFeeAssistantWidget::createFeeChecker() {
    m_checkerGroup = new QGroupBox("Transaction Fee Checker", this);
    auto *layout = new QGridLayout(m_checkerGroup);
    
    layout->addWidget(new QLabel("Your Priority Score:"), 0, 0);
    m_priorityScoreLabel = new QLabel("Calculating...", this);
    m_priorityScoreLabel->setStyleSheet("font-weight: bold;");
    layout->addWidget(m_priorityScoreLabel, 0, 1);
    
    layout->addWidget(new QLabel("Free Transaction:"), 1, 0);
    m_freeEligibleLabel = new QLabel("Checking...", this);
    layout->addWidget(m_freeEligibleLabel, 1, 1);
    
    layout->addWidget(new QLabel("Estimated Fee:"), 2, 0);
    m_estimatedFeeLabel = new QLabel("Calculating...", this);
    layout->addWidget(m_estimatedFeeLabel, 2, 1);
    
    m_checkFeeBtn = new QPushButton("Check Current Transaction", this);
    m_checkFeeBtn->setStyleSheet("QPushButton { background-color: #4CAF50; color: white; padding: 8px; }");
    layout->addWidget(m_checkFeeBtn, 3, 0, 1, 2);
    
    connect(m_checkFeeBtn, &QPushButton::clicked,
            this, &ZeroFeeAssistantWidget::onCheckFeeClicked);
    
    m_mainLayout->addWidget(m_checkerGroup);
}

void ZeroFeeAssistantWidget::createMarketStats() {
    m_marketGroup = new QGroupBox("Fee Market Statistics", this);
    auto *layout = new QGridLayout(m_marketGroup);
    
    layout->addWidget(new QLabel("Free Zone Pressure:"), 0, 0);
    m_freeZonePressureLabel = new QLabel("Loading...", this);
    layout->addWidget(m_freeZonePressureLabel, 0, 1);
    
    layout->addWidget(new QLabel("Network Congestion:"), 1, 0);
    m_congestionProgress = new QProgressBar(this);
    m_congestionProgress->setRange(0, 100);
    layout->addWidget(m_congestionProgress, 1, 1);
    
    layout->addWidget(new QLabel("Median Fee (Last Block):"), 2, 0);
    m_medianFeeLabel = new QLabel("Loading...", this);
    layout->addWidget(m_medianFeeLabel, 2, 1);
    
    m_refreshMarketBtn = new QPushButton("Refresh Market Data", this);
    layout->addWidget(m_refreshMarketBtn, 3, 0, 1, 2);
    
    connect(m_refreshMarketBtn, &QPushButton::clicked,
            this, &ZeroFeeAssistantWidget::onRefreshMarketClicked);
    
    m_mainLayout->addWidget(m_marketGroup);
}

void ZeroFeeAssistantWidget::createPriorityCalculator() {
    m_priorityGroup = new QGroupBox("Priority Information", this);
    auto *layout = new QVBoxLayout(m_priorityGroup);
    
    m_coinAgeInfoLabel = new QLabel("💡 Coin Age: Older coins have higher priority", this);
    m_coinAgeInfoLabel->setWordWrap(true);
    layout->addWidget(m_coinAgeInfoLabel);
    
    m_txHistoryInfoLabel = new QLabel("📊 Transaction History: More activity = higher priority", this);
    m_txHistoryInfoLabel->setWordWrap(true);
    layout->addWidget(m_txHistoryInfoLabel);
    
    m_thresholdInfoLabel = new QLabel("🎯 Free Threshold: Priority > 57,600,000 = FREE transaction", this);
    m_thresholdInfoLabel->setWordWrap(true);
    m_thresholdInfoLabel->setStyleSheet("font-weight: bold; color: #2E8B57;");
    layout->addWidget(m_thresholdInfoLabel);
    
    m_mainLayout->addWidget(m_priorityGroup);
}

void ZeroFeeAssistantWidget::updateFeeMarket() {
    // Simulate market data - in production this would query actual fee market
    int congestion = 23; // 23% congestion
    
    m_freeZonePressureLabel->setText(QString("%1% utilized").arg(congestion));
    m_congestionProgress->setValue(congestion);
    
    // Color code congestion
    if (congestion < 50) {
        m_congestionProgress->setStyleSheet("QProgressBar::chunk { background-color: #4CAF50; }"); // Green
    } else if (congestion < 80) {
        m_congestionProgress->setStyleSheet("QProgressBar::chunk { background-color: #FF9800; }"); // Orange
    } else {
        m_congestionProgress->setStyleSheet("QProgressBar::chunk { background-color: #f44336; }"); // Red
    }
    
    m_medianFeeLabel->setText("0.00001 GLC");
    
    // Update priority score (simulate based on wallet)
    double priorityScore = 89234567.0; // Simulated - above free threshold
    m_priorityScoreLabel->setText(QString("%1").arg(static_cast<qint64>(priorityScore)));
    
    if (priorityScore >= 57600000.0) {
        m_freeEligibleLabel->setText("✓ YES - Transaction will be FREE");
        m_freeEligibleLabel->setStyleSheet("color: #2E8B57; font-weight: bold;");
        m_estimatedFeeLabel->setText("0 GLC (FREE!)");
        m_estimatedFeeLabel->setStyleSheet("color: #2E8B57; font-weight: bold;");
    } else {
        m_freeEligibleLabel->setText("✗ No - Fee required");
        m_freeEligibleLabel->setStyleSheet("color: #DC143C;");
        m_estimatedFeeLabel->setText("~0.00001 GLC");
        m_estimatedFeeLabel->setStyleSheet("color: #DC143C;");
    }
}

void ZeroFeeAssistantWidget::onCheckFeeClicked() {
    QString message = "Current Transaction Analysis:\n\n";
    
    double priority = 89234567.0; // Simulated
    message += QString("Priority Score: %1\n").arg(static_cast<qint64>(priority));
    message += QString("Free Threshold: 57,600,000\n\n");
    
    if (priority >= 57600000.0) {
        message += "🎉 Your transaction qualifies for the FREE zone!\n\n";
        message += "• Goes into first 5% of block (1.6MB)\n";
        message += "• No fees required\n";
        message += "• Confirmed within 1-2 blocks\n";
    } else {
        message += "💰 Your transaction requires a small fee.\n\n";
        message += "Suggested fee: 0.00001 GLC\n";
        message += "Reason: Priority score below free threshold\n\n";
        message += "💡 Tip: Let your coins age longer for higher priority!";
    }
    
    QMessageBox::information(this, "Transaction Fee Analysis", message);
}

void ZeroFeeAssistantWidget::onRefreshMarketClicked() {
    m_freeZonePressureLabel->setText("Refreshing...");
    m_medianFeeLabel->setText("Refreshing...");
    
    // Simulate refresh delay
    QTimer::singleShot(1000, this, [this]() {
        updateFeeMarket();
    });
}

void ZeroFeeAssistantWidget::checkTransactionFee(qint64 amount, const QString& address) {
    // This would be called from the send coins dialog
    // Simulate fee calculation
    bool likelyFree = true; // Based on current priority
    qint64 estimatedFee = likelyFree ? 0 : 1000; // 0.00001 GLC if not free
    
    emit feeEstimateReady(estimatedFee, likelyFree);
}

} // namespace goldcoin::qt

#include "qt_pop_widgets.moc"