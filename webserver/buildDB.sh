# install mysql and library
apt install mysql-client mysql-server libmysqlclient-dev

echo 'CREATE database hxsql;
USE hxsql;
CREATE TABLE user(
        username char(50) NULL, 
        passwd char(50) NULL
)ENGINE=InnoDB;' > createDB.sql

echo 
echo 'Please enter password for mysql:'
echo 'And then enter "source createDB.sql"'
echo

mysql -u root -p