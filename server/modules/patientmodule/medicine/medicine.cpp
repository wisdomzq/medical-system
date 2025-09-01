#include "medicine.h"
#include "core/database/database.h"
#include "core/database/database_config.h"
#include "core/network/messagerouter.h"
#include "core/logging/logging.h"
#include <QDateTime>
#include <QDir>
#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QSet>
#include <QSqlError>
#include <QUrlQuery>
#include <algorithm>

static QString localMedicationsJsonPath()
{
    // 可执行目录: build/server ; 项目根目录: build/.. => ../../
    QString path = QDir::currentPath() + "/../../resources/medications/medications.json";
    return path;
}
static QString moduleImageDir()
{
    QString path = QDir::currentPath() + "/../../server/modules/patientmodule/medicine/img";
    QDir d(path);
    if (d.exists()) {
        qInfo() << "[MedicineModule] 图片目录路径:" << d.absolutePath();
        return d.absolutePath();
    }
    qWarning() << "[MedicineModule] 图片目录不存在:" << path;
    return QString();
}

MedicineModule::MedicineModule(QObject* parent)
    : QObject(parent)
{
    connect(&MessageRouter::instance(), &MessageRouter::requestReceived,
        this, &MedicineModule::onRequest);
    connect(this, &MedicineModule::businessResponse,
            &MessageRouter::instance(), &MessageRouter::onBusinessResponse);
    loadLocalMeta();
    
    // 测试图片目录
    QString imgDir = moduleImageDir();
    if (!imgDir.isEmpty()) {
        qInfo() << "[MedicineModule] 图片目录可用:" << imgDir;
        QDir dir(imgDir);
        QStringList imageFiles = dir.entryList(QStringList() << "*.png" << "*.jpg" << "*.jpeg", QDir::Files);
        qInfo() << "[MedicineModule] 找到" << imageFiles.size() << "个图片文件:" << imageFiles;
    } else {
        qWarning() << "[MedicineModule] 图片目录不可用";
    }
}

void MedicineModule::onRequest(const QJsonObject& payload)
{
    const QString action = payload.value("action").toString();
    qInfo() << "[MedicineModule] 收到动作" << action;
    if (action == "get_medications")
        return handleGetMedications(payload);
    if (action == "search_medications")
        return handleSearchMedications(payload);
    if (action == "search_medications_remote")
        return handleRemoteSearch(payload);
}

void MedicineModule::handleGetMedications(const QJsonObject& payload)
{
    DBManager db(DatabaseConfig::getDatabasePath());
    QJsonArray list;
    bool ok = db.getMedications(list);
    
    // 过滤掉医疗器械类药品
    if (ok) {
        QStringList medicalDevices = {"血糖试纸", "电子体温计", "一次性医用口罩"};
        QJsonArray filtered;
        for (const auto& item : list) {
            QJsonObject o = item.toObject();
            QString name = o.value("name").toString();
            if (!medicalDevices.contains(name)) {
                filtered.append(item);
            }
        }
        list = filtered;
    }
    
    // 合并本地图片与描述补全
    if (ok) {
        for (int i = 0; i < list.size(); ++i) {
            QJsonObject o = list[i].toObject();
            auto name = o.value("name").toString();
            for (const auto& lmv : m_localMeta) {
                QJsonObject lo = lmv.toObject();
                if (lo.value("name").toString() == name) {
                    if (o.value("description").toString().isEmpty())
                        o["description"] = lo.value("description");
                    if (o.value("precautions").toString().isEmpty())
                        o["precautions"] = lo.value("precautions");
                    QString img = lo.value("image").toString();
                    if (!img.isEmpty()) {
                        QString resPath = QStringLiteral("resources/medications/images/%1").arg(img);
                        QString modDir = moduleImageDir();
                        QString modPath = modDir.isEmpty() ? QString() : (modDir + "/" + img);
                        if (QFile::exists(resPath)) {
                            o["image_path"] = resPath;
                        } else if (!modPath.isEmpty() && QFile::exists(modPath)) {
                            QFile f(modPath);
                            if (f.open(QIODevice::ReadOnly)) {
                                QByteArray b64 = f.readAll().toBase64();
                                o["image_base64"] = QString::fromLatin1(b64);
                            }
                        }
                    }
                    // 若仍无图片，尝试按药品名称匹配模块 img 下同名文件（任意扩展）
                    if (!o.contains("image_path") && !o.contains("image_base64")) {
                        QString modDir = moduleImageDir();
                        if (!modDir.isEmpty()) {
                            QString base = o.value("name").toString();
                            QStringList exts = { ".png", ".jpg", ".jpeg" };
                            for (const QString& ext : exts) {
                                QString candidate = modDir + "/" + base + ext;
                                if (QFile::exists(candidate)) {
                                    QFile f(candidate);
                                    if (f.open(QIODevice::ReadOnly))
                                        o["image_base64"] = QString::fromLatin1(f.readAll().toBase64());
                                    break;
                                }
                            }
                        }
                    }
                    break;
                }
            }
            // 对于所有药品，都尝试按药品名称匹配图片文件（如果还没有base64数据的话）
            if (!o.contains("image_base64")) {
                QString modDir = moduleImageDir();
                if (!modDir.isEmpty()) {
                    QString base = o.value("name").toString();
                    QStringList exts = { ".png", ".jpg", ".jpeg", ".webp" };
                    qInfo() << "[MedicineModule] 尝试为药品" << base << "查找图片，目录:" << modDir;
                    for (const QString& ext : exts) {
                        QString candidate = modDir + "/" + base + ext;
                        qInfo() << "[MedicineModule] 检查图片文件:" << candidate;
                        if (QFile::exists(candidate)) {
                            QFile f(candidate);
                            if (f.open(QIODevice::ReadOnly)) {
                                QByteArray imageData = f.readAll();
                                o["image_base64"] = QString::fromLatin1(imageData.toBase64());
                                // 清除不正确的image_path
                                if (o.contains("image_path")) {
                                    o.remove("image_path");
                                }
                                qInfo() << "[MedicineModule] 为药品" << base << "加载图片成功:" << candidate << "大小:" << imageData.size() << "字节";
                            } else {
                                qWarning() << "[MedicineModule] 无法打开图片文件:" << candidate;
                            }
                            break;
                        }
                    }
                    if (!o.contains("image_base64")) {
                        qWarning() << "[MedicineModule] 未找到药品" << base << "的图片文件";
                    }
                }
            }
            list[i] = o;
        }
        
        // 添加调试信息：检查每个药品的图片数据
        qInfo() << "[MedicineModule] 药品图片加载统计:";
        for (int i = 0; i < list.size(); ++i) {
            QJsonObject o = list[i].toObject();
            QString medName = o.value("name").toString();
            bool hasImagePath = o.contains("image_path");
            bool hasImageBase64 = o.contains("image_base64");
            qInfo() << "[MedicineModule] 药品:" << medName 
                    << "hasImagePath:" << hasImagePath 
                    << "hasImageBase64:" << hasImageBase64;
            
            if (hasImageBase64) {
                QString base64 = o.value("image_base64").toString();
                qInfo() << "[MedicineModule] 药品" << medName << "base64数据长度:" << base64.length();
            }
        }
        
        // 按ID从小到大排序
        if (ok) {
            qInfo() << "[MedicineModule] 排序前药品数量:" << list.size();
            if (!list.isEmpty()) {
                qInfo() << "[MedicineModule] 排序前第一个药品ID:" << list.first().toObject().value("id").toInt();
                qInfo() << "[MedicineModule] 排序前最后一个药品ID:" << list.last().toObject().value("id").toInt();
            }
            
            // 转换为std::vector进行排序
            std::vector<QJsonValue> sortedList;
            for (const auto& item : list) {
                sortedList.push_back(item);
            }
            
            std::sort(sortedList.begin(), sortedList.end(), [](const QJsonValue& a, const QJsonValue& b) {
                return a.toObject().value("id").toInt() < b.toObject().value("id").toInt();
            });
            
            // 转回QJsonArray
            list = QJsonArray();
            for (const auto& item : sortedList) {
                list.append(item);
            }
            
            if (!list.isEmpty()) {
                qInfo() << "[MedicineModule] 排序后第一个药品ID:" << list.first().toObject().value("id").toInt();
                qInfo() << "[MedicineModule] 排序后最后一个药品ID:" << list.last().toObject().value("id").toInt();
            }
        }
    }
    QJsonObject resp;
    resp["type"] = "medications_response";
    resp["success"] = ok;
    if (ok)
        resp["data"] = list;
    else
        resp["error"] = "获取药品列表失败";
    sendResponse(resp, payload);
}

void MedicineModule::handleSearchMedications(const QJsonObject& payload)
{
    DBManager db(DatabaseConfig::getDatabasePath());
    const QString keyword = payload.value("keyword").toString();
    qInfo() << "[MedicineModule] 本地/DB 搜索关键字=" << keyword;
    QJsonArray list;
    bool ok = db.searchMedications(keyword, list);
    
    // 过滤掉医疗器械类药品
    if (ok) {
        QStringList medicalDevices = {"血糖试纸", "电子体温计", "一次性医用口罩"};
        QJsonArray filtered;
        for (const auto& item : list) {
            QJsonObject o = item.toObject();
            QString name = o.value("name").toString();
            if (!medicalDevices.contains(name)) {
                filtered.append(item);
            }
        }
        list = filtered;
    }
    
    // 为数据库返回的每个药品添加图片
    if (ok) {
        for (int i = 0; i < list.size(); ++i) {
            QJsonObject o = list[i].toObject();
            
            // 强制为所有搜索结果加载图片（无论是否已有image_path）
            if (!o.contains("image_base64")) {
                QString modDir = moduleImageDir();
                if (!modDir.isEmpty()) {
                    QString base = o.value("name").toString();
                    QStringList exts = { ".png", ".jpg", ".jpeg", ".webp" };
                    qInfo() << "[MedicineModule] 搜索结果尝试为药品" << base << "查找图片，目录:" << modDir;
                    for (const QString& ext : exts) {
                        QString candidate = modDir + "/" + base + ext;
                        qInfo() << "[MedicineModule] 搜索时检查图片文件:" << candidate;
                        if (QFile::exists(candidate)) {
                            QFile f(candidate);
                            if (f.open(QIODevice::ReadOnly)) {
                                QByteArray imageData = f.readAll();
                                o["image_base64"] = QString::fromLatin1(imageData.toBase64());
                                // 清除错误的image_path
                                if (o.contains("image_path")) {
                                    o.remove("image_path");
                                }
                                qInfo() << "[MedicineModule] 为搜索结果药品" << base << "加载图片成功:" << candidate << "大小:" << imageData.size() << "字节";
                            } else {
                                qWarning() << "[MedicineModule] 无法打开搜索结果图片文件:" << candidate;
                            }
                            break;
                        }
                    }
                    if (!o.contains("image_base64")) {
                        qWarning() << "[MedicineModule] 搜索时未找到药品" << base << "的图片文件";
                    }
                }
            }
            list[i] = o;
        }
    }
    
    // 注释掉本地 meta 补充逻辑，只返回数据库中的药品
    // if (ok && !keyword.isEmpty()) {
    //     // 将本地 meta 中名称或通用名或分类包含关键字的条目（若数据库未返回）补充
    //     ...本地补充逻辑已移除...
    // }
    // 严格再次过滤：仅保留 name 或 generic_name 包含关键字的项
    if (ok && !keyword.isEmpty()) {
        QJsonArray filtered;
        for (const auto& v : list) {
            QJsonObject o = v.toObject();
            QString name = o.value("name").toString();
            QString gen = o.value("generic_name").toString();
            if (name.contains(keyword, Qt::CaseInsensitive) || gen.contains(keyword, Qt::CaseInsensitive))
                filtered.append(o);
        }
        list = filtered;
    }
    
        // 按ID从小到大排序
        if (ok) {
            qInfo() << "[MedicineModule] 排序前药品数量:" << list.size();
            if (!list.isEmpty()) {
                qInfo() << "[MedicineModule] 排序前第一个药品ID:" << list.first().toObject().value("id").toInt();
                qInfo() << "[MedicineModule] 排序前最后一个药品ID:" << list.last().toObject().value("id").toInt();
            }
            
            // 转换为std::vector进行排序
            std::vector<QJsonValue> sortedList;
            for (const auto& item : list) {
                sortedList.push_back(item);
            }
            
            std::sort(sortedList.begin(), sortedList.end(), [](const QJsonValue& a, const QJsonValue& b) {
                return a.toObject().value("id").toInt() < b.toObject().value("id").toInt();
            });
            
            // 转回QJsonArray
            list = QJsonArray();
            for (const auto& item : sortedList) {
                list.append(item);
            }
            
            if (!list.isEmpty()) {
                qInfo() << "[MedicineModule] 排序后第一个药品ID:" << list.first().toObject().value("id").toInt();
                qInfo() << "[MedicineModule] 排序后最后一个药品ID:" << list.last().toObject().value("id").toInt();
            }
        }
    
    QJsonObject resp;
    resp["type"] = "medications_response";
    resp["success"] = ok;
    if (ok)
        resp["data"] = list;
    else
        resp["error"] = "搜索药品失败";
    sendResponse(resp, payload);
}

// ...existing code...
void MedicineModule::handleRemoteSearch(const QJsonObject& payload)
{
    const QString keyword = payload.value("keyword").toString();
    qInfo() << "[MedicineModule] 远程搜索关键字=" << keyword;
    // DuckDuckGo 简易查询 (公共 API, 无密钥) 仅作示例
    QUrl url("https://api.duckduckgo.com/");
    QUrlQuery q;
    q.addQueryItem("q", keyword + QStringLiteral(" 药品"));
    q.addQueryItem("format", "json");
    q.addQueryItem("no_redirect", "1");
    q.addQueryItem("no_html", "1");
    url.setQuery(q);
    QNetworkRequest req(url);
    req.setHeader(QNetworkRequest::UserAgentHeader, "MedicalSystem/1.0");
    QNetworkReply* r = m_nam.get(req);
    // 保存原始请求上下文到 reply 属性
    r->setProperty("orig", QJsonDocument(payload).toJson());
    connect(r, &QNetworkReply::finished, this, &MedicineModule::onRemoteFinished);
}

void MedicineModule::onRemoteFinished()
{
    QNetworkReply* r = qobject_cast<QNetworkReply*>(sender());
    if (!r)
        return;
    QByteArray data = r->readAll();
    QJsonObject orig;
    orig = QJsonDocument::fromJson(r->property("orig").toByteArray()).object();
    QJsonObject resp;
    resp["type"] = "medications_response";
    if (r->error() != QNetworkReply::NoError) {
        resp["success"] = false;
        resp["error"] = QStringLiteral("远程搜索失败:%1").arg(r->errorString());
    } else {
        QJsonDocument doc = QJsonDocument::fromJson(data);
        QJsonObject j = doc.object();
        QJsonArray arr;
        QJsonObject m;
        m["id"] = 0;
        m["name"] = orig.value("keyword").toString();
        m["generic_name"] = "";
        m["category"] = "远程";
        m["manufacturer"] = "";
        m["specification"] = "";
        m["price"] = 0;
        m["stock_quantity"] = 0;
        m["description"] = j.value("Abstract").toString();
        m["image_url"] = j.value("Image").toString();
        arr.append(m);
        resp["success"] = true;
        resp["data"] = arr;
    }
    sendResponse(resp, orig);
    r->deleteLater();
}

void MedicineModule::loadLocalMeta()
{
    QString metaPath = localMedicationsJsonPath();
    QFile f(metaPath);
    if (!f.exists()) {
        qWarning() << "[MedicineModule] 本地药品元数据不存在:" << metaPath;
        return;
    }
    if (f.open(QIODevice::ReadOnly)) {
        QJsonDocument doc = QJsonDocument::fromJson(f.readAll());
        if (doc.isArray()) {
            m_localMeta = doc.array();
            qInfo() << "[MedicineModule] 载入本地元数据条目数=" << m_localMeta.size();
        } else
            qWarning() << "[MedicineModule] 元数据格式非数组";
    } else {
        qWarning() << "[MedicineModule] 打开元数据失败:" << metaPath;
    }
}

void MedicineModule::sendResponse(QJsonObject resp, const QJsonObject& orig)
{
    if (orig.contains("uuid"))
        resp["request_uuid"] = orig.value("uuid").toString();
    Log::response("Medicine", resp);
    emit businessResponse(resp);
}