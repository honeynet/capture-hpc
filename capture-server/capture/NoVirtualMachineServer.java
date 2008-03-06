package capture;

import java.util.LinkedList;
import java.util.Observable;
import java.util.Observer;
import java.util.Calendar;
import java.util.concurrent.LinkedBlockingDeque;
import static java.lang.Thread.sleep;

public class NoVirtualMachineServer implements VirtualMachineServer, Observer {
    private LinkedList<VirtualMachine> virtualMachines;

    private String address;
    private int port;
    private String username;
    private String password;
    private int uniqueId;

    public NoVirtualMachineServer(String address, int port, String username,
                                  String password) {
        System.out.println("Create No VM");

        virtualMachines = new LinkedList<VirtualMachine>();

        this.address = address;
        this.port = port;
        this.username = username;
        this.password = password;
        uniqueId = this.hashCode();
    }

    public boolean revertVirtualMachineStateAsync(VirtualMachine vm) {
        WorkItem item = new WorkItem("revert", vm);
        item.vm.setState(VM_STATE.REVERTING);

        String serverListenAddress = (String) ConfigManager.getInstance().getConfigOption("server-listen-address");
        int serverListenPort = Integer.parseInt(ConfigManager.getInstance().getConfigOption("server-listen-port"));
        System.out.println("Start client with the following parameters.");
        System.out.println("-s " + serverListenAddress);
        System.out.println("-p " + serverListenPort);
        System.out.println("-a " + uniqueId);
        System.out.println("-b " + item.vm.getVmUniqueId());


        item.vm.setLastContact(Calendar.getInstance().getTimeInMillis());
        item.vm.setState(VM_STATE.RUNNING);
        return true;
    }

    public void addVirtualMachine(VirtualMachine vm) {
        vm.addObserver(this);
        System.out.println("[" + address + ":" + port + "] VM added");
        virtualMachines.add(vm);
        vm.setState(VM_STATE.WAITING_TO_BE_REVERTED);
    }

    public LinkedList<VirtualMachine> getVirtualMachines() {
        return virtualMachines;
    }

    public String getAddress() {
        return address;
    }

    public int getPort() {
        return port;
    }

    public boolean isLocatedAt(String address, String port) {
        int iport = Integer.parseInt(port);
        return ((this.address.equals(address)) && (this.port == iport));
    }

    public void update(Observable arg0, Object arg1) {
        VirtualMachine vm = (VirtualMachine) arg0;
        if (vm.getState() == VM_STATE.WAITING_TO_BE_REVERTED) {
            revertVirtualMachineStateAsync(vm);
        }
    }

}