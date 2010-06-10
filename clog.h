typedef struct {
        char name[];
        int (*filter)(const struct dirent **file);
        int (*print)(char *file);
} Filehandler;

static int menu(char *dir);
static int isdir(const struct dirent *entry);
static int istxt(const struct dirent *entry);
static int mtimecmp(const struct dirent **a, const struct dirent **b);
static int readText(const char *path);
static int read_entries(char *path);
static void print_header();
static void print_footer(); 
