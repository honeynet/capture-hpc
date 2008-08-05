package capture;

import java.util.Calendar;
import java.util.LinkedList;
import java.util.TimerTask;


public class VirtualMachinesStateChecker extends TimerTask {

    private LinkedList<VirtualMachineServer> virtualMachineServers;


    public VirtualMachinesStateChecker(LinkedList<VirtualMachineServer> virtualMachineServers) {
        this.virtualMachineServers = virtualMachineServers;
    }

    public void run() {
        try {
            for (VirtualMachineServer vmServer : virtualMachineServers) {
                for (VirtualMachine vm : vmServer.getVirtualMachines()) {
                    long currentTime = Calendar.getInstance().getTimeInMillis();
                    if (vm.getState() == VM_STATE.RUNNING) {
                        long diff = currentTime - vm.getLastContact();
                        if (diff >= (1000 * Integer.parseInt(ConfigManager.getInstance().getConfigOption("client_inactivity_timeout")))) {
                            Stats.clientInactivity++;
                            System.out.println(vm.getLogHeader() + " Client inactivity, reverting VM");
                            setError(vm, ERROR_CODES.CAPTURE_CLIENT_INACTIVITY);

                            if (ConfigManager.getInstance().getConfigOption("halt_on_revert") != null && ConfigManager.getInstance().getConfigOption("halt_on_revert").equals("true")) { //if option is set, vm is not reverted, but rather server is halted.
                                System.out.println("Halt on revert set.");
                                System.out.println("Revert called - exiting with code -20.");
                                System.exit(-20);
                            }
                            
                            vm.setState(VM_STATE.WAITING_TO_BE_REVERTED);
                        }

                        diff = currentTime - vm.getTimeOfLastStateChange();
                        if (diff >= (1000 * Integer.parseInt(ConfigManager.getInstance().getConfigOption("vm_stalled_during_operation_timeout")))) {
                            Stats.vmStalled++;
                            System.out.println(vm.getLogHeader() + " VM stalled during operation, reverting VM");
                            setError(vm, ERROR_CODES.VM_STALLED);

                            if (ConfigManager.getInstance().getConfigOption("halt_on_revert") != null && ConfigManager.getInstance().getConfigOption("halt_on_revert").equals("true")) { //if option is set, vm is not reverted, but rather server is halted.
                                System.out.println("Halt on revert set.");
                                System.out.println("Revert called - exiting with code -20.");
                                System.exit(-20);
                            }

                            vm.setState(VM_STATE.WAITING_TO_BE_REVERTED);
                        }
                    } else {
                        if (vm.getState() == VM_STATE.REVERTING) {
                            vm.setLastContact(Calendar.getInstance().getTimeInMillis());
                        }
                        long diff = currentTime - vm.getTimeOfLastStateChange();
                        if (diff >= (1000 * Integer.parseInt(ConfigManager.getInstance().getConfigOption("vm_stalled_after_revert_timeout")))) {
                            Stats.vmStalled++;
                            System.out.println(vm.getLogHeader() + " VM stalled, reverting VM");
                            setError(vm, ERROR_CODES.VM_STALLED);

                            if (ConfigManager.getInstance().getConfigOption("halt_on_revert") != null && ConfigManager.getInstance().getConfigOption("halt_on_revert").equals("true")) { //if option is set, vm is not reverted, but rather server is halted.
                                System.out.println("Halt on revert set.");
                                System.out.println("Revert called - exiting with code -20.");
                                System.exit(-20);
                            }

                            vm.setState(VM_STATE.WAITING_TO_BE_REVERTED);
                        }
                    }
                }
            }
        } catch (Exception e) {
            e.printStackTrace(System.out);
        }
    }

    private void setError(VirtualMachine vm, ERROR_CODES error_codes) {
        if (vm != null) {
            Client c = vm.getClient();
            if (c != null) {
                UrlGroup group = c.getVisitingUrlGroup();
                if (group != null) {
                    group.setMajorErrorCode(error_codes.errorCode);
                    group.setUrlGroupState(URL_GROUP_STATE.ERROR);
                }
            }
        }
    }
}
