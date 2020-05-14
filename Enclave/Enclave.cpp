#include "Enclave_t.h"
#include "sqlite3.h"
#include <string>
#include <sgx_tcrypto.h>

sqlite3 *db;
sgx_aes_gcm_128bit_key_t cur_key = {0x60, 0x3d, 0xeb, 0x10, 0x15, 0xca, 0x71, 0xbe, 0x2b, 0x73, 0xae, 0xf0, 0x85, 0x7d, 0x77, 0x81};
uint8_t empty_iv[SGX_AESGCM_IV_SIZE];
sgx_aes_gcm_128bit_tag_t gmac, gmac1;

void encrypt(const char *src_text, int src_len, char *des_text)
{
	sgx_status_t ret;
	ret = sgx_rijndael128GCM_encrypt(&cur_key, (const uint8_t *)src_text, (uint32_t)src_len, (uint8_t *)des_text, (const uint8_t *)&empty_iv, SGX_AESGCM_IV_SIZE, NULL, 0, &gmac);
	if (ret != SGX_SUCCESS)
		ocall_println_string("Failed to encrypt!");
}

void decrypt(const char *src_text, int src_len, char *des_text)
{
	sgx_status_t ret;
	sgx_rijndael128GCM_decrypt(&cur_key, (const uint8_t *)src_text, (uint32_t)src_len, (uint8_t *)des_text, (const uint8_t *)&empty_iv, SGX_AESGCM_IV_SIZE, NULL, 0, &gmac1);
	if (ret != SGX_SUCCESS)
		ocall_println_string("Failed to decrypt!");
}

char *findstr(const char *str, char *sub)
{
	if (NULL == str || NULL == sub)
	{
		return NULL;
	}

	char *p = NULL, *q = NULL, *c = NULL;
	int found = 0;
	p = const_cast<char *>(str);
	while (*p != '\0')
	{
		q = sub;
		c = p;
		while (*c == *q && *q != '\0')
		{
			q++;
			c++;
		}
		if ('\0' == *q)
		{
			found = 1;
			break;
		}
		p++;
	}
	if (1 == found)
	{
		return p;
	}
	else
	{
		return NULL;
	}
}

static int callback(void *NotUsed, int argc, char **argv, char **azColName)
{
	int i;

	for (i = 0; i < argc; i++)
	{
		std::string azColName_str = azColName[i];
		std::string argv_str = (argv[i] ? argv[i] : "NULL");
		std::string tmp = (azColName_str + " = " + argv_str + "\n");
		/*--------------------添加加密操作，加密tmp---------------------------*/
		uint32_t length = tmp.length();
		char des_text[length];
		encrypt(tmp.c_str(), length, des_text);
		const char *enc_tmp = des_text;
		//ocall_print_string(tmp.c_str());
		ocall_print_string(enc_tmp); //print enc_text to debug
		ocall_write_file("tmp.txt", enc_tmp, length);
	}
	ocall_print_string("\n");
	return 0;
}

void ecall_initdb(const char *sql)
{
	/*--------------------添加解密操作，解密sql---------------------------*/

	uint32_t length = strlen(sql);
	char des_text[length];
	decrypt(sql, length, des_text);
	const char *dec_sql = des_text;
	//ocall_print_string(sql);
	//ocall_print_string("\n");
	int rc;
	char *zErrMsg = 0;
	rc = sqlite3_exec(db, dec_sql, 0, 0, &zErrMsg);
}

void ecall_opendb()
{
	int rc;
	char buff[1024];
	int len;

	rc = sqlite3_open(":memory:", &db);
	if (rc)
	{
		ocall_println_string("SQLite error - can't open database connection: ");
		ocall_println_string(sqlite3_errmsg(db));
		return;
	}
	ocall_print_string("Enclave: Created database connection to :memory:");
}

int ecall_execute_sql(const char *sql)
{
	int rc;
	char *r = NULL;
	char *str2 = "select";
	char *zErrMsg = 0;

	/*--------------------添加解密操作，解密sql---------------------------*/
	uint32_t length = strlen(sql);
	char des_text[length];
	decrypt(sql, length, des_text);
	const char *dec_sql = des_text;

	r = findstr(sql, str2);
	rc = sqlite3_exec(db, dec_sql, callback, 0, &zErrMsg);

	//SQL语法错误 2
	/*--------------------添加加密操作，加密tmp---------------------------*/

	if (rc)
	{
		std::string tmp = "SQLite error: ";
		tmp += sqlite3_errmsg(db);

		uint32_t length=tmp.length();
		char des_text[length];
		encrypt(tmp.c_str(), length, des_text);
		const char *enc_tmp = des_text;

		//ocall_println_string(tmp.c_str());
		ocall_println_string(enc_tmp);
		ocall_write_file("tmp.txt", enc_tmp, length);
		return 2;
	}
	//select操作成功 1 (操作在回调函数里面）
	if (NULL != r)
	{
		return 1;
	}
	//非select操作成功 0
	/*--------------------添加加密操作，加密tmp---------------------------*/
	else
	{
		std::string tmp = "DONE\n";

		uint32_t length=tmp.length();
		char des_text[length];
		encrypt(tmp.c_str(), length, des_text);
		const char *enc_tmp = des_text;

		//ocall_println_string(tmp.c_str());
		ocall_println_string(enc_tmp);
		ocall_write_file("tmp.txt", enc_tmp, length);
		return 0;
	}
}

void ecall_closedb()
{
	sqlite3_close(db);
	ocall_println_string("Enclave: Closed database connection");
}
