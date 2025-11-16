/**
 * @file MainWindow.cpp
 * @brief Implementation der Main Window View
 */

#include "MainWindow.h"
#include "ui_MainWindow.h"

#include <QApplication>
#include <QFile>
#include <QHBoxLayout>
#include <QIODevice>
#include <QLabel>
#include <QListWidget>
#include <QMessageBox>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QProcess>
#include <QProcessEnvironment>
#include <QRegularExpression>
#include <QShortcut>
#include <QSplitter>
#include <QStatusBar>
#include <QThread>
#include <QTimer>
#include <QVBoxLayout>
#include <QtCharts/QChart>
#include <QtCharts/QLineSeries>

namespace MachinaMindAIAgent {

MainWindow::MainWindow(MainPresenter* presenter, QWidget* parent)
    : QMainWindow(parent), ui(new Ui::MainWindow), m_presenter(presenter), 
      m_backendProcess(nullptr), m_healthCheckTimer(nullptr), m_healthCheckAttempts(0) {
    ui->setupUi(this);
    setupUi();
    setupConnections();
    // Backend starten
    startBackend();
}

MainWindow::~MainWindow() {
    // Backend beenden
    if (m_healthCheckTimer) {
        delete m_healthCheckTimer;
        m_healthCheckTimer = nullptr;
    }
    
    delete ui;
}

// ==================== UI Setup ====================

void MainWindow::setupUi() {
    setWindowTitle("MachinaMindAIAgent - Industrial Machine Intelligence");
    resize(1100, 800);
    setFont(QFont("Roboto", 12));
    // Top Bar (Connection + Machine Selection)
    ui->topLayout->addWidget(new QLabel("Server:"), 0, 0);

    m_serverInput = new QLineEdit("http://localhost:8000");
    ui->topLayout->addWidget(m_serverInput, 0, 1);

    auto* connectButton = new QPushButton("Verbinden");
    ui->topLayout->addWidget(connectButton, 0, 2);
    connect(connectButton, &QPushButton::clicked, this, &MainWindow::onConnectButtonClicked);
    ui->topLayout->addItem(new QSpacerItem(0, 0, QSizePolicy::Minimum, QSizePolicy::Minimum), 0, 3);

    ui->topLayout->addWidget(new QLabel("Maschine:"), 0, 3);

    m_machineComboBox = new QComboBox();
    ui->topLayout->addWidget(m_machineComboBox, 0, 5);
    m_machineComboBox->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);

    auto* refreshButton = new QPushButton("Aktualisieren");
    ui->topLayout->addWidget(refreshButton, 0, 6);
    connect(refreshButton, &QPushButton::clicked, this, &MainWindow::onRefreshButtonClicked);
    ui->topLayout->addItem(new QSpacerItem(0, 0, QSizePolicy::Expanding, QSizePolicy::Minimum), 0, 7);
    
    // Chat Toggle Button (zeigt/versteckt Chat-Panel)
    auto* chatToggleButton = new QPushButton();
    chatToggleButton->setIcon(QIcon(":/icons/aiChat.png"));
    chatToggleButton->setIconSize(QSize(30, 30));
    chatToggleButton->setToolTip("Chat ein-/ausblenden");
    chatToggleButton->setProperty("themeButton", true);
    ui->topLayout->addWidget(chatToggleButton, 0, 8);
    
    // Dark/Light Mode Toggle Button (oben rechts)
    auto* themeButton = new QPushButton();
    themeButton->setIcon(QIcon(":/icons/light.png"));
    themeButton->setIconSize(QSize(30, 30));
    themeButton->setToolTip("Theme wechseln (Dark/Light)");
    themeButton->setProperty("themeButton", true);
    ui->topLayout->addWidget(themeButton, 0, 9);
    
    // Theme Toggle Funktionalität
    connect(themeButton, &QPushButton::clicked, [this, themeButton]() {
        static bool isDarkMode = true;
        isDarkMode = !isDarkMode;
        
        if (isDarkMode) {
            // Dark Theme laden
            QFile darkFile(":/styles/dark_theme.qss");
            if (!darkFile.open(QIODevice::ReadOnly)) {
                darkFile.setFileName("styles/dark_theme.qss");
                darkFile.open(QIODevice::ReadOnly);
            }
            QString darkStyle = darkFile.readAll();
            qApp->setStyleSheet(darkStyle);
            themeButton->setIcon(QIcon(":/icons/light.png"));
            
            // Chat Header Icon für Dark Mode
            if (m_chatHeaderLabel) {
                QPixmap chatIcon(":/icons/chat-bot.png");
                if (!chatIcon.isNull()) {
                    m_chatHeaderLabel->setPixmap(chatIcon.scaled(32, 32, Qt::KeepAspectRatio, Qt::SmoothTransformation));
                }
            }
        } else {
            // Light Theme laden
            QFile lightFile(":/styles/light_theme.qss");
            if (!lightFile.open(QIODevice::ReadOnly)) {
                lightFile.setFileName("styles/light_theme.qss");
                lightFile.open(QIODevice::ReadOnly);
            }
            QString lightStyle = lightFile.readAll();
            qApp->setStyleSheet(lightStyle);
            themeButton->setIcon(QIcon(":/icons/night-mode.png"));
            
            // Chat Header Icon für Light Mode
            if (m_chatHeaderLabel) {
                QPixmap chatIcon(":/icons/chat-botDark.png");
                if (!chatIcon.isNull()) {
                    m_chatHeaderLabel->setPixmap(chatIcon.scaled(32, 32, Qt::KeepAspectRatio, Qt::SmoothTransformation));
                }
            }
        }
    });

    // Chart
    m_chartView = new QChartView();
    m_chartView->setRenderHint(QPainter::Antialiasing);
    ui->leftLayout->addWidget(m_chartView);

    // Events Table
    m_eventsTable = new QTableWidget();
    m_eventsTable->setColumnCount(4);
    m_eventsTable->setAlternatingRowColors(true);
    m_eventsTable->setHorizontalHeaderLabels({"Zeit", "Level", "Nachricht", "Maschine"});
    m_eventsTable->setMaximumHeight(250);
    m_eventsTable->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    m_eventsTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
    
    // Grid-Linien und Fokus-Rahmen entfernen
    m_eventsTable->setShowGrid(false);
    m_eventsTable->setFocusPolicy(Qt::NoFocus);
    
    // Ganze Zeile auswählen (nicht einzelne Zellen)
    m_eventsTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_eventsTable->setSelectionMode(QAbstractItemView::SingleSelection);
    
    // Ecke zwischen Scrollbars stylen
    m_eventsTable->setCornerButtonEnabled(false);
    
    // Spaltenbreiten konfigurieren
    m_eventsTable->horizontalHeader()->setSectionResizeMode(0, QHeaderView::ResizeToContents); // Zeit
    m_eventsTable->horizontalHeader()->setSectionResizeMode(1, QHeaderView::ResizeToContents); // Level
    m_eventsTable->horizontalHeader()->setSectionResizeMode(2, QHeaderView::Stretch);          // Nachricht
    m_eventsTable->horizontalHeader()->setSectionResizeMode(3, QHeaderView::ResizeToContents); // Maschine

    ui->leftLayout->addWidget(m_eventsTable);

    // Chat Toggle Funktionalität
    connect(chatToggleButton, &QPushButton::clicked, [this, chatToggleButton]() {
        static bool isChatVisible = false;
        isChatVisible = !isChatVisible;
        
        ui->rightWidget->setVisible(isChatVisible);
        chatToggleButton->setToolTip(isChatVisible ? "Chat ausblenden" : "Chat einblenden");
    });
    ui->rightWidget->setVisible(false);

    // AI Assistant Header mit Icon und Text
    QWidget* chatHeaderWidget = new QWidget();
    QHBoxLayout* chatHeaderLayout = new QHBoxLayout(chatHeaderWidget);
    chatHeaderLayout->setContentsMargins(5, 5, 5, 5);
    chatHeaderLayout->setSpacing(10);
    
    m_chatHeaderLabel = new QLabel();
    QPixmap chatIcon(":/icons/chat-bot.png");  // Startet mit Dark Mode Icon
    if (!chatIcon.isNull()) {
        m_chatHeaderLabel->setPixmap(chatIcon.scaled(32, 32, Qt::KeepAspectRatio, Qt::SmoothTransformation));
    } else {
        m_chatHeaderLabel->setText("🤖");  // Fallback emoji
    }
    
    QLabel* chatHeaderText = new QLabel("AI Assistant");
    QFont headerFont;
    headerFont.setPointSize(12);
    headerFont.setBold(true);
    chatHeaderText->setFont(headerFont);
    
    chatHeaderLayout->addWidget(m_chatHeaderLabel);
    chatHeaderLayout->addWidget(chatHeaderText);
    chatHeaderLayout->addStretch();
    
    ui->rightLayout->addWidget(chatHeaderWidget);

    m_chatDisplay = new QListWidget();
    m_chatDisplay->setFrameStyle(QFrame::NoFrame);
    m_chatDisplay->setSpacing(4);
    m_chatDisplay->setWordWrap(true);
    m_chatDisplay->setSelectionMode(QAbstractItemView::NoSelection);
    m_chatDisplay->setFocusPolicy(Qt::NoFocus);
    m_chatDisplay->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    m_chatDisplay->setVerticalScrollMode(QAbstractItemView::ScrollPerPixel);
    m_chatDisplay->setResizeMode(QListView::Adjust);
    m_chatDisplay->setSizeAdjustPolicy(QAbstractScrollArea::AdjustToContents);
    
    // Scroll-Geschwindigkeit in kleinen Schritten (5 Pixel pro Schritt)
    m_chatDisplay->verticalScrollBar()->setSingleStep(5);
    m_chatDisplay->verticalScrollBar()->setPageStep(100);
    
    ui->rightLayout->addWidget(m_chatDisplay);
    
    // Willkommensnachricht hinzufügen
    appendChatMessage("System", 
        "Willkommen! Ich bin Ihr KI-Assistent für industrielle Maschinen-Intelligenz.\n\n"
        "Wählen Sie eine Maschine aus und ich kann Ihnen bei der Analyse helfen!");

    // Chat Input - QTextEdit für mehrzeilige Eingabe
    m_chatInput = new QTextEdit();
    m_chatInput->setPlaceholderText("Frage eingeben...");
    m_chatInput->setMinimumHeight(32);
    m_chatInput->setMaximumHeight(120);
    m_chatInput->setFixedHeight(50); // Initial klein starten
    m_chatInput->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    m_chatInput->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    
    // Höhe automatisch anpassen bei Texteingabe
    connect(m_chatInput, &QTextEdit::textChanged, [this]() {
        QTextDocument* doc = m_chatInput->document();
        int docHeight = static_cast<int>(doc->size().height()) + 16; // Mehr Padding
        int minH = 50, maxH = 120;
        int newHeight = std::min(std::max(docHeight, minH), maxH);
        m_chatInput->setFixedHeight(newHeight);
    });
    
    ui->rightLayout->addWidget(m_chatInput);
    // Buttons unter dem Chat Input
    m_sendButton = new QPushButton("Senden ➤");
    auto* analyzeButton = new QPushButton("Analysieren");

    ui->rightLayout->addWidget(m_sendButton);
    ui->rightLayout->addWidget(analyzeButton);

    // Status Bar
    statusBar()->showMessage("Bereit");

    // Splitter-Größen setzen: leftWidget nimmt den Rest, rightWidget bekommt 400px
    QList<int> sizes;
    sizes << (width() - 400) << 400;  // leftWidget | rightWidget (400px)
    // Enable hover tracking for splitter handle
    ui->splitter->handle(1)->setAttribute(Qt::WA_Hover, true);
    ui->splitter->setSizes(sizes);

    // Store button references
    connect(m_sendButton, &QPushButton::clicked, this, &MainWindow::onSendButtonClicked);
    connect(analyzeButton, &QPushButton::clicked, this, &MainWindow::onAnalyzeButtonClicked);
}

void MainWindow::setupConnections() {
    connect(m_machineComboBox, QOverload<int>::of(&QComboBox::currentIndexChanged), this,
            &MainWindow::onMachineSelectionChanged);

    // Enter zum Senden, Shift+Enter für neue Zeile
    m_chatInput->installEventFilter(this);
}

// ==================== Slots ====================

void MainWindow::onMachineSelectionChanged(int index) {
    if (index >= 0) {
        int machineId = m_machineComboBox->currentData().toInt();
        m_presenter->onMachineSelected(machineId);
    }
}

void MainWindow::onRefreshButtonClicked() {
    m_presenter->onRefreshClicked();
}

void MainWindow::onSendButtonClicked() {
    QString message = m_chatInput->toPlainText();
    if (!message.isEmpty()) {
        // User-Nachricht anzeigen
        appendChatMessage("User", message);
        
        // Status-Nachricht anzeigen
        appendChatMessage("System", "⏳ Denke nach...");
        
        // An Presenter senden
        m_presenter->onSendChatMessage(message);
        m_chatInput->clear();
    }
}

void MainWindow::onAnalyzeButtonClicked() {
    m_presenter->onAnalyzeClicked();
}

void MainWindow::onConnectButtonClicked() {
    m_presenter->onConnectClicked(m_serverInput->text());
}

// ==================== Event Filter ====================

bool MainWindow::eventFilter(QObject* obj, QEvent* event) {
    if (obj == m_chatInput && event->type() == QEvent::KeyPress) {
        QKeyEvent* keyEvent = static_cast<QKeyEvent*>(event);
        
        // Enter ohne Shift → Senden
        if (keyEvent->key() == Qt::Key_Return || keyEvent->key() == Qt::Key_Enter) {
            if (!(keyEvent->modifiers() & Qt::ShiftModifier)) {
                onSendButtonClicked();
                return true; // Event gefiltert, nicht weiter propagieren
            }
            // Shift+Enter → Neue Zeile (Standard-Verhalten)
        }
    }
    
    return QMainWindow::eventFilter(obj, event);
}

// ==================== IMainView Interface ====================

void MainWindow::closeEvent(QCloseEvent *event) {
    qDebug() << "Close event triggered. Stopping backend...";
    stopBackend(); // Backend synchron beenden
    qDebug() << "Backend stopped. Accepting close event.";
    event->accept(); // Fenster jetzt schließen
}


void MainWindow::showError(const QString& message) {
    QMessageBox::critical(this, "Fehler", message);
    statusBar()->showMessage("❌ " + message, 5000);
}

void MainWindow::showInfo(const QString& message) {
    statusBar()->showMessage("ℹ️ " + message, 3000);
}

void MainWindow::setConnectionStatus(bool connected) {
    QString status = connected ? "✅ Verbunden" : "❌ Getrennt";
    statusBar()->showMessage(status);
}

void MainWindow::removeLastChatMessage() {
    int count = m_chatDisplay->count();
    if (count > 0) {
        QListWidgetItem* item = m_chatDisplay->takeItem(count - 1);
        delete item;
    }
}

void MainWindow::appendChatMessage(const QString& role, const QString& message) {
    // Erstelle ein Widget für die Nachricht
    QWidget* messageWidget = new QWidget();
    messageWidget->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Minimum);
    QHBoxLayout* layout = new QHBoxLayout(messageWidget);
    layout->setContentsMargins(8, 5, 8, 5);
    layout->setSpacing(0);
    
    // Erstelle das Label für die Bubble
    QLabel* bubble = new QLabel();
    bubble->setWordWrap(true);
    bubble->setTextFormat(Qt::RichText);
    bubble->setTextInteractionFlags(Qt::TextSelectableByMouse);
    bubble->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Minimum);
    
    // Maximale Breite für Bubbles (80% der Chat-Breite)
    int maxBubbleWidth = m_chatDisplay->width() * 0.8;
    bubble->setMaximumWidth(maxBubbleWidth);
    
    // Text verarbeiten
    QString processedMessage = message.toHtmlEscaped().replace("\n", "<br>");
    processedMessage = processedMessage.replace(QRegularExpression("\\*\\*([^*]+)\\*\\*"), "<b>\\1</b>");
    
    if (role == "User") {
        // User Bubble - Rechts, blau
        layout->addStretch(3);
        
        bubble->setText(QString(
            "<div style='padding: 2px;'>"
            "<div style='font-size: 9pt; margin-bottom: 4px;'><b>🧑 You</b></div>"
            "<div style='font-size: 10pt;'>%1</div>"
            "</div>"
        ).arg(processedMessage));
        
        bubble->setStyleSheet(
            "QLabel { "
            "  background-color: rgba(52, 152, 219, 0.3); "
            "  padding: 10px 12px; "
            "  border-radius: 15px; "
            "}"
        );
        layout->addWidget(bubble, 7);
        
    } else if (role == "System") {
        // System Bubble - Zentriert, grau
        layout->addStretch(1);
        
        bubble->setText(QString(
            "<div style='padding: 2px; text-align: center;'>"
            "<div style='font-size: 9pt; color: #888;'>%1</div>"
            "</div>"
        ).arg(processedMessage));
        
        bubble->setStyleSheet(
            "QLabel { "
            "  background-color: rgba(128, 128, 128, 0.2); "
            "  padding: 6px 12px; "
            "  border-radius: 15px; "
            "}"
        );
        layout->addWidget(bubble, 8);
        layout->addStretch(1);
        
    } else {
        // AI/Assistant Bubble - Links, grün/blau
        bubble->setText(QString(
            "<div style='padding: 2px;'>"
            "<div style='font-size: 9pt; margin-bottom: 4px;'><b>🤖 AI Assistant</b></div>"
            "<div style='font-size: 10pt;'>%1</div>"
            "</div>"
        ).arg(processedMessage));
        
        bubble->setStyleSheet(
            "QLabel { "
            "  background-color: rgba(146, 201, 255, 0.25); "
            "  padding: 10px 12px; "
            "  border-radius: 15px; "
            "}"
        );
        layout->addWidget(bubble, 7);
        layout->addStretch(3);
    }
    
    // Größe korrekt berechnen
    messageWidget->adjustSize();
    QSize widgetSize = messageWidget->sizeHint();
    if (widgetSize.height() < 80) {
        widgetSize.setHeight(80); // Mindesthöhe für kleine Nachrichten
    }
    // Füge zur Liste hinzu
    QListWidgetItem* item = new QListWidgetItem(m_chatDisplay);
    item->setSizeHint(widgetSize);
    m_chatDisplay->addItem(item);
    m_chatDisplay->setItemWidget(item, messageWidget);
    
    // Auto-scroll nach unten
    m_chatDisplay->scrollToBottom();
}

void MainWindow::updateMachineList(const QVector<Machine>& machines) {
    m_machineComboBox->clear();

    for (const auto& machine : machines) {
        m_machineComboBox->addItem(QString("%1 (%2)").arg(machine.name, machine.type), machine.id);
    }

    showInfo(QString("%1 Maschinen geladen").arg(machines.size()));
}

void MainWindow::updateChart(const QVector<Measurement>& measurements) {
    auto* chart = new QChart();
    chart->setTitle("Sensor-Daten (Letzte Messungen)");

    // Group by sensor type
    QMap<QString, QLineSeries*> seriesBySensor;

    for (const auto& m : measurements) {
        if (!seriesBySensor.contains(m.sensorType)) {
            auto* series = new QLineSeries();
            series->setName(m.sensorType);
            seriesBySensor[m.sensorType] = series;
        }

        qint64 timestamp = m.timestamp.toMSecsSinceEpoch();
        seriesBySensor[m.sensorType]->append(timestamp, m.value);
    }

    for (auto* series : seriesBySensor) {
        chart->addSeries(series);
    }

    chart->createDefaultAxes();
    chart->legend()->setVisible(true);

    m_chartView->setChart(chart);
}

void MainWindow::updateEventsTable(const QVector<Event>& events) {
    m_eventsTable->setRowCount(events.size());

    for (int i = 0; i < events.size(); ++i) {
        const auto& event = events[i];

        m_eventsTable->setItem(i, 0,
                               new QTableWidgetItem(event.timestamp.toString("hh:mm:ss")));
        m_eventsTable->setItem(i, 1, new QTableWidgetItem(event.levelString()));
        m_eventsTable->setItem(i, 2, new QTableWidgetItem(event.message));
        m_eventsTable->setItem(i, 3, new QTableWidgetItem(QString::number(event.machineId)));

        // Color coding - Farben mit höherer Alpha für bessere Sichtbarkeit
        QColor color;
        QColor textColor = QColor(255, 255, 255); // Weißer Text standardmäßig
        switch (event.level) {
            case Event::Critical:
                color = QColor(255, 0, 0, 100);      // Rot mit mehr Deckkraft
                textColor = QColor(255, 255, 255);    // Weißer Text
                break;
            case Event::Error:
                color = QColor(255, 100, 0, 100);    // Orange mit mehr Deckkraft
                textColor = QColor(255, 255, 255);    // Weißer Text
                break;
            case Event::Warning:
                color = QColor(255, 200, 0, 100);    // Gelb mit mehr Deckkraft
                textColor = QColor(50, 50, 50);       // Dunkler Text für Kontrast
                break;
            default:
                color = QColor(52, 73, 94, 0);          // Standard Theme Hintergrund
                textColor = QColor(236, 240, 241);    // Standard Text
        }

        for (int col = 0; col < 4; ++col) {
            m_eventsTable->item(i, col)->setBackground(color);
            m_eventsTable->item(i, col)->setForeground(textColor);
        }
    }
    
    // Spaltenbreite nicht neu berechnen, damit die breiteste Spalte erhalten bleibt
    // m_eventsTable->resizeColumnsToContents();
}

void MainWindow::setAnalysisResult(const QString& summary, int anomalyCount) {
    QString message =
        QString("🔍 Analyse abgeschlossen:\n%1 Anomalien gefunden\n\n%2").arg(anomalyCount).arg(summary);

    appendChatMessage("System", message);
    showInfo(QString("Analyse: %1 Anomalien").arg(anomalyCount));
}

// ==================== Backend Management ====================

void MainWindow::startBackend() {
    // Prüfen, ob das Backend bereits läuft, bevor wir es starten
    QNetworkAccessManager* preCheckManager = new QNetworkAccessManager(this);
    QNetworkRequest request(QUrl("http://localhost:8000/health"));
    request.setTransferTimeout(1000); // Kurzer Timeout von 1 Sekunde

    QNetworkReply* reply = preCheckManager->get(request);

    connect(reply, &QNetworkReply::finished, this, [this, reply, preCheckManager]() {
        if (reply->error() == QNetworkReply::NoError) {
            // Backend läuft bereits!
            qDebug() << "Backend läuft bereits. Überspringe Startvorgang.";
            statusBar()->showMessage("✅ Backend bereits aktiv. Verbinde...", 3000);
            
            // Direkt zum Verbinden und Initialisieren übergehen
            if (m_presenter) {
                m_presenter->initialize();
            }
            QTimer::singleShot(500, this, &MainWindow::onConnectButtonClicked);
        } else {
            // Backend läuft nicht, starte es.
            startBackendProcess();
        }
        reply->deleteLater();
        preCheckManager->deleteLater();
    });
}

void MainWindow::startBackendProcess() {
    if (m_backendProcess) {
        return; // Backend läuft bereits
    }
    
    m_backendProcess = new QProcess(this);
    
    // Arbeitsverzeichnis auf backend/ setzen
    QString backendPath = QCoreApplication::applicationDirPath() + "/../../backend";
    m_backendProcess->setWorkingDirectory(backendPath);
    
    // Python direkt starten (OHNE cmd.exe)
#ifdef Q_OS_WIN
    QString program = backendPath + "/venv/Scripts/python.exe";
    QStringList arguments;
    arguments << "api/main.py";
#else
    QString program = backendPath + "/venv/bin/python";
    QStringList arguments;
    arguments << "api/main.py";
#endif
    
    qDebug() << "Python Pfad:" << program;
    qDebug() << "Backend Pfad:" << backendPath;

    // Environment setzen (Reload deaktivieren für Produktionsmodus)
    QProcessEnvironment env = QProcessEnvironment::systemEnvironment();
    env.insert("MACHINAMIND_RELOAD", "0");
    m_backendProcess->setProcessEnvironment(env);
    
    // Prozess-Output verbinden (für Debugging)
    connect(m_backendProcess, &QProcess::readyReadStandardOutput, this, [this]() {
        QString output = m_backendProcess->readAllStandardOutput();
        qDebug() << "Backend Output:" << output;
    });
    
    connect(m_backendProcess, &QProcess::readyReadStandardError, this, [this]() {
        QString error = m_backendProcess->readAllStandardError();
        qDebug() << "Backend Error:" << error;
    });
    
    // Prozess starten
    statusBar()->showMessage("🚀 Backend wird gestartet...", 0);
    m_backendProcess->start(program, arguments);
    if (m_backendProcess->waitForStarted(5000)) {
        qDebug() << "Backend-Prozess gestartet, PID:" << m_backendProcess->processId();
        
        // Health Check Timer initialisieren
        m_healthCheckAttempts = 0;
        if (!m_healthCheckTimer) {
            m_healthCheckTimer = new QTimer(this);
            connect(m_healthCheckTimer, &QTimer::timeout, this, &MainWindow::checkBackendHealth);
        }
        
        // Health Check alle 2 Sekunden
        statusBar()->showMessage("⏳ Warte auf Backend-Bereitschaft (RAG-Modell wird geladen, ~100s)...", 0);
        m_healthCheckTimer->start(2000);
    } else {
        showError("Backend konnte nicht gestartet werden!");
        delete m_backendProcess;
        m_backendProcess = nullptr;
    }
}

void MainWindow::checkBackendHealth() {
    m_healthCheckAttempts++;
    
    // Timeout nach 60 Versuchen (120 Sekunden)
    if (m_healthCheckAttempts > 60) {
        m_healthCheckTimer->stop();
        statusBar()->showMessage("❌ Backend-Start timeout nach 120 Sekunden", 5000);
        showError("Backend hat nicht innerhalb von 120 Sekunden geantwortet.\n"
                  "Das RAG-Modell benötigt möglicherweise länger zum Laden.");
        return;
    }
    
    // Status-Update alle 5 Versuche (10 Sekunden)
    if (m_healthCheckAttempts % 5 == 0) {
        statusBar()->showMessage(
            QString("⏳ Warte auf Backend... (%1s vergangen, RAG-Modell lädt...)").arg(m_healthCheckAttempts * 2),
            0
        );
    }
    
    // Health Check durchführen
    QNetworkAccessManager* manager = new QNetworkAccessManager(this);
    QNetworkRequest request(QUrl("http://localhost:8000/health"));
    request.setTransferTimeout(1000); // 1 Sekunde Timeout
    
    QNetworkReply* reply = manager->get(request);
    
    connect(reply, &QNetworkReply::finished, this, [this, reply, manager]() {
        if (reply->error() == QNetworkReply::NoError) {
            // Backend ist bereit!
            m_healthCheckTimer->stop();
            statusBar()->showMessage(
                QString("✅ Backend bereit nach %1 Sekunden!").arg(m_healthCheckAttempts * 2),
                5000
            );
            qDebug() << "Backend health check erfolgreich nach" << m_healthCheckAttempts << "Versuchen";
            
            // Presenter initialisieren (lädt Maschinen etc.)
            if (m_presenter) {
                m_presenter->initialize();
            }
            
            // Automatisch verbinden (nach kurzer Pause)
            QTimer::singleShot(500, this, &MainWindow::onConnectButtonClicked);
        } else {
            // Backend noch nicht bereit, weiter warten
            qDebug() << "Health check fehlgeschlagen (Versuch" << m_healthCheckAttempts << "):" << reply->errorString();
        }
        
        reply->deleteLater();
        manager->deleteLater();
    });
}

void MainWindow::stopBackend() {
    // Timer stoppen
    if (m_healthCheckTimer && m_healthCheckTimer->isActive()) {
        m_healthCheckTimer->stop();
    }
    
    if (m_backendProcess && m_backendProcess->state() == QProcess::Running) {
        statusBar()->showMessage("⏹️ Backend wird beendet...", 2000);
        qDebug() << "Beende Backend-Prozess, PID:" << m_backendProcess->processId();
        
        // Zuerst versuchen, den Prozess sauber zu beenden (terminate)
        m_backendProcess->terminate();
        
        // Synchron auf das Ende warten (max. 3 Sekunden)
        if (!m_backendProcess->waitForFinished(3000)) {
            qDebug() << "Backend hat nicht auf terminate reagiert. Erzwinge Beendigung (kill)...";
            // Wenn es nicht funktioniert, Prozess zwangsweise beenden (kill)
            m_backendProcess->kill();
            m_backendProcess->waitForFinished(1000); // Kurz warten, bis der Kill verarbeitet ist
        }
        delete m_backendProcess;
        m_backendProcess = nullptr;
        qDebug() << "Backend-Prozess beendet";
    }
}

}  // namespace MachinaMindAIAgent
