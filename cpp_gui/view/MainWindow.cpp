/**
 * @file MainWindow.cpp
 * @brief Implementation der Main Window View
 */

#include "MainWindow.h"

#include <QApplication>
#include <QFile>
#include <QHBoxLayout>
#include <QIODevice>
#include <QLabel>
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
    : QMainWindow(parent), ui(nullptr), m_presenter(presenter), 
      m_backendProcess(nullptr), m_healthCheckTimer(nullptr), m_healthCheckAttempts(0) {
    setupUi();
    setupConnections();
    // Backend starten
    startBackend();
}

MainWindow::~MainWindow() {
    // Backend beenden
    if (m_healthCheckTimer) { // Nur noch Timer aufräumen
        delete m_healthCheckTimer;
        m_healthCheckTimer = nullptr;
    }
    
    // ui is nullptr, no need to delete
}

// ==================== UI Setup ====================

void MainWindow::setupUi() {
    setWindowTitle("MachinaMindAIAgent - Industrial Machine Intelligence");
    resize(1400, 900);

    // Central Widget
    auto* centralWidget = new QWidget(this);
    setCentralWidget(centralWidget);

    auto* mainLayout = new QVBoxLayout(centralWidget);

    // Top Bar (Connection + Machine Selection)
    auto* topLayout = new QHBoxLayout();
    topLayout->addWidget(new QLabel("Server:"));

    m_serverInput = new QLineEdit("http://localhost:8000");
    topLayout->addWidget(m_serverInput);

    auto* connectButton = new QPushButton("Verbinden");
    topLayout->addWidget(connectButton);
    connect(connectButton, &QPushButton::clicked, this, &MainWindow::onConnectButtonClicked);

    topLayout->addSpacing(20);
    topLayout->addWidget(new QLabel("Maschine:"));

    m_machineComboBox = new QComboBox();
    topLayout->addWidget(m_machineComboBox);
    m_machineComboBox->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);

    auto* refreshButton = new QPushButton("Aktualisieren");
    topLayout->addWidget(refreshButton);
    connect(refreshButton, &QPushButton::clicked, this, &MainWindow::onRefreshButtonClicked);

    topLayout->addStretch();
    
    // Dark/Light Mode Toggle Button (oben rechts)
    auto* themeButton = new QPushButton("☀️");
    themeButton->setToolTip("Theme wechseln (Dark/Light)");
    themeButton->setProperty("themeButton", true);
    topLayout->addWidget(themeButton);
    
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
            themeButton->setText("☀️");
        } else {
            // Light Theme laden
            QFile lightFile(":/styles/light_theme.qss");
            if (!lightFile.open(QIODevice::ReadOnly)) {
                lightFile.setFileName("styles/light_theme.qss");
                lightFile.open(QIODevice::ReadOnly);
            }
            QString lightStyle = lightFile.readAll();
            qApp->setStyleSheet(lightStyle);
            themeButton->setText("🌙");
        }
    });
    
    mainLayout->addLayout(topLayout);

    // Main Content (Splitter)
    auto* splitter = new QSplitter(Qt::Horizontal);

    // Left: Chart + Events Table
    auto* leftWidget = new QWidget();
    auto* leftLayout = new QVBoxLayout(leftWidget);

    // Chart
    m_chartView = new QChartView();
    m_chartView->setRenderHint(QPainter::Antialiasing);
    leftLayout->addWidget(m_chartView);

    // Events Table
    m_eventsTable = new QTableWidget();
    m_eventsTable->setColumnCount(4);
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

    leftLayout->addWidget(m_eventsTable);

    splitter->addWidget(leftWidget);

    // Right: Chat Interface
    auto* rightWidget = new QWidget();
    auto* rightLayout = new QVBoxLayout(rightWidget);

    rightLayout->addWidget(new QLabel("AI Chat:"));

    m_chatDisplay = new QTextEdit();
    m_chatDisplay->setReadOnly(true);
    rightLayout->addWidget(m_chatDisplay);

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
    
    rightLayout->addWidget(m_chatInput);

    // Buttons unter dem Chat Input
    auto* chatButtonLayout = new QHBoxLayout();
    m_sendButton = new QPushButton("Senden");
    chatButtonLayout->addWidget(m_sendButton);

    auto* analyzeButton = new QPushButton("Analysieren");
    chatButtonLayout->addWidget(analyzeButton);
    
    chatButtonLayout->addStretch();
    rightLayout->addLayout(chatButtonLayout);

    splitter->addWidget(rightWidget);
    splitter->setSizes({800, 600});

    mainLayout->addWidget(splitter);

    // Status Bar
    statusBar()->showMessage("Bereit");

    // Store button references
    connect(m_sendButton, &QPushButton::clicked, this, &MainWindow::onSendButtonClicked);
    connect(analyzeButton, &QPushButton::clicked, this, &MainWindow::onAnalyzeButtonClicked);
}

void MainWindow::setupConnections() {
    connect(m_machineComboBox, QOverload<int>::of(&QComboBox::currentIndexChanged), this,
            &MainWindow::onMachineSelectionChanged);

    // Ctrl+Enter für Senden in chat input
    QShortcut* sendShortcut = new QShortcut(QKeySequence("Ctrl+Return"), m_chatInput);
    connect(sendShortcut, &QShortcut::activated, this, &MainWindow::onSendButtonClicked);
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

void MainWindow::appendChatMessage(const QString& role, const QString& message) {
    QString formatted;
    if (role == "User") {
        formatted = QString("<p><b style='color: #007BFF;'>🧑 User:</b> %1</p>").arg(message);
    } else {
        // AI Antwort formatieren - Sterne entfernen und als Überschriften formatieren
        QString processedMessage = message;
        processedMessage = processedMessage.toHtmlEscaped();
        // ** entfernen und als Überschriften formatieren
        processedMessage = processedMessage.replace(QRegularExpression("\\*\\*([^*]+)\\*\\*"), 
                                                   "<h4 style='margin: 10px 0 5px 0;'>\\1</h4>");
        
        // Normale Zeilenumbrüche
        processedMessage = processedMessage.replace("\n", "<br>");
        
        formatted = QString("<p><b style='color: green;'>🤖 Assistant:</b></p>%1")
                        .arg(processedMessage);
    }

    m_chatDisplay->append(formatted);
    m_chatDisplay->verticalScrollBar()->setValue(m_chatDisplay->verticalScrollBar()->maximum());
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

        // Color coding
        QColor color;
        switch (event.level) {
            case Event::Critical:
                color = QColor(255, 0, 0, 50);
                break;
            case Event::Error:
                color = QColor(255, 100, 0, 50);
                break;
            case Event::Warning:
                color = QColor(255, 200, 0, 50);
                break;
            default:
                color = QColor(255, 255, 255);
        }

        for (int col = 0; col < 4; ++col) {
            m_eventsTable->item(i, col)->setBackground(color);
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
