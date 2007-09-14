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
        //System.out.println("ClientsPinger running");

		/* TODO Note there is a bug here that needs to be fixed. This is poorly designed.
		 * If the client.send function fails, the state of the client changes to be
		 * disconnected which removes the client for the clients list the for loop is
		 * iterating. We cheat this bug not really caring about it because on the next
		 * run of this timer everything is ok.
		 */
		for(int i = 0; i < clients.size(); i++)
		{
			Client client = clients.get(i);
			if(client.getClientState() != CLIENT_STATE.CONNECTING ||
					client.getClientState() != CLIENT_STATE.DISCONNECTED)
			{
				client.send("<ping/>\n");
			}
			
		}
		//System.out.println("ClientsPinger done");
	}

}
