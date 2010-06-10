/*
 * clog - the fastest and simplest blog on earth
 *
 * Author: Matthias Fassl <mf@x1598.at>
 * Date: 2008-06-21
 * License: AFLv3 http://www.opensource.org/licenses/afl-3.0.php
 */

#define _BSD_SOURCE
#define BUFSIZ 1024

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/dir.h>
#include <sys/stat.h>
#include <errno.h>
#include <time.h>
/* FastCGI Library http://www.fastcgi.com/devkit/doc/fcgi-devel-kit.htm#S3.1 */
#include <fcgi_stdio.h>

#include "config.h"

static int menu(char *dir);
static int isdir(const struct dirent *entry);
static int istxt(const struct dirent *entry);
static int mtimecmp(const struct dirent **a, const struct dirent **b);
static int read_entry(const char *path);
static int read_entries(char *path);
static void print_header();
static void print_footer(); 


int main(void) {
	/* FastCGI */
        while(FCGI_Accept() >= 0) {
		char *query_str, *dir;
	
		/* send Content-type, so that the browser displays something */
		fputs("Content-Type: ",stdout);
		fputs(content_type,stdout);
		fputs("\r\n\r\n",stdout);

		/* read the query string from the url to get the get parameters */
		query_str = getenv("QUERY_STRING");

		/* get the directory that should be read */
		strtok(query_str,"=");
		dir = strtok(NULL,"="); 

		/* change dir to root_dir */
		if(chdir(root_dir) == -1) {
			perror("Couldn't change to root directory");
		}

		print_header();

		fputs("<div class=\"menu\">\r\n",stdout);
		menu(dir);
		fputs("</div>\r\n",stdout);

		/* show all entries in the chosen directory */
		read_entries(dir);

		print_footer();
	}
	return 0;
}

static int 
isdir(struct dirent const *entry) {
	struct stat buf;

	/* eleminiate the . and .. directories from the list */
	if ((strcmp(entry->d_name, ".") == 0) ||
		(strcmp(entry->d_name, "..") == 0)){
		return 0;
	}
	/* get file stats */
	if(stat(entry->d_name,&buf)!=0) {
		perror("couldn't get file stats");
	}
	/* check if file is a directory */
	if(S_ISDIR(buf.st_mode)) {
		return -1;
	}
	return 0;
}

static int
menu(char* dir) {
	struct dirent **directories;
	char *next_dir;
	int num_dir, count;

	/* check if directory path has value and initialize path to next directory */
	if(dir != NULL) {
		next_dir = strchr(dir,'/');
		if(next_dir != NULL)
			*next_dir = '\0';
	}

	/* list all directories in this directory */
	num_dir = scandir(".",&directories,isdir,alphasort);	
	if(num_dir > 0) {
		fputs("<ul>\n",stdout);
		for(count = 0; count < num_dir;count++) {
			fputs("\t<li><a href=\"",stdout);
			fputs(mainfile,stdout);
			fputs(directories[count]->d_name,stdout);
			fputs("\">",stdout);
			fputs(directories[count]->d_name,stdout);
			fputs("</a></li>\n",stdout);
		
			/* list directories in subdirectory */
			if(dir != NULL && strcmp(dir,directories[count]->d_name) == 0) {
				chdir(dir);
				if(next_dir != NULL) {
					next_dir++;
				}
				menu(next_dir);
			}
		}
		fputs("</ul>\n",stdout);
	}
	return 0;
}

static int 
mtimecmp(const struct dirent **file1,const struct dirent **file2) {
	struct stat file1_stat,file2_stat;

	/* get stats for both files */
	stat((*file1)->d_name,&file1_stat);
	stat((*file2)->d_name,&file2_stat);

	/* compare modification time */
	return file1_stat.st_mtime-file2_stat.st_mtime;
}

static int 
istxt(struct dirent const *entry) {
	struct stat buf;
	char *end;

	/* get stats for the file */
        if(stat(entry->d_name,&buf)) {
        	perror("couldn't get stats for file");
	}

	/* check if file is a regular file and if it has and ending (.*) */
        if(S_ISREG(buf.st_mode) && (end=strrchr(entry->d_name,'.'))!=NULL){
		/* check if the ending is .txt (configure via config.h?) */
		if(!strcmp(end,".txt")) {
                       	return -1;
		}
	}

        return 0;
}

static int 
read_entry(const char *path) {
	FILE *entry;
	struct stat entry_stat;
	char *titlesuffix, *content;
	
	titlesuffix=strrchr(path,'.');
	*titlesuffix='\0';

	/* print title */
	fputs("<div class=\"texteintrag\">\r\n<h3>",stdout);
	fputs(path,stdout);
	fputs("</h3>\r\n",stdout);
	*titlesuffix='.';

	/* print modification time */
	if(stat(path,&entry_stat)) {
		perror("couldn't get stats for file");
	}
	fputs("<div class=\"date\">",stdout);
	fputs(ctime(&entry_stat.st_mtime),stdout);
	fputs("</div>\r\n",stdout);

	/* print content */
	content = calloc(entry_stat.st_size+1,1);
	if((entry = fopen(path,"r"))!=NULL){
		fputs("<p>",stdout);
		fread(content,1,entry_stat.st_size,entry);
		/* TODO: convert markdown text here */
		fputs(content,stdout);	
		fputs("</p>\r\n",stdout);
		fputs("</div>\r\n",stdout);

		fclose(entry);
		free(content);
		return -1;
	}	
	return 0;
}

static int 
read_entries(char* path) {
	struct dirent **files;
	int num;

	if(path == NULL) {
		path="/";
	}
		
	fputs("<div class=\"textblock\">\r\n<h2>",stdout);
	/* print path as page title */
	fputs(path,stdout);
	fputs("</h2>\r\n",stdout);

	/* get all regular files that end with .txt and sort them by */
	/* modification time */
	if((num = scandir(".",&files,istxt,mtimecmp)) == -1) {
		perror("scandir failed");
		return -1;
	}
	while(num--){
		/* read each file */
		read_entry(files[num]->d_name);
	}
	fputs("</div>\r\n",stdout);

	free(files);
	return 0;
}

static void 
print_header() {
	fputs("<?xml version=\"1.0\" encoding=\"",stdout);
	fputs(encoding,stdout);
	fputs("\"?>\r\n<!DOCTYPE html PUBLIC \"-//W3C//DTD XHTML 1.1//EN\" \"http://www.w3.org/TR/xhtml11/DTD/xhtml11.dtd\">\r\n<html xmlns=\"http://www.w3.org/1999/xhtml\" xml:lang=\"en\">\r\n<head>\r\n<title>",stdout);
	fputs(title,stdout);
	fputs("</title>\r\n<link rel=\"stylesheet\" type=\"text/css\" href=\"",stdout);
	fputs(cssfile,stdout);
	fputs("\" />\r\n</head>\r\n<body>\r\n<h1>",stdout);
	fputs(title,stdout);
	fputs("</h1>\r\n<p><i>",stdout);
	fputs(slogan,stdout);
	fputs("</i></p>\r\n",stdout);
}

static void 
print_footer() {
	fputs("</body>\r\n</html>\r\n",stdout);
}
