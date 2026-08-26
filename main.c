#include <gtk/gtk.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <stdbool.h>
#include <time.h>

#include "function.c"      // Include SQLite operations and template struct

// ========== Command-line argument global variables (assigned by GOption) ==========
int test_difficulty = 0;   // Default difficulty: 0 (easy)
int test_count = 0;        // 0 means test all words of that difficulty

// ========== UI global variables ==========
bool startAns = false;
int currentWordIndex = 0;
GtkWidget *main_window = NULL;
GtkWidget *entry = NULL;
GtkLabel *tipLabel = NULL;
GtkWidget *btnStart = NULL;
GtkWidget *btnClose = NULL;

// Temporary storage arrays by difficulty (used during loading)
struct WordPair hardPairs[2000];
int hardCount = 0;

struct WordPair medPairs[2000];
int medCount = 0;

struct WordPair simPairs[2000];
int simCount = 0;

// ========== Function forward declarations ==========
static void tryToEntry(GtkButton *btn, gpointer user_data);
static void closeWindow(GtkButton *btn, GtkWindow *window);
static void enterAnsCallBack(GtkWidget *widget, gpointer data);
static void nextWord(void);
static int tryToExcuteSQL(sqlite3 *db, char *sql);
static void shuffle_words(void);

// ========== Core functions ==========

static void nextWord(void) {
    if (currentWordIndex < tempcnt) {
        gtk_label_set_text(tipLabel, template[currentWordIndex].Chinese);  // Display Chinese
        gtk_editable_set_text(GTK_EDITABLE(entry), "");
        startAns = true;
        gtk_widget_set_sensitive(entry, TRUE);
        gtk_widget_grab_focus(entry);
    } else {
        gtk_label_set_text(tipLabel, "测试已全部通过，恭喜！");
        if (main_window) {
            gtk_widget_grab_focus(main_window);
        }
        startAns = false;
        gtk_widget_set_sensitive(entry, FALSE);
    }
}

static void shuffle_words(void) {
    if (tempcnt <= 1) return;
    for (int i = tempcnt - 1; i > 0; i--) {
        int j = rand() % (i + 1);
        struct WordPair tmp = template[i];
        template[i] = template[j];
        template[j] = tmp;
    }
}

static void tryToEntry(GtkButton *btn, gpointer user_data) {
    gtk_widget_set_sensitive(GTK_WIDGET(btn), FALSE);

    sqlite3 *db = NULL;
    DBstatus = sqlite3_open("words/Words.db", &db);
    if (DBstatus != SQLITE_OK) {
        gtk_label_set_text(tipLabel, "打开数据库失败！");
        gtk_widget_set_sensitive(GTK_WIDGET(btn), TRUE);
        return;
    }

    tryToExcuteSQL(db, "DROP TABLE IF EXISTS wordAns;");
    initDB(db);

    FILE *fp = fopen("words/words.txt", "r");
    if (fp == NULL) {
        gtk_label_set_text(tipLabel, "打开单词库文件失败，请检查路径！");
        sqlite3_close(db);
        gtk_widget_set_sensitive(GTK_WIDGET(btn), TRUE);
        return;
    }

    int lineCount = countFileLines(fp);
    if (lineCount == 0) {
        gtk_label_set_text(tipLabel, "单词库文件为空！");
        fclose(fp);
        sqlite3_close(db);
        gtk_widget_set_sensitive(GTK_WIDGET(btn), TRUE);
        return;
    }
    if (lineCount > 2000) {
        gtk_label_set_text(tipLabel, "单词库行数超过2000，请分割！");
        fclose(fp);
        sqlite3_close(db);
        gtk_widget_set_sensitive(GTK_WIDGET(btn), TRUE);
        return;
    }

    gtk_label_set_text(tipLabel, "正在加载单词库，请稍候……");

    char buffer[512];
    rewind(fp);
    hardCount = medCount = simCount = 0;

    while (fgets(buffer, sizeof(buffer), fp)) {
        buffer[strcspn(buffer, "\n")] = 0;
        char *chinese = strtok(buffer, " ");
        char *english = strtok(NULL, " ");
        if (chinese == NULL || english == NULL) {
            gtk_label_set_text(tipLabel, "文件格式错误（每行应为 释义 单词）");
            fclose(fp);
            sqlite3_close(db);
            gtk_widget_set_sensitive(GTK_WIDGET(btn), TRUE);
            return;
        }
        int len = strlen(chinese);
        if (len > 30) {
            strcpy(hardPairs[hardCount].Chinese, chinese);
            strcpy(hardPairs[hardCount].English, english);
            hardCount++;
        } else if (len > 10) {
            strcpy(medPairs[medCount].Chinese, chinese);
            strcpy(medPairs[medCount].English, english);
            medCount++;
        } else {
            strcpy(simPairs[simCount].Chinese, chinese);
            strcpy(simPairs[simCount].English, english);
            simCount++;
        }
    }
    fclose(fp);

    // Write to database
    for (int i = 0; i < hardCount; i++)
        insertData(db, hardPairs[i].Chinese, hardPairs[i].English, 2);
    for (int i = 0; i < medCount; i++)
        insertData(db, medPairs[i].Chinese, medPairs[i].English, 1);
    for (int i = 0; i < simCount; i++)
        insertData(db, simPairs[i].Chinese, simPairs[i].English, 0);

    gtk_label_set_text(tipLabel, "加载完成，准备测试……");

    findData(db, test_difficulty);   // Query by specified difficulty
    sqlite3_close(db);

    // Shuffle order, truncate if count specified
    shuffle_words();
    if (test_count > 0 && test_count < tempcnt) {
        tempcnt = test_count;
    }

    // Switch UI
    gtk_widget_set_visible(btnStart, FALSE);
    gtk_widget_set_visible(btnClose, FALSE);
    gtk_widget_set_visible(entry, TRUE);
    gtk_widget_set_sensitive(entry, TRUE);

    currentWordIndex = 0;
    nextWord();

    gtk_widget_set_sensitive(GTK_WIDGET(btn), TRUE);
}

static void enterAnsCallBack(GtkWidget *widget, gpointer data) {
    if (!startAns) return;
    const gchar *userAns = gtk_editable_get_text(GTK_EDITABLE(widget));
    if (strcmp(userAns, template[currentWordIndex].English) == 0) {
        g_print("回答正确！\n");
        currentWordIndex++;
        nextWord();
    } else {
        g_print("回答错误，再试一次。\n");
        gtk_editable_set_text(GTK_EDITABLE(widget), "");
        gtk_widget_grab_focus(widget);
    }
}

static void closeWindow(GtkButton *btn, GtkWindow *window) {
    gtk_window_destroy(window);
}

static int tryToExcuteSQL(sqlite3 *db, char *sql) {
    char *errMsg = NULL;
    int rc = sqlite3_exec(db, sql, NULL, 0, &errMsg);
    if (rc != SQLITE_OK) {
        g_print("SQL错误: %s\n", errMsg);
        sqlite3_free(errMsg);
    }
    return rc;
}

// ========== UI initialization ==========
static void activate(GtkApplication *app, gpointer user_data) {
    main_window = gtk_application_window_new(app);
    gtk_window_set_title(GTK_WINDOW(main_window), "单词卡");
    gtk_window_set_default_size(GTK_WINDOW(main_window), 800, 600);

    GtkWidget *vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 30);
    gtk_window_set_child(GTK_WINDOW(main_window), vbox);

    // Display current options
    char info[256];
    snprintf(info, sizeof(info),
             "请创建 words/words.txt\n当前难度: %d  数量: %s",
             test_difficulty, test_count == 0 ? "全部" : "随机抽取");
    tipLabel = GTK_LABEL(gtk_label_new(info));
    gtk_label_set_wrap(GTK_LABEL(tipLabel), TRUE);
    gtk_box_append(GTK_BOX(vbox), GTK_WIDGET(tipLabel));

    entry = gtk_entry_new();
    gtk_entry_set_placeholder_text(GTK_ENTRY(entry), "输入英文单词后按回车");
    gtk_widget_set_visible(entry, FALSE);
    gtk_widget_set_sensitive(entry, FALSE);
    g_signal_connect(entry, "activate", G_CALLBACK(enterAnsCallBack), NULL);
    gtk_box_append(GTK_BOX(vbox), entry);

    GtkWidget *hbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 20);
    gtk_box_append(GTK_BOX(vbox), hbox);

    btnStart = gtk_button_new_with_label("准备好了！");
    btnClose = gtk_button_new_with_label("退出");

    g_signal_connect(btnStart, "clicked", G_CALLBACK(tryToEntry), NULL);
    g_signal_connect(btnClose, "clicked", G_CALLBACK(closeWindow), main_window);

    gtk_box_append(GTK_BOX(hbox), btnStart);
    gtk_box_append(GTK_BOX(hbox), btnClose);
    
    gtk_widget_set_halign(vbox, GTK_ALIGN_CENTER);
    gtk_widget_set_valign(vbox, GTK_ALIGN_CENTER);
    gtk_widget_set_halign(GTK_WIDGET(tipLabel), GTK_ALIGN_CENTER);
    gtk_label_set_xalign(GTK_LABEL(tipLabel), 0.5);
    gtk_widget_set_halign(hbox, GTK_ALIGN_CENTER);

    gtk_window_present(GTK_WINDOW(main_window));
}

// ========== Main function (uses GOption to parse arguments) ==========
int main(int argc, char **argv) {
    // Custom command-line options
    GOptionEntry entries[] = {
        {"difficulty", 'd', 0, G_OPTION_ARG_INT, &test_difficulty,
         "测试难度（0简单，1中等，2困难）", "0-2"},
        {"number", 'n', 0, G_OPTION_ARG_INT, &test_count,
         "测试单词数量（0表示全部）", "N"},
        {NULL}
    };

    GtkApplication *app = gtk_application_new("rrnu.dowblog.example", G_APPLICATION_DEFAULT_FLAGS);
    g_application_add_main_option_entries(G_APPLICATION(app), entries);
    g_signal_connect(app, "activate", G_CALLBACK(activate), NULL);

    srand((unsigned)time(NULL));

    int status = g_application_run(G_APPLICATION(app), argc, argv);
    g_object_unref(app);
    return status;
}
