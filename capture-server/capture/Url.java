package capture;

import java.io.BufferedWriter;
import java.io.File;
import java.io.FileOutputStream;
import java.io.FileWriter;
import java.io.IOException;
import java.io.OutputStreamWriter;
import java.io.UnsupportedEncodingException;
import java.net.URI;
import java.net.URISyntaxException;
import java.net.URLDecoder;
import java.net.URLEncoder;
import java.text.DateFormat;
import java.text.DecimalFormat;
import java.text.SimpleDateFormat;
import java.util.Calendar;
import java.util.Date;
import java.util.GregorianCalendar;
import java.util.Observable;
import java.util.regex.Matcher;

enum URL_STATE {
	NONE,
	QUEUED,
	VISITING,
	VISITED,
	ERROR
}

public class Url extends Observable {

	private URI url;
	private String clientProgram;
	private int visitTime;
	private int errorCount;
	private ERROR_CODES majorErrorCode;
	private long minorErrorCode;
	private boolean malicious;
	private URL_STATE urlState;
	private long visitStartTime;
	private long visitFinishTime;
	private String logFileDate;
	
	private BufferedWriter logFile;

	public Url(String u, String cProgram, int vTime) throws URISyntaxException
	{
		url = new URI(u);
		malicious = false;
		if(cProgram == null || cProgram == "") {
			clientProgram = "iexplore";
		} else {
			clientProgram = cProgram;
		}
		visitTime = vTime;
		urlState = URL_STATE.NONE;
	}
	
    private String getLogfileDate(long time) {
        SimpleDateFormat sf = new SimpleDateFormat("ddMMyyyy_HHmmss");        
        return sf.format(new Date(time));
    }
	
	public void writeEventToLog(String event)
	{
		try {
			logFile.write(event);
			logFile.newLine();
			logFile.flush();
		} catch (UnsupportedEncodingException e) {
			e.printStackTrace();
		} catch (IOException e) {
			e.printStackTrace();
		}    
	}
	
	public String toVisitEvent()
	{
		String urlEvent = "<visit ";
		urlEvent += "url=\"";
		urlEvent += this.getEscapedUrl();
		if(clientProgram != null)
		{
			urlEvent += "\" program=\"";
			urlEvent += clientProgram;
		}
		urlEvent += "\" time=\"";
		urlEvent += visitTime;
		urlEvent += "\" />";
		return urlEvent;
	}
	
	public boolean isMalicious()
	{
		return malicious;
	}
	
	public void setMalicious(boolean malicious)
	{
		this.malicious = malicious;
	}
	
	public URL_STATE getUrlState()
	{
		return urlState;
	}
	
	public void setUrlState(URL_STATE newState)
	{
        File f = new File("log");
        if(!f.isDirectory())
        {
            f.mkdir();
        }
        if(urlState == newState)
        	return;

		urlState = newState;
		System.out.println("\tUrlSetState: " + newState.toString());
		String date = DateFormat.getDateTimeInstance().format(new Date());
		if(urlState == URL_STATE.VISITING) {
			visitStartTime = Calendar.getInstance().getTimeInMillis();
			try {
				String logFileName = filenameEscape(url.toString())+ "_" + this.getLogfileDate(visitStartTime);
				logFile = new BufferedWriter(new OutputStreamWriter(new FileOutputStream("log" + File.separator + logFileName + ".log"), "UTF-8"));
			} catch (IOException e) {
				e.printStackTrace();
			}
			Logger.getInstance().writeToProgressLog("\"" + date + "\",\"visiting\",\"" + url + "\",\"" + clientProgram + "\",\"" + visitTime + "\"");
			
		} else if(urlState == URL_STATE.VISITED) {
			Logger.getInstance().writeToProgressLog("\"" + date + "\",\"visited\",\"" + url + "\",\"" + clientProgram + "\",\"" + visitTime + "\"");
			visitFinishTime = Calendar.getInstance().getTimeInMillis();
			if(this.malicious)
			{			
				Logger.getInstance().writeToMaliciousUrlLog("\"" + date + "\",\"malicious\",\"" + url + 
						"\",\"" + clientProgram + "\",\"" + visitTime + "\"");
			} else {
			    Logger.getInstance().writeToSafeUrlLog("\"" + date + "\",\"benign\",\"" + url +
						"\",\"" + clientProgram + "\",\"" + visitTime + "\"");
			}
			try {
			    if(logFile!=null) {
				logFile.close();
			    }
			} catch (IOException e) {
				e.printStackTrace();
			}
		} else if(urlState == URL_STATE.ERROR) {
		    Logger.getInstance().writeToProgressLog("\"" + date + "\",\"error"+errorCount+":"+majorErrorCode+"-"+minorErrorCode+"\",\"" + url + "\",\"" + clientProgram + "\",\"" + visitTime + "\"");
			try {
			    if(logFile!=null) {
				logFile.close();
			    }
			} catch (IOException e) {
				e.printStackTrace();
			}
			errorCount++;
		}
		this.setChanged();
		this.notifyObservers();
	}

    public int getVisitTime() {
	return visitTime;
    }

	public String getClientProgram() {
		return clientProgram;
	}

	public int getErrorCount() {
		return errorCount;
	}
	
	public String getEscapedUrl()
	{
		try {
			return URLEncoder.encode(url.toString(), "UTF-8");
		} catch (UnsupportedEncodingException e) {
			e.printStackTrace();
			return "";
		}
	}

	public String getUrl() {
		return url.toString();
	}
	
	private String filenameEscape(String text)
	{
		String escaped; 
		escaped = text;
		escaped = escaped.replaceAll("\\\\", "%5C");
		escaped = escaped.replaceAll("/", "%2F");
		escaped = escaped.replaceAll(":", "%3A");
		escaped = escaped.replaceAll("\\?", "%3F");
		escaped = escaped.replaceAll("\"", "%22");
		escaped = escaped.replaceAll("\\*", "%2A");
		escaped = escaped.replaceAll("<", "%3C");
		escaped = escaped.replaceAll(">", "%3E");
		escaped = escaped.replaceAll("\\|", "%7C");
		escaped = escaped.replaceAll("&", "%26");
		return escaped;
	}
	
	public String getUrlAsFileName() {
		return this.filenameEscape(this.getUrl()) + "_" + this.getLogfileDate(visitStartTime);
	}

	public ERROR_CODES getMajorErrorCode() {
		return majorErrorCode;
	}

	public void setMajorErrorCode(long majorErrorCode) {
		for (ERROR_CODES e : ERROR_CODES.values())
		{
			if(majorErrorCode == e.errorCode)
			{
				this.majorErrorCode = e;
			}
		}
		
	}

	public long getMinorErrorCode() {
		return minorErrorCode;
	}

	public void setMinorErrorCode(long minorErrorCode) {
		this.minorErrorCode = minorErrorCode;
	}
}
