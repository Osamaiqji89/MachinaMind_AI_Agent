/**
 * @file MainWindow.h
 * @brief Main Window - View im MVP-Pattern
 */

#ifndef MachinaMindAIAgent_WINDOW_H
#define MachinaMindAIAgent_WINDOW_H

#include <QComboBox>
#include <QHeaderView>
#include <QLineEdit>
#include <QMainWindow>
#include <QProcess>
#include <QPushButton>
#include <QScrollBar>
#include <QTableWidget>
#include <QTextEdit>
#include <QLabel>
#include <QtCharts/QChartView>
#include <QCloseEvent>

#include "../presenter/MainPresenter.h"

QT_BEGIN_NAMESPACE
namespace Ui {
class MainWindow;
}
QT_END_NAMESPACE

namespace MachinaMindAIAgent {

/**
 * @brief Main Window (View Implementation)
 * 
 * Implementiert IMainView Interface
 * Reine UI-Logik - keine Business Logic!
 */
class MainWindow : public QMainWindow, public IMainView {
    Q_OBJECT

public:
    explicit MainWindow(MainPresenter* presenter, QWidget* parent = nullptr);
    ~MainWindow() override;

    // IMainView Interface
    void showError(const QString& message) override;
    void showInfo(const QString& message) override;
    void setConnectionStatus(bool connected) override;
    void appendChatMessage(const QString& role, const QString& message) override;
    void updateMachineList(const QVector<Machine>& machines) override;
    void updateChart(const QVector<Measurement>& measurements) override;
    void updateEventsTable(const QVector<Event>& events) override;
    void setAnalysisResult(const QString& summary, int anomalyCount) override;

private slots:
    void onMachineSelectionChanged(int index);
    void onRefreshButtonClicked();
    void onSendButtonClicked();
    void onAnalyzeButtonClicked();
    void onConnectButtonClicked();

protected:
    void closeEvent(QCloseEvent *event) override;

private:
    void setupUi();
    void setupConnections();
    void startBackend();
    void startBackendProcess();
    void stopBackend();
    void checkBackendHealth();

    Ui::MainWindow* ui;
    MainPresenter* m_presenter;

    // UI Components (wenn nicht via Designer)
    QComboBox* m_machineComboBox;
    QTextEdit* m_chatDisplay;
    QTextEdit* m_chatInput;  // Geändert von QLineEdit zu QTextEdit
    QPushButton* m_sendButton;
    QTableWidget* m_eventsTable;
    QChartView* m_chartView;
    QLineEdit* m_serverInput;
    
    // Backend Process
    QProcess* m_backendProcess;
    QTimer* m_healthCheckTimer;
    int m_healthCheckAttempts;
};

}  // namespace MachinaMindAIAgent

#endif  // MachinaMindAIAgent_WINDOW_H
