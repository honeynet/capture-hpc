package capture;

import java.io.IOException;
import java.net.InetAddress;
import java.net.ServerSocket;
import java.net.Socket;
import java.util.HashMap;
import java.util.LinkedList;
import java.util.Observable;
import java.util.Observer;
import java.util.Timer;
import java.util.concurrent.locks.ReentrantReadWriteLock;


public class ClientsController extends Observable implements Observer, Runnable, EventObserver {
	private LinkedList<Client> clients;
	private ReentrantReadWriteLock clientsLock;
	private ClientsPinger clientsPinger;
	private HashMap<String, ExclusionList> exclusionLists;

	private ClientsController()
	{
		clients = new LinkedList<Client>();
		clientsLock = new ReentrantReadWriteLock();
		clientsPinger = new ClientsPinger(clients, clientsLock);
		exclusionLists = new HashMap<String, ExclusionList>();
		EventsController.getInstance().addEventObserver("exclusion-list", this);
		Timer clientsPingerTask = new Timer(true);
		clientsPingerTask.schedule(clientsPinger, 0, 10000);
		Thread t = new Thread(this, "ClientController");
		t.start();
	}
	
	private static class ClientsControllerHolder
	{ 
		private final static ClientsController instance = new ClientsController();
	}
	 
	public static ClientsController getInstance()
	{
		return ClientsControllerHolder.instance;
	}

	public void run() {
        try {
        	int serverListenPort = Integer.parseInt(ConfigManager.getInstance().getConfigOption("server-listen-port"));
            String serverListenAddress = (String) ConfigManager.getInstance().getConfigOption("server-listen-address");
        	ServerSocket listener = new ServerSocket(serverListenPort, 0, InetAddress.getByName(serverListenAddress));

            System.out.println("CaptureServer: Listening for connections");
            Socket clientSocket;
            while (true) {
            	clientSocket = listener.accept();
            	this.addClient(clientSocket);
            }
        } catch (IOException ioe) {
        	System.out.println("CaptureServer: exception - " + ioe);
            ioe.printStackTrace(System.out);
        }
	}
	
	private void addClient(Socket clientSocket)
	{
        Client client = new Client(clientSocket);
        client.addObserver(this);
        clientsLock.writeLock().lock();
        clients.add(client);
        clientsLock.writeLock().unlock();
        //this.sendExclusionLists(client);
        client.send("<connect server=\"2.1\" />");
    	this.setChanged();
    	this.notifyObservers(client);
	}
	
	private void sendExclusionLists(Client c)
	{
		for(ExclusionList ex : exclusionLists.values())
		{
			for(Element e : ex.getExclusionListElements()) 
			{
				System.out.println("Sending exclusion list element");
				c.sendXMLElement(e);
			}
		}
	}
	
	private void removeClient(Client client)
	{
		clientsLock.writeLock().lock();
		clients.remove(client);
		clientsLock.writeLock().unlock();
	}

	public void update(Observable arg0, Object arg1) {
		Client client = (Client)arg0;
		if(client.getClientState() == CLIENT_STATE.DISCONNECTED)
		{
			this.removeClient(client);
		} else if(client.getClientState() == CLIENT_STATE.CONNECTED) {
            client.send("<option name=\"capture-network-packets-malicious\" value=\"" +
                    ConfigManager.getInstance().getConfigOption("capture-network-packets-malicious") + "\"/>");
			client.send("<option name=\"capture-network-packets-benign\" value=\"" + 
					ConfigManager.getInstance().getConfigOption("capture-network-packets-benign") + "\"/>");
			client.send("<option name=\"collect-modified-files\" value=\"" + 
					ConfigManager.getInstance().getConfigOption("collect-modified-files") + "\"/>");
	        if(ConfigManager.getInstance().getConfigOption("send-exclusion-lists").equals("true"))
	        {
	        	this.sendExclusionLists(client);
	        }
		}
	}

	public void update(Element event) {
		if(event.name.equals("exclusion-list")) {
			if(event.attributes.containsKey("add-list")) {
				if(event.attributes.containsKey("monitor")) {
					String monitor = event.attributes.get("monitor");					
					if(event.attributes.containsKey("file")) {					
						String file = event.attributes.get("file");	
						ExclusionList ex = new ExclusionList(monitor, file);
						if(ex.parseExclusionList()) {
							exclusionLists.put(monitor, ex);
							System.out.println("ExclusionList added: for " + monitor + " monitor");
						}
					}
				}
			} else {
				System.out.println("ClientsController: exclusion-list function not supported");
			}
		}
		
	}
}
