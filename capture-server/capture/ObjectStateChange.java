package capture;

import java.util.List;
import java.util.ArrayList;
import java.text.ParseException;

/**
 * PROJECT: Capture-HPC
 * DATE: Apr 28, 2008
 * FILE: StateChange
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
public class ObjectStateChange extends StateChange {

    private String object1;
    private String object2;

    public ObjectStateChange(String type, String time, String processId, String process, String action, String object1, String object2) throws ParseException {
        this.type = type;
        setTime(time);
        this.processId = Integer.parseInt(processId);
        this.process = process;
        this.action = action;
        this.object1 = object1;
        this.object2 = object2;
    }

    public boolean equals(Object o) {
        if (this == o) return true;
        if (!(o instanceof ObjectStateChange)) return false;

        ObjectStateChange that = (ObjectStateChange) o;

        if (processId != that.processId) return false;
        if (action != null ? !action.equals(that.action) : that.action != null) return false;
        if (object1 != null ? !object1.equals(that.object1) : that.object1 != null) return false;
        if (object2 != null ? !object2.equals(that.object2) : that.object2 != null) return false;
        if (process != null ? !process.equals(that.process) : that.process != null) return false;
        if (time != null ? !time.equals(that.time) : that.time != null) return false;
        if (type != null ? !type.equals(that.type) : that.type != null) return false;

        return true;
    }

    public int hashCode() {
        int result;
        result = (type != null ? type.hashCode() : 0);
        result = 31 * result + (time != null ? time.hashCode() : 0);
        result = 31 * result + processId;
        result = 31 * result + (process != null ? process.hashCode() : 0);
        result = 31 * result + (action != null ? action.hashCode() : 0);
        result = 31 * result + (object1 != null ? object1.hashCode() : 0);
        result = 31 * result + (object2 != null ? object2.hashCode() : 0);
        return result;
    }

    public String toCSV() {
        return "\""+type+"\",\""+getTimeString()+"\",\""+processId+"\",\""+process+"\",\""+action+"\",\""+object1+"\",\""+object2+"\"\n";

    }
}
