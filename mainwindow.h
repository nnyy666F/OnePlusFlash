#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QProcess>
#include <QTimer>
#include <QFileDialog>
#include <QMessageBox>
#include <QTableWidget>
#include <QLabel>
#include <QPushButton>
#include <QProgressBar>
#include <QComboBox>
#include <QLineEdit>
#include <QGroupBox>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFormLayout>
#include <QRegularExpression>

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private slots:
    void on_browseButton_clicked();
    void on_flashButton_clicked();
    void on_stopButton_clicked();
    void updateDeviceList();
    void handleProcessOutput();
    void handleProcessFinished(int exitCode, QProcess::ExitStatus exitStatus);
    void checkDeviceConnection();
    void on_installDriverAction_triggered();
    void on_installRom();

private:
    Ui::MainWindow *ui;
    QProcess *flashProcess;
    QTimer *deviceCheckTimer;
    QString firmwarePath;
    QString selectedDevice;
    bool isFlashing;

    // UI组件
    QLabel *statusLabel;
    QProgressBar *progressBar;
    QTableWidget *deviceTable;
    QLineEdit *firmwarePathEdit;
    QPushButton *browseButton, *flashButton, *stopButton, *refreshButton;
    QComboBox *partitionComboBox;
    QGroupBox *deviceGroup, *firmwareGroup, *optionsGroup;

    void initUI();
    void initConnections();
    void setupDeviceTable();
    void enableUI(bool enable);
    void initMenuBar();
};
#endif // MAINWINDOW_H
