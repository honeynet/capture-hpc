package capture;

import org.xml.sax.XMLReader;
import org.xml.sax.InputSource;
import org.xml.sax.SAXException;
import org.xml.sax.helpers.XMLReaderFactory;

import java.net.Socket;
import java.net.URLEncoder;
import java.io.*;
import java.util.*;
import java.text.SimpleDateFormat;
import java.text.DateFormat;

public class MockClient implements Runnable {
    private String serverListenAddress;
    private int uniqueId;
    private int vmUniqueId;
    private DataOutputStream output = null;
    private List maliciousURLsURIEncoded = new ArrayList();
    private boolean sendFile = true;
    private String visitEndResponse;

    public MockClient(String serverListenAddress, int uniqueId, int vmUniqueId) {
        this.serverListenAddress = serverListenAddress;
        this.uniqueId = uniqueId;
        this.vmUniqueId = vmUniqueId;

        try {
            maliciousURLsURIEncoded.add(URLEncoder.encode("http://www.google.fr", "UTF-8"));
        } catch (UnsupportedEncodingException e) {
            e.printStackTrace(System.out);
        }
    }

    public void connect() {
        Thread mockClientThread = new Thread(this,"MockClientThread");
        mockClientThread.start();
    }

    public void run() {
        System.out.println("Client connecting ... ");

        Socket client = null;

        DataInputStream input = null;

        MockMsgParser parser = new MockMsgParser(this);

        try {
            client = new Socket(serverListenAddress,7070);

            input = new DataInputStream(client.getInputStream());
            BufferedReader inputReader = new BufferedReader(new InputStreamReader(input));
            output = new DataOutputStream(client.getOutputStream());

            while(client.isConnected()) {
                StringBuffer msg = new StringBuffer();
                char c = (char) inputReader.read();
                while(c!=0 && c!=65535 && client.isConnected()) {
                    msg.append(c);
                    c = (char) inputReader.read();
                }
                if(c==0) {
                parser.createEl(msg.toString());
                }
            }


        } catch (IOException e) {
            e.printStackTrace(System.out);
        } catch (SAXException e) {
            e.printStackTrace(System.out);
        } finally {
            if(input!=null) {
                try { input.close(); } catch (IOException io) { io.printStackTrace(System.out); }
            }
            if(output!=null) {
               try { output.close(); } catch (IOException io) { io.printStackTrace(System.out); }
            }
            if(client!=null) {
                try { client.close(); } catch (IOException io) { io.printStackTrace(System.out); }
            }
        }
    }

    public void handleConnectEvent(Element currentElement) {
        System.out.println("Mock Client: Connect");
        String connectResponse = "<connect server=\"2.0\" vm-server-id=\""+uniqueId+"\" vm-id=\""+vmUniqueId+"\"/>\n";
        try {
            output.writeBytes(connectResponse);
            System.out.println("Mock Client: Sent " + connectResponse);
        } catch (IOException e) {
            e.printStackTrace(System.out);
        }
    }

    public void handlePing() {
        System.out.println("Mock Client: Ping");
        String pong = "<Pong/>\n";
        try {
            output.writeBytes(pong);
            System.out.println("Mock Client: Sent " + pong);
        } catch (IOException e) {
            e.printStackTrace(System.out);
        }
    }


    public void handleVisitEvent(Element currentElement) {
        try {
            System.out.println("Mock Client: Visit Event");

            String identifier = currentElement.attributes.get("identifier");
            String program = currentElement.attributes.get("program");
            int time = Integer.parseInt(currentElement.attributes.get("time"));

            DateFormat dfm = new SimpleDateFormat("d/M/yyyy H:m:s.S");
            //dfm.setTimeZone(TimeZone.getTimeZone("Europe/Zurich"));
            String startTime = dfm.format(new Date(System.currentTimeMillis()));

            String visitStartResponse = "<visit-event type=\"start\" time=\""+startTime+"\" malicious=\"\" major-error-code=\"\" minor-error-code=\"\">";

            List items = currentElement.childElements;
            for (Iterator iterator = items.iterator(); iterator.hasNext();) {
                Element element = (Element) iterator.next();
                String url = element.attributes.get("url");
                String startTimeUrl = dfm.format(new Date(System.currentTimeMillis()));
                visitStartResponse += "<item url=\"" + url + "\" time=\"" +  startTimeUrl + "\" major-error-code=\"\" minor-error-code=\"\"/>";
            }
            visitStartResponse += "</visit-event>\n";
            output.writeBytes(visitStartResponse);
            System.out.println("Mock Client: Sent " + visitStartResponse);


            Thread.sleep(time * 1000);

            String endTime = dfm.format(new Date(System.currentTimeMillis()));

            items = currentElement.childElements;
            String visitedUrls = "";
            String malicious = "0";
            int i = 0;
            for (Iterator iterator = items.iterator(); iterator.hasNext();) {
                Element element = (Element) iterator.next();
                String url = element.attributes.get("url");
                if(maliciousURLsURIEncoded.contains(url)) {
                    malicious = "1";
                    //send a sample event change
                    String eventTime = dfm.format(new Date(System.currentTimeMillis()));
                    String sampleEvent = "<system-event time=\""+eventTime+"\" type=\"file\" process=\"C:\\WINDOWS\\explorer.exe\" action=\"Write\" object=\"C:\\tmp\\TEST.exe\"/>\n";
                    output.writeBytes(sampleEvent);
                }
                String endTimeUrl = dfm.format(new Date(System.currentTimeMillis()));
                visitedUrls += "<item url=\"" + url + "\" time=\"" +  endTimeUrl + "\" major-error-code=\"\" minor-error-code=\"\"/>";
            }

            //268435729
            visitEndResponse = "<visit-event type=\"finish\" time=\""+endTime+"\" malicious=\""+malicious+"\" major-error-code=\"\" minor-error-code=\"\">";
            visitEndResponse += visitedUrls;
            visitEndResponse += "</visit-event>\n";

            if(sendFile) {
                String fileSendRequest = "<file name=\"C:\\Program Files\\capture\\capture_2292007_1048.zip\" size=\"33\" type=\"zip\"/>\n";
                output.writeBytes(fileSendRequest);
                System.out.println("Mock Client: Sent " + fileSendRequest);


            } else {


            output.writeBytes(visitEndResponse);
            System.out.println("Mock Client: Sent " + visitEndResponse);
            }

        } catch (IOException e) {
            e.printStackTrace(System.out);

        } catch (InterruptedException e) {
            e.printStackTrace(System.out);
        }

    }

    public void handleFileEvent(Element currentElement) {
        try {
            String name = currentElement.attributes.get("name");
            System.out.println("Mock Client: File Accept Msg " + name);

            String filePart = "<part name=\"C:\\Program Files\\capture\\capture_2292007_1048.zip\" part-start=\"0\" encoding=\"base64\" part-end=\"33\">evQ+Z0tTd3gsaCx64jGhulbMTF3QBzHAg4bmBCTNqD</part>\n";
            output.writeBytes(filePart);
            System.out.println("Mock Client: Sent " + filePart);

            String fileFinish = "<file-finished name=\"C:\\Program Files\\capture\\capture_2292007_1048.zip\" size=\"33\"/>\n";
            output.writeBytes(fileFinish);
            System.out.println("Mock Client: Sent " + fileFinish);

            output.writeBytes(visitEndResponse);
            System.out.println("Mock Client: Sent " + visitEndResponse);
        } catch (IOException e) {
            e.printStackTrace(System.out);  
        }
    }
}
