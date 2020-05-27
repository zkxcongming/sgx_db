#include <iostream>
#include <sstream>
#include <fstream>
#include <string>
#include "sgx_urts.h"
#include "sgx_utils/sgx_utils.h"
#include "Enclave_u.h"
#include <sys/types.h>
#include <sys/socket.h>
#include <stdio.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <stdlib.h>
#include <errno.h>

#define SERVER_PORT 7000
#define QUEUE 20
#define MAX_PATH FILENAME_MAX
#define ENCLAVE_FILENAME "enclave.signed.so"

using namespace std;

const char *dbname = "test.db";
sgx_enclave_id_t eid = 0;
char token_path[MAX_PATH] = {'\0'};
sgx_launch_token_t token = {0};
sgx_status_t ret = SGX_ERROR_UNEXPECTED;
int updated = 0;

void ocall_print_error(const char *str) { cerr << str << endl; }
void ocall_print_string(const char *str) { cout << str; }
void ocall_println_string(const char *str) { cout << str << endl; }

void ocall_write_file(const char *FILE_NAME, const char *szAppendStr, size_t buf_len)
{
	FILE *fp;
	fp = fopen(FILE_NAME, "a+");
	if (fp != NULL)
	{
		int ret_val = fwrite(szAppendStr, sizeof(char), buf_len, fp);
		if (ret_val != buf_len)
		{
			printf("Failed to write to file - returned value: %d\n", ret_val);
		}
		int out_fclose = fclose(fp);
		if (out_fclose != 0)
		{
			printf("Failed to close the file - %s\n", FILE_NAME);
			exit(0);
		}
	}
	else
	{
		printf("Failed to open the file - %s - for writing\n", FILE_NAME);
		exit(0);
	}
}

//read the test.db to reconstruct database inside the enclave
void ocall_init()
{
	FILE *fp1;
	fp1 = fopen("test.db", "r");
	char buf[1024];
	int len = 0;
	char tmp_char;

	while ((tmp_char = fgetc(fp1)) != -1)
	{
		buf[len++] = (char)tmp_char;
		if (tmp_char != 3)
			continue;
		if (buf[len - 2] == '\2' && buf[len - 3] == '\1')
		{
			buf[len - 3] = '\0';
		}
		if (len != 0)
		{
			ecall_initdb(eid, buf);
			memset(buf, 0, len);
			len = 0;
		}
	}
	fclose(fp1);
}

static int callback(void *data, int argc, char **argv, char **azColName)
{
	int i;
	for (i = 0; i < argc; i++)
	{
		printf("%s = %s\n", azColName[i], argv[i] ? argv[i] : "NULL");
	}
	printf("\n");
	return 0;
}

int main()
{
	fd_set rfds;
	struct timeval tv;
	int retval, maxfd;

	ret = sgx_create_enclave(ENCLAVE_FILENAME, SGX_DEBUG_FLAG, &token, &updated, &eid, NULL);
	if (ret != SGX_SUCCESS)
	{
		cerr << "Error: creating enclave" << endl;
		return -1;
	}
	cout << "Info: SQLite SGX enclave successfully created." << endl;

	ret = ecall_opendb(eid);
	if (ret != SGX_SUCCESS)
	{
		cerr << "Error: Making an ecall_open()" << endl;
		return -1;
	}
	ocall_init();

	//socket configuration
	int serverSocket;
	struct sockaddr_in server_addr;
	struct sockaddr_in clientAddr;
	int addr_len = sizeof(clientAddr);
	int client;
	char buffer[200], buf[200], SQLError[] = "SQLite error:";
	int iDataNum;

	if ((serverSocket = socket(AF_INET, SOCK_STREAM, 0)) < 0)
	{
		perror("socket");
		return 1;
	}

	bzero(&server_addr, sizeof(server_addr));
	server_addr.sin_family = AF_INET;
	server_addr.sin_port = htons(SERVER_PORT);
	server_addr.sin_addr.s_addr = htonl(INADDR_ANY);

	if (bind(serverSocket, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0)
	{
		perror("connect");
		return 1;
	}

	if (listen(serverSocket, 5) < 0)
	{
		perror("listen");
		return 1;
	}

	client = accept(serverSocket, (struct sockaddr *)&clientAddr, (socklen_t *)&addr_len);
	if (client < 0)
	{
		perror("accept");
	}
	printf("\nwaiting...\n");

	//start communicate with client
	while (1)
	{
		char *zErrMsg = 0;
		const char *data = "Callback function called";

		//receive
		printf("\n\nRECV:\n");
		char buffer[1024];
		memset(buffer, 0, sizeof(buffer));
		int len = recv(client, buffer, sizeof(buffer), 0);
		if (strcmp(buffer, "quit") == 0)
			break;

		//clear tmp.log
		FILE *fp;
		fp = fopen("tmp.log", "w+");
		fclose(fp);

		//send
		int sqlok;
		ecall_execute_sql(eid, &sqlok, buffer);

		//tmp.log contains the data to be sent to the client in one cycle
		ifstream in("tmp.log", ios::in);
		istreambuf_iterator<char> beg(in), end;
		string tmpbuf(beg, end);
		in.close();
		int i;
		memset(buf, 0, 200);
		for (i = 0; i < tmpbuf.length(); ++i)
		{
			buf[i] = tmpbuf[i];
		}
		buf[i] = '\0';

		//if the query is not 'select', put it into the database
		if (sqlok == 0)
		{
			fp = fopen("test.db", "a+");
			if (fp != NULL)
			{
				fwrite(buffer, sizeof(char), strlen(buffer), fp);
				fwrite("\1\2\3", sizeof(char), 3, fp); // "\1\2\3" to split a query from another
			}
			fclose(fp);
		}
		send(client, buf, strlen(buf), 0);
	}

	close(serverSocket);

	ret = ecall_closedb(eid);
	if (ret != SGX_SUCCESS)
	{
		cerr << "Error: Making an ecall_closedb()" << endl;
		return -1;
	}

	ret = sgx_destroy_enclave(eid);
	if (ret != SGX_SUCCESS)
	{
		cerr << "Error: destroying enclave" << endl;
		return -1;
	}

	return 0;
}
