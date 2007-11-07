package capture;

import java.util.*;
import java.text.ParseException;
import java.text.SimpleDateFormat;
import java.security.InvalidParameterException;

enum URL_GROUP_STATE {
    NONE,
    QUEUED,
    VISITING,
    VISITED,
    ERROR
}

public class UrlGroup extends Observable {
    private URL_GROUP_STATE urlGroupState;
    private List<Url> urlList = new ArrayList<Url>();
    private String clientProgram = "iexplorer";
    private int visitTime = 5;
    private int identifier;
    private boolean initialGroup = true;


    private static Random generator = new Random();
    private boolean malicious;
    private long minorErrorCode;
    private long majorErrorCode;
    private int errorCount = 0;
    private Date visitStartTime;
    private Date visitFinishTime;

    public UrlGroup(List<Url> urlList, boolean initialGroup) {
        urlGroupState = URL_GROUP_STATE.NONE;
        this.identifier = generator.nextInt();
        this.initialGroup = initialGroup;
        this.urlList = urlList;

        if(urlList.size()==0) {
            throw new InvalidParameterException("Cant create url group with size 0.");
        }

        Url firstUrl = urlList.get(0);
        this.clientProgram = firstUrl.getClientProgram();
        this.visitTime = firstUrl.getVisitTime();
        for (Iterator<Url> iterator = urlList.iterator(); iterator.hasNext();) {
            Url url = iterator.next();
            url.setGroupId(identifier);
            url.setInitialGroup(initialGroup);
            if(!this.clientProgram.equals(url.getClientProgram())) {
                System.out.println("Invalid url group. Different client program. Setting to " + this.clientProgram);
            }
            if(this.visitTime!=url.getVisitTime()) {
                System.out.println("Invalid url group. Different visit times. Setting to " + this.visitTime);
            }
        }
    }

    public Url getUrl(String uri) {
        for (Iterator<Url> iterator = urlList.iterator(); iterator.hasNext();) {
            Url url = iterator.next();
            if(url.getEscapedUrl().equals(uri)) {
                return url;
            }
        }
        throw new NullPointerException("Unable to find Url " + uri + ".");
    }

    public int getErrorCount() {
        return errorCount;
    }

    public void setUrlGroupState(URL_GROUP_STATE newState) {
        urlGroupState = newState;

        if(urlGroupState==URL_GROUP_STATE.ERROR) {
            errorCount++;
        }

        for (Iterator<Url> iterator = urlList.iterator(); iterator.hasNext();) {
            Url url = iterator.next();
            if(urlGroupState==URL_GROUP_STATE.VISITING) {

                url.setUrlState(URL_STATE.VISITING);
            } else if (urlGroupState==URL_GROUP_STATE.VISITED) {
                if(url.getMajorErrorCode()==ERROR_CODES.OK) {
                    url.setUrlState(URL_STATE.VISITED);
                } else {
                    url.setUrlState(URL_STATE.ERROR);
                }
            } else if (urlGroupState==URL_GROUP_STATE.ERROR) {
                if(url.getMajorErrorCode()==ERROR_CODES.OK) {
                    url.setMajorErrorCode(this.majorErrorCode);
                }
                url.setUrlState(URL_STATE.ERROR);
            }
        }

        this.setChanged();
        this.notifyObservers();
    }


    public URL_GROUP_STATE getUrlGroupState() {
        return urlGroupState;
    }

    public String toVisitEvent()
	{
		String urlGroupEvent = "<visit ";
        urlGroupEvent += "identifier=\"" + identifier + "\" program=\"" + clientProgram + "\" time=\"" + visitTime + "\">";
		for (Iterator<Url> iterator = urlList.iterator(); iterator.hasNext();) {
            Url url = iterator.next();
            urlGroupEvent += "<item url=\"" + url.getEscapedUrl() + "\"/>";
        }
		urlGroupEvent += "</visit>";
		return urlGroupEvent;
	}

    public int size() {
        return urlList.size();
    }

    
    public void writeEventToLog(String event) {
        if(urlList.size()==1) {
            Url url = urlList.get(0);
            url.writeEventToLog(event);
        }
    }

    public int getIdentifier() {
        return identifier;
    }

    public void setMalicious(boolean malicious) {
        this.malicious = malicious;
        if(!this.malicious) {
            for (Iterator<Url> iterator = urlList.iterator(); iterator.hasNext();) {
                Url url = iterator.next();
                url.setMalicious(false);
            }
        } else {
            if(size()==1) {
                Url url = urlList.get(0);
                url.setMalicious(true);
            }
        }

    }

    public boolean isMalicious() {
        return this.malicious;
    }

    public void setMajorErrorCode(long majorErrorCode) {
        this.majorErrorCode = majorErrorCode;
    }

    public void setMinorErrorCode(long minorErrorCode) {
        this.minorErrorCode = minorErrorCode;
    }

    public String getGroupAsFileName() {
        if(size()==1) {
            Url url = urlList.get(0);
            return url.getUrlAsFileName();
        } else {
            return identifier+"";
        }
    }

    public List<Url> getUrlList() {
        return urlList;
    }

    public long getMinorErrorCode() {
        return minorErrorCode;
    }

    public long getMajorErrorCode() {
        return minorErrorCode;
    }

    public void setVisitStartTime(String visitStartTime) {
        try {
            SimpleDateFormat sf = new SimpleDateFormat("d/M/yyyy H:m:s.S");
            this.visitStartTime=sf.parse(visitStartTime);
        } catch (ParseException e) {
            e.printStackTrace();  //To change body of catch statement use File | Settings | File Templates.
        }
    }

    public void setVisitFinishTime(String visitFinishTime) {
        try {
            SimpleDateFormat sf = new SimpleDateFormat("d/M/yyyy H:m:s.S");
            this.visitFinishTime=sf.parse(visitFinishTime);
        } catch (ParseException e) {
            e.printStackTrace();  //To change body of catch statement use File | Settings | File Templates.
        }
    }

    public void setInitialGroup(boolean b) {
        initialGroup = b;
        for (Iterator<Url> iterator = urlList.iterator(); iterator.hasNext();) {
            Url url = iterator.next();
            url.setInitialGroup(b);
        }
    }
}
