package capture;

import java.util.LinkedList;
import java.util.TimerTask;
import java.util.concurrent.locks.ReentrantReadWriteLock;

public class ClientsPinger extends TimerTask {
	private LinkedList<Client> clients;

	public ClientsPinger(LinkedList<Client> clients, ReentrantReadWriteLock clientsLock)
	{
		this.clients = clients;
	}

	public void run() {
		for(int i = 0; i < clients.size(); i++)
		{
			Client client = clients.get(i);
			if(client.getClientState() != CLIENT_STATE.CONNECTING ||
					client.getClientState() != CLIENT_STATE.DISCONNECTED)
			{
                System.out.println(client.getVirtualMachine().getLogHeader() + " <ping/>");
                client.send("<ping/>\n");
			}
			
		}
		//System.out.println("ClientsPinger done");
	}

}
