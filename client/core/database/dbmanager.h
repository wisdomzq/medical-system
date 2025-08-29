#pragma once
#include <string>
#include <QString>

class DBManager {
public:
    DBManager(const std::string& dbPath);
    ~DBManager();
    // All database operations are moved to the server.
    // This class will be a proxy for network requests.
};
