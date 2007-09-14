package capture;

import java.io.BufferedReader;
import java.io.IOException;
import java.io.InputStreamReader;
import java.io.StringReader;
import java.net.SocketException;
import java.util.Calendar;
import java.util.HashMap;

import org.xml.sax.Attributes;
import org.xml.sax.InputSource;
import org.xml.sax.SAXException;
import org.xml.sax.XMLReader;
import org.xml.sax.helpers.DefaultHandler;
import org.xml.sax.helpers.XMLReaderFactory;


public class ClientEventController extends DefaultHandler implements Runnable {
	private Client client;
	private HashMap<String,ClientFileReceiver> clientFileReceivers;
	private boolean running;
	private Element currentElement;
	
	public ClientEventController(Client c)
	{
		client = c;
		currentElement = null;
		clientFileReceivers = new HashMap<String, ClientFileReceiver>();
		this.client.send("<file-accept />");
	}
	
	public void run() {
		running = true;
		String buffer = new String();
        try {
            char[] readBuffer = new char[2048];
            int read = 0;
            //InputStreamReader in = new InputStreamReader(client.getClientSocket().getInputStream(), "UTF-8");
            BufferedReader in = new BufferedReader(new InputStreamReader(client.getClientSocket().getInputStream(), "UTF-8"));       
            //String buffer = "";
            
            while(((buffer = in.readLine()) != null))
            {
            	
            	//System.out.println("Buffer length: " + read);
    			if(client.getVirtualMachine() != null)
    			{
    				client.getVirtualMachine().setLastContact(Calendar.getInstance().getTimeInMillis());
    			}
    			/*
    			for(int i = 0; i < read; i++) 
    			{
    				if(readBuffer[i] == '\r')
    				{
    					if(readBuffer[i+1] == '\n')
    					{
    						XMLReader xr = XMLReaderFactory.createXMLReader();
    		 	            xr.setContentHandler(this);
    		 	            xr.setErrorHandler(this);
    		     			xr.parse(new InputSource(new StringReader(buffer) ));
    		     			System.out.println("At: " + (i+1) + " of " + read );
    		     			buffer = "";
    					}
    				}
    				buffer += readBuffer[i];		
    			}
    			*/
    			//buffer = buffer.substring(0, buffer.length());
    			//System.out.println(buffer);
    			//buffer = buffer.trim();
    			System.out.println(buffer);
    			
    			if(buffer.length() > 0)
    			{
    				XMLReader xr = XMLReaderFactory.createXMLReader();
    				xr.setContentHandler(this);
    				xr.setErrorHandler(this);
    				xr.parse(new InputSource(new StringReader(buffer) ));
    			}
    			
            }
        } catch (SocketException e) {
        	String where = "[unknown server]";
        	if(client.getVirtualMachine() != null)
        		where = client.getVirtualMachine().getLogHeader();
        	System.err.println(where + " " + e.getMessage());   		
        } catch (IOException e) {
        	String where = "[unknown server]";
        	if(client.getVirtualMachine() != null)
        		where = client.getVirtualMachine().getLogHeader();
        	System.err.println(where + " " + e.getMessage());
        	System.err.println(where + "\nIOException: Buffer=" +
        			buffer + "\n\n" + e.toString());      		
		} catch (SAXException e) {
        	String where = "[unknown server]";
        	if(client.getVirtualMachine() != null)
        		where = client.getVirtualMachine().getLogHeader();
        	System.err.println(where + " " + e.getMessage());
			System.err.println(where + "\nSAXException: Buffer=" +  
					buffer + "\n\n" + e.toString());      		
		} finally {
			client.setClientState(CLIENT_STATE.DISCONNECTED);
			running = false;
		}
        
	}
	
	public boolean isRunning()
	{
		return this.running;
	}
	
	private Element constructElement(Element parent, String name, Attributes atts)
	{
		Element e = new Element();
		e.name = name;
		for(int i = 0; i < atts.getLength(); i++)
		{
			//System.out.println(atts.getLocalName(i) + " -> " + atts.getValue(i));
			e.attributes.put(atts.getLocalName(i), atts.getValue(i));
		}
		if(parent != null)
		{
			parent.childElements.add(e);
			e.parent = parent;
		}
		
		return e;
	}
	
	public void startElement (String uri, String name,
		      String qName, Attributes atts)
	{
		currentElement = this.constructElement(currentElement, name, atts);
	}	
	
	public void endElement (String uri, String name, String qName)
	{
		//long startTime = System.nanoTime();
		if(currentElement.parent == null)
		{
			if(name.equals("system-event"))
			{
				client.parseEvent(currentElement);
			} else if(name.equals("connect")) {
				client.parseConnectEvent(currentElement);
			} else if(name.equals("pong")) {
	        	String where = "[unknown server]";
	        	if(client.getVirtualMachine() != null)
	        		where = client.getVirtualMachine().getLogHeader();
	        	System.out.println(where + " Got pong");
			} else if(name.equals("visit-event")) {
				System.out.println("Got visit event");
				client.parseVisitEvent(currentElement);
			} else if(name.equals("client")) {
				
			} else if(name.equals("file")) {
				String where = "[unknown server]";
	        	if(client.getVirtualMachine() != null)
	        		where = client.getVirtualMachine().getLogHeader();
	        	System.out.println(where + " Downloading file");
				ClientFileReceiver file = new ClientFileReceiver(client, currentElement);
				String fileName = currentElement.attributes.get("name");
				if(!clientFileReceivers.containsKey(fileName))
				{
					clientFileReceivers.put(fileName, file);
				} else {
					System.err.println("ClientFileReceiver: ERROR already downloading file - " + fileName);
				}
			} else if(name.equals("part")) {
				String fileName = currentElement.attributes.get("name");
				if(clientFileReceivers.containsKey(fileName))
				{
					ClientFileReceiver file = clientFileReceivers.get(fileName);
					file.receiveFilePart(currentElement);
				} else {
					System.err.println("ClientFileReceiver: ERROR receiver not found for file - " + fileName);
				}
			} else if(name.equals("file-finished")) {
	        	String where = "[unknown server]";
	        	if(client.getVirtualMachine() != null)
	        		where = client.getVirtualMachine().getLogHeader();
	        	System.out.println(where + " Finished downloading file");
				String fileName = currentElement.attributes.get("name");
				if(clientFileReceivers.containsKey(fileName))
				{
					ClientFileReceiver file = clientFileReceivers.get(fileName);
					file.receiveFileEnd(currentElement);
					clientFileReceivers.remove(fileName);
				} else {
					System.err.println("ClientFileReceiver: ERROR receiver not found for file - " + fileName);
				}
			}
			currentElement = null;
		} else {
			currentElement = currentElement.parent;
		}
	}
	
	public void characters (char ch[], int start, int length)
    {
		currentElement.data = new String(ch, start, length);
		currentElement.data.trim();
    }
}