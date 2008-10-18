package capture;

import java.util.*;
import java.util.zip.ZipInputStream;
import java.util.zip.ZipEntry;
import java.io.*;
import java.text.SimpleDateFormat;

/**
 * PROJECT: Capture-HPC
 * DATE: Apr 13, 2008
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
public class DefaultPostprocessor extends Postprocessor {

    public void DefaultPostprocessor() {
        
    }
    

    public void update(Observable o, Object arg) {
        //nothing
    }

    /* Sets the configuration of the postprocessor. Allows the postprocessor to be configured via the
    * existing config.xml configuration file.
    *
    * @param configuration - from the CDATA element of the postprocessor xml tag of config.xml
    */
    public void setConfiguration(String configuration) {
        //no custom config here
    }

    public boolean processing() {
        return true; //bc it doesnt really process anything.
    }
}