package pcap;

import java.util.*;
import java.util.zip.ZipInputStream;
import java.util.zip.ZipEntry;
import java.io.*;
import java.text.SimpleDateFormat;

import ffdetect.*;
import httpextractor.HTTPExtractor;
import httpextractor.HTTPExtractorException;
import data.HTTPRequest;
import dnsmapper.DNSMapper;
import capture.*;

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
public class PcapPostprocessor extends Postprocessor {

    public void DefaultPostprocessor() {

    }


    public void update(Observable o, Object arg) {
        Url url = (Url) o;

        if (url.getUrlState() == URL_STATE.VISITED) {
            String pcapFile = "log" + File.separator + new Random().nextLong() + ".pcap";
            String zipFile = "log" + File.separator + url.getUrlAsFileName() + ".zip";
            String httpRequestsFile = "log" + File.separator + url.getUrlAsFileName() + ".httpRequests";
            String dnsRequestsFile = "log" + File.separator + url.getUrlAsFileName() + ".dnsRequests";

            try {
                extractPcap(zipFile, pcapFile);

                extractHTTPRequests(pcapFile, httpRequestsFile);
                extractDNSLookups(pcapFile, dnsRequestsFile);


            } catch (IOException e) {
                System.out.println("Error post processing URL " + url.getEscapedUrl() + ": " + e.getMessage() + ".");
            }
        }
    }

    public void extractPcap(String zipFile, String outputFile) throws IOException {
        FileInputStream fis = new FileInputStream(zipFile);
        ZipInputStream zin = new ZipInputStream(new BufferedInputStream(fis));
        ZipEntry entry;
        while ((entry = zin.getNextEntry()) != null) {
            if (entry.getName().endsWith(".pcap")) {
                int BUFFER = 2048;
                int count;
                byte data[] = new byte[BUFFER];

                FileOutputStream fos = new
                        FileOutputStream(outputFile);
                BufferedOutputStream dest = new
                        BufferedOutputStream(fos, BUFFER);
                while ((count = zin.read(data, 0, BUFFER)) != -1) {
                    //System.out.write(x);
                    dest.write(data, 0, count);
                }
                dest.flush();
                dest.close();
                break;
            }
        }
        zin.close();
    }

    private void extractHTTPRequests(String pcapFile, String outputFile) {
        try {
            HTTPExtractor httpExtractor = new HTTPExtractor(pcapFile);
            FileWriter out = new FileWriter(outputFile, false);
            SimpleDateFormat sf = new SimpleDateFormat("dd/MM/yyyy HH:mm:ss.S");
            out.write("\"Date\",\"HTTP Request\"\n");
            List<HTTPRequest> httpRequests = httpExtractor.getHTTPRequests();
            for (Iterator<HTTPRequest> httpRequestIterator = httpRequests.iterator(); httpRequestIterator.hasNext();) {
                HTTPRequest httpRequest = httpRequestIterator.next();
                out.write("\"" + sf.format(httpRequest.getDate()) + "\",\"" + httpRequest.getRequestURL() + "\"\n");
            }
            out.flush();
            out.close();
        } catch (HTTPExtractorException e) {
            System.out.println("Unable to extract http requests: " + e.getMessage() + ".");
            e.printStackTrace();

        } catch (IOException e) {
            System.out.println("Unable to extract http requests: " + e.getMessage() + ".");
            e.printStackTrace();
        }
    }

    private void extractDNSLookups(final String pcapFile, final String outputFile) {


        Runnable dnsRunnable = new Runnable() {
            public void run() {
                try {

                    DNSMapper dnsMapper = new DNSMapper();
                    FileWriter out = new FileWriter(outputFile, false);
                    out.write("\"Domain Name\",\"IP Address\",\"Fast Flux\"\n");
                    Map<String, String> dnsMap = dnsMapper.getDNSMap(pcapFile);
                    List<String> domainNames = new ArrayList<String>();
                    domainNames.addAll(dnsMap.keySet());
                    long maxTTL = 1800;
                    int noThreads = 30;
                    Map<String, Boolean> ffDNSMap = FFDetect.isFastFlux(domainNames, maxTTL, noThreads);
                    for (Iterator<String> domainNamesIt = domainNames.iterator(); domainNamesIt.hasNext();) {
                        String domainName = domainNamesIt.next();
                        String ipAddress = dnsMap.get(domainName);
                        if (ffDNSMap.get(domainName) == null) {
                            //ff couldnt be determined
                            out.write("\"" + domainName + "\",\"" + ipAddress + "\",\"error\"\n");
                        } else {
                            //ff coudl be determined
                            boolean isFastFlux = ffDNSMap.get(domainName);
                            if (isFastFlux) {
                                out.write("\"" + domainName + "\",\"" + ipAddress + "\",\"FF\"\n");
                            } else {
                                out.write("\"" + domainName + "\",\"" + ipAddress + "\",\"No FF\"\n");
                            }
                        }
                    }
                    out.flush();
                    out.close();

                    Thread.sleep(1000);
                    new File(pcapFile).delete();
                } catch (Exception e) {
                    System.out.println("Unable to extract DNS requests: " + e.getMessage() + ".");
                    e.printStackTrace();
                }
            }
        };
        Thread dnsThread = new Thread(dnsRunnable);

        dnsThread.start();
        threads.add(dnsThread);
    }


    private Collection<Thread> threads = new ArrayList<Thread>();

    public static void main(String args[]) throws Exception {
        System.out.println("PROJECT: Capture-HPC\n" +
                "VERSION: 1.0\n" +
                "DATE: Sept 30, 2008\n" +
                "COPYRIGHT HOLDER: Victoria University of Wellington, NZ\n" +
                "AUTHORS:\n" +
                "\tChristian Seifert (christian.seifert@gmail.com)\n" +
                "\n" +
                "Capture-HPC is free software; you can redistribute it and/or modify\n" +
                "it under the terms of the GNU General Public License, V2 as published by\n" +
                "the Free Software Foundation.\n" +
                "\n" +
                "Capture-HPC is distributed in the hope that it will be useful,\n" +
                "but WITHOUT ANY WARRANTY; without even the implied warranty of\n" +
                "MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the\n" +
                "GNU General Public License for more details.\n" +
                "\n" +
                "You should have received a copy of the GNU General Public License\n" +
                "along with Capture-HPC; if not, write to the Free Software\n" +
                "Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301,USA\n\n");

        if (args.length != 2) {
            printUsage();
        } else {
            if (args[0].equals("-z")) {
                String zipFileName = args[1];
                if (zipFileName.endsWith(".zip")) {
                    zipFileName = zipFileName.substring(0, zipFileName.length() - 4);

                    String path = "." + File.separator;
                    if (zipFileName.lastIndexOf(File.separator) > -1) {
                        path = zipFileName.substring(0, zipFileName.lastIndexOf(File.separator)) + File.separator;
                        zipFileName = zipFileName.substring(zipFileName.lastIndexOf(File.separator) + 1);
                    }

                    System.out.println(path);
                    System.out.println(zipFileName);


                    PcapPostprocessor dp = new PcapPostprocessor();
                    dp.extractPcap(path + zipFileName + ".zip", path + zipFileName + ".pcap");
                    dp.extractHTTPRequests(path + zipFileName + ".pcap", path + zipFileName + ".http.log");
                    dp.extractDNSLookups(path + zipFileName + ".pcap", path + zipFileName + ".dns.log");

                    while(dp.processing()) {
                        Thread.sleep(1000);
                    }
                } else {
                    printUsage();
                }
            } else {
                printUsage();
            }
        }
    }

    private static void printUsage() {
        System.out.println("PcapProcessor extracts pcap from zip file and analyzes it.\n" +
                "It extracts all http requests and dns lookups and determines whether any domain names are part of \n" +
                "a fast flux domain. Two reports in csv format are written to disk:\n" +
                "Usage:\n" +
                "java -jar PcapPostprocessor.jar -z [zip filename]\n" +
                "For example: java -jar PcapPostprocessor.jar -z [zip filename]\n" +
                "\n");
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
        for (Iterator<Thread> threadIterator = threads.iterator(); threadIterator.hasNext();) {
            Thread thread = threadIterator.next();
            if (thread.isAlive()) {
                return true;
            }
        }
        return false;
    }
}