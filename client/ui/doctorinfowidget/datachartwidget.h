#ifndef DATACHARTWIDGET_H
#define DATACHARTWIDGET_H

#include <QWidget>
#include <QString>

class QPushButton;
class QComboBox;
class QVBoxLayout;
class QHBoxLayout;
class QDateEdit;
class QLabel;
class QPaintEvent;
class QCustomPlot; // 我们将使用自定义绘图

// 基础数据图表生成模块
class DataChartWidget : public QWidget {
    Q_OBJECT
    
public:
    explicit DataChartWidget(const QString& doctorName, QWidget* parent = nullptr);
    ~DataChartWidget();

protected:
    void paintEvent(QPaintEvent* event) override;

private slots:
    void generateChart();
    void refreshData();
    void onDataTypeChanged();
    void exportChart();

private:
    void setupUI();
    void loadSampleData();
    void updateChart(const QString& dataType);
    void drawChart(QPainter* painter);
    
    QString doctorName_;
    
    // UI控件
    QComboBox* dataTypeCombo_;
    QPushButton* generateBtn_;
    QPushButton* refreshBtn_;
    QPushButton* exportBtn_;
    QDateEdit* startDateEdit_;
    QDateEdit* endDateEdit_;
    QLabel* statusLabel_;
    
    QVBoxLayout* mainLayout_;
    QHBoxLayout* controlLayout_;
    
    // 图表数据
    QVector<double> chartData_;
    QStringList chartLabels_;
    QString currentDataType_;
    double maxValue_;
    double minValue_;
};

#endif // DATACHARTWIDGET_H
