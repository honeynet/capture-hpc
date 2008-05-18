package capture;

import java.io.*;
import java.net.SocketException;
import java.net.Socket;
import java.util.Calendar;
import java.util.HashMap;

import org.xml.sax.Attributes;
import org.xml.sax.InputSource;
import org.xml.sax.SAXException;
import org.xml.sax.XMLReader;
import org.xml.sax.helpers.DefaultHandler;
import org.xml.sax.helpers.XMLReaderFactory;


public class ClientEventController extends DefaultHandler implements Runnable {
    private HashMap<String, ClientFileReceiver> clientFileReceivers;
    private Element currentElement;
    private Socket clientSocket;
    private Client client;
    private HashMap<String, ExclusionList> exclusionLists;
    private ClientsPinger clientsPinger;

    public ClientEventController(Socket clientSocket, HashMap<String, ExclusionList> exclusionLists, ClientsPinger clientsPinger) {
        this.clientSocket = clientSocket;
        this.exclusionLists = exclusionLists;
        currentElement = null;
        clientFileReceivers = new HashMap<String, ClientFileReceiver>();
        this.clientsPinger = clientsPinger;

        Thread receiver = new Thread(this, "ClientEC");
        receiver.start();
    }


    public void contactClient() {
        String message = "<connect server=\"2.5\" />";
        if (this.clientSocket.isConnected()) {
            try {
                BufferedWriter out = new BufferedWriter(new OutputStreamWriter(clientSocket.getOutputStream()));
                if (!message.endsWith("\0")) {
                    message += '\0';
                }
                out.write(message);
                out.flush();
            } catch (IOException e) {
                e.printStackTrace(System.out);
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

                    client = new Client(exclusionLists);
                    client.setSocket(clientSocket);
                    client.setVirtualMachine(vm);
                    vm.setClient(client);
                    client.addObserver(clientsPinger);

                    client.setClientState(CLIENT_STATE.CONNECTED);
                    client.setClientState(CLIENT_STATE.WAITING);

                    break;
                }
            }
        }
    }

    public void parseReconnectEvent(Element element) {
        String vmServerId = element.attributes.get("vm-server-id");
        String vmId = element.attributes.get("vm-id");
        if ((vmServerId != null && vmId != null) &&
                (vmServerId != "" && vmId != "")) {
            VirtualMachineServer vmServer = VirtualMachineServerController.getInstance().getVirtualMachineServer(vmServerId);

            int id = Integer.parseInt(vmId);
            for (VirtualMachine vm : vmServer.getVirtualMachines()) {
                if (vm.getVmUniqueId() == id) {
                    try {
                        client = vm.getClient();
                        if (client != null) {
                            client.getClientSocket().close();
                        }
                    } catch (IOException e) {
                        e.printStackTrace(System.out);
                    }
                    if (client != null) {
                        client.setSocket(clientSocket);
                    }
                    break;
                }
            }
        }
    }


    public void run() {
        String buffer = new String();
        try {
            BufferedReader in = new BufferedReader(new InputStreamReader(clientSocket.getInputStream(), "UTF-8"));

            while (!clientSocket.isClosed() && ((buffer = in.readLine()) != null)) {

                if (client != null && client.getVirtualMachine() != null) {
                    client.getVirtualMachine().setLastContact(Calendar.getInstance().getTimeInMillis());
                }
                //buffer = buffer.substring(0, buffer.length());
                //System.out.println(buffer);
                //buffer = buffer.trim();
                System.out.println("Received msg from client: "+buffer);

                if (buffer.length() > 0) {
                    XMLReader xr = XMLReaderFactory.createXMLReader();
                    xr.setContentHandler(this);
                    xr.setErrorHandler(this);
                    xr.parse(new InputSource(new StringReader(buffer)));
                }

            }
        } catch (SocketException e) {
            String where = "[unknown server]";
            if (client != null && client.getVirtualMachine() != null) {
                where = client.getVirtualMachine().getLogHeader();
                if (client.getVisitingUrlGroup() != null) {
                    client.getVisitingUrlGroup().setMajorErrorCode(ERROR_CODES.SOCKET_ERROR.errorCode);
                }
            }
            System.out.println(where + " " + e.getMessage());
        } catch (IOException e) {
            String where = "[unknown server]";
            if (client != null && client.getVirtualMachine() != null) {
                where = client.getVirtualMachine().getLogHeader();
                if (client.getVisitingUrlGroup() != null) {
                    client.getVisitingUrlGroup().setMajorErrorCode(ERROR_CODES.SOCKET_ERROR.errorCode);
                }
            }
            System.out.println(where + " " + e.getMessage());
            System.out.println(where + "\nIOException: Buffer=" +
                    buffer + "\n\n" + e.toString());
            e.printStackTrace(System.out);
        } catch (SAXException e) {
            String where = "[unknown server]";
            if (client != null && client.getVirtualMachine() != null)
                where = client.getVirtualMachine().getLogHeader();
            System.out.println(where + " " + e.getMessage());
            System.out.println(where + "\nSAXException: Buffer=" +
                    buffer + "\n\n" + e.toString());
            e.printStackTrace(System.out);
        } catch (Exception e) {
            String where = "[unknown server]";
            if (client != null && client.getVirtualMachine() != null) {
                where = client.getVirtualMachine().getLogHeader();
                if (client.getVisitingUrlGroup() != null) {
                    client.getVisitingUrlGroup().setMajorErrorCode(ERROR_CODES.SOCKET_ERROR.errorCode);
                }
            }
            System.out.println(where + " " + e.getMessage());
            System.out.println(where + "\nException: Buffer=" +
                    buffer + "\n\n" + e.toString());
            e.printStackTrace(System.out);
        }

    }


    private Element constructElement(Element parent, String name, Attributes atts) {
        Element e = new Element();
        e.name = name;
        for (int i = 0; i < atts.getLength(); i++) {
            //System.out.println(atts.getLocalName(i) + " -> " + atts.getValue(i));
            e.attributes.put(atts.getLocalName(i), atts.getValue(i));
        }
        if (parent != null) {
            parent.childElements.add(e);
            e.parent = parent;
        }

        return e;
    }

    public void startElement(String uri, String name,
                             String qName, Attributes atts) {
        currentElement = this.constructElement(currentElement, name, atts);
    }

    public void endElement(String uri, String name, String qName) {
        //long startTime = System.nanoTime();
        if (currentElement.parent == null) {
            if (name.equals("system-event")) {
                if (client != null) client.parseEvent(currentElement);
            } else if (name.equals("connect")) {
                parseConnectEvent(currentElement);
            } else if (name.equals("reconnect")) {
                parseReconnectEvent(currentElement);
            } else if (name.equals("pong")) {
                String where = "[unknown server]";
                if (client != null && client.getVirtualMachine() != null)
                    where = client.getVirtualMachine().getLogHeader();
                System.out.println(where + " Got pong");
            } else if (name.equals("visit-event")) {
                if (client != null) client.parseVisitEvent(currentElement);
            } else if (name.equals("client")) {

            } else if (name.equals("file")) {
                String where = "[unknown server]";
                if (client != null && client.getVirtualMachine() != null)
                    where = client.getVirtualMachine().getLogHeader();
                System.out.println(where + " Downloading file");
                ClientFileReceiver file = new ClientFileReceiver(client, currentElement);
                String fileName = currentElement.attributes.get("name");
                if (!clientFileReceivers.containsKey(fileName)) {
                    clientFileReceivers.put(fileName, file);
                } else {
                    System.out.println("ClientFileReceiver: ERROR already downloading file - " + fileName);
                }
            } else if (name.equals("part")) {
                String fileName = currentElement.attributes.get("name");
                if (clientFileReceivers.containsKey(fileName)) {
                    ClientFileReceiver file = clientFileReceivers.get(fileName);
                    file.receiveFilePart(currentElement);
                } else {
                    System.out.println("ClientFileReceiver: ERROR receiver not found for file - " + fileName);
                }
            } else if (name.equals("file-finished")) {
                String where = "[unknown server]";
                if (client != null && client.getVirtualMachine() != null)
                    where = client.getVirtualMachine().getLogHeader();
                System.out.println(where + " Finished downloading file");
                String fileName = currentElement.attributes.get("name");
                if (clientFileReceivers.containsKey(fileName)) {
                    ClientFileReceiver file = clientFileReceivers.get(fileName);
                    file.receiveFileEnd(currentElement);
                    clientFileReceivers.remove(fileName);
                } else {
                    System.out.println("ClientFileReceiver: ERROR receiver not found for file - " + fileName);
                }
            }
            currentElement = null;
        } else {
            currentElement = currentElement.parent;
        }
    }

    public void characters(char ch[], int start, int length) {
        currentElement.data = new String(ch, start, length);
        currentElement.data.trim();
    }


}
