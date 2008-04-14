package capture;

import java.net.URISyntaxException;
import java.util.concurrent.LinkedBlockingDeque;
import java.util.concurrent.PriorityBlockingQueue;
import java.util.*;
import java.text.SimpleDateFormat;

public class UrlsController extends Observable implements EventObserver, Observer {
    private PriorityBlockingQueue<Url> urlQueue;
    private LinkedList<Url> visitingList;
    private LinkedList<Url> visitedList;
    private LinkedList<Url> errorUrlList;
    private UrlFactory urlFactory;

    private static int INITIAL_URL_QUEUE_CAPACITY = 20000;

    private UrlsController() {
        visitingList = new LinkedList<Url>();
        visitedList = new LinkedList<Url>();
        errorUrlList = new LinkedList<Url>();
        urlFactory = new UrlFactory();

        urlQueue = new PriorityBlockingQueue<Url>(INITIAL_URL_QUEUE_CAPACITY, new Comparator<Url> () {

            public int compare(Url o1, Url o2) {
                int p1 = (int) (o1.getPriority()*100);
                int p2 = (int) (o2.getPriority()*100);
                return (p2 - p1);
            }
        });
        EventsController.getInstance().addEventObserver("url", this);
    }

    private final static UrlsController instance = new UrlsController();

    public static UrlsController getInstance() {
        return instance;
    }

    public int getQueueSize() {
        return urlQueue.size();
    }
    public void update(Element event) {
        if (event.name.equals("url")) {
            if (event.attributes.containsKey("add")) {
                addUrlEvent(event);
            } else if (event.attributes.containsKey("remove")) {
                removeUrlEvent(event);
            }
        }
    }

    private void addUrlEvent(Element event) {

        try {
            Url url = urlFactory.getUrl(event);
            this.addUrl(url);
        } catch (URISyntaxException e) {
            String date = currentTime();
            String url = event.attributes.get("url");
            ERROR_CODES majorErrorCode = ERROR_CODES.INVALID_URL;
            Logger.getInstance().writeToErrorLog("\"" + date + "\",\"error:" + majorErrorCode + "\",\"nogroup\",\"" + url + "\",\"unkown\",\"unknown\"");
            System.out.println(e.getMessage());
        }

    }

    private String currentTime() {
        long current = System.currentTimeMillis();
        Date currentDate = new Date(current);
        SimpleDateFormat sdf = new SimpleDateFormat("MMM d, yyyy h:mm:ss a");
        String strDate = sdf.format(currentDate);
        return strDate;
    }

    private boolean inUrlQueue(Url url) {
        for (Iterator<Url> urlIterator = urlQueue.iterator(); urlIterator.hasNext();) {
            Url url1 = urlIterator.next();
            if(url.getEscapedUrl().equalsIgnoreCase(url1.getEscapedUrl()) && url.getClientProgram().equals(url1.getClientProgram()) && url.getVisitTime()==url1.getVisitTime()) {
                return true;
            }
        }
        return false;
    }

    private void addUrl(Url url) {
        if(!inUrlQueue(url)) {
            url.addObserver(this);
            urlQueue.add(url);
            this.setChanged();
            this.notifyObservers(url);
        } else {
            System.out.println("Dupe URL " + url.getUrl() + ". Not adding.");
        }
    }

    private void removeUrlEvent(Element event) {
        try {
            Url url = urlFactory.getUrl(event);
            if (urlQueue.contains(url)) {
                urlQueue.remove(url);
                this.setChanged();
                this.notifyObservers(url);
            }
        } catch (URISyntaxException e) {
            System.out.println(e.getMessage());
        }

    }

    public Url takeUrl() throws InterruptedException {
        return urlQueue.take();
    }

    public void update(Observable arg0, Object arg1) {
        Url url = (Url) arg0;
        if (url.getUrlState() == URL_STATE.VISITING) {
            visitingList.add(url);
        } else if (url.getUrlState() == URL_STATE.VISITED) {
            visitingList.remove(url);
            visitedList.add(url);
        } else if (url.getUrlState() == URL_STATE.ERROR) {
            visitingList.remove(url);

            String date = url.getVisitFinishTime();
            String urlString = url.getUrl();
            String clientProgram = url.getClientProgram();
            int visitTime = url.getVisitTime();
            ERROR_CODES majorErrorCode = url.getMajorErrorCode();
            long minorErrorCode = url.getMinorErrorCode();
            if(!errorUrlList.contains(url)) {
                Logger.getInstance().writeToErrorLog("\"" + date + "\",\"error:" + majorErrorCode + "-" + minorErrorCode + "\",\"" + url.getGroupID() + "\",\"" + urlString + "\",\""  + clientProgram + "\",\"" + visitTime + "\"");
                errorUrlList.add(url);
            }
        }
    }
}
