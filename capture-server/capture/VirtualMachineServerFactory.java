package capture;

import java.util.HashMap;

public class VirtualMachineServerFactory {
	
	public VirtualMachineServer getVirtualMachineServer(String type, 
			HashMap<String, String> attributes)
	{
		if(type.equals("vmware-server"))
		{
			String address = attributes.get("address");
			int port = Integer.parseInt(attributes.get("port"));
			String username = attributes.get("username");
			String password = attributes.get("password");
            
			VMwareServer vmServer = new VMwareServer(address, port, username, password);
			return vmServer;
		}
		return null;
	}	

}
