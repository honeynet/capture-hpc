package capture;

import java.io.BufferedWriter;
import java.io.IOException;
import java.io.OutputStreamWriter;
import java.net.Socket;
import java.util.Observable;
import java.util.List;
import java.util.Iterator;

enum CLIENT_STATE {
    CONNECTING,
    CONNECTED,
    UPDATING,
    UPLOADING,
    WAITING,
    VISITING,
    DISCONNECTED
}


/**
 * Manages a remote client allowing messages to be sent and received. Remote clients which
 * successfully connect to the server will have a virtual machine object associated with it.
 * To connect, a client must successfully send back to the server a connect event which
 * contains the unique id of both the virtual machine server which the virtual machine is
 * hosted on and the virtual machine. The client is not aware of what virtual machine server
 * is hosted on instead it interacts with the virtual machine instead. When the client needs
 * resetting for example after a malicious web-site has been visited the client simply sets the
 * state of the virtual machine to be needing resetting. The virtual machine server that manages
 * the virtual machine will independently detect this state change and act upon it.
 * <p>
 * A remote client has 4 states at which it can be in:
 * <ul>
 * <li>CONNECTED - Remote client has connected but no virtual machine mapped to it
 * <li>WAITING -
 * <li>VISITING
 * <li>DISCONNECTED
 * </ul>
 * <p>A remote client has 4 states. Connected is when the remote client has connected but has not
 * responded to the connect element. The state moves from being connected to waiting when the
 * remote client has responded to the connect event so that a virtual machine can be mapped to
 * the client. In this waiting state the client waits for a URL to be inserted into the queue.
 * Once a URL has been dequeued, sent, and a visiting event received the client will be put into
 * visiting state. When the remote client has finished visiting a URL the client moves to a waiting
 * state if the URL was classified as being benign or disconnect if it was malicious</p>
 * <p/>
 * <p><pre>Threads</pre>A client contains 2 threads. A ClientEventController which runs in the background and receives
 * messages from the remote client. And another which waits for URLs to be inserted into the URL
 * queue.</p>
 *
 * @author Ramon
 * @version 2.0
 * @see ClientEventController, VirtualMachine
 */
public class Client extends Observable implements Runnable {
    private Socket clientSocket;
    private UrlGroup visitingUrlGroup;
    private CLIENT_STATE clientState;
    private VirtualMachine virtualMachine;
    private ClientEventController clientEventController;
    private Thread controller;

    private BufferedWriter out;

    private Thread urlRetriever;

    public Client(Socket c) {
        clientState = CLIENT_STATE.CONNECTING;
        clientSocket = c;
        try {
            out = new BufferedWriter(new OutputStreamWriter(clientSocket.getOutputStream()));
            clientEventController = new ClientEventController(this);
            controller = new Thread(clientEventController, "ClientEC");
            controller.start();
            urlRetriever = new Thread(this, "ClientUrl");
            urlRetriever.start();
        } catch (IOException e) {
            e.printStackTrace(System.out);
            if(visitingUrlGroup!=null) {
                visitingUrlGroup.setMajorErrorCode(ERROR_CODES.CAPTURE_CLIENT_CONNECTION_RESET.errorCode);
                visitingUrlGroup.setUrlGroupState(URL_GROUP_STATE.ERROR);
            }

            clientState = CLIENT_STATE.DISCONNECTED;
        }

    }

    public String errorCodeToString(long error) {
        for (ERROR_CODES e : ERROR_CODES.values()) {
            if (error == e.errorCode) {
                return e.toString();
            }
        }
        return "";
    }

    public boolean send(String message) {
        if (this.getClientState() != CLIENT_STATE.DISCONNECTED) {
            try {
                if (!message.endsWith("\0")) {
                    message += '\0';
                }
                out.write(message);
                out.flush();
                return true;
            } catch (IOException e) {
                this.setClientState(CLIENT_STATE.DISCONNECTED);
            }
        }
        return false;
    }

    public boolean sendXMLElement(Element e) {
        String msg = "<";
        msg += e.name;
        for (String key : e.attributes.keySet()) {
            msg += " ";
            msg += key;
            msg += "=\"";
            msg += e.attributes.get(key);
            msg += "\"";
        }
        msg += "/>";
        return this.send(msg);
    }

    public void reset() {
        if (this.getClientState() != CLIENT_STATE.VISITING) {
            this.setClientState(CLIENT_STATE.DISCONNECTED);
        }
    }

    public void disconnect() {
        if (virtualMachine != null) {
            virtualMachine.setClient(null);
            virtualMachine.setState(VM_STATE.WAITING_TO_BE_REVERTED);
        }
        if (visitingUrlGroup != null) {
            visitingUrlGroup.setUrlGroupState(URL_GROUP_STATE.ERROR);
        }
        try {
            out.close();
            clientSocket.close();
        } catch (IOException e) {
            e.printStackTrace(System.out);
        }
    }

    public void parseEvent(Element element) {
        //only written to log if size of group is one
        if (this.getClientState() == CLIENT_STATE.VISITING) {
            String type = element.attributes.get("type");
            String time = element.attributes.get("time");
            String process = element.attributes.get("process");
            String action = element.attributes.get("action");
            String object = element.attributes.get("object");
            visitingUrlGroup.setMalicious(true);
            visitingUrlGroup.writeEventToLog("\"" + type + "\",\"" + time +
                    "\",\"" + process + "\",\"" + action + "\",\"" +
                    object + "\"");
        }
    }

    public void parseVisitEvent(Element element) {
        String type = element.attributes.get("type");
        if (type.equals("start")) {
            System.out.println(this.getVirtualMachine().getLogHeader() + " Visiting group " + visitingUrlGroup.getIdentifier());
            visitingUrlGroup.setVisitStartTime(element.attributes.get("time"));

            //iterate through url elements and set startvisitTime & visiting state
            List<Element> items = element.childElements;
            for (Iterator<Element> iterator = items.iterator(); iterator.hasNext();) {
                Element item = iterator.next();
                String uri = item.attributes.get("url");
                Url url = visitingUrlGroup.getUrl(uri);
                url.setVisitStartTime(item.attributes.get("time"));
            }
            visitingUrlGroup.setUrlGroupState(URL_GROUP_STATE.VISITING); //will set underlying url state


            this.setClientState(CLIENT_STATE.VISITING);
        } else if (type.equals("finish")) {
            System.out.print(this.getVirtualMachine().getLogHeader() + " Visited ");
            visitingUrlGroup.setVisitFinishTime(element.attributes.get("time"));

            //iterate through url elements and set startvisitTime & visiting state
            List<Element> items = element.childElements;
            int errorCount = 0;
            for (Iterator<Element> iterator = items.iterator(); iterator.hasNext();) {
                Element item = iterator.next();
                String uri = item.attributes.get("url");
                Url url = visitingUrlGroup.getUrl(uri);
                url.setVisitFinishTime(item.attributes.get("time"));
                String urlMajorErrorCode = item.attributes.get("major-error-code");
                String urlMinorErrorCode = item.attributes.get("minor-error-code");
                if (!(urlMajorErrorCode.equals("0") || urlMajorErrorCode.equals(""))) {
                    errorCount++;
                    url.setMajorErrorCode(Long.parseLong(urlMajorErrorCode));
                    url.setMinorErrorCode(Long.parseLong(urlMinorErrorCode));
                }
            }
            if (errorCount == items.size()) { //all URLs contained errors
                long major = ERROR_CODES.PROCESS_ERROR.errorCode;
                System.out.println(this.getVirtualMachine().getLogHeader() + " Visit error - Major: " + errorCodeToString(major) + " ");
                visitingUrlGroup.setMajorErrorCode(major);
                visitingUrlGroup.setUrlGroupState(URL_GROUP_STATE.ERROR);   //will set underlying url state
                this.setClientState(CLIENT_STATE.DISCONNECTED);
                visitingUrlGroup = null;
            } else {

                String malicious = element.attributes.get("malicious");
                if (malicious.equals("1")) {
                    System.out.println("MALICIOUS " + visitingUrlGroup.getIdentifier());
                    visitingUrlGroup.setMalicious(true);
                    visitingUrlGroup.setUrlGroupState(URL_GROUP_STATE.VISITED);  //will set underlying url state
                    visitingUrlGroup = null;
                    this.setClientState(CLIENT_STATE.DISCONNECTED);
                } else {
                    System.out.println("BENIGN " + visitingUrlGroup.getIdentifier());
                    visitingUrlGroup.setMalicious(false);
                    visitingUrlGroup.setUrlGroupState(URL_GROUP_STATE.VISITED);  //will set underlying url state
                    visitingUrlGroup = null;
                    this.setClientState(CLIENT_STATE.WAITING);
                }
            }
        } else if (type.equals("error")) {
            String major = element.attributes.get("error-code");
            List<Element> items = element.childElements;
            for (Iterator<Element> iterator = items.iterator(); iterator.hasNext();) {
                Element item = iterator.next();
                String uri = item.attributes.get("url");
                Url url = visitingUrlGroup.getUrl(uri);
                url.setVisitFinishTime(item.attributes.get("time"));
                String urlMajorErrorCode = item.attributes.get("major-error-code");
                String urlMinorErrorCode = item.attributes.get("minor-error-code");
                if (!urlMajorErrorCode.equals("")) {
                    url.setMajorErrorCode(Long.parseLong(urlMajorErrorCode));
                    url.setMinorErrorCode(Long.parseLong(urlMinorErrorCode));
                }
            }

            if (!(major.equals("268435731") || major.equals("268436224"))) {
                System.out.println(this.getVirtualMachine().getLogHeader() + " Visit error - Major: " + major + " ");
                visitingUrlGroup.setMajorErrorCode(Long.parseLong(major));
                visitingUrlGroup.setUrlGroupState(URL_GROUP_STATE.ERROR);   //will set underlying url state
                this.setClientState(CLIENT_STATE.DISCONNECTED);
                visitingUrlGroup = null;
            } else {
                System.out.print(this.getVirtualMachine().getLogHeader() + " Visited ");
                visitingUrlGroup.setUrlGroupState(URL_GROUP_STATE.VISITED);   //will set underlying url state

                if (visitingUrlGroup.isMalicious()) {
                    System.out.println("MALICIOUS " + visitingUrlGroup.getIdentifier());

                    this.setClientState(CLIENT_STATE.DISCONNECTED);
                } else {
                    System.out.println("BENIGN " + visitingUrlGroup.getIdentifier());
                    this.setClientState(CLIENT_STATE.WAITING);
                }
                visitingUrlGroup = null;
            }
        }
    }

    public void parseConnectEvent(Element element) {
        String vmServerId = element.attributes.get("vm-server-id");
        String vmId = element.attributes.get("vm-id");
        if ((vmServerId != null && vmId != null) &&
                (vmServerId != "" && vmId != "")) {
            VirtualMachineServer vmServer = VirtualMachineServerController.getInstance().getVirtualMachineServer(vmServerId);

            int id = Integer.parseInt(vmId);
            for (VirtualMachine vm : vmServer.getVirtualMachines()) {
                if (vm.getVmUniqueId() == id) {

                    virtualMachine = vm;
                    virtualMachine.setClient(this);
                    this.setClientState(CLIENT_STATE.CONNECTED);
                    this.setClientState(CLIENT_STATE.WAITING);
                    break;
                }
            }
        }
    }

    public CLIENT_STATE getClientState() {
        return clientState;
    }

    public synchronized void setClientState(CLIENT_STATE newState) {
        if (clientState == newState)
            return;
        clientState = newState;
        if (this.getVirtualMachine() != null) {
            System.out.println(this.getVirtualMachine().getLogHeader() + " ClientSetState: " + newState.toString());
        } else {
            System.out.println("[unknown server] ClientSetState: " + newState.toString());
        }
        if (clientState == CLIENT_STATE.WAITING) {
            virtualMachine.setState(VM_STATE.RUNNING);
            synchronized (urlRetriever) {
                urlRetriever.notify();
            }
        } else if (clientState == CLIENT_STATE.DISCONNECTED) {
            this.disconnect();
            synchronized (urlRetriever) {
                urlRetriever.notify(); //in order to let urlRetriever thread stop
            }
        }
        this.setChanged();
        this.notifyObservers();
    }

    public Socket getClientSocket() {
        return clientSocket;
    }

    public UrlGroup getVisitingUrlGroup() {
        return visitingUrlGroup;
    }

    public void setVisitingUrlGroup(UrlGroup visitingUrlGroup) {
        this.visitingUrlGroup = visitingUrlGroup;
        boolean success = this.send(this.visitingUrlGroup.toVisitEvent());
        if (!success) {
            System.out.println("Sent URL to disconnected client");
            this.visitingUrlGroup.setMajorErrorCode(0x10000111);
            this.visitingUrlGroup.setUrlGroupState(URL_GROUP_STATE.ERROR);
        }
    }

    public VirtualMachine getVirtualMachine() {
        return virtualMachine;
    }

    public void run() {
        synchronized (urlRetriever) {
            while (clientState != CLIENT_STATE.DISCONNECTED) {
                try {
                    urlRetriever.wait();
                    if (clientState == CLIENT_STATE.WAITING) {
                        this.setVisitingUrlGroup(UrlGroupsController.getInstance().takeUrlGroup());
                    }
                } catch (InterruptedException e) {
                    e.printStackTrace(System.out);
                }
            }
        }
    }
}
