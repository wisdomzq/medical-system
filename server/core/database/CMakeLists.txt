set(DATABASE_SOURCES
    database.cpp
    dbterminal.cpp
)

set(DATABASE_HEADERS
    database.h
    dbterminal.h
)

target_sources(${PROJECT_NAME} PRIVATE
    ${DATABASE_SOURCES}
    ${DATABASE_HEADERS}
)