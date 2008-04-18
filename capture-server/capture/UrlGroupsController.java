package capture;

import java.util.*;
import java.util.concurrent.LinkedBlockingDeque;

public class UrlGroupsController extends Observable implements Observer {
    private LinkedBlockingDeque<UrlGroup> urlGroupQueue;
    private List<UrlGroup> visitingList;
    private List<UrlGroup> visitedList;

    private UrlGroupsController() {
        urlGroupQueue = new LinkedBlockingDeque<UrlGroup>();
        visitedList = new ArrayList<UrlGroup>();
        visitingList = new ArrayList<UrlGroup>();
    }

    private final static UrlGroupsController instance = new UrlGroupsController();


    public static UrlGroupsController getInstance() {
        return instance;
    }

    public UrlGroup takeUrlGroup() throws InterruptedException {
        UrlGroup urlGroup = null;
        if (urlGroupQueue.size() == 0) {
            List<Url> urlList = new ArrayList<Url>();
            int initialUrlGroupSize = getUrlGroupSize();

            if(UrlsController.getInstance().getQueueSize()<initialUrlGroupSize) { //if we dont have enough to fill group, just do all
                initialUrlGroupSize = UrlsController.getInstance().getQueueSize();
            }
            
            if(initialUrlGroupSize==0) { //if all done, we wait for new urls to be input into capture
                initialUrlGroupSize = 1;
            }

            for (int i = 0; i < initialUrlGroupSize; i++) {
                Url url = UrlsController.getInstance().takeUrl();
                urlList.add(url);
            }
            urlGroup = new UrlGroup(urlList, true);
            urlGroup.addObserver(this);
        } else {
            urlGroup = urlGroupQueue.pop();
        }
        return urlGroup;
    }

    private int getUrlGroupSize() {
        // e.g. 1.12%
        double maliciousPercentage = Double.parseDouble(ConfigManager.getInstance().getConfigOption("p_m"));

        int k = 80;

        if (maliciousPercentage < 0.005) { //0.5%
            k = 80;
        } else if (maliciousPercentage < 0.010) {
            k = 40;
        } else if (maliciousPercentage < 0.015) {
            k = 27;
        } else if (maliciousPercentage < 0.020) {
            k = 20;
        } else if (maliciousPercentage < 0.025) {
            k = 16;
        } else if (maliciousPercentage < 0.030) {
            k = 13;
        } else if (maliciousPercentage < 0.035) {
            k = 11;
        } else if (maliciousPercentage < 0.040) {
            k = 10;
        } else if (maliciousPercentage < 0.045) {
            k = 9;
        } else if (maliciousPercentage < 0.050) {
            k = 8;
        } else if (maliciousPercentage < 0.055) {
            k = 7;
        } else if (maliciousPercentage < 0.060) {
            k = 7;
        } else if (maliciousPercentage < 0.065) {
            k = 6;
        } else if (maliciousPercentage < 0.070) {
            k = 6;
        } else if (maliciousPercentage < 0.075) {
            k = 5;
        } else if (maliciousPercentage < 0.080) {
            k = 5;
        } else if (maliciousPercentage < 0.085) {
            k = 5;
        } else if (maliciousPercentage < 0.090) {
            k = 5;
        } else if (maliciousPercentage < 0.095) {
            k = 4;
        } else if (maliciousPercentage < 0.09) {
            k = 4;
        } else {
            k = 1;
        }
        return k;
    }

    public void update(Observable o, Object arg) {
        UrlGroup urlGroup = (UrlGroup) o;

        if (urlGroup.getUrlGroupState() == URL_GROUP_STATE.VISITING) {
            visitingList.add(urlGroup);

        } else if (urlGroup.getUrlGroupState() == URL_GROUP_STATE.VISITED) {
            visitingList.remove(urlGroup);
            visitedList.add(urlGroup);

            if (urlGroup.isMalicious() && urlGroup.size() > 1) {
                List<Url> urls = urlGroup.getUrlList();
                List<Url> urls1 = new ArrayList<Url>();
                List<Url> urls2 = new ArrayList<Url>();

                int b = urls.size() / 2;
                for (int i = 0; i < urls.size(); i++) {
                    if (i < b) {
                        Url url = urls.get(i);
                        urls1.add(url);
                    } else {
                        Url url = urls.get(i);
                        urls2.add(url);
                    }
                }

                UrlGroup urlGroup1 = new UrlGroup(urls1, false);
                urlGroup1.addObserver(this);
                urlGroupQueue.push(urlGroup1);

                UrlGroup urlGroup2 = new UrlGroup(urls2, false);
                urlGroup2.addObserver(this);
                urlGroupQueue.push(urlGroup2);
            }

        } else if (urlGroup.getUrlGroupState() == URL_GROUP_STATE.ERROR) {
            visitingList.remove(urlGroup);

            List<Url> urls = urlGroup.getUrlList();
            boolean networkReset = true;
            for (Iterator<Url> iterator = urls.iterator(); iterator.hasNext();) {
                Url url = iterator.next();
                url.setMajorErrorCode(urlGroup.getMajorErrorCode().errorCode);
                if(url.getMajorErrorCode()==ERROR_CODES.NETWORK_ERROR && url.getMinorErrorCode()==2148270085L) {
                    networkReset = true;
                }
                url.setUrlState(URL_STATE.ERROR);
            }

            //certain errors will be retried: vm_stalled, client inactivity, network reset
            boolean retry    = networkReset;
            if(urlGroup.getMajorErrorCode()==ERROR_CODES.VM_STALLED || urlGroup.getMajorErrorCode()==ERROR_CODES.CAPTURE_CLIENT_INACTIVITY || urlGroup.getMajorErrorCode()== ERROR_CODES.CAPTURE_CLIENT_CONNECTION_RESET) {
                retry = true;
            }

            if(retry && urlGroup.getErrorCount()<=1) {
                System.out.println("Retrying to visit group " + urlGroup.getIdentifier());
                urlGroup.getIdentifier();   //generate new identifier, so group can be differentiated in the log
                urlGroupQueue.push(urlGroup);
            }

        }
    }


}


