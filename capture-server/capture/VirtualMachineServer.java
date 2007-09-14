package capture;

import java.util.LinkedList;


public interface VirtualMachineServer {
	
	public boolean revertVirtualMachineStateAsync(VirtualMachine vm);
	
	public void addVirtualMachine(VirtualMachine vm);
	public LinkedList<VirtualMachine> getVirtualMachines();
	
	public String getAddress();
	public int getPort();
	
	public boolean isLocatedAt(String address, String port);
}
