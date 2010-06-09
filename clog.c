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
//FastCGI Library http://www.fastcgi.com/devkit/doc/fcgi-devel-kit.htm#S3.1
#include <fcgi_stdio.h>

#include "config.h"

static int 
get_dir(struct dirent const *entry) {
	struct stat buf;

	//eleminiate the . and .. directories from the list
	if ((strcmp(entry->d_name, ".") == 0) ||
		(strcmp(entry->d_name, "..") == 0)){
		return 0;
	}
	else{
		//get file stats
		if(stat(entry->d_name,&buf)!=0)
			perror(NULL);

		//check if file is a directory
		if(S_ISDIR(buf.st_mode))
			return -1;
	}
	return 0;
}

static int
menu(char* dir,char* lastdir) {
	struct dirent **files;
	char *firstpart,*lastpart;

	//extract the first directory and save the rest of the path
	firstpart = strtok(dir,"/");
	lastpart = strtok(NULL,"\0");

	if(lastdir!=NULL && strcmp(lastdir,".")!=0){
		size_t lastdirlen=strlen(lastdir);
		size_t firstpartlen=strlen(firstpart);
		char* buffer=(char*)malloc(firstpartlen+lastdirlen+2);
		memcpy(buffer,lastdir,lastdirlen);
		memcpy(buffer+lastdirlen,"/",1);
		memcpy(buffer+lastdirlen+1,firstpart,firstpartlen);
		lastdir=buffer;
	}
	else if(strcmp(firstpart,".")!=0){
		size_t firstpartlen=strlen(firstpart)+1;
		lastdir=(char*)malloc(firstpartlen);
		memcpy(lastdir,firstpart,firstpartlen);
	}
	if(chdir(firstpart)==-1) 
		perror(NULL);
		
	int n=scandir(".",&files,get_dir,alphasort);	
	if(n>0){ 
		fputs("<ul>\n",stdout);
		int count;
		for(count=0;count<n;count++){
			fputs("\t<li><a href=\"",stdout);
			fputs(mainfile,stdout);

			if(lastdir!=NULL){
				fputs(lastdir,stdout);
				fputs("/",stdout);
			}

			fputs(files[count]->d_name,stdout);
			fputs("\">",stdout);
			fputs(files[count]->d_name,stdout);
			fputs("</a></li>\n",stdout);

			if(lastpart !=NULL &&
				strstr(lastpart,files[count]->d_name)==lastpart)
				menu(lastpart,lastdir);
		}
		fputs("</ul>\n",stdout);
	}

	free(files);
	free(lastdir);

	return 0;
}

static int 
mtimesort(const struct dirent **a,const struct dirent **b) {
	struct stat filbuf1,filbuf2;

	//get stats for both files
	stat((*a)->d_name,&filbuf1);
	stat((*b)->d_name,&filbuf2);

	//compare modification time
	if(filbuf1.st_mtime<filbuf2.st_mtime)
		return -1;
	if(filbuf1.st_mtime==filbuf2.st_mtime)
		return 0;
	else
		return 1;
}

static int 
get_txt(struct dirent const *entry) {
	struct stat buf;
	char *end;

	//get stats for the file
        if(stat(entry->d_name,&buf))
        	perror(NULL);

	//check if file is a regular file and if it has and ending (.*)
        if(S_ISREG(buf.st_mode) && (end=strrchr(entry->d_name,'.'))!=NULL){
		//check if the ending is .txt (configure via config.h?)
		if(!strcmp(end,".txt"))
                       	return -1;
	}

        return 0;
}

static int 
read_entry(char *path) {
	FILE* entry;
	struct stat entrybuf;
	char* entry_title;
	
	size_t pathlen=strlen(path)+1;
	entry_title=(char*)malloc(pathlen);
	memcpy(entry_title,path,pathlen);
	char* titlesuffix=strrchr(entry_title,'.');
	*titlesuffix='\0';

	//print title
	fputs("<div class=\"texteintrag\">\r\n<h3>",stdout);
	fputs(entry_title,stdout);
	fputs("</h3>\r\n",stdout);
	free(entry_title);

	//print modification time
	if(stat(path,&entrybuf))
		perror(NULL);
	fputs("<div class=\"date\">",stdout);
	fputs(ctime(&entrybuf.st_mtime),stdout);
	fputs("</div>\r\n",stdout);

	//print content
	if((entry = fopen(path,"r"))!=NULL){
		fputs("<p>",stdout);
		char foo;
		while((foo=fgetc(entry))!=EOF)
			fputc(foo,stdout);	
		fputs("</p>\r\n",stdout);
		
		fclose(entry);
		fputs("</div>\r\n",stdout);
	}	
	else
		return 0;

	return -1;
}

static int 
read_entries(char* path) {
	struct dirent **files;

	if(path==NULL)
		path="/";
		
	fputs("<div class=\"textblock\">\r\n<h2>",stdout);
	//print path as page title
	fputs(path,stdout);
	fputs("</h2>\r\n",stdout);

	//get all regular files that end with .txt and sort them by
	//modification time
	int num = scandir(".",&files,get_txt,mtimesort);
	while(num--){
		//read each file
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

int main(void) {
	//FastCGI
	int count = 0;
        while(FCGI_Accept() >= 0) {
		char *query_str,*dir,*menu_path;
	
		//send Content-type, so that the browser displays something
		fputs("Content-Type: ",stdout);
		fputs(content_type,stdout);
		fputs("\r\n\r\n",stdout);

		//read the query string from the url to get the get parameters
		query_str = getenv("QUERY_STRING");

		//get the directory that should be read
		strtok(query_str,"=");
		dir = strtok(NULL,"="); 

		//change dir to root_dir
		if(chdir(root_dir)==-1)
			perror(NULL);

		//show menu
		if(dir!=NULL){
			size_t len = strlen (dir);
			menu_path = (char *) malloc (len + 3);
			menu_path[0] = '.';
			menu_path[1] = '/';
			memcpy (menu_path+2, dir, len + 1);
		}
		else{
			menu_path=(char*)malloc(2);
			menu_path[0]='.';
			menu_path[1]='\0';
		}
		print_header();

		fputs("<div class=\"menu\">\r\n",stdout);
		menu(menu_path,NULL);
		fputs("</div>\r\n",stdout);

		//show all entries in the chosen directory
		read_entries(dir);

		print_footer();

		free(menu_path);
	}
	return 0;
}
