#include "medicationpage.h"
#include "core/network/src/client/communicationclient.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QLabel>
#include <QJsonDocument>
#include <QNetworkAccessManager>
#include <QNetworkRequest>
#include <QUrl>
#include <QUrlQuery>
#include <QPixmap>
#include <QBuffer>
#include <QApplication>
#include <QFile>

MedicationSearchPage::MedicationSearchPage(CommunicationClient *c, const QString &p, QWidget *parent)
    : BasePage(c,p,parent) {
    auto *outer = new QVBoxLayout(this);
    auto *hl = new QHBoxLayout();
    m_searchEdit = new QLineEdit(this);
    m_searchEdit->setPlaceholderText(QStringLiteral("输入药品名称/通用名/类别进行搜索(留空获取全部)"));
    m_searchBtn = new QPushButton(QStringLiteral("搜索"), this);
    m_remoteBtn = new QPushButton(QStringLiteral("远程搜索"), this);
    hl->addWidget(m_searchEdit,1);
    hl->addWidget(m_searchBtn);
    hl->addWidget(m_remoteBtn);
    outer->addLayout(hl);

    m_table = new QTableWidget(this);
    m_table->setColumnCount(11);
    QStringList headers {"ID","名称","通用名","类别","厂家","规格","价格","库存","使用说明","注意事项","图片"};
    m_table->setHorizontalHeaderLabels(headers);
    m_table->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    m_table->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_table->setSelectionBehavior(QAbstractItemView::SelectRows);
    outer->addWidget(m_table,1);

    connect(m_searchBtn,&QPushButton::clicked,this,&MedicationSearchPage::onSearch);
    connect(m_searchEdit,&QLineEdit::returnPressed,this,&MedicationSearchPage::onSearch);
    connect(m_remoteBtn,&QPushButton::clicked,this,&MedicationSearchPage::remoteSearch);

    // 初始加载全部
    sendSearchRequest("");
}

void MedicationSearchPage::onSearch(){
    sendSearchRequest(m_searchEdit->text().trimmed());
}

void MedicationSearchPage::sendSearchRequest(const QString &keyword){
    QJsonObject payload; if(keyword.isEmpty()){ payload["action"] = "get_medications"; } else { payload["action"] = "search_medications"; payload["keyword"] = keyword; }
    m_client->sendJson(payload);
}

void MedicationSearchPage::handleResponse(const QJsonObject &obj){
    if(obj.value("type").toString() != "medications_response") return;
    if(!obj.value("success").toBool()) return; // 可加错误显示
    QJsonArray arr = obj.value("data").toArray();
    populateTable(arr);
}

void MedicationSearchPage::populateTable(const QJsonArray &arr){
    // 如果当前是搜索（有关键字）并存在精确匹配，仅保留精确匹配
    QString kw = m_searchEdit->text().trimmed();
    QVector<QJsonObject> rows;
    if(!kw.isEmpty()){
        QVector<QJsonObject> exact;
        for(const auto &v: arr){ QJsonObject o=v.toObject(); if(o.value("name").toString()==kw || o.value("generic_name").toString().compare(kw,Qt::CaseInsensitive)==0) exact.push_back(o); }
        if(!exact.isEmpty()) rows = exact; else { for(const auto &v: arr) rows.push_back(v.toObject()); }
    } else { for(const auto &v: arr) rows.push_back(v.toObject()); }
    m_table->setRowCount(rows.size());
    int row=0; for(const auto &o: rows){
        auto setText=[&](int col,const QString &text){ auto *item=new QTableWidgetItem(text); m_table->setItem(row,col,item); };
        setText(0, QString::number(o.value("id").toInt()));
        setText(1, o.value("name").toString());
        setText(2, o.value("generic_name").toString());
        setText(3, o.value("category").toString());
        setText(4, o.value("manufacturer").toString());
        setText(5, o.value("specification").toString());
        setText(6, QString::number(o.value("price").toDouble(),'f',2));
        setText(7, QString::number(o.value("stock_quantity").toInt()));
    setText(8, o.value("description").toString());
    setText(9, o.value("precautions").toString());
        // 图片列: 异步获取占位
        QLabel *imgLabel = new QLabel; imgLabel->setFixedSize(64,64); imgLabel->setScaledContents(true);
        QString localPath = o.value("image_path").toString();
        if(!localPath.isEmpty() && QFile::exists(localPath)){
            QPixmap pix(localPath);
            if(!pix.isNull()) { imgLabel->setPixmap(pix.scaled(64,64,Qt::KeepAspectRatio, Qt::SmoothTransformation)); }
            else { imgLabel->setText("图"); }
        } else if(o.contains("image_base64")) {
            QByteArray raw = QByteArray::fromBase64(o.value("image_base64").toString().toLatin1());
            QPixmap pix; if(pix.loadFromData(raw)) imgLabel->setPixmap(pix.scaled(64,64,Qt::KeepAspectRatio, Qt::SmoothTransformation)); else imgLabel->setText("图");
        } else {
            imgLabel->setText("...");
            fetchImageForRow(row, o.value("name").toString());
        }
        m_table->setCellWidget(row,10,imgLabel);
        ++row;
    }
}

void MedicationSearchPage::remoteSearch(){
    QString kw = m_searchEdit->text().trimmed();
    if(kw.isEmpty()) return; // 需要关键词
    QJsonObject p; p["action"]="search_medications_remote"; p["keyword"]=kw; m_client->sendJson(p);
}

void MedicationSearchPage::fetchImageForRow(int row,const QString &medName){
    static QNetworkAccessManager *nam = new QNetworkAccessManager(QApplication::instance());
    QByteArray enc = QUrl::toPercentEncoding(medName.left(6));
    QUrl url(QStringLiteral("https://dummyimage.com/128x128/87ceeb/001133.png&text=%1").arg(QString::fromLatin1(enc)));
    QNetworkReply *reply = nam->get(QNetworkRequest(url));
    m_replyRowMap.insert(reply,row);
    connect(reply,&QNetworkReply::finished,this,&MedicationSearchPage::onImageDownloaded);
}

void MedicationSearchPage::onImageDownloaded(){
    QNetworkReply *reply = qobject_cast<QNetworkReply*>(sender()); if(!reply) return; int row = m_replyRowMap.take(reply); QByteArray data = reply->readAll(); reply->deleteLater();
    QPixmap pix; if(pix.loadFromData(data)){
        if(row >=0 && row < m_table->rowCount()){
            if(QWidget *w = m_table->cellWidget(row,10)){
                QLabel *lbl = qobject_cast<QLabel*>(w); if(lbl){ lbl->setPixmap(pix); }
            }
        }
    }
}
