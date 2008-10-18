package capture;

import java.util.Map;
import java.util.HashMap;

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
public class PostprocessorFactory {

    private static Postprocessor pp;

    public static Postprocessor getPostprocessor(String className) throws FactoryException {
        try {
            Object postprocessor = Class.forName(className).newInstance();
            if(postprocessor instanceof Postprocessor) {
                pp = (Postprocessor)postprocessor;
                return pp;
            } else {
                throw new FactoryException(className + " is not of type Postprocessor. Make sure it extends the abstract Postprocessor class.");
            }
        } catch (InstantiationException e) {
            throw new FactoryException("Unable to instantiate " + className + ".",e);
        } catch (IllegalAccessException e) {
            throw new FactoryException("Unable to instantiate " + className + ".",e);
        } catch (ClassNotFoundException e) {
            throw new FactoryException("Unable to find " + className + ". Make sure it is located in lib..",e);
        }
    }

    public static Postprocessor getDefaultPostprocessor() {
        pp = new DefaultPostprocessor();
        return pp;
    }

    public static Postprocessor getActivePostprocessor() {
        return pp;
    }
}
