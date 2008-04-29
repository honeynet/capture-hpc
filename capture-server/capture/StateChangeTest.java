package capture;

import junit.framework.JUnit4TestAdapter;
import static junit.framework.Assert.assertTrue;
import static junit.framework.Assert.assertFalse;
import org.junit.Test;
import static org.junit.Assert.assertEquals;
import org.junit.runner.JUnitCore;

import java.text.ParseException;
import java.util.List;

/**
 * PROJECT: Capture-HPC
 * DATE: Apr 28, 2008
 * FILE: StateChangeTest
 * COPYRIGHT HOLDER: Victoria University of Wellington, NZ
 * AUTHORS: Christian Seifert (christian.seifert@gmail.com)
 * <p/>
 * This file is part of Capture-HPC.
 * <p/>
 * Capture-HPC is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 * <p/>
 * Capture-HPC is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * <p/>
 * You should have received a copy of the GNU General Public License
 * along with Capture-HPC; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
 */
public class StateChangeTest {
    @Test
    public void testSetTime_validTimeString() {
        String validTimeString = "24/4/2008 15:58:40.189";
        try {
            StateChange sc = new StateChange() {
                public String toCSV() {
                    return "";
                }
            };
            sc.setTime(validTimeString);
        } catch (ParseException e) {
            e.printStackTrace();
            assertTrue("Exception thrown when trying to setTime with a valid time string:" + validTimeString, false);
        }
    }

    @Test
    public void testAddFileStateChange_matchingProcess() throws ParseException {
        //"file",,"4","System","Write","C:\Users\Christian Seifert\ntuser.dat.LOG1"
        String type = "process";
        String time = "24/4/2008 16:3:0.140";
        String parentProcessId = "4";
        String parentProcess = "CaptureClient.exe";
        String action = "created";
        String processId = "231";
        String process = "iexplorer.exe";
        ProcessStateChange psc = new ProcessStateChange(type, time, parentProcessId, parentProcess, action, processId, process);

        String ftype = "file";
        String ftime = "24/4/2008 16:7:0.140";
        String fprocessId = "231";
        String fprocess = "iexplorer.exe";
        String faction = "Write";
        String fobject1 = "C:\\tmp\\test.txt";
        String fobject2 = "-1";
        ObjectStateChange fsc = new ObjectStateChange(ftype, ftime, fprocessId, fprocess, faction, fobject1, fobject2);

        boolean added = psc.addStateChange(fsc);
        assertTrue("File state change of process wasnt added to process.", added);
    }

    @Test
    public void testAddFileStateChange_mismatchingProcess() throws ParseException {
        //"file",,"4","System","Write","C:\Users\Christian Seifert\ntuser.dat.LOG1"
        String type = "process";
        String time = "24/4/2008 16:3:0.140";
        String parentProcessId = "4";
        String parentProcess = "CaptureClient.exe";
        String action = "created";
        String processId = "231";
        String process = "iexplorer.exe";
        ProcessStateChange psc = new ProcessStateChange(type, time, parentProcessId, parentProcess, action, processId, process);

        String ftype = "file";
        String ftime = "24/4/2008 16:7:0.140";
        String fprocessId = "999";
        String fprocess = "iexplorer.exe";
        String faction = "Write";
        String fobject1 = "C:\\tmp\\test.txt";
        String fobject2 = "-1";
        ObjectStateChange fsc = new ObjectStateChange(ftype, ftime, fprocessId, fprocess, faction, fobject1,fobject2);

        boolean added = psc.addStateChange(fsc);
        assertFalse("File state change of process was added to wrong process.", added);
    }

    @Test
    public void testAddProcessStateChange_matchingProcess() throws ParseException {
        //"file",,"4","System","Write","C:\Users\Christian Seifert\ntuser.dat.LOG1"
        String type = "process";
        String time = "24/4/2008 16:3:0.140";
        String parentProcessId = "4";
        String parentProcess = "CaptureClient.exe";
         String action = "created";
        String processId = "231";
        String process = "iexplorer.exe";
        ProcessStateChange psc = new ProcessStateChange(type, time, parentProcessId, parentProcess, action,processId, process);

        String ptype = "process";
        String ptime = "24/4/2008 16:7:0.140";
        String pParentProcessId = "231";
        String pparentPprocess = "iexplorer.exe";
         String paction = "created";
        String pprocessId = "421";
        String pprocess = "C:\\tmp\\malware.exe";
        ProcessStateChange psc2 = new ProcessStateChange(ptype, ptime, pParentProcessId, pparentPprocess, paction,pprocessId, pprocess);

        boolean added = psc.addStateChange(psc2);
        assertTrue("Process state change of process wasnt added to process.", added);

    }

    @Test
    public void testAddFileStateChange_matchingChildProcess() throws ParseException {
        //"file",,"4","System","Write","C:\Users\Christian Seifert\ntuser.dat.LOG1"
        String type = "process";
        String time = "24/4/2008 16:3:0.140";
        String parentProcessId = "4";
        String parentProcess = "CaptureClient.exe";
         String action = "created";
        String processId = "231";
        String process = "iexplorer.exe";
        ProcessStateChange psc = new ProcessStateChange(type, time, parentProcessId, parentProcess, action,processId, process);

        String ptype = "process";
        String ptime = "24/4/2008 16:7:0.140";
        String pParentProcessId = "231";
        String pparentPprocess = "iexplorer.exe";
         String paction = "created";
        String pprocessId = "421";
        String pprocess = "C:\\tmp\\malware.exe";
        ProcessStateChange psc2 = new ProcessStateChange(ptype, ptime, pParentProcessId, pparentPprocess, paction,pprocessId, pprocess);

        boolean added = psc.addStateChange(psc2);

        String ftype = "file";
        String ftime = "24/4/2008 16:7:0.140";
        String fprocessId = "421";
        String fprocess = "C:\\tmp\\malware.exe";
        String faction = "Write";
        String fobject1 = "C:\\tmp\\test.txt";
        String fobject2 = "-1";
        ObjectStateChange fsc = new ObjectStateChange(ftype, ftime, fprocessId, fprocess, faction, fobject1,fobject2);


        boolean addedToChild = psc.addStateChange(fsc);
        assertTrue("File state change of process wasnt added to child process.", addedToChild);
    }

    @Test
    public void testGetAllStateChanges_orderedProcesses() throws ParseException {
        //"file",,"4","System","Write","C:\Users\Christian Seifert\ntuser.dat.LOG1"
        String type = "process";
        String time = "24/4/2008 16:3:0.140";
        String parentProcessId = "4";
        String parentProcess = "CaptureClient.exe";
         String action = "created";
        String processId = "231";
        String process = "iexplorer.exe";
        ProcessStateChange psc = new ProcessStateChange(type, time, parentProcessId, parentProcess, action,processId, process);

        String ptype = "process";
        String ptime = "24/4/2008 16:7:0.140";
        String pParentProcessId = "231";
        String pparentPprocess = "iexplorer.exe";
         String paction = "created";
        String pprocessId = "421";
        String pprocess = "C:\\tmp\\malware.exe";
        ProcessStateChange psc2 = new ProcessStateChange(ptype, ptime, pParentProcessId, pparentPprocess, paction,pprocessId, pprocess);

        boolean added = psc.addStateChange(psc2);

        List allStateChanges = psc.getAllStateChanges();
        assertEquals("Not all state changes returned.", 2, allStateChanges.size());
        assertEquals("First state change incorrect.", psc, allStateChanges.get(0));
        assertEquals("Second state change incorrect.", psc2, allStateChanges.get(1));

    }

    @Test
    public void testGetAllStateChanges_unorderedProcesses() throws ParseException {
        //"file",,"4","System","Write","C:\Users\Christian Seifert\ntuser.dat.LOG1"
        String type = "process";
        String time = "24/4/2008 16:10:0.140";
        String parentProcessId = "4";
        String parentProcess = "CaptureClient.exe";
         String action = "created";
        String processId = "231";
        String process = "iexplorer.exe";
        ProcessStateChange psc = new ProcessStateChange(type, time, parentProcessId, parentProcess, action,processId, process);

        String ptype = "process";
        String ptime = "24/4/2008 16:7:0.140";
        String pParentProcessId = "231";
        String pparentPprocess = "iexplorer.exe";
         String paction = "pcreated";
        String pprocessId = "421";
        String pprocess = "C:\\tmp\\malware.exe";
        ProcessStateChange psc2 = new ProcessStateChange(ptype, ptime, pParentProcessId, pparentPprocess, paction, pprocessId, pprocess);

        boolean added = psc.addStateChange(psc2);

        List allStateChanges = psc.getAllStateChanges();
        assertEquals("Not all state changes returned.", 2, allStateChanges.size());
        assertEquals("First state change incorrect.", psc2, allStateChanges.get(0));
        assertEquals("Second state change incorrect.", psc, allStateChanges.get(1));
    }

    @Test
    public void testGetAllStateChanges_orderedMixed() throws ParseException {
        //"file",,"4","System","Write","C:\Users\Christian Seifert\ntuser.dat.LOG1"
        String type = "process";
        String time = "24/4/2008 16:3:0.140";
        String parentProcessId = "4";
        String parentProcess = "CaptureClient.exe";
         String action = "created";
        String processId = "231";
        String process = "iexplorer.exe";
        ProcessStateChange psc = new ProcessStateChange(type, time, parentProcessId, parentProcess, action,processId, process);

        String ftype = "file";
        String ftime = "24/4/2008 16:7:0.140";
        String fprocessId = "231";
        String fprocess = "C:\\tmp\\malware.exe";
        String faction = "Write";
        String fobject1 = "C:\\tmp\\test.txt";
        String fobject2 = "-1";
        ObjectStateChange fsc = new ObjectStateChange(ftype, ftime, fprocessId, fprocess, faction, fobject1,fobject2);

        boolean added = psc.addStateChange(fsc);

        String ptype = "process";
        String ptime = "24/4/2008 16:7:0.141";
        String pParentProcessId = "231";
        String pparentPprocess = "iexplorer.exe";
         String paction = "created";
        String pprocessId = "421";
        String pprocess = "C:\\tmp\\malware.exe";
        ProcessStateChange psc2 = new ProcessStateChange(ptype, ptime, pParentProcessId, pparentPprocess, paction, pprocessId, pprocess);

        added = psc.addStateChange(psc2);

        List allStateChanges = psc.getAllStateChanges();
        assertEquals("Not all state changes returned.", 3, allStateChanges.size());
        assertEquals("First state change incorrect.", psc, allStateChanges.get(0));
        assertEquals("Second state change incorrect.", fsc, allStateChanges.get(1));
        assertEquals("Third state change incorrect.", psc2, allStateChanges.get(2));

    }

    @Test
    public void testToCSV_process() throws ParseException {
        String expected = "\"process\",\"24/04/2008 16:03:00.140\",\"4\",\"CaptureClient.exe\",\"created\",\"231\",\"iexplorer.exe\"\n";

        String type = "process";
        String time = "24/04/2008 16:3:0.140";
        String parentProcessId = "4";
        String parentProcess = "CaptureClient.exe";
         String action = "created";
        String processId = "231";
        String process = "iexplorer.exe";
        ProcessStateChange psc = new ProcessStateChange(type, time, parentProcessId, parentProcess, action, processId, process);

        String actual = psc.toCSV();
        assertEquals("process toCSV not correct.", expected, actual);
    }


    @Test
    public void testToCSV_file() throws ParseException {
        String expected = "\"file\",\"24/04/2008 16:07:00.140\",\"231\",\"C:\\tmp\\malware.exe\",\"Write\",\"C:\\tmp\\test.txt\",\"-1\"\n";

        String ftype = "file";
        String ftime = "24/4/2008 16:7:0.140";
        String fprocessId = "231";
        String fprocess = "C:\\tmp\\malware.exe";
        String faction = "Write";
        String fobject1 = "C:\\tmp\\test.txt";
        String fobject2 = "-1";
        ObjectStateChange fsc = new ObjectStateChange(ftype, ftime, fprocessId, fprocess, faction, fobject1,fobject2);

        String actual = fsc.toCSV();
        assertEquals("file toCSV not correct.", expected, actual);
    }

}
