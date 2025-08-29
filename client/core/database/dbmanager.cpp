#include "dbmanager.h"

DBManager::DBManager(const std::string& dbPath) {
    // This class is now a proxy for network requests.
    // The actual database operations are on the server.
}

DBManager::~DBManager() {
    // No database connection to close.
}
