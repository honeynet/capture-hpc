package capture;

import java.util.*;
import java.util.concurrent.locks.ReentrantReadWriteLock;

public class ClientsPinger extends TimerTask implements Observer {
    private Set<Client> clients;

    public ClientsPinger() {
        this.clients = new TreeSet<Client>();
    }

    public void addClient(Client c) {
        clients.add(c);
    }

    public void removeClient(Client c) {
        clients.remove(c);
    }

    public void update(Observable arg0, Object arg1) {
		Client client = (Client)arg0;
		if(client.getClientState() == CLIENT_STATE.DISCONNECTED)
		{
			removeClient(client);
        } else if(client.getClientState() == CLIENT_STATE.CONNECTED) {
            addClient(client);
        }
    }

    public void run() {
        for (Iterator<Client> clientIterator = clients.iterator(); clientIterator.hasNext();) {
            Client client = clientIterator.next();
            if (client.getClientState() != CLIENT_STATE.CONNECTING ||
                    client.getClientState() != CLIENT_STATE.DISCONNECTED) {
                System.out.println(client.getVirtualMachine().getLogHeader() + " <ping/>");
                client.send("<ping/>\n");
            }

        }
    }

}
