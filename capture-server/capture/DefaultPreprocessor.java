package capture;

/**
 * PROJECT: Capture-HPC
 * DATE: Apr 13, 2008
 * FILE: DefaultPreprocessor
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
public class DefaultPreprocessor extends Preprocessor {

    /* The meat of the preprocessor. Urls are passed from the input file to the preprocessor.
    * If URLs shall be processed by Capture, the preprocessor must call the addUrlToCaptureQueue function.
    *
    * @param url - url as per capture format (including prg and delay)
    */
    public void preprocess(String url) {
        //just pass through
        addUrlToCaptureQueue(url);

    }

    /* Sets the configuration of the preprocessor. Allows the preprocessor to be configured via the
    * existing config.xml configuration file.
    *
    * @param configuration - from the CDATA element of the preprocessor xml tag of config.xml
    */
    public void setConfiguration(String configuration) {
        //no custom config here
    }
}
