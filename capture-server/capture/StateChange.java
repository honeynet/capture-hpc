package capture;

import java.util.Date;
import java.text.SimpleDateFormat;
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
public abstract class StateChange {
    protected String type;
    protected int processId;
    protected String process;
    protected Date time;
    protected String action;
    
    public int getProcessId() {
        return processId;
    }

    static SimpleDateFormat sdf = new SimpleDateFormat("dd/MM/yyyy HH:mm:ss.S");
    
    public void setTime(String timeString) throws ParseException {

        time = sdf.parse(timeString);
    }

    public Date getTime() {
        return time;
    }

    public String getTimeString() {
        String timeString = sdf.format(getTime());
        return timeString;

    }

    public abstract String toCSV();

    public String getProcess() {
        return process;
    }

    public String getType() {
        return type;
    }
}
