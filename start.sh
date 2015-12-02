#准备环境变量
sed -i 's/MYSQL_SERVER_ADDR/'$API_PORT'/g'  ./config/userQuery.conf



/usr/src/datahubapi/LdpApiServer -c /usr/src/datahubapi/config/userQuery.conf
