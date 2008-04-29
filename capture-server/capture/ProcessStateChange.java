package capture;

import java.util.*;
import java.text.ParseException;

/**
 * PROJECT: Capture-HPC
 * DATE: Apr 28, 2008
 * FILE: ProcessStateChange
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
public class ProcessStateChange extends StateChange {


    private int parentProcessId;
    private String parentProcess;

    private List<ObjectStateChange> stateChanges = new ArrayList<ObjectStateChange>();

    private List<ProcessStateChange> childProcesses = new ArrayList<ProcessStateChange>();

    public ProcessStateChange(String type, String time, String parentProcessId, String parentProcess, String action, String processId, String process) throws ParseException {
        this.type = type;
        setTime(time);
        this.action = action;
        this.processId = Integer.parseInt(processId);
        this.process = process;
        this.parentProcessId = Integer.parseInt(parentProcessId);
        this.parentProcess = parentProcess;
    }

    public boolean addStateChange(StateChange sc) {
        if(sc instanceof ObjectStateChange) {
            ObjectStateChange osc = (ObjectStateChange) sc;
            if(osc.getProcessId()==processId) {
                stateChanges.add(osc);
                return true;
            }
        }

        if(sc instanceof ProcessStateChange) {
            ProcessStateChange psc = (ProcessStateChange) sc;
            if(psc.getParentProcessId() == this.processId) {
                childProcesses.add(psc);
                return true;
            }
        }

        for (Iterator<ProcessStateChange> childStateChangeIterator = childProcesses.iterator(); childStateChangeIterator.hasNext();) {
            ProcessStateChange childProcess = childStateChangeIterator.next();
            boolean added = childProcess.addStateChange(sc);
            if(added) {
                return true;
            }
        }

        return false;
    }

    public String toCSV() {
        return "\""+type+"\",\""+getTimeString()+"\",\""+parentProcessId+"\",\""+parentProcess+"\",\""+action+"\",\""+processId+"\",\""+process+"\"\n";
    }

    public List<StateChange> getAllStateChanges() {
        List<StateChange> allStateChanges = new ArrayList<StateChange>();

        allStateChanges.addAll(stateChanges);
        for (Iterator<ProcessStateChange> stateChangeIterator = childProcesses.iterator(); stateChangeIterator.hasNext();) {
            ProcessStateChange processStateChange = stateChangeIterator.next();
            allStateChanges.addAll(processStateChange.getAllStateChanges());
        }
        allStateChanges.add(this);

        Collections.sort(allStateChanges, new Comparator() {
            public int compare(Object o1, Object o2) {
                Date d1 = ((StateChange)o1).getTime();
                Date d2 = ((StateChange)o2).getTime();
                int dateDiff = d1.compareTo(d2);
                if(dateDiff==0) {
                    String type1 = ((StateChange)o1).getType();
                    String type2 = ((StateChange)o2).getType();
                    return type2.compareTo(type1);
                } else {
                    return dateDiff;
                }
            }
        });

        return allStateChanges;
    }

    public int getParentProcessId() {
        return parentProcessId;
    }

    public boolean equals(Object o) {
        if (this == o) return true;
        if (!(o instanceof ProcessStateChange)) return false;

        ProcessStateChange that = (ProcessStateChange) o;

        if (processId != that.processId) return false;
        if (parentProcessId != that.parentProcessId) return false;
        if (parentProcess != null ? !parentProcess.equals(that.parentProcess) : that.parentProcess != null)
            return false;
        if (process != null ? !process.equals(that.process) : that.process != null) return false;
        if (action != null ? !action.equals(that.action) : that.action!= null) return false;
         if (time != null ? !time.equals(that.time) : that.time != null) return false;
        if (type != null ? !type.equals(that.type) : that.type != null) return false;

        return true;
    }

    public int hashCode() {
        int result;
        result = (type != null ? type.hashCode() : 0);
        result = 31 * result + (time != null ? time.hashCode() : 0);
        result = 31 * result + processId;
        result = 31 * result + parentProcessId;
        result = 31 * result + (parentProcess != null ? parentProcess.hashCode() : 0);
        result = 31 * result + (action != null ? action.hashCode() : 0);
        result = 31 * result + (process != null ? process.hashCode() : 0);
        return result;
    }
}
