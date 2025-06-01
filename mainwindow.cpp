#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <QCoreApplication>
#include <QDir>
#include <QMenuBar>
#include <QMenu>
#include <QAction>
#include <QDesktopServices>
#include <QMessageBox>
#include <QUrl>
#include <QDesktopServices>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    setWindowTitle("1+Mlash");
    setMinimumSize(800, 600);

    flashProcess = new QProcess(this);
    deviceCheckTimer = new QTimer(this);
    isFlashing = false;

    initUI();
    initConnections();
    setupDeviceTable();
    initMenuBar(); // 调用初始化菜单栏的函数

    // 启动设备检测定时器
    deviceCheckTimer->start(2000);
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::initUI()
{
    // 创建主布局
    QWidget *centralWidget = new QWidget(this);
    setCentralWidget(centralWidget);
    QVBoxLayout *mainLayout = new QVBoxLayout(centralWidget);

    // 设备信息组
    deviceGroup = new QGroupBox("设备信息", centralWidget);
    QVBoxLayout *deviceLayout = new QVBoxLayout(deviceGroup);
    deviceTable = new QTableWidget(0, 3, deviceGroup);
    deviceTable->setHorizontalHeaderLabels({"设备ID", "设备名称", "状态"});
    deviceTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    deviceTable->setSelectionMode(QAbstractItemView::SingleSelection);
    refreshButton = new QPushButton("刷新设备", deviceGroup);
    deviceLayout->addWidget(deviceTable);
    deviceLayout->addWidget(refreshButton);

    // 固件选择组
    firmwareGroup = new QGroupBox("固件选择", centralWidget);
    QHBoxLayout *firmwareLayout = new QHBoxLayout(firmwareGroup);
    firmwarePathEdit = new QLineEdit(firmwareGroup);
    firmwarePathEdit->setReadOnly(true);
    browseButton = new QPushButton("浏览...", firmwareGroup);
    firmwareLayout->addWidget(firmwarePathEdit);
    firmwareLayout->addWidget(browseButton);

    // 刷机选项组
    optionsGroup = new QGroupBox("刷机选项", centralWidget);
    QFormLayout *optionsLayout = new QFormLayout(optionsGroup);
    partitionComboBox = new QComboBox(optionsGroup);
    partitionComboBox->addItems({"全部分区", "系统", "引导", "恢复模式", "基带"});
    optionsLayout->addRow("刷机分区:", partitionComboBox);

    // 操作按钮
    QHBoxLayout *buttonLayout = new QHBoxLayout();
    flashButton = new QPushButton("开始刷机", centralWidget);
    flashButton->setMinimumHeight(40);
    stopButton = new QPushButton("停止", centralWidget);
    stopButton->setMinimumHeight(40);
    stopButton->setEnabled(false);
    buttonLayout->addStretch();
    buttonLayout->addWidget(flashButton);
    buttonLayout->addWidget(stopButton);

    // 状态显示
    QHBoxLayout *statusLayout = new QHBoxLayout();
    statusLabel = new QLabel("就绪", centralWidget);
    progressBar = new QProgressBar(centralWidget);
    progressBar->setRange(0, 100);
    progressBar->setValue(0);
    statusLayout->addWidget(statusLabel);
    statusLayout->addWidget(progressBar);

    // 添加所有组件到主布局
    mainLayout->addWidget(deviceGroup);
    mainLayout->addWidget(firmwareGroup);
    mainLayout->addWidget(optionsGroup);
    mainLayout->addLayout(buttonLayout);
    mainLayout->addLayout(statusLayout);
}

void MainWindow::initConnections()
{
    connect(browseButton, &QPushButton::clicked, this, &MainWindow::on_browseButton_clicked);
    connect(flashButton, &QPushButton::clicked, this, &MainWindow::on_flashButton_clicked);
    connect(stopButton, &QPushButton::clicked, this, &MainWindow::on_stopButton_clicked);
    connect(refreshButton, &QPushButton::clicked, this, &MainWindow::updateDeviceList);
    connect(deviceCheckTimer, &QTimer::timeout, this, &MainWindow::checkDeviceConnection);
    connect(flashProcess, &QProcess::readyReadStandardOutput, this, &MainWindow::handleProcessOutput);
    connect(flashProcess, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished),
            this, &MainWindow::handleProcessFinished);
}

void MainWindow::setupDeviceTable()
{
    deviceTable->setColumnWidth(0, 150);
    deviceTable->setColumnWidth(1, 200);
    deviceTable->setColumnWidth(2, 100);
}

void MainWindow::updateDeviceList()
{
    deviceTable->setRowCount(0);

    QString appPath = QCoreApplication::applicationDirPath();
    QDir adbDir(appPath);
    qDebug()<<appPath;

    QString fastbootPath = adbDir.filePath("adb/fastboot.exe");
    QString adbPath = adbDir.filePath("adb/adb.exe");

    if (!QFile::exists(fastbootPath)) {
        qDebug() << "Fastboot not found at:" << fastbootPath;
        statusLabel->setText("Fastboot工具未找到");
    } else {
        QProcess fastbootProcess;
        fastbootProcess.start(fastbootPath, QStringList() << "devices");
        if (fastbootProcess.waitForFinished()) {
            QString output = fastbootProcess.readAllStandardOutput();
            QStringList lines = output.split('\n', Qt::SkipEmptyParts);

            for (const QString &line : lines) {
                QStringList parts = line.split('\t', Qt::SkipEmptyParts);
                if (parts.size() >= 2) {
                    int row = deviceTable->rowCount();
                    deviceTable->insertRow(row);
                    deviceTable->setItem(row, 0, new QTableWidgetItem(parts[0]));
                    deviceTable->setItem(row, 1, new QTableWidgetItem("一加设备"));
                    deviceTable->setItem(row, 2, new QTableWidgetItem("Fastboot模式"));
                }
            }
        } else {
            qDebug() << "Fastboot command failed:" << fastbootProcess.errorString();
        }
    }

    if (!QFile::exists(adbPath)) {
        qDebug() << "ADB not found at:" << adbPath;
        statusLabel->setText("ADB工具未找到");
    } else {
        QProcess adbProcess;
        adbProcess.start(adbPath, QStringList() << "devices");
        if (adbProcess.waitForFinished()) {
            QString output = adbProcess.readAllStandardOutput();
            QStringList lines = output.split('\n', Qt::SkipEmptyParts);

            // 跳过标题行
            if (lines.size() > 1) {
                for (int i = 1; i < lines.size(); i++) {
                    QStringList parts = lines[i].split('\t', Qt::SkipEmptyParts);
                    if (parts.size() >= 2) {
                        int row = deviceTable->rowCount();
                        deviceTable->insertRow(row);
                        deviceTable->setItem(row, 0, new QTableWidgetItem(parts[0]));
                        deviceTable->setItem(row, 1, new QTableWidgetItem("一加设备"));
                        deviceTable->setItem(row, 2, new QTableWidgetItem("ADB模式"));
                    }
                }
            }
        } else {
            qDebug() << "ADB command failed:" << adbProcess.errorString();
        }
    }

    if (deviceTable->rowCount() > 0) {
        statusLabel->setText("已检测到设备");
    } else {
        statusLabel->setText("未检测到设备，请确保设备已进入Fastboot或ADB模式");
    }
}

void MainWindow::checkDeviceConnection()
{
    static int previousDeviceCount = -1;
    int currentDeviceCount = deviceTable->rowCount();

    if (previousDeviceCount != currentDeviceCount || currentDeviceCount == 0) {
        updateDeviceList();
        previousDeviceCount = currentDeviceCount;
    }
}

void MainWindow::on_browseButton_clicked()
{
    QString filePath = QFileDialog::getOpenFileName(
        this, "选择固件文件", "", "固件文件 (*.zip *.img);;所有文件 (*)"
        );

    if (!filePath.isEmpty()) {
        firmwarePath = filePath;
        firmwarePathEdit->setText(filePath);
        statusLabel->setText("固件已选择");
    }
}

void MainWindow::on_flashButton_clicked()
{
    if (deviceTable->selectedItems().isEmpty()) {
        QMessageBox::warning(this, "警告", "请先选择要刷机的设备");
        return;
    }
    if (firmwarePath.isEmpty()) {
        QMessageBox::warning(this, "警告", "请先选择固件文件");
        return;
    }
    int selectedRow = deviceTable->currentRow();
    if (selectedRow >= 0) {
        selectedDevice = deviceTable->item(selectedRow, 0)->text();
    }
    QMessageBox::StandardButton reply;
    reply = QMessageBox::question(this, "确认刷机",
                                  "即将开始刷机操作，这可能会清除设备上的所有数据。\n是否继续？",
                                  QMessageBox::Yes | QMessageBox::No
                                  );

    if (reply == QMessageBox::Yes) {
        isFlashing = true;
        enableUI(false);
        flashButton->setEnabled(false);
        stopButton->setEnabled(true);
        progressBar->setValue(0);

        // 根据选择的分区类型构建不同的刷机命令
        QString partition = partitionComboBox->currentText();
        QString command;

        if (partition == "全部分区") {
            // 完整刷机命令
            command = QString("fastboot flashall -w \"%1\"").arg(firmwarePath);
        } else if (partition == "系统") {
            command = QString("fastboot flash system \"%1\"").arg(firmwarePath);
        } else if (partition == "引导") {
            command = QString("fastboot flash boot \"%1\"").arg(firmwarePath);
        } else if (partition == "恢复模式") {
            command = QString("fastboot flash recovery \"%1\"").arg(firmwarePath);
        } else if (partition == "基带") {
            command = QString("fastboot flash radio \"%1\"").arg(firmwarePath);
        }

        flashProcess->start("cmd.exe", QStringList() << "/c" << command);
        statusLabel->setText("正在刷机...");
    }
}

void MainWindow::on_stopButton_clicked()
{
    if (flashProcess->state() == QProcess::Running) {
        flashProcess->terminate();
        if (!flashProcess->waitForFinished(3000)) {
            flashProcess->kill();
        }
    }

    isFlashing = false;
    enableUI(true);
    flashButton->setEnabled(true);
    stopButton->setEnabled(false);
    statusLabel->setText("刷机已取消");
}

void MainWindow::handleProcessOutput()
{
    QString output = flashProcess->readAllStandardOutput();
    // 这里可以解析输出内容来更新进度条
    // 实际应用中需要根据fastboot的输出格式来解析

    // 简单示例：根据输出内容更新进度
    if (output.contains("Erasing 'system'")) {
        progressBar->setValue(10);
    } else if (output.contains("Sending 'system'")) {
        progressBar->setValue(30);
    } else if (output.contains("Writing 'system'")) {
        progressBar->setValue(60);
    } else if (output.contains("Finished processing system")) {
        progressBar->setValue(80);
    }

    // 可以将输出显示在日志窗口中
    // ui->logTextEdit->append(output);
}

void MainWindow::handleProcessFinished(int exitCode, QProcess::ExitStatus exitStatus)
{
    isFlashing = false;
    enableUI(true);
    flashButton->setEnabled(true);
    stopButton->setEnabled(false);

    if (exitStatus == QProcess::NormalExit && exitCode == 0) {
        progressBar->setValue(100);
        statusLabel->setText("刷机成功！请重启设备");
        QMessageBox::information(this, "成功", "刷机操作已完成");
    } else {
        statusLabel->setText("刷机失败");
        QString error = flashProcess->readAllStandardError();
        QMessageBox::critical(this, "错误", QString("刷机失败: %1").arg(error));
    }

    // 刷新设备列表
    updateDeviceList();
}

void MainWindow::enableUI(bool enable)
{
    browseButton->setEnabled(enable);
    refreshButton->setEnabled(enable);
    partitionComboBox->setEnabled(enable);
    deviceTable->setEnabled(enable);
}

void MainWindow::initMenuBar()
{
    QMenuBar *menuBar = new QMenuBar(this);
    setMenuBar(menuBar);

    QMenu *toolsMenu = menuBar->addMenu("驱动");
    QAction *installDriverAction = toolsMenu->addAction("安装驱动");
    connect(installDriverAction, &QAction::triggered, this, &MainWindow::on_installDriverAction_triggered);

    QMenu *moreMenu = menuBar->addMenu("更多");
    QAction *installRom = moreMenu->addAction("固件获取");
    connect(installRom, &QAction::triggered, this, &MainWindow::on_installRom);
}

void MainWindow::on_installDriverAction_triggered()
{
    QMessageBox msgBox(this);
    msgBox.setWindowTitle("选择系统版本");
    msgBox.setText("请选择你的Windows系统版本：");
    msgBox.setStandardButtons(QMessageBox::Yes | QMessageBox::No | QMessageBox::Cancel);
    msgBox.button(QMessageBox::Yes)->setText("Windows 10 或更高");
    msgBox.button(QMessageBox::No)->setText("Windows 7/8");
    msgBox.button(QMessageBox::Cancel)->setText("取消");

    int result = msgBox.exec();
    if (result == QMessageBox::Yes) {
        QDesktopServices::openUrl(QUrl("https://zhuanlan.zhihu.com/p/366904302"));
    } else if (result == QMessageBox::No) {
        QMessageBox::information(this, "提示",
                                 "Windows 7/8系统可能无法正常安装驱动，请升级到Windows 10或更高版本。");
    }
}

void MainWindow::on_installRom()
{
    qDebug()<<"1`11";
    QMessageBox msgBox(this);
    msgBox.setWindowTitle("固件获取");
    msgBox.setText("⚠️ 刷机有风险，请提前做好备份！！！");
    msgBox.setInformativeText(
        "你可以在以前的链接获取Rom刷机包：\n"
        "https://zhuanlan.zhihu.com/p/366904302"
        );
    msgBox.setStandardButtons(QMessageBox::Ok);
    // msgBox.setIcon(QMessageBox::Warning);
    msgBox.exec(); // 阻塞显示对话框
}
