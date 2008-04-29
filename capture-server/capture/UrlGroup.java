package capture;

import java.util.*;
import java.text.ParseException;
import java.text.SimpleDateFormat;
import java.security.InvalidParameterException;
import java.net.URLEncoder;
import java.net.URLDecoder;
import java.io.UnsupportedEncodingException;

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
    private int errorCount = 0;


    private static Random generator = new Random();
    private boolean malicious;
    private ERROR_CODES majorErrorCode = ERROR_CODES.OK;
    private Date visitStartTime;
    private Date visitFinishTime;
    private String algorithm;

    public UrlGroup(List<Url> urlList, boolean initialGroup) {
        urlGroupState = URL_GROUP_STATE.NONE;
        generateIdentifier();
        this.initialGroup = initialGroup;
        this.urlList = urlList;

        if (urlList.size() == 0) {
            throw new InvalidParameterException("Cant create url group with size 0.");
        }

        Url firstUrl = urlList.get(0);
        this.clientProgram = firstUrl.getClientProgram();
        this.visitTime = firstUrl.getVisitTime();
        for (Iterator<Url> iterator = urlList.iterator(); iterator.hasNext();) {
            Url url = iterator.next();
            url.setGroupId(identifier);
            url.setInitialGroup(initialGroup);
            if (!this.clientProgram.equals(url.getClientProgram())) {
                System.out.println("Invalid url group. Different client program. Setting to " + this.clientProgram);
            }
            if (this.visitTime != url.getVisitTime()) {
                System.out.println("Invalid url group. Different visit times. Setting to " + this.visitTime);
            }
        }
    }

    public void generateIdentifier() {
        this.identifier = generator.nextInt();
    }

    /**
     * @param uri - url encoded URL. However, because encoding might have slight differences, we will decode and encode
     */
    public Url getUrl(String uri) {
        try {
            String decodedURI = URLDecoder.decode(uri, "UTF-8");
            String encodedURI = URLEncoder.encode(decodedURI, "UTF-8");


            for (Iterator<Url> iterator = urlList.iterator(); iterator.hasNext();) {
                Url url = iterator.next();
                if (url.getEscapedUrl().equalsIgnoreCase(encodedURI)) {
                    return url;
                }
            }

            System.out.println("URL " + encodedURI + " not found. Looking through group.");
            for (Iterator<Url> iterator = urlList.iterator(); iterator.hasNext();) {
                Url url = iterator.next();
                System.out.println("URL: " + url.getEscapedUrl());
            }
            throw new NullPointerException("Unable to find Url " + uri + ".");
        } catch (UnsupportedEncodingException e) {
            e.printStackTrace(System.out);
            throw new NullPointerException("Unable to find Url " + uri + ".");
        }
    }

    public void setUrlGroupState(URL_GROUP_STATE newState) {
        urlGroupState = newState;
        if (urlGroupState == URL_GROUP_STATE.ERROR) {
            errorCount++;
        }
        if (urlGroupState == URL_GROUP_STATE.ERROR) {
            errorCount++;
        }

        for (Iterator<Url> iterator = urlList.iterator(); iterator.hasNext();) {
            Url url = iterator.next();
            if (urlGroupState == URL_GROUP_STATE.VISITING) {

                url.setUrlState(URL_STATE.VISITING);
            } else if (urlGroupState == URL_GROUP_STATE.VISITED) {
                if (url.getMajorErrorCode() == ERROR_CODES.OK) {
                    url.setUrlState(URL_STATE.VISITED);
                } else {
                    url.setUrlState(URL_STATE.ERROR);
                }
            } else if (urlGroupState == URL_GROUP_STATE.ERROR) {
                if (url.getMajorErrorCode() == ERROR_CODES.OK) {
                    url.setMajorErrorCode(this.majorErrorCode.errorCode);
                }
                url.setUrlState(URL_STATE.ERROR);

            }
        }

        this.setChanged();
        this.notifyObservers();
    }


    public int getErrorCount() {
        return errorCount;
    }

    public URL_GROUP_STATE getUrlGroupState() {
        return urlGroupState;
    }

    public String toVisitEvent() {
        String urlGroupEvent = "<visit-event ";
        urlGroupEvent += "identifier=\"" + identifier + "\" program=\"" + clientProgram + "\" time=\"" + visitTime + "\">";
        for (Iterator<Url> iterator = urlList.iterator(); iterator.hasNext();) {
            Url url = iterator.next();
            urlGroupEvent += "<item url=\"" + url.getEscapedUrl() + "\"/>";
        }
        urlGroupEvent += "</visit-event>";
        return urlGroupEvent;
    }

    public int size() {
        return urlList.size();
    }


    public void writeEventToLog(Map<String,String> urlCSVMap) {
        for (Iterator<Url> urlIterator = urlList.iterator(); urlIterator.hasNext();) {
            Url url = urlIterator.next();
            String csv = urlCSVMap.get(url.getUrl());
            if (csv == null || csv.equals("")) {
                if(urlList.size() == 1 && urlCSVMap.size()==1) {
                    csv = urlCSVMap.values().iterator().next();
                    url = urlList.get(0);
                    url.writeEventToLog(csv);
                    break;
                } else {
                    System.out.println("WARNING: Couldnt find csv for URL " + url.getUrl());
                }
            } else {
                url.writeEventToLog(csv);
            }

        }
    }

    public void writeEventToLog(String event) {
        if (urlList.size() == 1) {
            Url url = urlList.get(0);
            url.writeEventToLog(event);
        }
    }


    public int getIdentifier() {
        return identifier;
    }

    public void setMalicious(boolean malicious) {
        this.malicious = malicious;
        if (!this.malicious) {
            for (Iterator<Url> iterator = urlList.iterator(); iterator.hasNext();) {
                Url url = iterator.next();
                url.setMalicious(false);
            }
        } else {
            if (size() == 1) {
                Url url = urlList.get(0);
                url.setMalicious(true);
            }
        }

    }

    public boolean isMalicious() {
        return this.malicious;
    }

    public void setMajorErrorCode(long majorErrorCode) {
        boolean validErrorCode = false;

        for (ERROR_CODES e : ERROR_CODES.values()) {
            if (majorErrorCode == e.errorCode) {
                validErrorCode = true;
                this.majorErrorCode = e;
            }
        }

        if (!validErrorCode) {
            System.out.println("Received invalid error code from client " + majorErrorCode);
            this.majorErrorCode = ERROR_CODES.INVALID_ERROR_CODE_FROM_CLIENT;
        }
    }

    public String getGroupAsFileName() {
        if (size() == 1) {
            Url url = urlList.get(0);
            return url.getUrlAsFileName();
        } else {
            return identifier + "";
        }
    }

    public List<Url> getUrlList() {
        return urlList;
    }


    public ERROR_CODES getMajorErrorCode() {
        if (majorErrorCode == null) {
            return ERROR_CODES.OK;
        }
        return majorErrorCode;
    }

    public void setVisitStartTime(String visitStartTime) {
        try {
            SimpleDateFormat sf = new SimpleDateFormat("d/M/yyyy H:m:s.S");
            this.visitStartTime = sf.parse(visitStartTime);
        } catch (ParseException e) {
            e.printStackTrace(System.out);
        }
    }

    public void setVisitFinishTime(String visitFinishTime) {
        try {
            SimpleDateFormat sf = new SimpleDateFormat("d/M/yyyy H:m:s.S");
            this.visitFinishTime = sf.parse(visitFinishTime);
        } catch (ParseException e) {
            e.printStackTrace(System.out);
        }
    }

    public void setInitialGroup(boolean b) {
        initialGroup = b;
        for (Iterator<Url> iterator = urlList.iterator(); iterator.hasNext();) {
            Url url = iterator.next();
            url.setInitialGroup(b);
        }
    }

    public void setAlgorithm(String algorithm) {
        this.algorithm = algorithm;
    }

    public String getAlgorithm() {
        return algorithm;
    }
}
