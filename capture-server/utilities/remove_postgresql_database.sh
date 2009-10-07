#!/bin/sh
# 
# shell script to remove Capture-HPC database (It uses some scripts from bacula.org)
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

echo "This script is going to remove dabase, related tables and user $username"
echo "If you are asked for password, please enter the password for the account you have used to install PostgreSQL!"
echo ""
echo "It is going to remove database $db_name and user $username..."
$bindir/psql -U $puser -d template1 $* <<END-OF-DATA
DROP DATABASE $db_name;
DROP USER $username;
END-OF-DATA
pstat=$?
if test $pstat = 0; 
then
   echo "Database ${db_name} have been removed successfully."
   exit 0
else
   echo "Database ${db_name} have been removed unsuccessfully."
   exit 1
fi


