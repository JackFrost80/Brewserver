# Brewserver
TCP Server for iSpindel written in C++

The Brewserver is only the Server for collecting the Data.
MySQL Database and Webpage must be used from https://github.com/avollkopf/iSpindel-TCP-Server

This is a Visual Studio Community Project. On the maschine must be installed:

Yaml-cpp
Curl
Mariadb-connector-c

Create config file config.yaml in /etc/conf.d/brewserver/

Add the following lines and fill in the needed parts

`#database info
---
dbuser: 
dbpasswd: 
db: 
dbhost: localhost
#general config
server_port: 9501
debug_enable: 1
#MySQL config
mysql_enable: 1
#Ubidots
ubidots_enable_iSpindle: 1
ubidots_enable_eManometer: 0
ubidots_token: `


At the momment must the server run in a screen.

Kind regards

JackFrost
