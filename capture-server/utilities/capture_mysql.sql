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

