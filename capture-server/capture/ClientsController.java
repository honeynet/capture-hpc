package capture;

import java.io.IOException;
import java.net.InetAddress;
import java.net.ServerSocket;
import java.net.Socket;
import java.util.*;


public class ClientsController extends Observable implements Runnable, EventObserver {
	private ClientsPinger clientsPinger;
	private HashMap<String, ExclusionList> exclusionLists;

	private ClientsController()
	{
		clientsPinger = new ClientsPinger();
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
            	this.handleConnection(clientSocket);
            }
        } catch (IOException ioe) {
        	System.out.println("CaptureServer: exception - " + ioe);
            ioe.printStackTrace(System.out);
        }
	}
	
	private void handleConnection(Socket clientSocket)
	{
        //pass to clientEventController, which will handle all aspects of this current connection
        ClientEventController clientEventController = new ClientEventController(clientSocket, exclusionLists, clientsPinger);
        clientEventController.contactClient();

        //Client client = clientEventController.getClient();


        this.setChanged();
    	//this.notifyObservers(client);
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
