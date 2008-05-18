package capture;

import java.util.*;
import java.text.ParseException;

/**
 * PROJECT: Capture-HPC
 * DATE: Apr 28, 2008
 * FILE: StateChangeHandler
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
public class StateChangeHandler {

    private List<ProcessStateChange> processTrees = new ArrayList<ProcessStateChange>();

    public StateChangeHandler() {
    }


    public boolean addStateChange(StateChange sc) throws ParseException {
        boolean added = false;
        //if process, it will be added to one of the trees or create a new one
        if (sc instanceof ProcessStateChange) {
            ProcessStateChange newPsc = (ProcessStateChange) sc;

            for (Iterator<ProcessStateChange> processStateChangeIterator = processTrees.iterator(); processStateChangeIterator.hasNext();) {
                ProcessStateChange processStateChange = processStateChangeIterator.next();
                added = processStateChange.addStateChange(newPsc);
                if (added) { //found process that this can be added to
                    break;
                }
            }

            if (!added) { //didnt find a processs - creating new process tree
                processTrees.add(newPsc);
                added = true;
            }
        }

        if (sc instanceof ObjectStateChange) {

            ObjectStateChange osc = (ObjectStateChange) sc;
            for (Iterator<ProcessStateChange> processStateChangeIterator = processTrees.iterator(); processStateChangeIterator.hasNext();) {
                ProcessStateChange processStateChange = processStateChangeIterator.next();
                added = processStateChange.addStateChange(osc);
                if (added) {
                    break;
                }
            }

            if (!added) { //first write event without having process available
                //todo - check whether the process is actually the client
                String type1 = "process";
                String time1 = sc.getTimeString();
                String parentProcessId1 = "0";
                String parentProcess1 = "CaptureClient.exe";
                String action1 = "created";
                String processId1 = sc.getProcessId() + "";
                String process1 = sc.getProcess();
                ProcessStateChange newPsc = new ProcessStateChange(type1,time1,parentProcessId1,parentProcess1,action1,processId1,process1);

                processTrees.add(newPsc);

                added = newPsc.addStateChange(osc);
                if(added) {
                    return added;   
                }
            }

        }

        return added;
    }

    /* gets state changes from state change handler in form of a map that maps urls to a string of state changes in csv form
     *
     * @param urlProcessIdMap - map that maps the url to a processId, e.g. http://www.google.com/,3131
     * @returns map that maps urls to a string of state changes in csv form, e.g. http://www.google.com,statechange1\nstatechange2
     *      where statechange1 and 2 are csv representations of the state change
     */
    public Map<String, String> getStateChanges(Map<String, Integer> urlProcessIdMap) {
        Map<String, String> urlStateChangesMap = new TreeMap<String, String>();
        Set<String> urls = urlProcessIdMap.keySet();
        for (Iterator<String> stringIterator = urls.iterator(); stringIterator.hasNext();) {
            String url = stringIterator.next().toLowerCase();
            int processId = urlProcessIdMap.get(url);
            String csv = getStateChangesCSV(processId);
            urlStateChangesMap.put(url, csv);
        }
        return urlStateChangesMap;
    }

    public String getStateChangesCSV(int processId) {
        for (Iterator<ProcessStateChange> processStateChangeIterator = processTrees.iterator(); processStateChangeIterator.hasNext();) {
            ProcessStateChange processStateChange = processStateChangeIterator.next();
            if (processStateChange.getProcessId() == processId || processStateChange.getParentProcessId() == processId) {
                return getCSVofTree(processStateChange);
            }
        }

        if(processTrees.size()==1 && processId == 0) {
            return getCSVofTree(processTrees.get(0));
        } else {
            return "";
        }
    }

    private String getCSVofTree(ProcessStateChange processStateChange) {
        List<StateChange> allStateChanges = processStateChange.getAllStateChanges();
        StringBuffer csvOfTree = new StringBuffer();
        for (Iterator<StateChange> stateChangeIterator = allStateChanges.iterator(); stateChangeIterator.hasNext();) {
            StateChange stateChange = stateChangeIterator.next();
            csvOfTree.append(stateChange.toCSV());
        }
        return csvOfTree.toString();
    }


    public int getProcessTreeCount() {
        return processTrees.size();
    }
}
