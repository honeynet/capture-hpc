package capture;

/**
 * PROJECT: Capture-HPC
 * DATE: June 24, 2009
 * FILE: Database.java
 * COPYRIGHT HOLDER: Victoria University of Wellington, NZ
 * AUTHORS: Van Lam Le (vanlamle@gmail.com)
 * <p/>
 * This file is part of Capture-HPC.
 * <p/>
 * Capture-HPC is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 * <p/>
 * Capture-HPC is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * <p/>
 * You should have received a copy of the GNU General Public License
 * along with Capture-HPC; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
 */

import java.io.*;
import java.text.SimpleDateFormat;
import java.util.Date;
import java.sql.Connection;
import java.sql.Statement;
import java.sql.PreparedStatement;
import java.sql.ResultSet;
import java.sql.ResultSetMetaData;
import java.sql.DriverManager;
import java.sql.SQLException;
import javax.sql.DataSource;
import org.apache.commons.dbcp.BasicDataSource;

public class MySQLDatabase extends Database {
	static String DRIVER="com.mysql.jdbc.Driver";
	static String currentOperation=null;  //hold current operation id
	static DataSource dataSource;   //connection pool
	
	public MySQLDatabase()
	{
		BasicDataSource ds= new BasicDataSource();
        ds.setDriverClassName(DRIVER);
        ds.setUsername(ConfigManager.getInstance().getConfigOption("database-username"));
        ds.setPassword(ConfigManager.getInstance().getConfigOption("database-password"));
        ds.setUrl(ConfigManager.getInstance().getConfigOption("database-url"));
        dataSource=ds;
	}

	
	//set a new value to currentOperation: move to a new operation
	public void setCurrentOperation(String operationid)
	{
		currentOperation=operationid;
	}

	//get current operation id
	public String getCurrentOperation()
	{
		return currentOperation;
	}

	// method to get a connection to database from connection pool
	public static Connection getConnection() {
		try
		{
			return dataSource.getConnection();
		}
		catch (SQLException e)
		{
			e.printStackTrace();
			return null;
		}
	}

	
	//load all values from status, clientprogram into memory. It can save time to access database
	public boolean loadTemporaryValue()
	{
		Connection con=this.getConnection();
		Statement	stmt;
		ResultSet rs;
		boolean result=false;
		try
		{
			stmt= con.createStatement();

			//Add status values into memory
			stmt.executeQuery("SELECT status_id, name FROM status");
			rs = stmt.getResultSet();
			while (rs.next()) 
			{
				ConfigManager.getInstance().addConfigOption("status-"+rs.getString(2).toLowerCase(), rs.getString(1));
			}

			//Add client program values into memory
			stmt.executeQuery("SELECT clientprogram_id, name FROM clientprogram");
			rs = stmt.getResultSet();
			while (rs.next()) 
			{
				ConfigManager.getInstance().addConfigOption("clientprogram-"+rs.getString(2).toLowerCase(), rs.getString(1));
			}
			stmt.close(); con.close();
			result=true;
		} catch( Exception e ) {e.printStackTrace();}
		return result;
	}

	//update urls status after inspectation. The status can be: benign, malicious, error
	public boolean setStatus(String urlid, String status, String visitStartTime, String visitFinishTime, String clientProgram)
	{
		Connection con=getConnection();
		Statement	stmt;
		ResultSet rs;
		String operationid=Database.getInstance().getCurrentOperation();
		String clientProgramid=ConfigManager.getInstance().getConfigOption("clientprogram-"+clientProgram);
		String statusid= ConfigManager.getInstance().getConfigOption("status-"+status);
		
		boolean result=false;
		try
		{
			SimpleDateFormat sf1 = new SimpleDateFormat("dd/MM/yyyy HH:mm:ss.S");
			SimpleDateFormat sf2 = new SimpleDateFormat("yyyy/MM/dd HH:mm:ss.S");	
			//convert dates into new format "yyyy/MM/dd HH:mm:ss.S"
			Date date = sf1.parse(visitStartTime);
			visitStartTime = sf2.format(date);
			date = sf1.parse(visitFinishTime);
			visitFinishTime = sf2.format(date);

			stmt= con.createStatement();

			con.setAutoCommit(false);
			stmt.executeUpdate("UPDATE url_operation SET visitstarttime=\'" +visitStartTime+"\', " +
				                                                                "visitfinishtime=\'" +visitFinishTime+"\', " +
				                                                                "clientprogram_id="+clientProgramid+", "+
				                                                                "status_id=\'" + statusid +
				                           "\' WHERE url_id="+urlid+" AND operation_id="+operationid);
			stmt.executeUpdate("UPDATE url SET lastvisittime=\'" +visitFinishTime+"\', " +
				                                                                "currentstatus=\'" + statusid + "\', operation_id="+operationid +
				                           " WHERE url_id="+urlid);
			con.commit();
			con.setAutoCommit(true);
			stmt.close(); con.close();
			result=true;
		} catch( Exception e ) {e.printStackTrace();}
		return result;
	}

	//finish a operation, update result for the operation
	public boolean finishOperation()
	{
		Connection con=this.getConnection();
		Statement	stmt;
		String operationid=Database.getInstance().getCurrentOperation();
		boolean result=false;
		try
		{
			Element e;
			stmt= con.createStatement();
			if (operationid!=null) {
				//Set visit finish time for operation
				SimpleDateFormat sf = new SimpleDateFormat("yyyy/MM/dd HH:mm:ss.S");
				String date = sf.format(new Date());
				stmt.executeUpdate("UPDATE operation SET visitfinishtime=\'" +date+"\' "+
												"WHERE operation_id="+operationid); 
			}
			stmt.close(); con.close();
			result=true;
		} catch( Exception e ) {e.printStackTrace();}
		return result;
	}

	//take all urls from a input text file, insert them into database
	public void loadInputUrlFromFile(final String inputUrlsFile) 
	{
		Element element;
		Connection con=this.getConnection();
		Statement	stmt;
                PreparedStatement ps;
		ResultSet rs;
		String line, url_id, honeypotid=null;
		String operationid=null;
		boolean check= true;
                long count=0;
		if (inputUrlsFile==null)
		{
			System.out.println("Error: There is no input-url file!");
			System.exit(1);
		}
		
		if ((ConfigManager.getInstance().getConfigOption("import_check")!=null) && (ConfigManager.getInstance().getConfigOption("import_check").toLowerCase().equals("false")))
		{
			check=false;
		}

		try
		{
			stmt= con.createStatement();
			//get honeypot id
			String serverip=ConfigManager.getInstance().getConfigOption("server-listen-address");
			String serverport=ConfigManager.getInstance().getConfigOption("server-listen-port");
			rs=stmt.executeQuery("SELECT honeypot_id FROM honeypot WHERE ipaddress=\'"+serverip+"\'");
			if (rs.next())
			{
				honeypotid=rs.getString(1);
			} else 
			{
				//insert a new honeypot 
				stmt.executeUpdate("INSERT INTO honeypot(ipaddress, port) Values(\'"+serverip+"\', "+serverport+")", Statement.RETURN_GENERATED_KEYS);
				rs=stmt.getGeneratedKeys();
				if (rs.next())
				{
					honeypotid=rs.getString(1);
				}  
				else 
				{
					System.out.println("System can't find any honeypot ip="+serverip);
					System.exit(0);
				}
			}
            setSystemStatus(true);
			//open url file
			BufferedReader in = new BufferedReader(new InputStreamReader(new FileInputStream(inputUrlsFile), "UTF-8"));
						
			//add new operation
			stmt.executeUpdate("INSERT INTO operation(description, honeypot_id) Values (\'"+inputUrlsFile+"\', \'"+honeypotid+"\')", Statement.RETURN_GENERATED_KEYS);
			rs=stmt.getGeneratedKeys();
			if (rs.next())
			{
				operationid=rs.getString(1); setCurrentOperation(operationid);
			}  
			System.out.println("The system is going to inspect urls in the new operation: " + operationid);

			//update visit start time for operation
			SimpleDateFormat sf = new SimpleDateFormat("yyyy/MM/dd HH:mm:ss.S");
			String date = sf.format(new Date());
			stmt.executeUpdate("UPDATE operation SET visitstarttime=\'" +date+"\' "+
												"WHERE operation_id="+operationid+" AND visitstarttime IS NULL"); 

			System.out.println("Please wait for inserting urls into database...");
			if (!check) //NO checking existance of url in database
			{
				while ((line=in.readLine())!=null) {
					if ((line.length() > 0)) {
						//line = line.trim().toLowerCase();
                                                line = line.trim();
						if (!line.startsWith("#")) {
                                                        ps = con.prepareStatement("INSERT INTO url(url) Values (?)", Statement.RETURN_GENERATED_KEYS);
                                                        ps.setString(1, line);
                                                        ps.executeUpdate();
							rs=ps.getGeneratedKeys(); rs.next();
							url_id=rs.getString(1);
							stmt.executeUpdate("INSERT INTO url_operation(url_id, operation_id) Values ("+url_id+", "+operationid+")");

							element = new Element();
							element.name = "url";
							element.attributes.put("add", "");
                            element.attributes.put("id", url_id);
							element.attributes.put("url", line);
							EventsController.getInstance().notifyEventObservers(element);
                                                        count++;
						}
					}
				}
			}
			else //checking existance of url in database. Inserting only if no existance
			{
				while ((line=in.readLine())!=null) {
					if ((line.length() > 0)) {
						//line = line.trim().toLowerCase();
                                                line = line.trim();
						if (!line.startsWith("#")) {
                                                        ps = con.prepareStatement("SELECT url_id FROM url WHERE url.url = ?");
                                                        ps.setString(1, line);
							rs=ps.executeQuery();
							if (!rs.next()) 
							{
                                                            ps = con.prepareStatement("INSERT INTO url(url) Values (?)", Statement.RETURN_GENERATED_KEYS);
                                                            ps.setString(1, line);
                                                            ps.executeUpdate();
                                                            rs=ps.getGeneratedKeys(); rs.next();
                                                            count++;
							}
                                                        //check URL id and operation id: not exist
                                                        url_id=rs.getString(1);
                                                        ps = con.prepareStatement("SELECT url_id, operation_id FROM url_operation WHERE url_id = ? AND operation_id= ?");
                                                        ps.setLong(1, Long.parseLong(url_id));
                                                        ps.setLong(2, Long.parseLong(operationid));
							rs=ps.executeQuery();
							if (!rs.next())
							{
                                                            stmt.executeUpdate("INSERT INTO url_operation(url_id, operation_id) Values ("+url_id+", "+operationid+")");
                                                            element = new Element();
                                                            element.name = "url";
                                                            element.attributes.put("add", "");
                                                            element.attributes.put("id", url_id);
                                                            element.attributes.put("url", line);
                                                            EventsController.getInstance().notifyEventObservers(element);
                                                        }
						}
					}
				}
			}
			con.close();
			System.out.println("******** IMPORT URLs INTO DATABASE: " + count + " URLs have been inserted into database! ********");
		} catch( Exception e ) {e.printStackTrace();}
	}

   //load all urls from url table and then run operation
	public void loadInputUrlFromDatabase()
	{
		Element element;
		Connection con=this.getConnection();
		Statement	stmt1, stmt2;
		ResultSet rs;
		String url, url_id, honeypotid=null;
		String operationid=null;
                long count=0;
		try
		{
			stmt1= con.createStatement();
			stmt2= con.createStatement();
			//get honeypot id
			String serverip=ConfigManager.getInstance().getConfigOption("server-listen-address");
			String serverport=ConfigManager.getInstance().getConfigOption("server-listen-port");
			rs=stmt1.executeQuery("SELECT honeypot_id FROM honeypot WHERE ipaddress=\'"+serverip+"\'");
			if (rs.next())
			{
				honeypotid=rs.getString(1);
			} else 
			{
				//insert a new honeypot 
				stmt1.executeUpdate("INSERT INTO honeypot(ipaddress, port) Values(\'"+serverip+"\', "+serverport+")", Statement.RETURN_GENERATED_KEYS);
				rs=stmt1.getGeneratedKeys();
				if (rs.next())
				{
					honeypotid=rs.getString(1);
				}  
				else 
				{
					System.out.println("System can't find any honeypot ip="+serverip);
					System.exit(0);
				}
			}

            setSystemStatus(true);
			//add new operation
			stmt1.executeUpdate("INSERT INTO operation(description, honeypot_id) Values (\'"+"Load urls from database"+"\', \'"+honeypotid+"\')", Statement.RETURN_GENERATED_KEYS);
			rs=stmt1.getGeneratedKeys();
			if (rs.next())
			{
				operationid=rs.getString(1); setCurrentOperation(operationid);
			}  
			System.out.println("The system is going to inspect urls in the new operation: " + operationid);

			//update visit start time for operation
			SimpleDateFormat sf = new SimpleDateFormat("yyyy/MM/dd HH:mm:ss.S");
			String date = sf.format(new Date());
			stmt1.executeUpdate("UPDATE operation SET visitstarttime=\'" +date+"\' "+
												"WHERE operation_id="+operationid+" AND visitstarttime IS NULL"); 

			//load urls from url table
			System.out.println("Loading urls from database....");
			rs=stmt1.executeQuery("SELECT url_id, url FROM url");
			while (rs.next()) {
							url_id=rs.getString(1);
							url=rs.getString(2);
							stmt2.executeUpdate("INSERT INTO url_operation(url_id, operation_id) Values ("+url_id+", "+operationid+")");

							element = new Element();
							element.name = "url";
							element.attributes.put("add", "");
                            element.attributes.put("id", url_id);
							element.attributes.put("url", url);
							EventsController.getInstance().notifyEventObservers(element);
                            count++;
			}
			con.close();
            System.out.println("******** LOADING URL FROM DATABASE: "+count+" urls have been loaded! ********");
		} catch( Exception e ) {e.printStackTrace();}
	}

   //resume last operation
	public boolean resumeLastOperation() 
	{
		Connection con=this.getConnection();
		Statement	stmt;
		ResultSet rs;
		String operationid=null;
		String serverip=ConfigManager.getInstance().getConfigOption("server-listen-address");
		boolean result=false;
                long count=0;
		try
		{
			Element e;
			stmt= con.createStatement();
			
			//find the last operation which still has unvisited urls.
			stmt.executeQuery("SELECT DISTINCT a.operation_id from url_operation a, operation b, honeypot c " +
						"WHERE a.operation_id=b.operation_id AND b.honeypot_id=c.honeypot_id AND a.status_id IS NULL " +
						" AND c.ipaddress=\'"+serverip+"\' order by operation_id DESC");
			rs = stmt.getResultSet();
			if (rs.next())
			{
                
				operationid=rs.getString(1);
				System.out.println("System is going to inspect urls in the operation: "+operationid);
				Database.getInstance().setCurrentOperation(operationid);
				setSystemStatus(true);
                
				//update visit start time for operation if it hasn't set yet
				SimpleDateFormat sf = new SimpleDateFormat("yyyy/MM/dd HH:mm:ss.S");
				String date = sf.format(new Date());
				stmt.executeUpdate("UPDATE operation SET visitstarttime=\'" +date+"\' "+
												"WHERE operation_id="+operationid+" AND visitstarttime IS NULL"); 
				
				//get all urls which haven't been visited yet
				stmt.executeQuery("SELECT url.url_id, url.url FROM url, url_operation " +
				"WHERE url_operation.url_id=url.url_id AND (url_operation.status_id IS NULL) AND url_operation.operation_id="+operationid);
				rs = stmt.getResultSet();
				while (rs.next()) 
				{
					e = new Element();
					e.name = "url";
					e.attributes.put("add", "");
                    e.attributes.put("id", rs.getString(1));
					e.attributes.put("url", rs.getString(2));
					EventsController.getInstance().notifyEventObservers(e);
                                        count++;
				}
			}
			stmt.close(); con.close();
			result=true;
		} catch( Exception e ) {e.printStackTrace();}
                System.out.println("******** RESUME: "+ count +" URLs have been loaded! ********");
		return result;
	}

	//import urls from text file without operation
	public void importUrlFromFile() 
	{

		Connection con=this.getConnection();
		PreparedStatement ps;
		ResultSet rs;
		String line, url_id, honeypotid=null;
		String operationid=null;
		String inputUrlsFile= ConfigManager.getInstance().getConfigOption("input_urls");
                long count=0;
		boolean check= true;
		
		if (inputUrlsFile==null)
		{
			System.out.println("Error: There is no input-url file!");
			System.exit(1);
		}
		
		if ((ConfigManager.getInstance().getConfigOption("import_check")!=null) && (ConfigManager.getInstance().getConfigOption("import_check").toLowerCase().equals("false")))
		{
			check=false;
		}
		try
		{	

			//open url file
			BufferedReader in = new BufferedReader(new InputStreamReader(new FileInputStream(inputUrlsFile), "UTF-8"));
						
			System.out.println("Please wait for importing urls into database...");
			if (!check)
			{
				while ((line=in.readLine())!=null) {
					if ((line.length() > 0)) {
						//line = line.trim().toLowerCase();
                                                line = line.trim();
						if (!line.startsWith("#")) {
                                                    ps = con.prepareStatement("INSERT INTO url(url) Values (?)");
                                                    ps.setString(1, line);
                                                    ps.executeUpdate();
                                                    count++;
						}
					}
				}
			}
			else 
			{
				while ((line=in.readLine())!=null) {
					if ((line.length() > 0)) {
						//line = line.trim().toLowerCase();
                                                line = line.trim();
						if (!line.startsWith("#")) {
                                                        ps = con.prepareStatement("SELECT url_id FROM url WHERE url.url = ?");
                                                        ps.setString(1, line);
							rs=ps.executeQuery();
							if (!rs.next()) 
							{
                                                            ps = con.prepareStatement("INSERT INTO url(url) Values (?)");
                                                            ps.setString(1, line);
                                                            ps.executeUpdate();
                                                            count++;
							} 
						}
					}
				}
			}
			con.close();
			System.out.println("******** IMPORTING URLs INTO DATABASEe: "+count+" URLs have been imported!********");
		} catch( Exception e ) {e.printStackTrace();}
	}


	//store collected files in database
	public void storeFile(String urlid, String fileName)
	{
		Connection con=this.getConnection();
		Statement	stmt;
		ResultSet rs;

		try
		{
			File file = new File(fileName);
			FileInputStream fis = new FileInputStream(file);
			PreparedStatement ps = con.prepareStatement("INSERT INTO file(url_id, operation_id,  filename, content) VALUES ("+urlid+", "+getCurrentOperation()+", ?, ?)");
			ps.setString(1, fileName.substring(4));
			ps.setBinaryStream(2, fis, (int)file.length());
			ps.executeUpdate();
			ps.close();
			fis.close();
			con.close();
		} catch( Exception e ) {e.printStackTrace();}
	}

	//store event in database
	public void storeEvent(String urlid, String event)
	{
		Connection con=this.getConnection();
		Statement	stmt;
		ResultSet rs;

		try
		{

			//split event
			event=event.substring(1, event.length()-1);
			String[] str = event.split("\",\"");

			SimpleDateFormat sf1 = new SimpleDateFormat("dd/MM/yyyy HH:mm:ss.S");
			SimpleDateFormat sf2 = new SimpleDateFormat("yyyy/MM/dd HH:mm:ss.S");
			//convert dates into new format "yyyy/MM/dd HH:mm:ss.S"
			Date date = sf1.parse(str[1]);
			str[1] = sf2.format(date);

			PreparedStatement ps = con.prepareStatement("INSERT INTO event(url_id, time, operation_id,  type,  process, action, object1, object2) "+
				" VALUES ("+urlid+", \'" +str[1]+"\', "+getCurrentOperation()+", ?, ?, ?, ?, ?)");
			ps.setString(1, str[0]);
			ps.setString(2, str[2]);
			ps.setString(3, str[3]);
			ps.setString(4, str[4]);
			ps.setString(5, str[5]);
			ps.executeUpdate();
			ps.close();
			con.close();
		} catch( Exception e ) {e.printStackTrace();}
	}

	// store info about error urls
	public void storeErrorUrl(String urlid, String majorError, String minorError)
	{
		Connection con=this.getConnection();
		Statement	stmt;
		ResultSet rs;

		try
		{

			PreparedStatement ps = con.prepareStatement("INSERT INTO error(url_id, operation_id,  majorerror, minorerror) VALUES ("+urlid+", "+getCurrentOperation()+", ?, ?)");
			ps.setString(1, majorError);
			ps.setString(2, minorError);
			ps.executeUpdate();
			ps.close();
			con.close();
		} catch( Exception e ) {e.printStackTrace();}
	}
    
    // set system status: True: running, False: stopping
    public void setSystemStatus(boolean status)
    {
  		Connection con=this.getConnection();
		Statement	stmt;
		ResultSet rs;
        String ch;
        if (status) { ch="T";}
        else {ch="F";}

		try
		{
            stmt=con.createStatement();
            stmt.executeUpdate("UPDATE honeypot SET status=\'"+ch+"\'"+"WHERE ipaddress=\'"+ConfigManager.getInstance().getConfigOption("server-listen-address")+"\'");
			stmt.close();
			con.close();
		} catch( Exception e ) {e.printStackTrace();}

    }
    // check system status
    public boolean getSystemStatus()
    {
        Connection con=this.getConnection();
		Statement	stmt;
		ResultSet rs;
        boolean result=true;
        try
        {
            stmt= con.createStatement();

			//find the oldest operation which still has unvisited urls.
			stmt.executeQuery("SELECT status FROM honeypot WHERE ipaddress=\'"+ConfigManager.getInstance().getConfigOption("server-listen-address")+"\'");
			rs = stmt.getResultSet();
			if (rs.next())
			{
				if (rs.getString(1).equals("F")) {
                    result=false;
                }
                else { result=true;}
            }
            stmt.close();
			con.close();
 		} catch( Exception e ) {e.printStackTrace();}
        return result;
    }
}

