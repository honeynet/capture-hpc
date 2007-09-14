package capture;

import java.util.Calendar;
import java.util.LinkedList;
import java.util.TimerTask;


public class VirtualMachinesStateChecker extends TimerTask {

	private LinkedList<VirtualMachineServer> virtualMachineServers;

	public VirtualMachinesStateChecker(LinkedList<VirtualMachineServer> virtualMachineServers)
	{
		this.virtualMachineServers = virtualMachineServers;
	}

	public void run()
	{
		try 
		{
			for(VirtualMachineServer vmServer : virtualMachineServers)
			{
				for(VirtualMachine vm : vmServer.getVirtualMachines())
				{
					long currentTime = Calendar.getInstance().getTimeInMillis();
					if(vm.getState() == VM_STATE.RUNNING)
					{		        		
		        		long diff = currentTime - vm.getLastContact();
		        		if(diff >= 30000)
		        		{
		        			System.out.println("[" + vmServer.getAddress() + ":" + vmServer.getPort() + "-" + 
		        					vm.getVmUniqueId() + "] Client inactivity, reverting VM");
		        			vm.setState(VM_STATE.WAITING_TO_BE_REVERTED);
		        		}
		        	} else {
		        		if(vm.getState() == VM_STATE.REVERTING)
		        		{
		        			vm.setLastContact(Calendar.getInstance().getTimeInMillis());
		        		}
		        		long diff = currentTime - vm.getTimeOfLastStateChange();
			        	if(diff >= 300000)
			        	{
			        		System.out.println("[" + vmServer.getAddress() + ":" + vmServer.getPort() + "-" + 
			    					vm.getVmUniqueId() + "] VM stalled, reverting VM");
			        		vm.setState(VM_STATE.WAITING_TO_BE_REVERTED);
			        	}
		        	}
				}
			}
		} catch (Exception e) {
			e.printStackTrace();
		}
	}
}
