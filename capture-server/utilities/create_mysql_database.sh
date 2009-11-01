#!/bin/sh
# 
# shell script to create Capture-HPC database (It uses some scripts from bacula.org)
#
# The path to mysql bin directory
bindir=/usr/bin

# Database name
db_name=capturehpc
# Database account which this script is going to create
username=capture
password=capture
host=127.0.0.1

echo "This script is going to create dabase with related tables and user $username with permission to access database."
echo "If you are asked for password, please enter the password for root user!"
echo "It is going to create database $db_name..."
if $bindir/mysql -u root -p <<END-OF-DATA
CREATE DATABASE $db_name;
END-OF-DATA
then
   echo "Creation of ${db_name} database succeeded."
else
   echo "Creation of ${db_name} database failed."
   exit 1
fi

# Creation of tables for Capture-HPC
echo "It is going to create tables in database $db_name..."
$bindir/mysql -u root -p <<END-OF-DATA
use $db_name;
create table clientprogram (
	clientprogram_id  serial,
	name varchar(100), 
	PRIMARY KEY(clientprogram_id)
);



create table status (
	status_id  char(1),
	name varchar(100), 
	PRIMARY KEY(status_id)
);

create table honeypot (
	honeypot_id  serial,
	ipaddress char(15),
	port  	integer,
    status  char(1),
  	Description  varchar(500),
	PRIMARY KEY(honeypot_id)
);


create table operation (
	operation_id serial,
	description varchar(500), 
	visitstarttime  char(23), 
	visitfinishtime char(23), 
	honeypot_id  integer references honeypot(honeypot_id), 
	PRIMARY KEY(operation_id)
);


create table url (url_id serial,
   	url varchar(2083) not null,
   	currentstatus char(1) references status(status_id), 
        lastvisittime char(23), 
        operation_id integer references operation(operation_id), 
        PRIMARY KEY(url_id)
);

create table url_operation (
	url_id  	integer references url(url_id),
  	operation_id  	integer references operation(operation_id),
  	clientprogram_id  integer references clientprogram(clientprogram_id),
	visitstarttime  char(23),
  	visitfinishtime char(23),
  	status_id  	char(1) references status(status_id),
  	webserverip  	char(15),
	PRIMARY KEY(url_id, operation_id)
);

create table file (
	url_id  	integer references url(url_id),
  	operation_id  	integer references operation(operation_id),
	filename  	varchar(500), 
  	content     mediumblob,
	PRIMARY KEY(url_id, operation_id, filename)
);

create table event (
	event_id    serial, 
	url_id  	integer references url(url_id),
  	operation_id  	integer references operation(operation_id),
	type            varchar(50),
        time  	        varchar(23),
  	process         varchar(500),
   	action    	varchar(50),
        object1  	varchar(500),
   	object2   	varchar(500), 
	PRIMARY KEY(event_id) 
);


create table error ( 
       url_id        integer references url(url_id), 
       operation_id   integer references operation(operation_id), 
       majorerror  varchar(50),
       minorerror  varchar(50), 
       PRIMARY KEY(url_id, operation_id) 
 );

insert into status(status_id,name) values('B', 'benign'); 
insert into status(status_id, name) values('M', 'malicious'); 
insert into status(status_id, name) values('E', 'error'); 
insert into clientprogram(name) values('iexplorebulk');
insert into clientprogram(name) values('iexplore');
insert into clientprogram(name) values('safari');
insert into clientprogram(name) values('firefox');
insert into clientprogram(name) values('opera');
insert into clientprogram(name) values('oowriter');
insert into clientprogram(name) values('acrobatreader');
insert into clientprogram(name) values('word');
END-OF-DATA
pstat=$?
if test $pstat = 0; 
then
   echo "Creation of Capture-HPC MySQL tables succeeded."
else
   echo "Creation of Capture-HPC MySQL tables failed."
   exit 1
fi

# Create new capture user
echo "It is going to create user $username for CaptureHPC..."
if $bindir/mysql -u root -p <<END-OF-DATA
use mysql;
grant all privileges on $db_name.* to $username@$host IDENTIFIED BY '$password';
grant all privileges on $db_name.* to $username@"%";
flush privileges;
END-OF-DATA
then
   echo "Privileges for account $username granted on database $db_name."
   exit 0
else
   echo "Error creating privileges."
   exit 1
fi

