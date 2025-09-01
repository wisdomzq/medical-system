#include "placeholderpages.h"
#include "core/network/communicationclient.h"
#include <QVBoxLayout>
#include <QLabel>

// 为其他占位页面添加简单标签
template <typename T>
static void ensureLayout(T* w, const char* text)
{
    auto* lay = new QVBoxLayout(w);
    lay->addWidget(new QLabel(QString::fromUtf8(text)));
    lay->addStretch();
}

HealthAssessmentPage::HealthAssessmentPage(CommunicationClient* c, const QString& p, QWidget* parent)
    : BasePage(c, p, parent)
{
    ensureLayout(this, "健康评估");
}
