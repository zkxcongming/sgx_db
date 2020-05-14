#include "Enclave_t.h" 
#include "sqlite3.h"
#include <string>

sqlite3* db; 

char * findstr(const char * str, char * sub){
	if(NULL == str || NULL == sub){return NULL;}
 
	char * p = NULL, * q = NULL, * c = NULL;
	int found = 0; 
	p = const_cast<char*>(str); 
	while(*p != '\0'){ 
		q = sub;c = p;
		while(*c == *q && *q != '\0'){ q++; c++; }
 		if('\0' == *q){	found = 1; break; }
		p++;
	} 
	if(1 == found){	return p;} 
	else { return NULL;}
}
 
static int callback(void *NotUsed, int argc, char **argv, char **azColName){
    int i;

    for(i = 0; i < argc; i++){
        std::string azColName_str = azColName[i];
        std::string argv_str = (argv[i] ? argv[i] : "NULL");
		std::string tmp = (azColName_str + " = " + argv_str + "\n");
        ocall_print_string(tmp.c_str());
        ocall_write_file ("tmp.txt",tmp.c_str(),tmp.length());
    }
    ocall_print_string("\n");
    return 0;
}

void ecall_initdb(const char *sql){
/*--------------------添加解密操作，解密buf---------------------------*/

	//ocall_print_string(sql);
	//ocall_print_string("\n");
	int rc;
	char *zErrMsg = 0;
	rc = sqlite3_exec(db,sql,0,0,&zErrMsg);
}

void ecall_opendb(){
    int rc; 
	char buff[1024];
	int len;

    rc = sqlite3_open(":memory:", &db); 
    if (rc) {
        ocall_println_string("SQLite error - can't open database connection: ");
        ocall_println_string(sqlite3_errmsg(db));
        return;
    }
    ocall_print_string("Enclave: Created database connection to :memory:");
}

int ecall_execute_sql(const char *sql){
    int rc;
	char *r=NULL;
	char * str2="select";
    char *zErrMsg = 0;
	r = findstr(sql,str2);
	
    rc = sqlite3_exec(db, sql, callback, 0, &zErrMsg);
	
	//SQL语法错误 2 
	/*--------------------添加加密操作，解密tmp---------------------------*/
    if (rc) {
		std::string tmp = "SQLite error: ";
		tmp += sqlite3_errmsg(db);
        ocall_println_string(tmp.c_str());
		ocall_write_file ("tmp.txt",tmp.c_str(),tmp.length());
        return 2;
    }
    //select操作成功 1 (操作在回调函数里面） 
	if(NULL != r){return 1;} 
	//非select操作成功 0  
	/*--------------------添加加密操作，加密tmp---------------------------*/
	else {
		std::string tmp = "DONE\n";
		ocall_println_string(tmp.c_str());
		ocall_write_file ("tmp.txt",tmp.c_str(),tmp.length());
		return 0;
	} 
}

void ecall_closedb(){
    sqlite3_close(db);
    ocall_println_string("Enclave: Closed database connection");
}
