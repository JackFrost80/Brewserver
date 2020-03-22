#include <stdio.h>
#include <string.h>
#include <string>
#include <unistd.h>
#include <stdlib.h>

using namespace std;

#define eManometer 0x00
#define IDS2	0X01

typedef struct DB_data {

	string db_name;
	string db_server;
	string db_passwd;
	string db_user;
	int mysql_enable;
	int db_port;
	int server_port;



}DB_data_t, * p_DB_data_t;

typedef struct UBI_data {

	int UBI_Enable_enmanometer;
	int UBI_Enable_ispindle;
	string UBI_token;

}UBI_data_t, * p_UBI_data_t;