package capture;

import org.junit.Test;

import java.text.ParseException;
import java.util.Map;
import java.util.TreeMap;

import static junit.framework.Assert.assertTrue;
import static junit.framework.Assert.assertEquals;
import static junit.framework.Assert.assertFalse;

/**
 * PROJECT: Capture-HPC
 * DATE: Apr 28, 2008
 * FILE: StateChangeHanderTest
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
public class StateChangeHanderTest {
    @Test
    public void testAddStateChange_twoNewProcessTrees() throws ParseException {
        StateChangeHandler sch = new StateChangeHandler();

        String type = "process";
        String time = "24/04/2008 16:3:0.140";
        String parentProcessId = "4";
        String parentProcess = "CaptureClient.exe";
        String action = "created";
        String processId = "231";
        String process = "iexplorer.exe";
        ProcessStateChange psc = new ProcessStateChange(type, time, parentProcessId, parentProcess, action, processId, process);
        boolean added = sch.addStateChange(psc);
        assertTrue("Valid process state change not successfully added to stateChangeHandler.",added);

        String type2 = "process";
        String time2 = "24/04/2008 16:7:0.140";
        String parentProcessId2 = "4";
        String parentProcess2 = "CaptureClient.exe";
        String action2 = "created";
        String processId2 = "233";
        String process2 = "iexplorer.exe";
        ProcessStateChange psc2 = new ProcessStateChange(type2, time2, parentProcessId2, parentProcess2, action2, processId2, process2);
        boolean added2 = sch.addStateChange(psc2);

        assertTrue("Valid process state change not successfully added to stateChangeHandler.",added2);

        assertEquals("Two independent processes didnt cause creation of two process trees.",2,sch.getProcessTreeCount());

    }

    @Test
    public void testAddStateChange_oneProcessTreesTwoStateChanges() throws ParseException {
        StateChangeHandler sch = new StateChangeHandler();

        String type = "process";
        String time = "24/04/2008 16:3:0.140";
        String parentProcessId = "4";
        String parentProcess = "CaptureClient.exe";
        String action = "created";
        String processId = "231";
        String process = "iexplorer.exe";
        ProcessStateChange psc = new ProcessStateChange(type, time, parentProcessId, parentProcess, action, processId, process);
        boolean added = sch.addStateChange(psc);
        assertTrue("Valid process state change not successfully added to stateChangeHandler.",added);

        String ftype = "file";
        String ftime = "24/4/2008 16:7:0.140";
        String fprocessId = "231";
        String fprocess = "iexplorer.exe";
        String faction = "Write";
        String fobject1 = "C:\\tmp\\test.txt";
        String fobject2 = "-1";
        ObjectStateChange fsc = new ObjectStateChange(ftype, ftime, fprocessId, fprocess, faction, fobject1, fobject2);
        boolean added2 = sch.addStateChange(fsc);
        assertTrue("Valid file state change not successfully added to stateChangeHandler.",added2);

        assertEquals("Two dependent statechanges didnt cause creation of one process trees.",1,sch.getProcessTreeCount());
    }

    @Test
    public void testAddStateChange_oneProcessTreesLostStateChanges() throws ParseException {
        StateChangeHandler sch = new StateChangeHandler();

        String type = "process";
        String time = "24/04/2008 16:3:0.140";
        String parentProcessId = "4";
        String parentProcess = "CaptureClient.exe";
        String action = "created";
        String processId = "231";
        String process = "iexplorer.exe";
        ProcessStateChange psc = new ProcessStateChange(type, time, parentProcessId, parentProcess, action, processId, process);
        boolean added = sch.addStateChange(psc);
        assertTrue("Valid process state change not successfully added to stateChangeHandler.",added);

        String ftype = "file";
        String ftime = "24/4/2008 16:7:0.140";
        String fprocessId = "543"; //doesnt match up, so this state change will not be added
        String fprocess = "iexplorer.exe";
        String faction = "Write";
        String fobject1 = "C:\\tmp\\test.txt";
        String fobject2 = "-1";
        ObjectStateChange fsc = new ObjectStateChange(ftype, ftime, fprocessId, fprocess, faction, fobject1, fobject2);
        boolean added2 = sch.addStateChange(fsc); //processStaeChange is created based on info of file state change
        assertTrue("Valid file state change not successfully added to stateChangeHandler.",added2);

        String type1 = "process";
        String time1 = "24/04/2008 16:7:0.140";
        String parentProcessId1 = "0";
        String parentProcess1 = "CaptureClient.exe";
        String action1 = "created";
        String processId1 = "543";
        String process1 = "iexplorer.exe";
        //no need to manually add...information is inferred and added. list it here for clarity sake

        assertEquals("Two dependent statechanges didnt cause creation of one process trees.",2,sch.getProcessTreeCount());

        String expected1 = "\"process\",\"24/04/2008 16:07:00.140\",\"0\",\"CaptureClient.exe\",\"created\",\"543\",\"iexplorer.exe\"\n";
        String expected2 = "\"file\",\"24/04/2008 16:07:00.140\",\"543\",\"iexplorer.exe\",\"Write\",\"C:\\tmp\\test.txt\",\"-1\"\n";
        String expectedCSV = expected1 + expected2;
        String actualCSV = sch.getStateChangesCSV(543);
        assertEquals("Second processTree CSV not as expected.",expectedCSV,actualCSV);

    }

    @Test
    public void testGetStateChangesCSV_oneProcessTreesTwoStateChanges() throws ParseException {
        StateChangeHandler sch = new StateChangeHandler();

        String type = "process";
        String time = "24/04/2008 16:3:0.140";
        String parentProcessId = "4";
        String parentProcess = "CaptureClient.exe";
        String action = "created";
        String processId = "231";
        String process = "iexplorer.exe";
        ProcessStateChange psc = new ProcessStateChange(type, time, parentProcessId, parentProcess, action, processId, process);
        boolean added = sch.addStateChange(psc);
        assertTrue("Valid process state change not successfully added to stateChangeHandler.",added);

        String ftype = "file";
        String ftime = "24/4/2008 16:7:0.140";
        String fprocessId = "231";
        String fprocess = "iexplorer.exe";
        String faction = "Write";
        String fobject1 = "C:\\tmp\\test.txt";
        String fobject2 = "-1";
        ObjectStateChange fsc = new ObjectStateChange(ftype, ftime, fprocessId, fprocess, faction, fobject1, fobject2);
        boolean added2 = sch.addStateChange(fsc);
        assertTrue("Valid file state change not successfully added to stateChangeHandler.",added2);


        String expected1 = "\"process\",\"24/04/2008 16:03:00.140\",\"4\",\"CaptureClient.exe\",\"created\",\"231\",\"iexplorer.exe\"\n";
        String expected2 = "\"file\",\"24/04/2008 16:07:00.140\",\"231\",\"iexplorer.exe\",\"Write\",\"C:\\tmp\\test.txt\",\"-1\"\n";
        String expectedCSV = expected1 + expected2;
        assertEquals("Two dependent statechanges didnt return expected CSV representation.",expectedCSV,sch.getStateChangesCSV(231));
    }

    @Test
    public void testGetStateChanges_TwoProcessTrees() throws ParseException {
        StateChangeHandler sch = new StateChangeHandler();

        String type = "process";
        String time = "24/04/2008 15:3:0.140";
        String parentProcessId = "4";
        String parentProcess = "CaptureClient.exe";
        String action = "created";
        String processId = "542";
        String process = "firefox.exe";
        ProcessStateChange psc = new ProcessStateChange(type, time, parentProcessId, parentProcess, action, processId, process);
        boolean added = sch.addStateChange(psc);
        assertTrue("Valid process state change not successfully added to stateChangeHandler.",added);


        String type2 = "process";
        String time2 = "24/04/2008 16:3:0.140";
        String parentProcessId2 = "4";
        String parentProcess2 = "CaptureClient.exe";
        String action2 = "created";
        String processId2 = "231";
        String process2 = "iexplorer.exe";
        ProcessStateChange psc2 = new ProcessStateChange(type2, time2, parentProcessId2, parentProcess2, action2, processId2, process2);
        added = sch.addStateChange(psc2);
        assertTrue("Valid process state change not successfully added to stateChangeHandler.",added);

        String ftype = "file";
        String ftime = "24/4/2008 16:7:0.140";
        String fprocessId = "231";
        String fprocess = "iexplorer.exe";
        String faction = "Write";
        String fobject1 = "C:\\tmp\\test.txt";
        String fobject2 = "-1";
        ObjectStateChange fsc = new ObjectStateChange(ftype, ftime, fprocessId, fprocess, faction, fobject1, fobject2);
        boolean added2 = sch.addStateChange(fsc);
        assertTrue("Valid file state change not successfully added to stateChangeHandler.",added2);


        String expectedURL1 = "http://www.google.com";
        String expectedCSV1 = "\"process\",\"24/04/2008 15:03:00.140\",\"4\",\"CaptureClient.exe\",\"created\",\"542\",\"firefox.exe\"\n";

        String expectedURL2 = "http://www.malware.com";
        String expected1 = "\"process\",\"24/04/2008 16:03:00.140\",\"4\",\"CaptureClient.exe\",\"created\",\"231\",\"iexplorer.exe\"\n";
        String expected2 = "\"file\",\"24/04/2008 16:07:00.140\",\"231\",\"iexplorer.exe\",\"Write\",\"C:\\tmp\\test.txt\",\"-1\"\n";
        String expectedCSV2 = expected1 + expected2;

        Map<String,String> expected = new TreeMap<String,String>();
        expected.put(expectedURL1,expectedCSV1);
        expected.put(expectedURL2,expectedCSV2);


        Map<String,Integer> urlProcessIdMap = new TreeMap<String,Integer>();
        urlProcessIdMap.put("http://www.google.com",542);
        urlProcessIdMap.put("http://www.malware.com",231);

        assertEquals("UrlCSVMap not as expected",expected,sch.getStateChanges(urlProcessIdMap));


    }

}
