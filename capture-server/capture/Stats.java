package capture;

import java.text.SimpleDateFormat;
import java.text.ParseException;
import java.util.Date;
import java.util.List;
import java.util.ArrayList;
import java.util.Iterator;

/**
 * PROJECT: Capture-HPC
 * DATE: Mar 12, 2008
 * FILE: Stats
 * COPYRIGHT HOLDER: Victoria University of Wellington, NZ
 * AUTHORS: Christian Seifert (christian.seifert@gmail.com)
 * <p/>
 * This file is part of Capture-HPC.
 * <p/>
 * Tiki is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 * <p/>
 * Tiki is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * <p/>
 * You should have received a copy of the GNU General Public License
 * along with Capture-HPC; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
 */
public class Stats {
    public static int visiting = 0;
    public static int visited = 0;
    public static int safe = 0;
    public static int malicious = 0;
    public static int urlError = 0;
    public static int vmRevert = 0;
    public static int clientInactivity = 0;
    public static int vmStalled = 0;
    private static List<Long> visitingTimes = new ArrayList<Long>();
    private static List<Long> firstStateChanges = new ArrayList<Long>();
    private static List<Long> revertTimes = new ArrayList<Long>();

    private static Date instantiationTime = new Date(System.currentTimeMillis());


    public static String getHeader() {
        return "\"time\",\"visiting\",\"visited\",\"safe\",\"malicious\",\"urlError\",\"vmRevert\",\"clientInactivity\",\"vmStalled\",\"avgRevertTime\",\"avgVisitingTime\",\"avgFirstStateChangeTime\",\"visitedUrlsPerDay\"";
    }

    public static String getStats() {
        return "\"" + currentTime() + "\",\"" + visiting + "\",\"" + visited + "\",\"" + safe + "\",\"" + malicious + "\",\"" + urlError + "\",\"" + vmRevert + "\",\"" + clientInactivity + "\",\"" + vmStalled + "\",\"" + getAvgRevertTime() +"\",\"" + getAvgUrlVisitingTime() + "\",\"" + getAvgFirstStateChangeTime() + "\",\"" + getVisitedUrlsPerDay() +"\"";
    }

    public static String currentTime() {
        long current = System.currentTimeMillis();
        Date currentDate = new Date(current);
        SimpleDateFormat sdf = new SimpleDateFormat("MMM d, yyyy h:mm:ss a");
        String strDate = sdf.format(currentDate);
        return strDate;
    }
    
    public static void addUrlVisitingTime(Date start, Date end, int classificationDelay) {
        long classificationDelayInMillis = classificationDelay * 1000;
        long visitingTime = (end.getTime() - start.getTime() - classificationDelayInMillis) / 1000;
        visitingTimes.add(visitingTime);
    }

    public static void addRevertTimeTime(Date start, Date end) {
        long revertTime = (end.getTime() - start.getTime()) / 1000;
        revertTimes.add(revertTime);
    }

    public static void addFirstStateChangeTime(Date firstStateChange, Date end, int classificationDelay) {
        long classificationDelayInMillis = classificationDelay * 1000;
        long secondsTillFirstStateChange = (firstStateChange.getTime() - (end.getTime() - classificationDelayInMillis)) / 1000;
        firstStateChanges.add(secondsTillFirstStateChange);
    }

    public static double getAvgUrlVisitingTime() {
        if (visitingTimes.size() > 0) {
            long totalVisitingTime = 0;
            for (Iterator<Long> visitingTimeIt = visitingTimes.iterator(); visitingTimeIt.hasNext();) {
                totalVisitingTime = totalVisitingTime + visitingTimeIt.next();
            }
            return totalVisitingTime / (1.0*visitingTimes.size());
        } else {
            return 0;
        }
    }

    public static double getAvgRevertTime() {
        if (revertTimes.size() > 0) {
            long totalRevertTime = 0;
            for (Iterator<Long> revertTimeIt = revertTimes.iterator(); revertTimeIt.hasNext();) {
                totalRevertTime  = totalRevertTime  + revertTimeIt.next();
            }
            return totalRevertTime  / (1.0*revertTimes.size());
        } else {
            return 0;
        }
    }
    
    public static double getAvgFirstStateChangeTime() {
        if (firstStateChanges.size() > 0) {


            long totalFirstStateChangeTime = 0;
            for (Iterator<Long> firstStateChangesIt = firstStateChanges.iterator(); firstStateChangesIt.hasNext();) {
                totalFirstStateChangeTime = totalFirstStateChangeTime + firstStateChangesIt.next();
            }
            return totalFirstStateChangeTime / (1.0*firstStateChanges.size());
        } else {
            return 0;
        }
    }


    public static long getVisitedUrlsPerDay() {
        Date currentTime = new Date(System.currentTimeMillis());
        long secondsPassed = (currentTime.getTime() - instantiationTime.getTime()) / 1000;
        long secondsPerDay = 60 * 60 * 24;
        long visitedUrlsPerDay = ((safe+malicious+urlError) * secondsPerDay)/secondsPassed;
        return visitedUrlsPerDay;
        
    }
}
