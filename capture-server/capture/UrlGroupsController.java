package capture;

import java.util.*;
import java.util.concurrent.LinkedBlockingDeque;

public class UrlGroupsController extends Observable implements Runnable,Observer {
    private LinkedBlockingDeque<UrlGroup> urlGroupQueue;
    private List<UrlGroup> visitingList;
    private List<UrlGroup> visitedList;
    private Thread queueMonitor;


    private UrlGroupsController() {
        urlGroupQueue = new LinkedBlockingDeque<UrlGroup>();
        visitedList = new ArrayList<UrlGroup>();
        visitingList = new ArrayList<UrlGroup>();

        boolean terminate = Boolean.parseBoolean(ConfigManager.getInstance().getConfigOption("terminate"));
        if(terminate) {
            queueMonitor = new Thread(this, "QueueMonitor");
            queueMonitor.start();
        }


    }

    public void run() {
        try {
            Thread.sleep(60*1000*5); //dont quit in the first five minutes as queue is building

            while(true) {
                if(visitingList.size()==0 && urlGroupQueue.size()==0 && UrlsController.getInstance().getQueueSize()==0 && (PostprocessorFactory.getActivePostprocessor()!=null && !PostprocessorFactory.getActivePostprocessor().processing())) {
                    System.out.println("No more urls in queues...exiting in 10 sec.");
                    Thread.sleep(10000);
                    if(visitingList.size()==0 && urlGroupQueue.size()==0 && UrlsController.getInstance().getQueueSize()==0 && (PostprocessorFactory.getActivePostprocessor()!=null && !PostprocessorFactory.getActivePostprocessor().processing())) {
                        System.out.println("exiting.");
                        System.exit(0);
                    }
                }
                Thread.sleep(10000);
            }
            

        } catch(InterruptedException e) {
            e.printStackTrace(System.out);
        }

    }

    private final static UrlGroupsController instance = new UrlGroupsController();


    public static UrlGroupsController getInstance() {
        return instance;
    }


    public int getAvailableURLSize() {
        return UrlsController.getInstance().getQueueSize();
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
        return Integer.parseInt(ConfigManager.getInstance().getConfigOption("group_size"));
    }

    public void update(Observable o, Object arg) {
        UrlGroup urlGroup = (UrlGroup) o;

        if (urlGroup.getUrlGroupState() == URL_GROUP_STATE.VISITING) {
            visitingList.add(urlGroup);

        } else if (urlGroup.getUrlGroupState() == URL_GROUP_STATE.VISITED) {
            visitingList.remove(urlGroup);
            visitedList.add(urlGroup);

            if (urlGroup.getAlgorithm().equals("dac") && urlGroup.isMalicious() && urlGroup.size() > 1) {
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

            boolean split = false;
            if (urlGroup.getAlgorithm().equals("dac") && urlGroup.isMalicious() && urlGroup.size() > 1) {
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

                split = true;
            }

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
            if(!split && retry && urlGroup.getErrorCount()<=1) {
                System.out.println("Retrying to visit group " + urlGroup.getIdentifier());
                urlGroup.generateIdentifier();   //generate new identifier, so group can be differentiated in the log
                urlGroupQueue.push(urlGroup);
            }

        }
    }


}


