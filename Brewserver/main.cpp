/* A simple server in the internet domain using TCP
   The port number is passed as an argument
   This version runs forever, forking off a separate
   process for each connection
*/
#include <stdio.h>
#include <string.h>
#include <string>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h> 
#include <sys/socket.h>
#include <netinet/in.h>
#include "rapidjson/document.h"
#include "rapidjson/writer.h"
#include "rapidjson/stringbuffer.h"
#include <mariadb/mysql.h>
#include <signal.h>
#include "yaml-cpp/yaml.h"
#include <curl/curl.h>
#include "main.h"

using namespace YAML;
using namespace rapidjson;
using namespace std;



struct data {
	char trace_ascii; /* 1 or 0 */
};




void dostuff(int, p_DB_data_t,p_UBI_data_t); /* function prototype */
void error(const char* msg)
{
	perror(msg);
	exit(1);
}

void show_error(MYSQL* mysql)
{
	printf("Error(%d) [%s] \"%s\"", mysql_errno(mysql),
		mysql_sqlstate(mysql),
		mysql_error(mysql));
	mysql_close(mysql);
	exit(-1);
}


int main(int argc, char* argv[])
{
	
	DB_data_t DB_data;
	p_DB_data_t p_DB_data = &DB_data;
	UBI_data_t ubi_data;
	p_UBI_data_t p_ubi_data = &ubi_data;
	Node config = LoadFile("/etc/conf.d/brewserver/config.yaml");
	p_DB_data->db_name = config["db"].as<string>();
	p_DB_data->db_server = config["dbhost"].as<string>();
	p_DB_data->db_passwd = config["dbpasswd"].as<string>();
	p_DB_data->db_user = config["dbuser"].as<string>();
	p_DB_data->mysql_enable = config["mysql_enable"].as<int>();
	

	p_ubi_data->UBI_Enable_enmanometer = config["ubidots_enable_eManometer"].as<int>();
	p_ubi_data->UBI_Enable_ispindle = config["ubidots_enable_iSpindle"].as<int>();
	p_ubi_data->UBI_token = config["ubidots_token"].as<string>();

	signal(SIGCHLD, SIG_IGN);
	int sockfd, newsockfd, portno, pid;
	socklen_t clilen;
	struct sockaddr_in serv_addr, cli_addr;

	/*if (argc < 2) {
		fprintf(stderr, "ERROR, no port provided\n");
		exit(1);
	}*/
	sockfd = socket(AF_INET, SOCK_STREAM, 0);
	if (sockfd < 0)
		error("ERROR opening socket");
	bzero((char*)& serv_addr, sizeof(serv_addr));
	portno = config["server_port"].as<int>();;
		printf("Server running on Port: %d \n", portno);
	serv_addr.sin_family = AF_INET;
	serv_addr.sin_addr.s_addr = INADDR_ANY;
	serv_addr.sin_port = htons(portno);
	if (bind(sockfd, (struct sockaddr*) & serv_addr,
		sizeof(serv_addr)) < 0)
		error("ERROR on binding");
	listen(sockfd, 5);
	clilen = sizeof(cli_addr);
	int n;
	char buffer[1024];
	
	while (1) {
		
		newsockfd = accept(sockfd,
			(struct sockaddr*) & cli_addr, &clilen);
		if (newsockfd < 0)
			error("ERROR on accept");
		pid = fork();
		if (pid < 0)
			error("ERROR on fork");
		if (pid == 0) {
			close(sockfd);
			dostuff(newsockfd, p_DB_data, p_ubi_data);
			exit(0);
		}
		else close(newsockfd);
	} /* end of while */
	close(sockfd);
	return 0; /* we never get here */
}

/******** DOSTUFF() *********************
 There is a separate instance of this function
 for each connection.  It handles all communication
 once a connnection has been established.
 *****************************************/
void dostuff(int sock, p_DB_data_t db_data,p_UBI_data_t ubi_data_)
{
	int n;
	
	Document d;
	Document Settings;

	char buffer[1024];
	bzero(buffer, 1024);
	n = read(sock, buffer, 1024);
	while ((n = read(sock, buffer + 1, sizeof(buffer) - 1)) > 0)
	{
		//buffer[n] = 0;
	}

	if (n < 0)
	{
		error("\n Read error \n");
	}
	printf("Here is the message: %s\n", buffer);
	d.Parse(buffer);
	
	if (d.HasMember("type"))
	{
		uint16_t type = 0xFFFF;
		string type_string = d["type"].GetString();
		printf("Erkannter Typ : %s\n", type_string.c_str());
		if (type_string.compare("eManometer") == 0)
		{
			type = eManometer;
		}
			
		if (type_string.compare("IDS2") == 0)
		{
			type = IDS2;
		}
			
		switch (type)
		{
		case eManometer:
		{
			if(db_data->mysql_enable == 1)
			{ 
				
				MYSQL Mysqldata;
				MYSQL* mysql = &Mysqldata;
				mysql_init(mysql);
				std::string query;
				if (!mysql_real_connect(mysql, db_data->db_server.c_str(), db_data->db_user.c_str(), db_data->db_passwd.c_str(), db_data->db_name.c_str(), 3306, NULL, 0))
				{
					show_error(mysql);

				}
				query = "INSERT INTO iGauge (Name,ID,Pressure,Temperature,Carbondioxid,RSSI) VALUES ('";
				assert(d.HasMember("name"));
				assert(d["name"].IsString());
				query += d["name"].GetString();
				query += "',";
				query += to_string(d["ID"].GetInt());
				query += ",";
				query += to_string(d["pressure"].GetFloat());
				query += ",";
				query += to_string(d["temperature"].GetFloat());
				query += ",";
				query += to_string(d["co2"].GetFloat());
				query += ",";
				query += to_string(d["RSSI"].GetInt());
				query += ")";

				if (mysql_query(mysql, query.c_str()))
					show_error(mysql);
				printf("Data added to MySQL\n");
			}

			
			if (ubi_data_->UBI_Enable_enmanometer == 1)
			{
				CURL* curl;
				CURLcode res;
				struct data config_trace;

				config_trace.trace_ascii = 1; /* enable ascii tracing */

				curl_global_init(CURL_GLOBAL_ALL);

				curl = curl_easy_init();
				if (curl) {
					//string curl_url = "http://things.ubidots.com/api/v1.6/devices/";
					//curl_url += d["name"].GetString();
					//curl_url += "/?token=" + ubi_data_->UBI_token;
					string url = "http://things.ubidots.com/api/v1.6/devices/"; // +d["name"].GetString() + "/?token=" + ubi_data_->UBI_token;
					printf("UBI URL: %s \n", url.c_str());
					url += d["name"].GetString();
					url += "/?token=";
					url += ubi_data_->UBI_token;
					printf("UBI URL2: %s \n", url.c_str());
					curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
					Document send_data;
					send_data.CopyFrom(d, send_data.GetAllocator());
					send_data.RemoveMember("token");
					send_data.RemoveMember("ID");
					send_data.RemoveMember("temp_units");
					send_data.RemoveMember("name");
					send_data.RemoveMember("type");
					StringBuffer output;
					Writer<rapidjson::StringBuffer> writer(output);
					send_data.Accept(writer);
					string data_ = output.GetString();
					printf("UBI Data: %s \n", data_.c_str());
					//curl_easy_setopt(curl, CURLOPT_POSTFIELDS, "{\"pressure\" : " . to_string(d["pressure"].GetFloat()) . ", \"carbondioxid\" : " .to_string(d["co2"].GetFloat()); . " }");
					curl_easy_setopt(curl, CURLOPT_POSTFIELDSIZE, data_.length());
					curl_easy_setopt(curl, CURLOPT_POSTFIELDS, data_.c_str());
					curl_easy_setopt(curl, CURLOPT_DEBUGDATA, &config_trace);

					/* the DEBUGFUNCTION has no effect until we enable VERBOSE */
					curl_easy_setopt(curl, CURLOPT_VERBOSE, 1L);
					struct curl_slist* headers = NULL;
					headers = curl_slist_append(headers, "Accept: application/json");
					headers = curl_slist_append(headers, "Content-Type: application/json");
					headers = curl_slist_append(headers, "charsets: utf-8");

					curl_easy_setopt(curl, CURLOPT_CUSTOMREQUEST, "POST");
					curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);

					res = curl_easy_perform(curl);

					if (res != CURLE_OK)
						fprintf(stderr, "curl_easy_perform() failed: %s\n",
							curl_easy_strerror(res));

					curl_easy_cleanup(curl);
				}
				curl_global_cleanup();
			}
		}
			break;
		case IDS2:
		{
			if (db_data->mysql_enable == 1)
			{
				MYSQL Mysqldata;
				MYSQL* mysql = &Mysqldata;
				mysql_init(mysql);
				std::string query;
				if (!mysql_real_connect(mysql, db_data->db_server.c_str(), db_data->db_user.c_str(), db_data->db_passwd.c_str(), db_data->db_name.c_str(), 3306, NULL, 0))
				{
					show_error(mysql);

				}
				query = "INSERT INTO iGauge (Name,ID,Pressure,Temperature,Carbondioxid,RSSI) VALUES ('";
				assert(d.HasMember("name"));
				assert(d["name"].IsString());
				query += d["name"].GetString();
				query += "',";
				query += to_string(d["ID"].GetInt());
				query += ",";
				query += to_string(d["pressure"].GetFloat());
				query += ",";
				query += to_string(d["temperature"].GetFloat());
				query += ",";
				query += to_string(d["co2"].GetFloat());
				query += ",";
				query += to_string(d["RSSI"].GetInt());
				query += ")";

				if (mysql_query(mysql, query.c_str()))
					show_error(mysql);
				printf("Data added to MySQL\n");
			}
			if (ubi_data_->UBI_Enable_enmanometer == 1)
			{
				CURL* curl;
				CURLcode res;
				struct data config_trace;

				config_trace.trace_ascii = 1; /* enable ascii tracing */

				curl_global_init(CURL_GLOBAL_ALL);

				curl = curl_easy_init();
				if (curl) {
					//string curl_url = "http://things.ubidots.com/api/v1.6/devices/";
					//curl_url += d["name"].GetString();
					//curl_url += "/?token=" + ubi_data_->UBI_token;
					string url = "http://things.ubidots.com/api/v1.6/devices/"; // +d["name"].GetString() + "/?token=" + ubi_data_->UBI_token;
					printf("UBI URL: %s \n", url.c_str());
					url += d["name"].GetString();
					url += "/?token=";
					url += ubi_data_->UBI_token;
					printf("UBI URL2: %s \n", url.c_str());
					curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
					Document send_data;
					send_data.CopyFrom(d, send_data.GetAllocator());
					send_data.RemoveMember("token");
					send_data.RemoveMember("ID");
					send_data.RemoveMember("temp_units");
					send_data.RemoveMember("name");
					send_data.RemoveMember("type");
					StringBuffer output;
					Writer<rapidjson::StringBuffer> writer(output);
					send_data.Accept(writer);
					string data_ = output.GetString();
					printf("UBI Data: %s \n", data_.c_str());
					//curl_easy_setopt(curl, CURLOPT_POSTFIELDS, "{\"pressure\" : " . to_string(d["pressure"].GetFloat()) . ", \"carbondioxid\" : " .to_string(d["co2"].GetFloat()); . " }");
					curl_easy_setopt(curl, CURLOPT_POSTFIELDSIZE, data_.length());
					curl_easy_setopt(curl, CURLOPT_POSTFIELDS, data_.c_str());
					curl_easy_setopt(curl, CURLOPT_DEBUGDATA, &config_trace);

					/* the DEBUGFUNCTION has no effect until we enable VERBOSE */
					curl_easy_setopt(curl, CURLOPT_VERBOSE, 1L);
					struct curl_slist* headers = NULL;
					headers = curl_slist_append(headers, "Accept: application/json");
					headers = curl_slist_append(headers, "Content-Type: application/json");
					headers = curl_slist_append(headers, "charsets: utf-8");

					curl_easy_setopt(curl, CURLOPT_CUSTOMREQUEST, "POST");
					curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);

					res = curl_easy_perform(curl);

					if (res != CURLE_OK)
						fprintf(stderr, "curl_easy_perform() failed: %s\n",
							curl_easy_strerror(res));

					curl_easy_cleanup(curl);
				}
				curl_global_cleanup();
			}
		}
		break;
		default:
			printf("Detected nothing \n");
			break;
		}
	}
	else
	{
		if (db_data->mysql_enable == 1)
		{
			MYSQL Mysqldata;
			MYSQL* mysql = &Mysqldata;
			mysql_init(mysql);
			std::string query;
			if (!mysql_real_connect(mysql, db_data->db_server.c_str(), db_data->db_user.c_str(), db_data->db_passwd.c_str(), db_data->db_name.c_str(), 3306, NULL, 0))
			{
				show_error(mysql);

			}
			printf("iSpindle detected: %s\n", buffer);
			query = "INSERT INTO Data (Name,ID,Angle,Temperature,Battery,Gravity,`Interval`,UserToken,RSSI) VALUES ('";
			assert(d.HasMember("name"));
			assert(d["name"].IsString());
			query += d["name"].GetString();
			query += "',";
			query += to_string(d["ID"].GetInt());
			query += ",";
			query += to_string(d["angle"].GetFloat());
			query += ",";
			query += to_string(d["temperature"].GetFloat());
			query += ",";
			query += to_string(d["battery"].GetFloat());
			query += ",";
			query += to_string(d["gravity"].GetFloat());
			query += ",";
			query += to_string(d["interval"].GetInt());
			query += ",'";
			query += d["token"].GetString();
			query += "',";
			query += to_string(d["RSSI"].GetInt());
			query += ")";
			if (mysql_query(mysql, query.c_str()))
				show_error(mysql);
			printf("Data added to MySQL\n");
		}
		if (ubi_data_->UBI_Enable_ispindle == 1)
		{
			CURL* curl;
			CURLcode res;
			struct data config_trace;

			config_trace.trace_ascii = 1; /* enable ascii tracing */

			curl_global_init(CURL_GLOBAL_ALL);

			curl = curl_easy_init();
			if (curl) {
				//string curl_url = "http://things.ubidots.com/api/v1.6/devices/";
				//curl_url += d["name"].GetString();
				//curl_url += "/?token=" + ubi_data_->UBI_token;
				string url = "http://things.ubidots.com/api/v1.6/devices/"; // +d["name"].GetString() + "/?token=" + ubi_data_->UBI_token;
				printf("UBI URL: %s \n", url.c_str());
				url += d["name"].GetString();
				url += "/?token=";
				url += ubi_data_->UBI_token;
				printf("UBI URL2: %s \n", url.c_str());
				curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
				Document send_data;
				send_data.CopyFrom(d, send_data.GetAllocator());
				send_data.RemoveMember("token");
				send_data.RemoveMember("ID");
				send_data.RemoveMember("temp_units");
				send_data.RemoveMember("name"); 
				StringBuffer output;
				Writer<rapidjson::StringBuffer> writer(output);
				send_data.Accept(writer);
				string data_ = output.GetString();
				printf("UBI Data: %s \n", data_.c_str());
				//curl_easy_setopt(curl, CURLOPT_POSTFIELDS, "{\"pressure\" : " . to_string(d["pressure"].GetFloat()) . ", \"carbondioxid\" : " .to_string(d["co2"].GetFloat()); . " }");
				curl_easy_setopt(curl, CURLOPT_POSTFIELDSIZE, data_.length());
				curl_easy_setopt(curl, CURLOPT_POSTFIELDS, data_.c_str());
				curl_easy_setopt(curl, CURLOPT_DEBUGDATA, &config_trace);

				/* the DEBUGFUNCTION has no effect until we enable VERBOSE */
				curl_easy_setopt(curl, CURLOPT_VERBOSE, 1L);
				struct curl_slist* headers = NULL;
				headers = curl_slist_append(headers, "Accept: application/json");
				headers = curl_slist_append(headers, "Content-Type: application/json");
				headers = curl_slist_append(headers, "charsets: utf-8");

				curl_easy_setopt(curl, CURLOPT_CUSTOMREQUEST, "POST");
				curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);

				res = curl_easy_perform(curl);

				if (res != CURLE_OK)
					fprintf(stderr, "curl_easy_perform() failed: %s\n",
						curl_easy_strerror(res));

				curl_easy_cleanup(curl);
			}
			curl_global_cleanup();

		}
	}
	char number = 21;
	n = write(sock, &number, 1);
	if (n < 0) error("ERROR writing to socket");
}