package capture;

import java.io.BufferedWriter;
import java.io.File;
import java.io.FileOutputStream;
import java.io.FileWriter;
import java.io.IOException;
import java.io.OutputStreamWriter;

public class Logger {

	private BufferedWriter maliciousLog;
	private BufferedWriter safeLog;
	private BufferedWriter progressLog;
	private BufferedWriter errorLog;
	
	private Logger()
	{
		try {
			File f = new File("log");
			if(!f.isDirectory())
			{
				f.mkdir();
			}
			 
			maliciousLog = new BufferedWriter(new OutputStreamWriter(new FileOutputStream("log"+File.separator+"malicious.log"), "UTF-8"));
			safeLog = new BufferedWriter(new OutputStreamWriter(new FileOutputStream("log"+File.separator+"safe.log"), "UTF-8"));
			progressLog = new BufferedWriter(new OutputStreamWriter(new FileOutputStream("log"+File.separator+"progress.log"), "UTF-8"));
			errorLog = new BufferedWriter(new OutputStreamWriter(new FileOutputStream("log"+File.separator+"error.log"), "UTF-8"));
		} catch (IOException e) {
			e.printStackTrace();
		}
	}
	 
	private static class LoggerHolder
	{ 
		private final static Logger instance = new Logger();
	}
	 
	public static Logger getInstance()
	{
		return LoggerHolder.instance;
	}
	
	public void writeToMaliciousUrlLog(String text) 
	{
		try {
			maliciousLog.write(text);
			maliciousLog.newLine();
			maliciousLog.flush();
		} catch (IOException e) {
			e.printStackTrace();
		}
	}
	
	public void writeToSafeUrlLog(String text) 
	{
		try {
			safeLog.write(text);
			safeLog.newLine();
			safeLog.flush();
		} catch (IOException e) {
			e.printStackTrace();
		}
	}
	
	public void writeToProgressLog(String text) 
	{
		try {
			progressLog.write(text);
			progressLog.newLine();
			progressLog.flush();
		} catch (IOException e) {
			e.printStackTrace();
		}
	}
	
	public void writeToErrorLog(String text) 
	{
		try {
			errorLog.write(text);
			errorLog.newLine();
			errorLog.flush();
		} catch (IOException e) {
			e.printStackTrace();
		}
	}
}
