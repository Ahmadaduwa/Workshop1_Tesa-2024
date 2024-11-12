#include "db_helper.h"
#include <stdio.h>
#include <sqlite3.h> // เพิ่มการ include sqlite3.h ที่นี่

// private constants
const char INIT_SQL_CMD[] = "CREATE TABLE IF NOT EXISTS data_table ("
                            "_id INTEGER PRIMARY KEY AUTOINCREMENT, "
                            "timestamp DATETIME DEFAULT CURRENT_TIMESTAMP, "
                            "value INTEGER)";

const char APPEND_SQL_CMD[] = "INSERT INTO data_table (value) VALUES (?)";
const char QUERY_SQL_CMD[] = "SELECT * FROM data_table ORDER BY timestamp DESC LIMIT 1";

// initialize database
void dbase_init(const char *db_name) {
    sqlite3 *db;

    if (sqlite3_open(db_name, &db) != SQLITE_OK) {
        fprintf(stderr, "Error opening %s database\n", db_name);
        sqlite3_close(db);
        return;
    }

    if (sqlite3_exec(db, INIT_SQL_CMD, NULL, NULL, NULL) != SQLITE_OK) {
        fprintf(stderr, "Error executing INIT_SQL_CMD: %s\n", sqlite3_errmsg(db));
        sqlite3_close(db);  // ปิดฐานข้อมูลหากเกิดข้อผิดพลาด
        return;
    } else {
        printf("Database and table initialized successfully.\n");
    }

    sqlite3_close(db);
}

// append data to the table
int dbase_append(const char *db_name, int value) {
    sqlite3 *db;
    sqlite3_stmt *stmt;

    if (sqlite3_open(db_name, &db) != SQLITE_OK) {
        fprintf(stderr, "Error opening %s database\n", db_name);
        sqlite3_close(db);
        return -1;
    }
    if (sqlite3_prepare_v2(db, APPEND_SQL_CMD, -1, &stmt, NULL) != SQLITE_OK) {
        fprintf(stderr, "Error executing APPEND_SQL_CMD: %s\n", sqlite3_errmsg(db));
        if (stmt) sqlite3_finalize(stmt);  // ปิด stmt หากมีข้อผิดพลาด
        sqlite3_close(db);
        return -1;
    }
    sqlite3_bind_int(stmt, 1, value);
    if (sqlite3_step(stmt) != SQLITE_DONE) {
        fprintf(stderr, "Error executing step in APPEND_SQL_CMD\n");
    }

    sqlite3_finalize(stmt);
    sqlite3_close(db);
    return 0;
}

// query last value from the table
int dbase_query(const char *db_name) {
    sqlite3 *db;
    sqlite3_stmt *stmt;
    int last_value = -1;

    if (sqlite3_open(db_name, &db) != SQLITE_OK) {
        fprintf(stderr, "Error opening %s database\n", db_name);
        sqlite3_close(db);
        return -1;
    }
    if (sqlite3_prepare_v2(db, QUERY_SQL_CMD, -1, &stmt, NULL) != SQLITE_OK) {
        fprintf(stderr, "Error executing QUERY_SQL_CMD: %s\n", sqlite3_errmsg(db)); 
        sqlite3_close(db);
        return -1;
    }
    if (sqlite3_step(stmt) == SQLITE_ROW) {
        const unsigned char *timestamp = sqlite3_column_text(stmt, 1);
        if (timestamp) {
            printf("Data timestamp: %s\n", timestamp);
        } else {
            printf("Timestamp is NULL\n");
        }
        last_value = sqlite3_column_int(stmt, 2); 
    }
    sqlite3_finalize(stmt);
    sqlite3_close(db);
    return last_value;
}
