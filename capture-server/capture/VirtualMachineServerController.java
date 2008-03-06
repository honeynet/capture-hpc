package capture;

import java.util.LinkedList;
import java.util.Observable;
import java.util.Timer;


public class VirtualMachineServerController extends Observable implements EventObserver {
	private LinkedList<VirtualMachineServer> virtualMachineServers;
	private VirtualMachineServerFactory vmServerFactory;
	private VirtualMachinesStateChecker virtualMachinesStateChecker;
	
	private VirtualMachineServerController()
	{
		virtualMachineServers = new LinkedList<VirtualMachineServer>();
		vmServerFactory = new VirtualMachineServerFactory();
		virtualMachinesStateChecker = new VirtualMachinesStateChecker(virtualMachineServers);
		Timer vmStateChecker = new Timer();
		vmStateChecker.scheduleAtFixedRate(virtualMachinesStateChecker,0,1000);
		EventsController.getInstance().addEventObserver("virtual-machine-server", this);
		EventsController.getInstance().addEventObserver("virtual-machine", this);
	}
	
	private static class VirtualMachineServerControllerHolder
	{ 
		private final static VirtualMachineServerController instance = new VirtualMachineServerController();
	}
	 
	public static VirtualMachineServerController getInstance()
	{
		return VirtualMachineServerControllerHolder.instance;
	}

	public void update(Element event) {
		//System.out.println("VMController: Got event: " + event.name);
		if(event.name.equals("virtual-machine-server"))
		{
			if(event.attributes.containsKey("add"))
			{
				addVirtualMachineServer(event);
			} else if(event.attributes.containsKey("remove")) {
				removeVirtualMachineServer(event);
			}
		} if(event.name.equals("virtual-machine")) {
			if(event.attributes.containsKey("add"))
			{
				addVirtualMachine(event);
			} else if(event.attributes.containsKey("remove")) {
				removeVirtualMachine(event);
			}
		}
	}
	
	public VirtualMachineServer getVirtualMachineServer(String vmServerId)
	{
		if(!vmServerId.equals(""))
		{
			int id = Integer.parseInt(vmServerId);
			for(VirtualMachineServer vmServer : virtualMachineServers)
			{
				if(vmServer.hashCode() == id)
				{
					return vmServer;
				}
			}
		}
		return null;
	}
	
	private void addVirtualMachineServer(Element event)
	{
		String type = event.attributes.get("type");
		
		VirtualMachineServer vmServer = vmServerFactory.getVirtualMachineServer(type, event.attributes);
		virtualMachineServers.add(vmServer);
		this.setChanged();
		this.notifyObservers(vmServer);
		for(Element childEvent: event.childElements)
		{
			if(childEvent.name.equals("virtual-machine"))
			{
				childEvent.attributes.put("server-address", event.attributes.get("address"));
				childEvent.attributes.put("server-port", event.attributes.get("port"));
				addVirtualMachine(childEvent);
				childEvent.attributes.remove("server-address");
				childEvent.attributes.remove("server-port");
			}
		}

	}
	
	private void removeVirtualMachineServer(Element event)
	{
		String address = event.attributes.get("address");
		String port = event.attributes.get("type");
		for(VirtualMachineServer vmServer : virtualMachineServers)
		{
			if(vmServer.isLocatedAt(address, port))
			{
				virtualMachineServers.remove(vmServer);
			}
		}
	}
	
	private void addVirtualMachine(Element event)
	{		
		String address = event.attributes.get("server-address");
		String port = event.attributes.get("server-port");
		for(VirtualMachineServer vmServer : virtualMachineServers)
		{
			if(vmServer.isLocatedAt(address, port))
			{
				String vmpath = event.attributes.get("vm-path");
				String vmusername = event.attributes.get("username");
				String vmpassword = event.attributes.get("password");
				String vmcapturepath = event.attributes.get("client-path");
				VirtualMachine vm = new VirtualMachine(vmServer, vmpath, vmusername, vmpassword, vmcapturepath);
				vmServer.addVirtualMachine(vm);
			}
		}
	}
	
	private void removeVirtualMachine(Element event)
	{
		String address = event.attributes.get("address");
		String port = event.attributes.get("type");
		for(VirtualMachineServer vmServer : virtualMachineServers)
		{
			if(vmServer.isLocatedAt(address, port))
			{
				// TODO not implemented yet
			}
		}
	}

}
