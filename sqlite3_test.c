#define _GNU_SOURCE
#include <sys/stat.h>
#include <time.h>
#include <unistd.h>
#include <fcntl.h>
#include "sqlite3.h"
#include "sqliteInt.h"

#define SZP_EVENTDB_PUSH_ROOT				"/tmp"					// The root path of the event push database files.
#define SZ_EVENTDB_EXT					".db"					// The extension file name of event database file.
#define MS_DB_BUSY_TIMEOUT				20000					// The timeout of access database.
#define CN_MAX_EVENT_PER_DB				1000					// The maximum event node per database file
#define	INC_ERR_EVENTDB					512					// The increment for the event database error.
#define PATH_MAX                			4096
#define CREATE_NUMBER					1280000

#define CMD_CREATE_EVENT_POOL_TABLE "CREATE TABLE EVENT_POOL \
		( \
			event_id INTEGER PRIMARY KEY UNIQUE, \
			event_info BLOB, \
			event_path_len INTEGER,\
			event_link TEXT, \
			event_path TEXT  \
		);"

static int fd;

int FNE_Pool_Create_DB(const char *pszDBPath)
{
	int iRet = 0;
	char str[2048];
	sqlite3* psqlDB = NULL;

	if(SQLITE_OK != (iRet = sqlite3_open(pszDBPath, &psqlDB)))
	{
		snprintf(str, sizeof(str), "Can't open database %s when create table!(%d)\n\n\n", pszDBPath, iRet);
		write(fd, str, strlen(str));
		goto exit;
	}
	// default busy timeout is 20sec, this can be tuned.
	if(SQLITE_OK != (iRet = sqlite3_busy_timeout(psqlDB, MS_DB_BUSY_TIMEOUT)))
	{
		snprintf(str, sizeof(str), "Can't set busy timeout for %s!(%d)\n\n\n", pszDBPath, iRet);
		write(fd, str, strlen(str));
		goto exit;
	}
	if(SQLITE_OK != (iRet = sqlite3_exec(psqlDB, CMD_CREATE_EVENT_POOL_TABLE, NULL, NULL, NULL)))
	{
		snprintf(str, sizeof(str), "Can't create table for %s!(%d)\n", pszDBPath, iRet);
		write(fd, str, strlen(str));
		goto exit;
	}
exit:

	if(psqlDB)	sqlite3_close(psqlDB);	
	return iRet;
}

int main(int argc, char** argv)
{
	int iRet = 0;
	char* szDBFile = argv[1];
	char* fname="/share/Public/qsync.log";
	fd = open(fname, O_WRONLY | O_CREAT);
	
char newline[256];
char cmd2[4096] = {""};
sprintf(cmd2,"./test");
FILE *fd2 = popen(cmd2, "r");
while((fgets(newline, 256, fd2)) != NULL) {
  iRet = atoi(newline);
   printf("We've got a newline %s", newline);
}
pclose(fd2);

//	iRet = FNE_Pool_Create_DB(szDBFile);
exit:
	close(fd);
	if(iRet != 0)
        {
                char cmd[4096] = {""};
                sprintf(cmd,"df -h >> /share/Public/qsync.log;free >> /share/Public/qsync.log");
                system(cmd);
        }
	printf("%d",iRet);
	return iRet;
}
