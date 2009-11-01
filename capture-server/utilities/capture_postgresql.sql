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
