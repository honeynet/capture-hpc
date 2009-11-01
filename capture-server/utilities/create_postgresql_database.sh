#!/bin/sh
# 
# shell script to create Capture-HPC database (It uses some scripts from bacula.org)
#
# The path to postgreSQL bin directory
bindir=/usr/bin
# user you have used to install PostgreSQL: postgres or pgsql
puser=postgres

# Database name
db_name=capturehpc
# Database account which this script is going to create
username=capture
password=capture

echo "This script is going to create dabase with related tables and user $username with permission to access database."
echo "If you are asked for password, please enter the password for the account you have used to install PostgreSQL!"
echo "It is going to create database $db_name..."
if $bindir/psql -U $puser -d template1 $* <<END-OF-DATA
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
$bindir/psql -U $puser -d $db_name $* <<END-OF-DATA
create table clientprogram (
	clientprogram_id  serial,
	name varchar(100), 
	CONSTRAINT clientprogram_pk PRIMARY KEY(clientprogram_id)
);



create table status (
	status_id  char(1),
	name varchar(100), 
	CONSTRAINT status_pk PRIMARY KEY(status_id)
);

create table honeypot (
	honeypot_id  serial,
	ipaddress inet,
	port  	integer,
    status  char(1),
  	Description  varchar,
	CONSTRAINT honeypot_pk PRIMARY KEY(honeypot_id)
);


create table operation (
	operation_id serial,
	description varchar, 
	visitstarttime  timestamp, 
	visitfinishtime timestamp, 
	honeypot_id  integer references honeypot(honeypot_id), 
	CONSTRAINT operation_pk PRIMARY KEY(operation_id)
);


create table url (url_id serial,
   	url varchar not null, 
   	currentstatus char(1) references status(status_id), 
        lastvisittime timestamp, 
        operation_id integer references operation(operation_id), 
        CONSTRAINT url_pk PRIMARY KEY(url_id)
);

create table url_operation (
	url_id  	integer references url(url_id),
  	operation_id  	integer references operation(operation_id),
  	clientprogram_id  integer references clientprogram(clientprogram_id),
	visitstarttime  timestamp,
  	visitfinishtime timestamp,
  	status_id  	char(1) references status(status_id),
  	webserverip  	inet,
	CONSTRAINT url_operation_pk PRIMARY KEY(url_id, operation_id)
);

create table file (
	url_id  	integer references url(url_id),
  	operation_id  	integer references operation(operation_id),
	filename  	varchar, 
  	content     bytea,
	CONSTRAINT file_pk PRIMARY KEY(url_id, operation_id, filename)
);

create table event (
	event_id    serial, 
	url_id  	integer references url(url_id),
  	operation_id  	integer references operation(operation_id),
	type            varchar(50),
        time  	        timestamp,
  	process         varchar,
   	action    	varchar(50),
        object1  	varchar,
   	object2   	varchar, 
	CONSTRAINT event_pk PRIMARY KEY(event_id) 
);

create table error ( 
       url_id        integer references url(url_id), 
       operation_id   integer references operation(operation_id), 
       majorerror  varchar(50),
       minorerror  varchar(50), 
       CONSTRAINT error_pk PRIMARY KEY(url_id, operation_id) 
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
   echo "Creation of Capture-HPC PostgreSQL tables succeeded."
else
   echo "Creation of Capture-HPC PostgreSQL tables failed."
   exit 1
fi

# Create new capture user
echo "It is going to create user $username for CaptureHPC..."
$bindir/psql -U $puser -d $db_name $* <<END-OF-DATA

create user $username with password '$password';
grant all privileges on database $db_name to $username;

grant all on table clientprogram                      to $username;
grant all on sequence clientprogram_clientprogram_id_seq to $username;
grant all on table error                              to $username;
grant all on table event                              to $username;
grant all on sequence event_event_id_seq                 to $username;
grant all on table file                               to $username;
grant all on table honeypot                           to $username;
grant all on sequence honeypot_honeypot_id_seq           to $username;
grant all on table operation                          to $username;
grant all on sequence operation_operation_id_seq         to $username;
grant all on table status                             to $username;
grant all on table url                                to $username;
grant all on table url_operation                      to $username;
grant all on sequence url_url_id_seq                     to $username;

END-OF-DATA
pstat=$?
if test $pstat = 0; 
then
   echo "Privileges for account $username granted on database $db_name."
   exit 0
else
   echo "Error creating privileges."
   exit 1
fi



