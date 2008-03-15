package capture;

import java.util.Calendar;
import java.util.Observable;
import java.util.Observer;
import java.util.Date;
import java.text.SimpleDateFormat;


/**
 * Virtual Machine object.
 *
 * @author Ramon
 * @version 2.0
 */
public class VirtualMachine extends Observable implements Observer {
    private String path;
    private String username;
    private String password;
    private String captureClientPath;
    private int vmUniqueId;
    private long lastContact;
    private VM_STATE vmState;
    private VirtualMachineServer server;
    private long vmStateTimeChange;
    private Client client;


    public VirtualMachine(VirtualMachineServer s, String p, String user, String pass, String cpath) {
        server = s;
        path = p;
        username = user;
        password = pass;
        captureClientPath = cpath;
        lastContact = Calendar.getInstance().getTimeInMillis();
        vmUniqueId = this.hashCode();
        vmStateTimeChange = Calendar.getInstance().getTimeInMillis();
        vmState = VM_STATE.STOPPED;
    }


    public void update(Observable arg0, Object arg1) {
        this.setChanged();
        this.notifyObservers(arg1);
    }

    /**
     * Returns the time since a virtual machine received a message from a client
     *
     * @return long
     */
    public long getLastContact() {
        return lastContact;
    }

    /**
     * Sets the last contact time
     */
    public void setLastContact(long lastContact) {
        this.lastContact = lastContact;
    }

    /**
     * Returns the current state of the virtual machine
     *
     * @return VM_STATE - the current state of the virtual machine
     */
    public VM_STATE getState() {
        return vmState;
    }

    public long getTimeOfLastStateChange() {
        return vmStateTimeChange;
    }

    /**
     * Sets the state of the virtual machine. This function also calls all the objects
     * which are observing the virtual machines state.
     *
     * @return VM_STATE - The current state of the virtual machine
     */
    public void setState(VM_STATE newState) {
        vmStateTimeChange = Calendar.getInstance().getTimeInMillis();
        if (newState == vmState) {
            return;
        }
        vmState = newState;
        if (vmState == VM_STATE.WAITING_TO_BE_REVERTED) {
            if (client != null && client.getClientState() != CLIENT_STATE.DISCONNECTED) {
                client.setClientState(CLIENT_STATE.DISCONNECTED);
            }
        }
        System.out.println(this.getLogHeader() + " VMSetState: " + newState.toString());
        this.setChanged();
        this.notifyObservers();
    }

    /**
     * Returns a boolean showing whether of not the virtual machine is in a running state
     *
     * @return boolean
     */
    public boolean isRunning() {
        return (vmState == VM_STATE.RUNNING);
    }

    public void setClient(Client client) {
        this.client = client;
    }

    public Client getClient() {
        return this.client;
    }

    public int getVmUniqueId() {
        return vmUniqueId;
    }

    public String getPassword() {
        return password;
    }

    public String getPath() {
        return path;
    }

    public String getUsername() {
        return username;
    }

    public String getCaptureClientPath() {
        return captureClientPath;
    }

    public String currentTime() {
        long current = System.currentTimeMillis();
        Date currentDate = new Date(current);
        SimpleDateFormat sdf = new SimpleDateFormat("MMM d, yyyy h:mm:ss a");
        String strDate = sdf.format(currentDate);
        return strDate;
    }

    public String getLogHeader() {
        return "[" + currentTime() + "-" + server.getAddress() + ":" + server.getPort() + "-" + this.getVmUniqueId() + "]";
    }

    public boolean equals(Object o) {
        if (this == o) return true;
        if (!(o instanceof VirtualMachine)) return false;

        VirtualMachine that = (VirtualMachine) o;

        if (vmUniqueId != that.vmUniqueId) return false;

        return true;
    }    
}

/**
 * State of the virtual machine
 */
enum VM_STATE {
    REVERTING,
    WAITING_TO_BE_REVERTED,
    RUNNING,
	STOPPED,
	SAVING,
	WAITING_TO_BE_SAVED,
	ERROR
}
