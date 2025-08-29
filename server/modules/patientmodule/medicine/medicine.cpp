 #include "medicine.h"
 #include "core/network/src/server/messagerouter.h"
 #include "core/network/src/protocol.h"
 #include "core/database/database.h"
 #include "core/database/database_config.h"
 #include <QUrlQuery>
 #include <QJsonDocument>
 #include <QJsonArray>
 #include <QSqlError>
#include <QDateTime>
#include <QFile>
#include <QDir>

static QString localMedicationsJsonPath(){
	return QDir::currentPath() + "/../resources/medications/medications.json"; // 运行目录 build/server 下, 回退到项目资源
}
static QString moduleImageDir(){
	// build/server 运行时，源码在 ../server/modules/patientmodule/medicine/img
	QDir d(QDir::currentPath()+"/../server/modules/patientmodule/medicine/img");
	if(d.exists()) return d.path();
	return QString();
}
 
 MedicineModule::MedicineModule(QObject *parent):QObject(parent){
	connect(&MessageRouter::instance(), &MessageRouter::requestReceived,
			 this, &MedicineModule::onRequest);
	loadLocalMeta();
 }
 
 void MedicineModule::onRequest(const QJsonObject &payload){
	 const QString action = payload.value("action").toString();
	 if(action == "get_medications") return handleGetMedications(payload);
	 if(action == "search_medications") return handleSearchMedications(payload);
	 if(action == "search_medications_remote") return handleRemoteSearch(payload);
 }
 
 void MedicineModule::handleGetMedications(const QJsonObject &payload){
	 DBManager db(DatabaseConfig::getDatabasePath());
	 QJsonArray list; bool ok = db.getMedications(list);
	// 合并本地图片与描述补全
	if(ok){
		for(int i=0;i<list.size();++i){
			QJsonObject o = list[i].toObject();
			auto name = o.value("name").toString();
			for(const auto &lmv : m_localMeta){
				QJsonObject lo = lmv.toObject();
				if(lo.value("name").toString() == name){
					if(o.value("description").toString().isEmpty()) o["description"] = lo.value("description");
					if(o.value("precautions").toString().isEmpty()) o["precautions"] = lo.value("precautions");
					QString img = lo.value("image").toString();
					if(!img.isEmpty()){
						QString resPath = QStringLiteral("resources/medications/images/%1").arg(img);
						QString modDir = moduleImageDir();
						QString modPath = modDir.isEmpty()? QString() : (modDir+"/"+img);
						if(QFile::exists(resPath)){
							o["image_path"] = resPath;
						} else if(!modPath.isEmpty() && QFile::exists(modPath)) {
							QFile f(modPath); if(f.open(QIODevice::ReadOnly)){
								QByteArray b64 = f.readAll().toBase64();
								o["image_base64"] = QString::fromLatin1(b64);
							}
						}
					}
					break;
				}
			}
			list[i] = o;
		}
	}
	 QJsonObject resp; resp["type"]="medications_response"; resp["success"]=ok; if(ok) resp["data"]=list; else resp["error"]="获取药品列表失败";
	 sendResponse(resp,payload);
 }
 
 void MedicineModule::handleSearchMedications(const QJsonObject &payload){
	 DBManager db(DatabaseConfig::getDatabasePath());
	 const QString keyword = payload.value("keyword").toString();
	 QJsonArray list; bool ok = db.searchMedications(keyword, list);
	if(ok && !keyword.isEmpty()){
		// 将本地 meta 中名称或通用名或分类包含关键字的条目（若数据库未返回）补充
		QSet<QString> existing;
		for(const auto &v: list) existing.insert(v.toObject().value("name").toString());
		for(const auto &mv : m_localMeta){
			QJsonObject lo = mv.toObject();
			QString nm = lo.value("name").toString();
			if(existing.contains(nm)) continue;
			QString concat = nm + lo.value("generic_name").toString() + lo.value("category").toString();
			if(concat.contains(keyword, Qt::CaseInsensitive)){
				QJsonObject o;
				o["id"] = 0; // 本地未入库
				o["name"] = nm;
				o["generic_name"] = lo.value("generic_name");
				o["category"] = lo.value("category");
				o["manufacturer"] = lo.value("manufacturer");
				o["specification"] = lo.value("specification");
				o["unit"] = lo.value("unit");
				o["price"] = lo.value("price");
				o["stock_quantity"] = lo.value("stock_quantity");
				o["description"] = lo.value("description");
				o["precautions"] = lo.value("precautions");
				QString img = lo.value("image").toString();
				if(!img.isEmpty()){
					QString resPath = QStringLiteral("resources/medications/images/%1").arg(img);
					QString modDir = moduleImageDir();
					QString modPath = modDir.isEmpty()? QString() : (modDir+"/"+img);
					if(QFile::exists(resPath)) o["image_path"] = resPath; else if(!modPath.isEmpty() && QFile::exists(modPath)) { QFile f(modPath); if(f.open(QIODevice::ReadOnly)) o["image_base64"] = QString::fromLatin1(f.readAll().toBase64()); }
				}
				list.append(o);
			}
		}
	}
	 QJsonObject resp; resp["type"]="medications_response"; resp["success"]=ok; if(ok) resp["data"]=list; else resp["error"]="搜索药品失败";
	 sendResponse(resp,payload);
 }
 
 void MedicineModule::handleRemoteSearch(const QJsonObject &payload){
	 const QString keyword = payload.value("keyword").toString();
	 // DuckDuckGo 简易查询 (公共 API, 无密钥) 仅作示例
	 QUrl url("https://api.duckduckgo.com/");
	 QUrlQuery q; q.addQueryItem("q", keyword + QStringLiteral(" 药品")); q.addQueryItem("format","json"); q.addQueryItem("no_redirect","1"); q.addQueryItem("no_html","1");
	 url.setQuery(q);
	 QNetworkRequest req(url); req.setHeader(QNetworkRequest::UserAgentHeader, "MedicalSystem/1.0");
	 QNetworkReply *r = m_nam.get(req);
	 // 保存原始请求上下文到 reply 属性
	 r->setProperty("orig", QJsonDocument(payload).toJson());
	 connect(r,&QNetworkReply::finished,this,&MedicineModule::onRemoteFinished);
 }
 
 void MedicineModule::onRemoteFinished(){
	 QNetworkReply *r = qobject_cast<QNetworkReply*>(sender()); if(!r) return; QByteArray data = r->readAll(); QJsonObject orig;
	 orig = QJsonDocument::fromJson(r->property("orig").toByteArray()).object();
	 QJsonObject resp; resp["type"]="medications_response";
	 if(r->error()!=QNetworkReply::NoError){
		 resp["success"] = false; resp["error"] = QStringLiteral("远程搜索失败:%1").arg(r->errorString());
	 } else {
		 QJsonDocument doc = QJsonDocument::fromJson(data); QJsonObject j = doc.object();
		 QJsonArray arr; QJsonObject m; m["id"] = 0; m["name"] = orig.value("keyword").toString(); m["generic_name"]=""; m["category"]="远程"; m["manufacturer"]=""; m["specification"]=""; m["price"]=0; m["stock_quantity"]=0; m["description"] = j.value("Abstract").toString(); m["image_url"] = j.value("Image").toString(); arr.append(m);
		 resp["success"]=true; resp["data"]=arr;
	 }
	 sendResponse(resp,orig);
	 r->deleteLater();
 }

void MedicineModule::loadLocalMeta(){
	QFile f(localMedicationsJsonPath());
	if(!f.exists()) return;
	if(f.open(QIODevice::ReadOnly)){
		QJsonDocument doc = QJsonDocument::fromJson(f.readAll());
		if(doc.isArray()) m_localMeta = doc.array();
	}
}
 
 void MedicineModule::sendResponse(QJsonObject resp, const QJsonObject &orig){
	 if(orig.contains("uuid")) resp["request_uuid"] = orig.value("uuid").toString();
	 MessageRouter::instance().onBusinessResponse(Protocol::MessageType::JsonResponse, resp);
 }
