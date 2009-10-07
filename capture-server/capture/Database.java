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

public abstract class Database {

	private static Database instance;
	
	//load all values from status, clientprogram into memory. It can save time to access database
	public abstract boolean loadTemporaryValue();
	//update urls status after inspectation. The status can be: benign, malicious, error	
	public abstract boolean setStatus(String urlid, String status, String visitStartTime, String visitFinishTime, String clientProgram);
	//finish a operation, update result for the operation
	public abstract boolean finishOperation();
	//resume last operation
	public abstract boolean resumeLastOperation();
	//take all urls from a input text file, insert them into database, and then run operation
	public abstract void loadInputUrlFromFile(final String inputUrlsFile);
	//load all urls from url table and then run operation
	public abstract void loadInputUrlFromDatabase();
	//import urls from text file without operation
    public abstract void importUrlFromFile(); 
	public abstract String getCurrentOperation();
	public abstract void setCurrentOperation(String operationid);
	//store collected files in database
	public abstract void storeFile(String urlid, String fileName);
	//store event in database
	public abstract void storeEvent(String urlid, String event);
	// store info about error urls
	public abstract void storeErrorUrl(String urlid, String majorError, String minorError);
    // set system status: True: running, False: stopping
    public abstract void setSystemStatus(boolean status);
    // check system status
    public abstract boolean getSystemStatus();

	//initialize type of database, make sure config.xml have been loaded
	public static void initialize()
	{
		if (ConfigManager.getInstance().getConfigOption("database-type").toLowerCase().equals("postgresql")) {
					instance = new PostgreSQLDatabase();
		}	else if ((ConfigManager.getInstance().getConfigOption("database-type").toLowerCase().equals("mysql")))
		{
			instance = new MySQLDatabase();
		}
		else 
		{
			System.out.println("Database "+ConfigManager.getInstance().getConfigOption("database-type")+" is not supported in this version!");
			System.exit(1);
		}
	}

	public static Database getInstance() 
	{
		return instance;
	}

}
