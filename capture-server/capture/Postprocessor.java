package capture;

import java.io.*;
import java.util.Observer;
import java.util.Observable;

/**
 * PROJECT: Capture-HPC
 * DATE: Apr 13, 2008
 * FILE: Postprocessor
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
public abstract class Postprocessor implements Observer {

    /** This is the meat function of the Postprocessor. Whenever the state of the Url object changes
    * this function is called. Primarily one would want to check for the visited state of the URL and then
    * do some action.
    *
    */
    public abstract void update(Observable o, Object arg);


  /** Sets the configuration of the postprocessor. Allows the postprocessor to be configured via the
     * existing config.xml configuration file.
     *
     * @param configuration - from the CDATA element of the postprocessor xml tag of config.xml
     */
	abstract public void setConfiguration(String configuration);


    /** Helper function that allows the postprocessor to modify the queue of input urls
     *
     * @param url - url as per capture format (including prg and delay)
     */
    protected void addUrlToCaptureQueue(String url) {
        Element e = new Element();
        e.name = "url";
        e.attributes.put("add", "");
        e.attributes.put("url", url);
        EventsController.getInstance().notifyEventObservers(e);
    }

    /** Helper to get the size of the input url queue.
     *
     * @return size of the input url queue
     */
    public int getCaptureQueueSize() {
        return UrlsController.getInstance().getQueueSize();
    }

    /** function returns true if post processor is finished processing all update requests.
     * this flag is used to assess whether capture can safely quit.
     *
     * @return true if finsished processing all update requests, false otherwise
     */
    public abstract boolean processing();
}
