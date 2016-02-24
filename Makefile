C_INCLUDE_PATH=/mnt/vdisk/git_4.2.0/DataService/DBMS/sqlite/sqlite-3.4.1/src:/mnt/vdisk/git_4.2.0/DataService/DBMS/sqlite/sqlite-3.4.1
export C_INCLUDE_PATH

gcc -o sqlite3test sqlite3_plugin.c -lsqlite3 -lpthread

scp sqlite3test admin@172.17.30.182:/share/F01
