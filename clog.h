typedef struct {
        char *name;
        int (*filter)(const struct dirent *file);
        int (*print)(const char *file);
} Filehandler;

static int menu(char *dir);
static int read_entries(char *path);
int isdir(const struct dirent *entry);
int isText(const struct dirent *entry);
int isMarkdown(const struct dirent *entry);
int isPDF(const struct dirent *entry);
int isPicture(const struct dirent *entry);
int check_fileending(const struct dirent *entry,char *fileending);
int showText(const char *path);
int showMarkdown(const char *path);
int showPicture(const char *path);
int showContent(const char *path,const char *content);
int mtimecmp(const struct dirent **a, const struct dirent **b);
static void print_header();
static void print_footer(); 
