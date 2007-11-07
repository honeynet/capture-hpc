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
import java.text.ParseException;
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
	private Boolean malicious;
	private URL_STATE urlState;
	private Date visitStartTime;
	private Date visitFinishTime;
	private String logFileDate;
	
	private BufferedWriter logFile;
    private long groupID;
    private boolean initialGroup;

    public Url(String u, String cProgram, int vTime) throws URISyntaxException
	{
		url = new URI(u);
		malicious = null;
		if(cProgram == null || cProgram == "") {
			clientProgram = "iexplore";
		} else {
			clientProgram = cProgram;
		}
		visitTime = vTime;
		urlState = URL_STATE.NONE;
	}

    public void setVisitStartTime(String visitStartTime)
    {
        try {
            SimpleDateFormat sf = new SimpleDateFormat("d/M/yyyy H:m:s.S");
            this.visitStartTime=sf.parse(visitStartTime);
        } catch (ParseException e) {
            e.printStackTrace();  //To change body of catch statement use File | Settings | File Templates.
        }
    }

    public void setGroupId(long groupID) {
        this.groupID = groupID;
    }

    public String getVisitStartTime() {
        SimpleDateFormat sf = new SimpleDateFormat("dd/MM/yyyy HH:mm:ss.S");
		return sf.format(visitStartTime);
    }

    public void setVisitFinishTime(String visitFinishTime)  {
        try {
            SimpleDateFormat sf = new SimpleDateFormat("d/M/yyyy H:m:s.S");
            this.visitFinishTime=sf.parse(visitFinishTime);
        } catch (ParseException e) {
            e.printStackTrace();  //To change body of catch statement use File | Settings | File Templates.
        }
    }

    public String getVisitFinishTime() {
        SimpleDateFormat sf = new SimpleDateFormat("dd/MM/yyyy HH:mm:ss.S");
		return sf.format(visitFinishTime);
    }

    private String getLogfileDate(long time) {
        SimpleDateFormat sf = new SimpleDateFormat("ddMMyyyy_HHmmss");        
        return sf.format(new Date(time));
    }
	
	public void writeEventToLog(String event)
	{
		try {
            if(logFile==null) {
                String logFileName = filenameEscape(url.toString())+ "_" + this.getLogfileDate(visitStartTime.getTime());
                logFile = new BufferedWriter(new OutputStreamWriter(new FileOutputStream("log" + File.separator + logFileName + ".log"), "UTF-8"));
            }

			logFile.write(event);
			logFile.newLine();
			logFile.flush();
		} catch (UnsupportedEncodingException e) {
			e.printStackTrace();
		} catch (IOException e) {
			e.printStackTrace();
		}    
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
		if(urlState == URL_STATE.VISITING) {
			String date = getVisitStartTime();
			Logger.getInstance().writeToProgressLog("\"" + date + "\",\"visiting\",\"" + groupID + "\",\"" + url + "\",\"" + clientProgram + "\",\"" + visitTime + "\"");

		} else if(urlState == URL_STATE.VISITED) {
            String date = getVisitFinishTime();
			Logger.getInstance().writeToProgressLog("\"" + date + "\",\"visited\",\"" + groupID + "\",\"" + url + "\",\"" + clientProgram + "\",\"" + visitTime + "\"");
			if(this.malicious!=null) {
                if(this.malicious)
                {
                    Logger.getInstance().writeToMaliciousUrlLog("\"" + date + "\",\"malicious\",\"" + groupID + "\",\"" + url +
                            "\",\"" + clientProgram + "\",\"" + visitTime + "\"");
                } else {
                    Logger.getInstance().writeToSafeUrlLog("\"" + date + "\",\"benign\",\"" + groupID + "\",\"" + url +
                            "\",\"" + clientProgram + "\",\"" + visitTime + "\"");
                }
            }
			try {
			    if(logFile!=null) {
				logFile.close();
			    }
			} catch (IOException e) {
				e.printStackTrace();
			}
		} else if(urlState == URL_STATE.ERROR) {
            String date = getVisitFinishTime();
		    Logger.getInstance().writeToProgressLog("\"" + date + "\",\"error"+errorCount+":"+majorErrorCode+"-"+minorErrorCode+"\",\"" + groupID + "\",\"" + url + "\",\"" + clientProgram + "\",\"" + visitTime + "\"");
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
		return this.filenameEscape(this.getUrl()) + "_" + this.getLogfileDate(visitStartTime.getTime());
	}

	public ERROR_CODES getMajorErrorCode() {
        if(majorErrorCode==null) {
            return ERROR_CODES.OK;
        }
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

    public long getGroupID() {
        return groupID;
    }

    public void setInitialGroup(boolean initialGroup) {
        this.initialGroup = initialGroup;
    }

    public boolean getInitialGroup() {
        return initialGroup;
    }
}
