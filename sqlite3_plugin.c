#define _GNU_SOURCE
#include <sys/stat.h>
#include <time.h>
#include <unistd.h>
#include <fcntl.h>
#include "sqlite3.h"
#include "sqliteInt.h"
#include <stdint.h>

#define SZP_EVENTDB_PUSH_ROOT				"/tmp"					// The root path of the event push database files.
#define SZP_EVENTDB_POP_ROOT			"/etc/qsync/home/event"	// The root path of the event pop database files.
#define SZ_EVENTDB_EXT					".db"					// The extension file name of event database file.
#define MS_DB_BUSY_TIMEOUT				20000					// The timeout of access database.
#define CN_MAX_EVENT_PER_DB				1000					// The maximum event node per database file
#define	INC_ERR_EVENTDB					512					// The increment for the event database error.
#define PATH_MAX                			4096
#define CREATE_NUMBER					10000
#define MAX_DB_NUMBER					1000

#define	ENOMEM		12	/* Out of memory */
#define	EINVAL		22	/* Invalid argument */
#define	ENODATA		61	/* No data available */
#define CMD_INSERT_EVENT				"INSERT INTO EVENT_POOL(event_info,event_path_len,event_link,event_path) VALUES (?1, ?2, ?3, ?4);"
#define CMD_DELETE_EVENT				"DELETE FROM EVENT_POOL WHERE event_id = %d;"
#define CMD_QUERY_ONE_EVENT				"SELECT * FROM EVENT_POOL LIMIT 1;"
#define	FN_SYMLINK			0x00000020L		///< symlink old_file symbolic_file
#define	FN_LINK				0x00000040L		///< link old_file new_file
#define	FN_RENAME			0x00000080L		///< rename old_file new_file

typedef enum { FALSE = 0, TRUE = 1 } BOOL;

#define CMD_CREATE_EVENT_POOL_TABLE "CREATE TABLE EVENT_POOL \
		( \
			event_id INTEGER PRIMARY KEY UNIQUE, \
			event_info BLOB, \
			event_path_len INTEGER,\
			event_link TEXT, \
			event_path TEXT  \
		);"

typedef struct _T_FNE_PROP1_
{
        uint32_t                        mFlag;          // The open / close flag of this file. (open, close)
        uint32_t                        mMode;          // The mode of this file. (open, chmod, mkdir)
        uint32_t                        idUser;         // The user ID of this file. (chown)
        uint32_t                        idGroup;        // The group ID of this file. (chown)
        uint32_t                        cnsModify;      // The modify time of this file in nano-second. (chtime)
        uint32_t                        csModify;       // The modify time of this file in sec. (chtime)
        uint32_t                        cnWrite;        // The number of write segments. (write)
        int64_t                         cbFile;         // The size of this file in byte. (truncate)
} T_FNE_PROP1, *PT_FNE_PROP1;

typedef struct _T_FNE_PROP2_
{
        struct _T_FNE_NODE_     *ptRename;      // The pointer of the next rename node.
        uint32_t                        cbOld;          // The size of the new path in byte (exclude the NULL char). (symlink, rename, link)
        char                            *pszpOld;       // The new path of this file. (symlink, rename, link)
} T_FNE_PROP2, *PT_FNE_PROP2;

typedef struct _T_FNE_NODE_
{
	struct _T_FNE_NODE_	*ptPeer;	// The pointer of the next peer with the same hash key.
	uint64_t			iTime;		// The time stamp of this entry.
	uint32_t			iHash;		// The hash key of this node.
	int32_t				idProc;		// The ID of the process that takes the action.
	uint32_t			mFlag;		// The bit mask to indicate the status. (FLAG_FNE_XXX)
	uint32_t			mCode;		// The bit mask to indicate the operation code. (all)
	uint32_t			cbPath;		// The size of the path in byte (exclude the NULL char). (all)
	union
	{
		T_FNE_PROP1		tepFile;
		T_FNE_PROP2		tepFiles;
	};
	char				szpFile[];	// The path of this file. (all)
} T_FNE_NODE, *PT_FNE_NODE, **PPT_FNE_NODE;


static int fd;
int bExit = 0;

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

int FNE_Pool_Push_To_DB(PT_FNE_NODE ptneNode, const char *pszDBFile, int bCommit)
{
	int iRet=0;
	char str[2048];
	static sqlite3* psqlDB = NULL;
	sqlite3_stmt* psqlStmt = NULL;

	if(bCommit)
		goto commit;
	
	if(NULL == ptneNode) return -EINVAL;

	if(NULL == psqlDB)
	{
		if(SQLITE_OK != (iRet = sqlite3_open(pszDBFile, &psqlDB)))
		{
			snprintf(str, sizeof(str), "Can't open database %s when push event!(%d)\n\n\n", pszDBFile, iRet);
                	write(fd, str, strlen(str));
			goto exit;
		}
		if(SQLITE_OK != (iRet = sqlite3_exec(psqlDB, "BEGIN", NULL, NULL, NULL)))
		{
			snprintf(str, sizeof(str), "Can't begin database %s when push event!(%d)\n\n\n", pszDBFile, iRet);
                        write(fd, str, strlen(str));
			goto exit;
		}
	}
	if(SQLITE_OK != (iRet = sqlite3_prepare_v2(psqlDB, CMD_INSERT_EVENT, strlen(CMD_INSERT_EVENT), &psqlStmt, NULL)))
	{
		snprintf(str, sizeof(str), "Can't prepare statement for %s when push event!(%d)\n\n\n", pszDBFile, iRet);
                write(fd, str, strlen(str));
		goto exit;
	}
	if(SQLITE_OK != (iRet = sqlite3_bind_blob(psqlStmt, 1, ptneNode, sizeof(T_FNE_NODE), SQLITE_TRANSIENT)))
	{
		snprintf(str, sizeof(str), "Can't bind struct for %s when push event!(%d)\n\n\n", pszDBFile, iRet);
                write(fd, str, strlen(str));
		goto exit;
	}
	if(SQLITE_OK != (iRet = sqlite3_bind_int(psqlStmt, 2, ptneNode->cbPath)))
	{
		snprintf(str, sizeof(str), "Can't bind path length for %s when push event!(%d)\n\n\n", pszDBFile, iRet);
                write(fd, str, strlen(str));
		goto exit;
	}
	if ((FN_SYMLINK | FN_LINK | FN_RENAME) & ptneNode->mCode)
	{
		if(SQLITE_OK != (iRet = sqlite3_bind_text(psqlStmt, 3, ptneNode->tepFiles.pszpOld, ptneNode->tepFiles.cbOld, SQLITE_TRANSIENT)))
		{
			snprintf(str, sizeof(str), "Can't bind old path for %s when push event!(%d)\n\n\n", pszDBFile, iRet);
                	write(fd, str, strlen(str));
			goto exit;
		}
	}
	if(SQLITE_OK != (iRet = sqlite3_bind_text(psqlStmt, 4, ptneNode->szpFile, ptneNode->cbPath, SQLITE_TRANSIENT)))
	{
		snprintf(str, sizeof(str), "Can't bind file path for %s when push event!(%d)\n\n\n", pszDBFile, iRet);
                write(fd, str, strlen(str));
		goto exit;
	}
	if(SQLITE_DONE != (iRet = sqlite3_step(psqlStmt)))
	{
		snprintf(str, sizeof(str), "Can't push event to %s!(%d)\n\n\n", pszDBFile, iRet);
                write(fd, str, strlen(str));
	}

	
exit:
	if(psqlStmt && (SQLITE_OK != (iRet = sqlite3_finalize(psqlStmt))))
	{
		snprintf(str, sizeof(str), "Finalize sql statement error(%d)\n\n\n", iRet);
                write(fd, str, strlen(str));
	}
	
commit:
	if(bCommit && psqlDB)
	{
		if(SQLITE_OK != (iRet = sqlite3_exec(psqlDB, "COMMIT", NULL, NULL, NULL)))
		{
			snprintf(str, sizeof(str), "Can't commit database %s when push event!(%d)\n\n\n", pszDBFile, iRet);
                	write(fd, str, strlen(str));
		}
		sqlite3_close(psqlDB);
		psqlDB = NULL;
	}

	return iRet;
}


static int FNE_Pool_Query_For_Pop_Callback(void *pArg, int cnCol, char **ppszVals, char **ppszColName)
{
	int cbPath = 0;
	PT_FNE_NODE ptneNode;

	
	cbPath = atoi(ppszVals[2]);
	ptneNode = malloc(sizeof(T_FNE_NODE) + cbPath + 1);
	memset(ptneNode, 0, sizeof(T_FNE_NODE) + cbPath + 1);
	memcpy(ptneNode, ppszVals[1], sizeof(T_FNE_NODE));
	
	if ((FN_SYMLINK | FN_LINK | FN_RENAME) & ptneNode->mCode)
	{
		ptneNode->tepFiles.pszpOld = malloc(ptneNode->tepFiles.cbOld+1);
		memset(ptneNode->tepFiles.pszpOld, 0, ptneNode->tepFiles.cbOld+1);
		memcpy(ptneNode->tepFiles.pszpOld, ppszVals[3], ptneNode->tepFiles.cbOld);
	}
	memcpy(ptneNode->szpFile, ppszVals[4], ptneNode->cbPath);

	*(PPT_FNE_NODE)pArg = ptneNode;
	
	return 0;
}


int FNE_Pool_Pop_From_DB(PPT_FNE_NODE pptneNode, const char *pszDBFile)
{
	int iRet=0;
	char str[2048];
	char *pszDelCmd=NULL;
	sqlite3* psqlDB = NULL;

	if(SQLITE_OK != (iRet = sqlite3_open(pszDBFile, &psqlDB)))
	{
		snprintf(str, sizeof(str), "Can't open database %s when pop event!(%d)\n\n\n", pszDBFile, iRet);
                write(fd, str, strlen(str));
		goto exit;
	}
        if(SQLITE_OK != (iRet = sqlite3_exec(psqlDB, CMD_QUERY_ONE_EVENT, FNE_Pool_Query_For_Pop_Callback, (void*)pptneNode, NULL)))
	{
		snprintf(str, sizeof(str), "Can't query event for database %s! (%d)\n\n\n", pszDBFile, iRet);
                write(fd, str, strlen(str));
		goto exit;
	}
	if(NULL == (pszDelCmd = sqlite3_mprintf(CMD_DELETE_EVENT, iRet)))
        {
		snprintf(str, sizeof(str), "Can't prepare delete command for database %s!\n\n\n", pszDBFile);
                write(fd, str, strlen(str));
		goto exit;
	}
	if(SQLITE_OK != (iRet = sqlite3_exec(psqlDB, pszDelCmd, NULL, NULL, NULL)))
	{
		snprintf(str, sizeof(str), "Can't delete event from database %s! (%d)\n\n\n", pszDBFile, iRet);
                write(fd, str, strlen(str));
		goto exit;
	}
exit:
	if(pszDelCmd)	sqlite3_free(pszDelCmd);
	if(psqlDB)	sqlite3_close(psqlDB);
	
	return iRet;
}


void FNE_DBFile_Move_Thread(void)
{
	int iRet=0, fdPush=-1, fdPop=-1;
	uint32_t idCurMoveDB=0;
	off_t cbMaxSend = 1<<20; //1MB
	char szPushPath[PATH_MAX];
	char szPopPath[PATH_MAX];
	char str[2048];

	sleep(2);
	while(0 == bExit)
	{
printf("idCurMoveDB:%d   CREATE_NUMBER:%d\n\n",idCurMoveDB, CREATE_NUMBER);
		ssize_t cbSent, cbBulk, cbFIO;
		off_t cbSize;
		struct stat tStat;

		memset(szPushPath, 0, PATH_MAX);
		snprintf(szPushPath, PATH_MAX, "%s/Job%d-%d%s", SZP_EVENTDB_PUSH_ROOT, idCurMoveDB/100, idCurMoveDB%100, SZ_EVENTDB_EXT);

		memset(szPopPath, 0, PATH_MAX);
		snprintf(szPopPath, PATH_MAX, "%s/Job%d-%d%s", SZP_EVENTDB_POP_ROOT, idCurMoveDB/100, idCurMoveDB%100, SZ_EVENTDB_EXT);
/*		while(1)
		{
			struct stat tStat;
			memset(szPopPath, 0, PATH_MAX);
			snprintf(szPopPath, PATH_MAX, "%s/Job%d-%d%s", SZP_EVENTDB_POP_ROOT, idCurMoveDB/100, idCurMoveDB%100, SZ_EVENTDB_EXT);

			if(0 == stat(szPopPath, &tStat))
			{

				if(0 == unlink(szPopPath))
					break;
				else
					idCurMoveDB++;
			}
			else
				break;
			sleep(10);
		}*/
		while(1)
		{
			fdPush=-1;
			fdPop=-1;

			if(-1 == (fdPush = open(szPushPath, O_RDONLY)))
			{
				snprintf(str, sizeof(str), "Can't move database file %s!\n\n\n", szPushPath);
                		write(fd, str, strlen(str));
				continue;
			}

			if(-1 == (fdPop = open(szPopPath, O_CREAT | O_WRONLY)))
			{
				snprintf(str, sizeof(str), "Can't create database file %s!\n\n\n", szPopPath);
                                write(fd, str, strlen(str));
				close(fdPush);
				iRet = -ENOMEM;

				goto exit;
			}

			if(-1 == fstat(fdPush, &tStat))
			{
				snprintf(str, sizeof(str), "Can't get state of database file %s!\n\n\n", szPushPath);
                                write(fd, str, strlen(str));
				close(fdPush);
				close(fdPop);

				continue;
			}
printf("szPushPath:%s    szPopPath:%s\n\n",szPushPath,szPopPath);

			for (cbSize = tStat.st_size; (cbSize > 0); cbSize -= cbBulk)
			{
				cbBulk = (cbSize > cbMaxSend) ? cbMaxSend : cbSize;
				for (cbSent=0; (cbBulk>cbSent); cbSent+=cbFIO)
				{
printf("SENDFILE\n");
					if (0 >= (cbFIO = sendfile(fdPop, fdPush, NULL, cbBulk-cbSent)))
					{
						iRet = -ENOMEM;
						snprintf(str, sizeof(str), "Can't move db file! (%s, %d, %d,%d/%d)\n\n\n", szPushPath, iRet, cbFIO, cbSent, cbBulk);
                         			write(fd, str, strlen(str));
						break;
					}
				}
				if(0 != iRet)
					goto exit;
			}
			close(fdPop);
			close(fdPush);
			unlink(szPushPath);
			szPushPath[0] = 0;
			idCurMoveDB++;
			break;
		}
		if (idCurMoveDB>=CREATE_NUMBER) bExit = 1;
		else usleep(85000);
	}
	
exit:
printf("STOP!\n");
	if(-1 != fdPush) close(fdPush);
	if(-1 != fdPop) close(fdPop);
	if(szPushPath[0]) unlink(szPushPath);
	pthread_exit(NULL);
}


int main(int argc, char** argv)
{
	int iRet = 0, i = 0, j = 0;
	char str[2048];
	char cmd[4096] = {""};
	char szDBFile[PATH_MAX]={0};
	char szPopPath[PATH_MAX];
	char* fname="/share/Public/qsync.log";
	fd = open(fname, O_WRONLY | O_CREAT);

	PT_FNE_NODE ptNode;
        PPT_FNE_NODE pptNode;

	pthread_t tidInsertThread;

	pptNode = &ptNode;
	if (NULL == (ptNode = malloc(sizeof(T_FNE_NODE)+PATH_MAX)))
	{
		iRet = -ENOMEM;
		snprintf(str, sizeof(str), "Can't allocate for a file infor node\n\n\n");
		write(fd, str, strlen(str));
		goto exit;
	}

	if (0 != pthread_create(&tidInsertThread, NULL, FNE_DBFile_Move_Thread, NULL))
	{
		snprintf(str, sizeof(str), "Can't create file move thread for event database!\n\n\n");
		write(fd, str, strlen(str));
		goto exit;
	}

	for(i=0; i< CREATE_NUMBER; i++)
	{
		memset(szDBFile, 0, PATH_MAX);
		snprintf(szDBFile, PATH_MAX, "%s/Job%d-%d.tmp", SZP_EVENTDB_PUSH_ROOT, i/100, i%100);
		memcpy(ptNode->szpFile, szDBFile, PATH_MAX);
		ptNode->tepFiles.pszpOld = szDBFile;
		memset(szPopPath, 0, PATH_MAX);
		snprintf(szPopPath, PATH_MAX, "%s/Job%d-%d%s", SZP_EVENTDB_PUSH_ROOT, i/100, i%100, SZ_EVENTDB_EXT);
		if(0 != (iRet = FNE_Pool_Create_DB(szDBFile)))
		{
			snprintf(str, sizeof(str), "Can't create database to ramdisk (%d)\n\n\n", iRet);
                	write(fd, str, strlen(str));
			goto exit;
		}
		for(j = 0;j<MAX_DB_NUMBER;j++)
                {
			ptNode->cbPath = strlen(szDBFile);
			ptNode->mCode = j;
			ptNode->tepFiles.cbOld = ptNode->cbPath;
			iRet = FNE_Pool_Push_To_DB(ptNode, szDBFile, FALSE);
			if(0 != iRet)
			{
				snprintf(str, sizeof(str), "Can't push event node to file %s (%d)\n\n\n", szDBFile, iRet);
                        	write(fd, str, strlen(str));
				goto exit;
			}
		}
		iRet = FNE_Pool_Push_To_DB(ptNode, szDBFile, TRUE);
		if(0 != iRet)
		{
			snprintf(str, sizeof(str), "Can't push event node to file %s (%d)\n\n\n", szDBFile, iRet);
			write(fd, str, strlen(str));
			goto exit;
		}
                sprintf(cmd,"mv %s %s",szDBFile ,szPopPath);
                system(cmd);
	}
	for(i=0; i< CREATE_NUMBER; i++)
        {
		snprintf(szDBFile, PATH_MAX, "%s/Job%d-%d%s", SZP_EVENTDB_POP_ROOT, i/100, i%100, SZ_EVENTDB_EXT);
		if(0 != (iRet = FNE_Pool_Pop_From_DB(pptNode, szDBFile)))
		{
			snprintf(str, sizeof(str), "Can't pop event node from file %s (%d)\n\n\n", szDBFile, iRet);
                        write(fd, str, strlen(str));
                        goto exit;
		}	
	}

exit:
	close(fd);
	free(ptNode);
	if(iRet != 0)
        {
                sprintf(cmd,"df -h >> /share/Public/qsync.log;free >> /share/Public/qsync.log");
                system(cmd);
		bExit = 1;
        }
	else pthread_join(tidInsertThread, &iRet);

	snprintf(szDBFile, PATH_MAX, "%s/Job*", SZP_EVENTDB_POP_ROOT);
	snprintf(szPopPath, PATH_MAX, "%s/Job*", SZP_EVENTDB_PUSH_ROOT);
	sprintf(cmd,"rm %s 1>/dev/null 2>/dev/null;rm %s 1>/dev/null 2>/dev/null",szDBFile ,szPopPath);
	system(cmd);
	return iRet;
}
