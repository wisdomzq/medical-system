#ifndef REMOTEDATAWIDGET_H
#define REMOTEDATAWIDGET_H

#include <QWidget>
#include <QString>
#include <QTimer>

class QPushButton;
class QTextEdit;
class QLabel;
class QProgressBar;
class QTableWidget;
class QVBoxLayout;
class QHBoxLayout;
class QGroupBox;
class QLineEdit;
class QComboBox;
class QSpinBox;

// 远距数据采集分析模块 - 调用智能心电仪API
class RemoteDataWidget : public QWidget {
    Q_OBJECT
    
public:
    explicit RemoteDataWidget(const QString& doctorName, QWidget* parent = nullptr);
    ~RemoteDataWidget();

private slots:
    void startDataCollection();
    void stopDataCollection();
    void analyzeData();
    void refreshDeviceList();
    void onDataReceived();
    void exportAnalysisReport();
    void clearHistory();
    
private:
    void setupUI();
    void setupDeviceInfo();
    void setupDataDisplay();
    void setupAnalysisPanel();
    void simulateECGData();
    void updateDataDisplay(const QString& data);
    void performDataAnalysis();
    void addAnalysisResult(const QString& result);
    
    QString doctorName_;
    
    // 设备控制
    QGroupBox* deviceGroup_;
    QComboBox* deviceCombo_;
    QLineEdit* deviceIPEdit_;
    QSpinBox* sampleRateSpinBox_;
    QPushButton* refreshDeviceBtn_;
    QPushButton* startBtn_;
    QPushButton* stopBtn_;
    QLabel* deviceStatusLabel_;
    QProgressBar* connectionProgress_;
    
    // 数据显示
    QGroupBox* dataGroup_;
    QTextEdit* rawDataEdit_;
    QLabel* dataCountLabel_;
    QTimer* dataTimer_;
    
    // 分析面板
    QGroupBox* analysisGroup_;
    QPushButton* analyzeBtn_;
    QPushButton* exportBtn_;
    QPushButton* clearBtn_;
    QTableWidget* analysisTable_;
    QTextEdit* analysisResultEdit_;
    
    QVBoxLayout* mainLayout_;
    
    // 数据状态
    bool isCollecting_;
    int dataCount_;
    QStringList collectedData_;
};

#endif // REMOTEDATAWIDGET_H
