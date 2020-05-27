#include "Enclave_t.h"
#include "sqlite3.h"
#include <string>
#include <sgx_tcrypto.h>

sqlite3 *db; //database inside the enclave
sgx_aes_gcm_128bit_key_t cur_key = {0x60, 0x3d, 0xeb, 0x10, 0x15, 0xca, 0x71, 0xbe, 0x2b, 0x73, 0xae, 0xf0, 0x85, 0x7d, 0x77, 0x81};
uint8_t empty_iv[SGX_AESGCM_IV_SIZE] = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
std::string rslt; //plain text of data to be sent to the client

void encrypt(const char *src_text, int src_len, char *des_text)
{
	sgx_status_t ret;
	ret = sgx_rijndael128GCM_encrypt(&cur_key, (const uint8_t *)src_text, (uint32_t)src_len, (uint8_t *)(des_text + 16), &empty_iv[0], SGX_AESGCM_IV_SIZE, NULL, 0, (sgx_aes_gcm_128bit_tag_t *)(des_text));

	if (ret != SGX_SUCCESS)
	{
		ocall_println_string("Failed to encrypt!");
		if (ret == SGX_ERROR_INVALID_PARAMETER)
			ocall_println_string("SGX_ERROR_INVALID_PARAMETER");
		if (ret == SGX_ERROR_MAC_MISMATCH)
			ocall_println_string("SGX_ERROR_MAC_MISMATCH");
		if (ret == SGX_ERROR_OUT_OF_MEMORY)
			ocall_println_string("SGX_ERROR_OUT_OF_MEMORY");
		if (ret == SGX_ERROR_UNEXPECTED)
			ocall_println_string("SGX_ERROR_UNEXPECTED");
	}
}

void decrypt(const char *src_text, int src_len, char *des_text)
{
	sgx_status_t ret;
	ret = sgx_rijndael128GCM_decrypt(&cur_key, (const uint8_t *)src_text + 16, (uint32_t)src_len - 16, (uint8_t *)des_text, &empty_iv[0], SGX_AESGCM_IV_SIZE, NULL, 0, (sgx_aes_gcm_128bit_tag_t *)(src_text));

	if (ret != SGX_SUCCESS)
	{
		ocall_println_string("Failed to decrypt!");
		if (ret == SGX_ERROR_INVALID_PARAMETER)
			ocall_println_string("SGX_ERROR_INVALID_PARAMETER");
		if (ret == SGX_ERROR_MAC_MISMATCH)
			ocall_println_string("SGX_ERROR_MAC_MISMATCH");
		if (ret == SGX_ERROR_OUT_OF_MEMORY)
			ocall_println_string("SGX_ERROR_OUT_OF_MEMORY");
		if (ret == SGX_ERROR_UNEXPECTED)
			ocall_println_string("SGX_ERROR_UNEXPECTED");
	}
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
		rslt += tmp;
	}
	ocall_print_string("\n");
	return 0;
}

void ecall_initdb(const char *sql)
{
	uint32_t length = strlen(sql);
	char des_text[length - 15];
	memset(des_text, 0, length - 15);
	decrypt(sql, length, des_text);
	const char *dec_sql = des_text;
	ocall_print_string(dec_sql);
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

	/*decrypt sql*/
	uint32_t length = strlen(sql);
	char des_text[length - 15];
	memset(des_text, 0, length - 15);
	decrypt(sql, length, des_text);
	const char *dec_sql = des_text;
	ocall_println_string(dec_sql);
	r = findstr(dec_sql, str2);
	rslt = "";
	rc = sqlite3_exec(db, dec_sql, callback, 0, &zErrMsg);
	//return 2 if sql include an error
	if (rc)
	{
		std::string tmp = "SQLite error: ";
		tmp += sqlite3_errmsg(db);
		rslt += tmp;

		ocall_println_string("\nSEND:");
		ocall_print_string(rslt.c_str());
		uint32_t rsltlength = rslt.length();
		char enc_rslt[rsltlength + 16];
		memset(enc_rslt, 0, rsltlength + 16);
		encrypt(rslt.c_str(), rsltlength, enc_rslt);
		const char *enc_tmp = enc_rslt;
		ocall_write_file("tmp.log", enc_tmp, rsltlength + 16);
		return 2;
	}
	//return 1 if sql include "select"
	if (NULL != r)
	{
		ocall_println_string("\nSEND:");
		ocall_print_string(rslt.c_str());
		uint32_t rsltlength = rslt.length();
		char enc_rslt[rsltlength + 16];
		memset(enc_rslt, 0, rsltlength + 16);
		encrypt(rslt.c_str(), rsltlength, enc_rslt);
		const char *enc_tmp = enc_rslt;
		ocall_write_file("tmp.log", enc_tmp, rsltlength + 16);

		return 1;
	}
	//return 0 if sql does not include "select"
	else
	{
		std::string tmp = "DONE\n";
		rslt += tmp;
		ocall_println_string("\nSEND:");
		ocall_print_string(rslt.c_str());
		uint32_t rsltlength = rslt.length();
		char enc_rslt[rsltlength + 16];
		memset(enc_rslt, 0, rsltlength + 16);
		encrypt(rslt.c_str(), rsltlength, enc_rslt);
		const char *enc_tmp = enc_rslt;
		ocall_write_file("tmp.log", enc_tmp, rsltlength + 16);
		return 0;
	}
}

void ecall_closedb()
{
	sqlite3_close(db);
	ocall_println_string("Enclave: Closed database connection");
}
