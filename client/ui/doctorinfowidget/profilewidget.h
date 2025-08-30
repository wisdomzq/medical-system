#ifndef PROFILEWIDGET_H
#define PROFILEWIDGET_H

#include <QWidget>
#include <QString>

class QJsonObject;

class QLineEdit;
class QTextEdit;
class QTimeEdit;
class QDoubleSpinBox;
class QSpinBox;
class QLabel;
class QPushButton;
class CommunicationClient;

// 个人信息模块：进入时请求后端信息，展示并可编辑后保存
class ProfileWidget : public QWidget {
    Q_OBJECT
public:
    explicit ProfileWidget(const QString& doctorName, QWidget* parent=nullptr);

private slots:
    void onConnected();
    void onJsonReceived(const QJsonObject& resp);
    void onChoosePhoto();
    void onSave();
    void onRefresh();

private:
    void requestProfile();
    QString buildWorkTitle() const; // 将上下班时间编码到 title 字段
    void parseWorkTitle(const QString& title); // 从 title 解析上下班时间

    QString doctorName_;
    CommunicationClient* client_ {nullptr};

    // UI
    QLineEdit* workNumberEdit_ {nullptr};
    QLineEdit* departmentEdit_ {nullptr};
    QTextEdit* bioEdit_ {nullptr}; // 映射到 specialization 字段
    QLabel* photoPreview_ {nullptr};
    QByteArray photoBytes_;
    QTimeEdit* workStartEdit_ {nullptr};
    QTimeEdit* workEndEdit_ {nullptr};
    QDoubleSpinBox* feeEdit_ {nullptr}; // consultation_fee
    QSpinBox* dailyLimitEdit_ {nullptr}; // max_patients_per_day
    QPushButton* saveBtn_ {nullptr};
    QPushButton* refreshBtn_ {nullptr};
};

#endif // PROFILEWIDGET_H
