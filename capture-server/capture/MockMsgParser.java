package capture;

import org.xml.sax.*;
import org.xml.sax.helpers.XMLReaderFactory;
import org.xml.sax.helpers.DefaultHandler;

import java.io.StringReader;
import java.io.IOException;

public class MockMsgParser extends DefaultHandler {
    private Element currentElement;
    private MockClient client;

    public MockMsgParser(MockClient client) {
        this.client = client;
    }

    public void createEl(String msg) throws SAXException, IOException {
        XMLReader xr = XMLReaderFactory.createXMLReader();
        xr.setContentHandler(this);
        xr.setErrorHandler(this);
        xr.parse(new InputSource(new StringReader(msg)));
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
            if(name.equals("connect")) {
				client.handleConnectEvent(currentElement);
			} else if(name.equals("ping")) {
	        	client.handlePing();
			} else if(name.equals("visit")) {
				client.handleVisitEvent(currentElement);
            } else if(name.equals("file-accept")) {
                client.handleFileEvent(currentElement);
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
