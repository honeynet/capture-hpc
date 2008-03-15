package capture;

import java.util.*;
import java.util.concurrent.LinkedBlockingDeque;
import java.io.*;
import java.text.SimpleDateFormat;

class WorkItem {
    public String function;
    public VirtualMachine vm;

    public WorkItem(String function, VirtualMachine vm) {
        this.function = function;
        this.vm = vm;
    }

    public boolean equals(Object o) {
        if (this == o) return true;
        if (!(o instanceof WorkItem)) return false;

        WorkItem workItem = (WorkItem) o;

        if (function != null ? !function.equals(workItem.function) : workItem.function != null) return false;
        if (vm != null ? !vm.equals(workItem.vm) : workItem.vm != null) return false;

        return true;
    }

    public int hashCode() {
        int result;
        result = (function != null ? function.hashCode() : 0);
        result = 31 * result + (vm != null ? vm.hashCode() : 0);
        return result;
    }
}

public class VMwareServer implements VirtualMachineServer, Observer, Runnable {
    private String address;
    private int port;
    private String username;
    private String password;
    private int uniqueId;
    private static int REVERT_TIMEOUT = 120;

    private LinkedList<VirtualMachine> virtualMachines;
    private LinkedBlockingDeque<WorkItem> queuedWorkItems;

    public VMwareServer(String address, int port, String username,
                        String password) {
        virtualMachines = new LinkedList<VirtualMachine>();
        queuedWorkItems = new LinkedBlockingDeque<WorkItem>();

        this.address = address;
        this.port = port;
        this.username = username;
        this.password = password;
        uniqueId = this.hashCode();
        Thread t = new Thread(this, "VMwareServer-" + address + ":" + port);
        t.start();

    }

    public boolean revertVirtualMachineStateAsync(VirtualMachine vm) {
        WorkItem item = new WorkItem("revert", vm);
        if (!queuedWorkItems.contains(item)) {
            queuedWorkItems.add(item);
        } else {
            System.out.println(vm.getLogHeader() + " REVERT already in progress.");
        }
        return true;
    }

    public void addVirtualMachine(VirtualMachine vm) {
        vm.addObserver(this);
        System.out.println("[" + address + ":" + port + "] VM added");
        virtualMachines.add(vm);
        vm.setState(VM_STATE.WAITING_TO_BE_REVERTED);
    }


    public void run() {
        while (true) {
            WorkItem item = queuedWorkItems.peek();

            if (item != null) {
                try {
                    if (item.function.equals("revert")) {
                        Stats.vmRevert++;
                        item.vm.setState(VM_STATE.REVERTING);

                        final String address = this.address;
                        final String username = this.username;
                        final String password = this.password;
                        final String vmPath = item.vm.getPath();
                        final String guestUsername = item.vm.getUsername();
                        final String guestPassword = item.vm.getPassword();
                        final String guestCmd = "cmd.exe";
                        final String cmdOptions = "/K " + item.vm.getCaptureClientPath() + " -s " + (String) ConfigManager.getInstance().getConfigOption("server-listen-address") + " -p " + (String) ConfigManager.getInstance().getConfigOption("server-listen-port") + " -a " + uniqueId + " -b " + item.vm.getVmUniqueId();

                        final int vmUniqueId = item.vm.getVmUniqueId();


                        String cmd = "";
                        if(System.getProperty("os.name","Windows").toLowerCase().contains("windows")) {
                            cmd = "revert.exe";
                        } else {
                            cmd = "revert";
                        }
                        final String[] revertCmd = {cmd, address, username, password, vmPath, guestUsername, guestPassword, guestCmd, cmdOptions};

                        //for(int i=0;i<revertCmd.length;i++)
                        //System.out.println(revertCmd[i]);

                        class VixThread extends Thread {
                            public int returnCode = 1;

                            public void run() {
                                Process vix = null;
                                BufferedReader stdInput = null;
                                try {

                                    vix = Runtime.getRuntime().exec(revertCmd);
                                    stdInput = new BufferedReader(new InputStreamReader(vix.getInputStream()));
                                    returnCode = vix.waitFor();
                                    String line = stdInput.readLine();
                                    while (line != null) {
                                        System.out.println(line);
                                        line = stdInput.readLine();
                                    }
                                } catch (InterruptedException e) {
                                    returnCode = 17; //VIX_TIMEOUT
                                    if (vix != null) {
                                        try {
                                            String line = stdInput.readLine();
                                            while (line != null) {
                                                System.out.println(line);
                                                line = stdInput.readLine();
                                            }
                                        } catch (Exception ef) {
                                            System.out.println(ef.getMessage());
                                            ef.printStackTrace(System.out);
                                        }
                                        vix.destroy();
                                    }
                                    System.out.println("[" + currentTime() + " " + address + ":" + port + "-" + vmUniqueId + "] Reverting VM timed out.");
                                } catch (IOException e) {
                                    returnCode = 11; //VIX_ERROR
                                    if (vix != null) {
                                        try {
                                            String line = stdInput.readLine();
                                            while (line != null) {
                                                System.out.println(line);
                                                line = stdInput.readLine();
                                            }
                                        } catch (Exception ef) {
                                            System.out.println(ef.getMessage());
                                            ef.printStackTrace(System.out);
                                        }
                                        vix.destroy();
                                    }
                                    e.printStackTrace(System.out);
                                    System.out.println("[" + currentTime() + " " + address + ":" + port + "-" + vmUniqueId + "] Unable to access external Vix library.");
                                }
                            }
                        }

                        VixThread vixThread = new VixThread();
                        vixThread.start();
                        for (int i = 0; i < REVERT_TIMEOUT; i++) {
                            if (vixThread.isAlive()) {
                                Thread.currentThread().sleep(1000);
                            } else {
                                i = REVERT_TIMEOUT; //vix completed before timeout was reached
                            }
                        }
                        if (vixThread.isAlive()) {
                            vixThread.interrupt();
                        }
                        int error = vixThread.returnCode;


                        if (error == 0) { //timeout regularly occurs, since vix could get stuck. if runprg is successful, all goes well. if not, the vmware checker will catch it.
                            item.vm.setLastContact(Calendar.getInstance().getTimeInMillis());
                            item.vm.setState(VM_STATE.RUNNING);
                        } else {
                            System.out.println("[" + currentTime() + " " + address + ":" + port + "-" + vmUniqueId + "] VMware error " + error);
                            item.vm.setState(VM_STATE.ERROR);
                        }

                    } else {
                        System.out.println("Invalid work item " + item.function);
                    }
                } catch (Exception e) {
                    System.out.println("Exception thrown in queue processor of VMware server " + e.toString());
                    e.printStackTrace(System.out);
                } finally {
                    queuedWorkItems.remove(item);
                }
            } else {
                try {
                    Thread.sleep(2500);
                } catch (InterruptedException e) {
                    e.printStackTrace(System.out);
                }
            }
        }
    }


    private String currentTime() {
        long current = System.currentTimeMillis();
        Date currentDate = new Date(current);
        SimpleDateFormat sdf = new SimpleDateFormat("MMM d, yyyy h:mm:ss a");
        String strDate = sdf.format(currentDate);
        return strDate;
    }

    public void update(Observable arg0, Object arg1) {
        VirtualMachine vm = (VirtualMachine) arg0;
        if (vm.getState() == VM_STATE.WAITING_TO_BE_REVERTED) {
            revertVirtualMachineStateAsync(vm);
        }
    }

    public LinkedList<VirtualMachine> getVirtualMachines() {
        return virtualMachines;
    }

    public boolean isLocatedAt(String address, String port) {
        int iport = Integer.parseInt(port);
        return ((this.address.equals(address)) && (this.port == iport));
    }

    public String getAddress() {
        return address;
    }

    public int getPort() {
        return port;
    }

}
