/*
 * clog - the fastest and simplest blog on earth
 *
 * Author: Matthias Fassl <mf@x1598.at>
 * Date: 2008-06-21
 * License: AFLv3 http://www.opensource.org/licenses/afl-3.0.php
 */

#define _BSD_SOURCE

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/dir.h>
#include <sys/stat.h>
#include <errno.h>
#include <time.h>

#include "clog.h"
#include "config.h"

int
main(int argc, char *argv[]) {
/*        while(FCGI_Accept() >= 0) { */
	while(1) {
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
		break;
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
read_entries(char* path) {
	struct dirent **files;
	int num, num_filehandlers;

	if(path == NULL) {
		path="/";
	}
		
	fputs("<div class=\"textblock\">\r\n<h2>",stdout);
	/* print path as page title */
	fputs(path,stdout);
	fputs("</h2>\r\n",stdout);

	/* get all regular files that end with .txt and sort them by */
	/* modification time */
	/* TODO: do this for all configured filehandlers */
	
	num_filehandlers = sizeof(filehandler)/sizeof(Filehandler);
	while(num_filehandlers--) {
		if((num = scandir(".",&files,filehandler[num_filehandlers].filter,&mtimecmp)) == -1) {
			perror("scandir failed");
			return -1;
		}
		while(num--){
			/* read each file */
			filehandler[num_filehandlers].print(files[num]->d_name);
		}
		free(files);
	}
	fputs("</div>\r\n",stdout);

	return 0;
}

/* Filters */
int 
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

int
isText(const struct dirent *entry) {
	return check_fileending(entry,".txt");
}

int
isMarkdown(const struct dirent *entry) {
	return check_fileending(entry,".markdown");
}

int
isPDF(const struct dirent *entry) {
	return check_fileending(entry,".pdf");
}

int
isPicture(const struct dirent *entry) {
	if(	check_fileending(entry,".jpg") ||
		check_fileending(entry,".png") ||
		check_fileending(entry,".gif"))  {
		return -1;
	}
	return 0;		
}

int 
check_fileending(struct dirent const *entry, char *fileending) {
	struct stat buf;
	char *end;

	/* get stats for the file */
        if(stat(entry->d_name,&buf)) {
        	perror("couldn't get stats for file");
	}

	/* check if file is a regular file and if it has and ending (.*) */
        if(S_ISREG(buf.st_mode) && (end=strrchr(entry->d_name,'.'))!=NULL){
		/* check if the ending is .txt (configure via config.h?) */
		if(!strcmp(end,fileending)) {
                       	return -1;
		}
	}

        return 0;
}

/* Actions for Filetypes */
int 
showContent(const char *path, const char *content) {
	struct stat entry_stat;
	char *titlesuffix;
	
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
	fputs("<p>",stdout);
	fputs(content,stdout);	
	fputs("</p>\r\n",stdout);
	fputs("</div>\r\n",stdout);
	
	return -1;
}

int
showText(const char *path) {
	FILE *entry;
	struct stat entry_stat;
	char *content;
	
	stat(path,&entry_stat);
	content = calloc(entry_stat.st_size+1,1);
	if((entry = fopen(path,"r"))!=NULL){
		fread(content,1,entry_stat.st_size,entry);
		fclose(entry);
	}	
	showContent(path,content);
	free(content);
	return -1;
}

int
showPicture(const char *path) {
	char *formatstring = "<img src=\"%s\" />";
	char *content = calloc(strlen(formatstring)+strlen(path)+1,1);
	sprintf(content,formatstring,path);

	showContent(path,content);
	free(content);
	return -1;
}

int 
mtimecmp(const struct dirent **file1,const struct dirent **file2) {
	struct stat file1_stat,file2_stat;

	/* get stats for both files */
	stat((*file1)->d_name,&file1_stat);
	stat((*file2)->d_name,&file2_stat);

	/* compare modification time */
	return file1_stat.st_mtime-file2_stat.st_mtime;
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
