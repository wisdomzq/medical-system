#ifndef DATABASE_CONFIG_H
#define DATABASE_CONFIG_H

#include <QString>
#include <QDir>
#include <QCoreApplication>

class DatabaseConfig {
public:
    static QString getDatabasePath() {
        QDir dir(QCoreApplication::applicationDirPath());
        // 向上查找直到找到 data 目录，最多上溯4级
        for (int i = 0; i < 4 && !dir.exists("data"); ++i) {
            dir.cdUp();
        }
        return dir.filePath("data/user.db");
    }
};

#endif // DATABASE_CONFIG_H
