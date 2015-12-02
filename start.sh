<<<<<<< HEAD
#!/bin/bash

#替换redis信息

sed -i 's/REDIS_SERVER_IP/'$REDIS_PORT_6379_TCP_ADDR'/g'  ./config/userQuery.conf
sed -i 's/REDIS_SERVER_IP/'$REDIS_PORT_6379_TCP_ADDR'/g'  ./config/userQuery.conf

#替换mysql信息
sed -i 's/MYSQL_SERVER_IP/'$MYSQL_PORT_3306_TCP_ADDR'/g'  ./config/userQuery.conf
sed -i 's/MYSQL_SERVER_PORT/$MYSQL_PORT_3306_TCP_PORT/g'  ./config/userQuery.conf
sed -i 's/MYSQL_SERVER_USERNAME/$MYSQL_USER/g'  ./config/userQuery.conf
sed -i 's/MYSQL_SERVER_PASSWORD/$MYSQL_PASSWORD/g'  ./config/userQuery.conf
sed -i 's/MYSQL_SERVER_DBNAME/$MYSQL_DATABASE/g'  ./config/userQuery.conf

sed -i 's/LOCAL_PORT/8089/g'  ./config/userQuery.conf

./LdpApiServer -c ./config/userQuery.conf -n query
=======
#准备环境变量
sed -i 's/MYSQL_SERVER_ADDR/'$API_PORT'/g'  ./config/userQuery.conf



/usr/src/datahubapi/LdpApiServer -c /usr/src/datahubapi/config/userQuery.conf
>>>>>>> f51cca90c28f1cb6b678b8ef5cdeb08f2601e004
