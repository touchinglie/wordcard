#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "sqlite3/sqlite3.h"

// ========== Global struct definition (Chinese-English) ==========
struct WordPair {
    char Chinese[100];   // Chinese meaning
    char English[100];   // English word
} template[2000];
int tempcnt = 0;
int DBstatus = SQLITE_ERROR;

// Count file lines
int countFileLines(FILE *fp) {
    if (!fp) return 0;
    char buffer[512];
    int cnt = 0;
    while (fgets(buffer, sizeof(buffer), fp)) cnt++;
    rewind(fp);
    return cnt;
}

// Clear cache
void cleanCache() {
    memset(template, 0, sizeof(template));
    tempcnt = 0;
}

// Initialize database table (does not close database)
int initDB(sqlite3 *db) {
    cleanCache();
    const char *sql = "CREATE TABLE IF NOT EXISTS wordAns ("
                      "ID INTEGER PRIMARY KEY AUTOINCREMENT, "
                      "WORD TEXT NOT NULL, "       // Chinese
                      "ANS TEXT NOT NULL, "        // English
                      "HARD INT NOT NULL);";
    char *errMsg = NULL;
    DBstatus = sqlite3_exec(db, sql, NULL, 0, &errMsg);
    if (DBstatus != SQLITE_OK) {
        fprintf(stderr, "创建表失败: %s\n", errMsg);
        sqlite3_free(errMsg);
    }
    return DBstatus;
}

// Insert data (Chinese, English, difficulty)
int insertData(sqlite3 *db, char *chinese, char *english, int hard) {
    char sql[300];
    snprintf(sql, sizeof(sql), "INSERT INTO wordAns (WORD, ANS, HARD) VALUES ('%s', '%s', %d);", chinese, english, hard);
    char *errMsg = NULL;
    DBstatus = sqlite3_exec(db, sql, NULL, 0, &errMsg);
    if (DBstatus != SQLITE_OK) {
        fprintf(stderr, "插入失败: %s\n", errMsg);
        sqlite3_free(errMsg);
    }
    return DBstatus;
}

// Query callback (called per row, stores result into template)
static int sCallback(void *NotUsed, int argc, char **argv, char **azColName) {
    if (argc >= 3) {  // At least get WORD and ANS columns
        strcpy(template[tempcnt].Chinese, argv[1] ? argv[1] : "");
        strcpy(template[tempcnt].English, argv[2] ? argv[2] : "");
        tempcnt++;
    }
    return 0;
}

// Query words by difficulty
int findData(sqlite3 *db, int hard) {
    cleanCache();
    char sql[50];
    snprintf(sql, sizeof(sql), "SELECT * FROM wordAns WHERE HARD = %d;", hard);
    char *errMsg = NULL;
    DBstatus = sqlite3_exec(db, sql, sCallback, NULL, &errMsg);
    if (DBstatus != SQLITE_OK) {
        fprintf(stderr, "查询失败: %s\n", errMsg);
        sqlite3_free(errMsg);
    }
    return DBstatus;
}
