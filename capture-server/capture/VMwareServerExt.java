package capture;

public class VMwareServerExt {
    static {
        try {
            System.loadLibrary("VMwareServerExt");
            System.out.println("VIX library loaded");
        } catch (UnsatisfiedLinkError le) {
            le.printStackTrace(System.err);

            System.out.println("\nMake sure VMwareServerExt.dll/libVMwareServerExt.so is in your LD_LIBRARY_PATH variable\n");
            System.out.println("Also make sure that the VMware VIX libraries are also available. " +
                    "On Windows these are libeay32.dll, ssleay32.dll, vix.dll. These are available " +
                    "from http://register.vmware.com/content/download.html and are called " +
                    "the \"Programming API\". Copy these into your Capture Server directory if you " +
                    "aren't sure of what to do");
            System.exit(1);
        }
    }

    public static void main(String args[]) {
	try {
        VMwareServerExt vmwareServerExt = new VMwareServerExt();

        String command = args[0];
        if (command.equalsIgnoreCase("nRevert")) {
            String address = args[1];
            String port = args[2];
            String vmUniqueId = args[3];
            String username = args[4];
            String password = args[5];
            String vmPath = args[6];
            String guestUsername = args[7];
            String guestPassword = args[8];
            String guestCmd = args[9];
            String guestCmdOptions = args[10];
            int timeout = Integer.parseInt(args[11]);

            /*System.out.println("Address " + address);
            System.out.println("Port " + port);
            System.out.println("vmUniqueId " + vmUniqueId);
            System.out.println("username " + username);
            System.out.println("password " + password);
            System.out.println("vmPath " + vmPath);
            System.out.println("guestUsername " + guestUsername);
            System.out.println("guestPassword " + guestPassword);
            System.out.println("guestCmd " + guestCmd);
            System.out.println("guestCmdOptions " + guestCmdOptions);
            System.out.println("timeout " + timeout);
            */

            int returnCode = vmwareServerExt.revertWorkItem(address, port, vmUniqueId, username, password, vmPath, guestUsername, guestPassword, guestCmd, guestCmdOptions, timeout);
            System.exit(returnCode);
        } else {
	    System.out.println("received non-nRevert command");
	    for(int i =0; i<args.length; i++) {
		System.out.println("args[" + i + "] " + args[i]);
	    }

            System.exit(3);
        }
	} catch(Exception e) {
	    System.out.println("Exception thrown" + e.toString());
	    e.printStackTrace();
            System.exit(1);
	} 
    }

    @SuppressWarnings("unused")
	private int hServer;

    public int revertWorkItem
            (String
            address, String
            port, String
            vmUniqueId, String
            username, String
            password, String
            vmPath, String
            guestUsername, String
            guestPassword, String
            guestCmd, String
            guestCmdOptions, int timeout) {
        int error = 0;

        System.out.println("[" + address + ":" + port + "] Connecting");
        error = nConnect(address, username, password);
        if (error != 0) {
            System.out.println("[" + address + ":" + port + "] ERROR - Connect: " + errorCodeToString(error));
        } else {
            System.out.println("[" + address + ":" + port + "] Connected");
            System.out.println("[" + address + ":" + port + "-" + vmUniqueId + "] Resetting VM");
            error = nRevertVM(vmPath);
            if (error == 0) {
                System.out.println("[" + address + ":" + port + "-" + vmUniqueId + "] waitForToolsInGuest");
                error = nWaitForToolsInGuest(vmPath, timeout);
                if (error == 0) {
                    System.out.println("[" + address + ":" + port + "-" + vmUniqueId + "] runProgramInGuest.");
                    if (guestPassword.equals("")) {
                        guestPassword = null;
                    }
                    error = nRunProgramInGuest(vmPath, guestUsername, guestPassword, guestCmd, guestCmdOptions);
                    if (error == 0) {
                        System.out.println("[" + address + ":" + port + "-" + vmUniqueId + "] VM Resetted");
                        error = nDisconnect();
                        if (error == 0) {
                            System.out.println("[" + address + ":" + port + "-" + vmUniqueId + "] VM Disconnected - we dont need it anymore at this point.");
                        } else {
                            System.out.println("[" + address + ":" + port + "-" + vmUniqueId + "] ERROR - Unable to disconnect");
                        }
                    } else {
                        System.out.println("[" + address + ":" + port + "-" + vmUniqueId + "] ERROR - Unable to start " + guestCmd + ".");
                    }
                } else {
                    System.out.println("[" + address + ":" + port + "-" + vmUniqueId + "] ERROR - Unable to detect VMware Tools.");
                }
            } else {
                System.out.println("[" + address + ":" + port + "-" + vmUniqueId + "] ERROR - Revert failed - not connected to server");
            }
        }
        return error;
    }

    public static String errorCodeToString
            (int error) {
        for (VIX_ERROR_CODES e : VIX_ERROR_CODES.values()) {
            if (error == e.errorCode) {
                return e.toString();
            }
        }
        return "";
    }

    public enum VIX_ERROR_CODES {
        VIX_OK										 (0),

        /* General errors */
        VIX_E_FAIL                                   (1),
        VIX_E_OUT_OF_MEMORY                          (2),
        VIX_E_INVALID_ARG                            (3),
        VIX_E_FILE_NOT_FOUND                         (4),
        VIX_E_OBJECT_IS_BUSY                         (5),
        VIX_E_NOT_SUPPORTED                          (6),
        VIX_E_FILE_ERROR                             (7),
        VIX_E_DISK_FULL                              (8),
        VIX_E_INCORRECT_FILE_TYPE                    (9),
        VIX_E_CANCELLED                              (10),
        VIX_E_FILE_READ_ONLY                         (11),
        VIX_E_FILE_ALREADY_EXISTS                    (12),
        VIX_E_FILE_ACCESS_ERROR                      (13),
        VIX_E_REQUIRES_LARGE_FILES                   (14),
        VIX_E_FILE_ALREADY_LOCKED                    (15),
        VIX_E_VMSERVER_NOT_FOUND					 (16),
        VIX_E_TIMEOUT               				 (17),

        /* Handle Errors */
        VIX_E_INVALID_HANDLE                         (1000),
        VIX_E_NOT_SUPPORTED_ON_HANDLE_TYPE           (1001),
        VIX_E_TOO_MANY_HANDLES                       (1002),

        /* XML errors */
        VIX_E_NOT_FOUND                              (2000),
        VIX_E_TYPE_MISMATCH                          (2001),
        VIX_E_INVALID_XML                            (2002),

        /* VM Control Errors */
        VIX_E_TIMEOUT_WAITING_FOR_TOOLS              (3000),
        VIX_E_UNRECOGNIZED_COMMAND                   (3001),
        VIX_E_OP_NOT_SUPPORTED_ON_GUEST              (3003),
        VIX_E_PROGRAM_NOT_STARTED                    (3004),
        VIX_E_VM_NOT_RUNNING                         (3006),
        VIX_E_VM_IS_RUNNING                          (3007),
        VIX_E_CANNOT_CONNECT_TO_VM                   (3008),
        VIX_E_POWEROP_SCRIPTS_NOT_AVAILABLE          (3009),
        VIX_E_NO_GUEST_OS_INSTALLED                  (3010),
        VIX_E_VM_INSUFFICIENT_HOST_MEMORY            (3011),
        VIX_E_SUSPEND_ERROR                          (3012),
        VIX_E_VM_NOT_ENOUGH_CPUS                     (3013),
        VIX_E_HOST_USER_PERMISSIONS                  (3014),
        VIX_E_GUEST_USER_PERMISSIONS                 (3015),
        VIX_E_TOOLS_NOT_RUNNING                      (3016),
        VIX_E_GUEST_OPERATIONS_PROHIBITED_ON_HOST    (3017),
        VIX_E_GUEST_OPERATIONS_PROHIBITED_ON_VM      (3018),
        VIX_E_ANON_GUEST_OPERATIONS_PROHIBITED_ON_HOST (3019),
        VIX_E_ANON_GUEST_OPERATIONS_PROHIBITED_ON_VM (3020),
        VIX_E_ROOT_GUEST_OPERATIONS_PROHIBITED_ON_HOST (3021),
        VIX_E_ROOT_GUEST_OPERATIONS_PROHIBITED_ON_VM (3022),
        VIX_E_MISSING_ANON_GUEST_ACCOUNT             (3023),
        VIX_E_CANNOT_AUTHENTICATE_WITH_GUEST         (3024),
        VIX_E_UNRECOGNIZED_COMMAND_IN_GUEST          (3025),

        /* VM Errors */
        VIX_E_VM_NOT_FOUND                           (4000),
        VIX_E_NOT_SUPPORTED_FOR_VM_VERSION           (4001),
        VIX_E_CANNOT_READ_VM_CONFIG                  (4002),
        VIX_E_TEMPLATE_VM                            (4003),
        VIX_E_VM_ALREADY_LOADED                      (4004),

        /* Property Errors */
        VIX_E_UNRECOGNIZED_PROPERTY                  (6000),
        VIX_E_INVALID_PROPERTY_VALUE                 (6001),
        VIX_E_READ_ONLY_PROPERTY                     (6002),
        VIX_E_MISSING_REQUIRED_PROPERTY              (6003),

        /* Completion Errors */
        VIX_E_BAD_VM_INDEX							(8000);

        public int errorCode;

        VIX_ERROR_CODES(int p) {
            errorCode = p;
        }
    }


    private native int nConnect
            (String
            address, String
            username, String
            password);

    private native int nRevertVM
            (String
            vmPath);

    private native int nDisconnect
            ();

    private native int nRunProgramInGuest
            (String
            vmPath,
             String
            guestUsername, String
            guestPassword, String
            guestCmd, String
            guestCmdOptions);

    private native int nWaitForToolsInGuest
            (String
            vmPath, int timeoutSeconds);


}
