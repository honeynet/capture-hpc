package capture;

import java.io.File;
import java.io.IOException;
import java.util.Observable;
import java.util.Observer;

import javax.xml.*;
import javax.xml.validation.*;
import javax.xml.parsers.*;
import javax.xml.transform.*;
import javax.xml.transform.dom.*;
import javax.xml.transform.stream.*;
 
import org.w3c.dom.*;
import org.xml.sax.*;


public class ConfigFile implements Observer, ErrorHandler {
	private Document configDocument;
	private boolean loaded;
	
	public ConfigFile()
	{
		loaded = false;
	}
	
	public void parse(String file)
	{
		ConfigManager.getInstance().addObserver(this);
		DocumentBuilderFactory factory =
            DocumentBuilderFactory.newInstance();
		 factory.setNamespaceAware(true);
		 //factory.

        try {
    		factory.newDocumentBuilder();
    		DocumentBuilder builder = factory.newDocumentBuilder();
			configDocument = builder.parse( file );

		    /* Validate the config file */
		    SchemaFactory factory2 = SchemaFactory.newInstance(XMLConstants.W3C_XML_SCHEMA_NS_URI);
		    Source schemaFile = new StreamSource(new File("config.xsd"));
		    Schema schema = factory2.newSchema(schemaFile);
		    Validator validator = schema.newValidator(); 
		    validator.setErrorHandler(this);

		    try {
		    	System.out.println("Validating " + file + " ...");
		        validator.validate(new DOMSource(configDocument));
		        System.out.println(file + " successfully validated");
		    } catch (SAXException e) {
		    	e.printStackTrace(System.out);
		    	System.exit(1);
		    }
			
			this.parseGlobalElements();
			this.parseDatabaseElements();
			this.parseExclusionListElements();
			this.parseServerElements();
			System.out.println("PARSING PREPROCESSOR");
			this.parsePreprocessorElements();
            System.out.println("PARSING POSTPROCESSOR");
			this.parsePostprocessorElements();
            loaded = true;
		} catch (SAXException e) {
			e.printStackTrace(System.out);
	    	System.exit(1);
		} catch (IOException e) {
			e.printStackTrace(System.out);
	    	System.exit(1);
		} catch (ParserConfigurationException e) {
			e.printStackTrace(System.out);
	    	System.exit(1);
		}
	}
	
	private void parseExclusionListElements()
	{
		NodeList exclusionLists = configDocument.getElementsByTagName("exclusion-list");
		for(int i = 0; i < exclusionLists.getLength(); i++)
		{
			Element e = new Element();
			NamedNodeMap serverMap = exclusionLists.item(i).getAttributes();
			e.name = "exclusion-list";
			e.attributes.put("add-list", "");
			e.attributes.put("file", serverMap.getNamedItem("file").getNodeValue());
			e.attributes.put("monitor", serverMap.getNamedItem("monitor").getNodeValue());
			
			EventsController.getInstance().notifyEventObservers(e);
		}
	}
	
	public void parseGlobalElements()
	{
		NodeList globalList = configDocument.getElementsByTagName("global");
		
		for(int i = 0; i < globalList.getLength(); i++)
		{
			NamedNodeMap attributesMap = globalList.item(i).getAttributes();
			for(int k = 0; k < attributesMap.getLength(); k++)
			{
				String option = attributesMap.item(k).getNodeName();
				String value = attributesMap.item(k).getNodeValue();
				ConfigManager.getInstance().addConfigOption(option, value);
			}
		}
	}

	public void parseDatabaseElements()
	{
		NodeList databaseList = configDocument.getElementsByTagName("database");
		for(int i = 0; i < databaseList.getLength(); i++)
		{
			NamedNodeMap attributesMap = databaseList.item(i).getAttributes();
			for(int k = 0; k < attributesMap.getLength(); k++)
			{
				String option = attributesMap.item(k).getNodeName();
				String value = attributesMap.item(k).getNodeValue();
				ConfigManager.getInstance().addConfigOption(option, value);
			}
		}
	}
        	
	public void parsePreprocessorElements() {
        NodeList preprocessor = configDocument.getElementsByTagName("preprocessor");
        Node n = preprocessor.item(0);

        if(n!=null) {
	    System.out.println("n is not null");
            String value = n.getAttributes().getNamedItem("classname").getNodeValue();
            ConfigManager.getInstance().addConfigOption("preprocessor-classname", value);

            String configValue = n.getFirstChild().getNextSibling().getNodeValue();
	    System.out.println("CONFIG: " + configValue);
            ConfigManager.getInstance().addConfigOption("preprocessor-configuration", configValue);
        } else {
	    System.out.println("n is null");
	}
    }

     public void parsePostprocessorElements() {
        NodeList postprocessor = configDocument.getElementsByTagName("postprocessor");
        Node n = postprocessor.item(0);

        if(n!=null) {
	    System.out.println("n is not null");
            String value = n.getAttributes().getNamedItem("classname").getNodeValue();
            ConfigManager.getInstance().addConfigOption("postprocessor-classname", value);

            String configValue = n.getFirstChild().getNextSibling().getNodeValue();
	    System.out.println("CONFIG: " + configValue);
            ConfigManager.getInstance().addConfigOption("postprocessor-configuration", configValue);
        } else {
	    System.out.println("n is null");
	}
    }

    public void parseServerElements()
	{
		NodeList globalList = configDocument.getElementsByTagName("virtual-machine-server");
        for(int i = 0; i < globalList.getLength(); i++)
		{
			Element event = new Element();
			NamedNodeMap serverMap = globalList.item(i).getAttributes();
			event.name = "virtual-machine-server";
			event.attributes.put("add", "");
			event.attributes.put("type", serverMap.getNamedItem("type").getNodeValue());
			event.attributes.put("address", serverMap.getNamedItem("address").getNodeValue());
			event.attributes.put("port", serverMap.getNamedItem("port").getNodeValue());
			event.attributes.put("username", serverMap.getNamedItem("username").getNodeValue());
			event.attributes.put("password", serverMap.getNamedItem("password").getNodeValue());
			
			NodeList vmElements = globalList.item(i).getChildNodes();
			for(int k = 0; k < vmElements.getLength(); k++)
			{
				if(vmElements.item(k).getNodeName().equals("virtual-machine"))
				{
					Element childEvent = new Element();
					NamedNodeMap vmMap = vmElements.item(k).getAttributes();
					childEvent.name = "virtual-machine";
					childEvent.attributes.put("vm-path", vmMap.getNamedItem("vm-path").getNodeValue());
					childEvent.attributes.put("username", vmMap.getNamedItem("username").getNodeValue());
					childEvent.attributes.put("password", vmMap.getNamedItem("password").getNodeValue());
					childEvent.attributes.put("client-path", vmMap.getNamedItem("client-path").getNodeValue());
					event.childElements.add(childEvent);
				}
			}
			EventsController.getInstance().notifyEventObservers(event);
		}
	}
	

	public void update(Observable sender, Object option) {
		if(loaded)
		{
			if(((String) option).equals("server"))
			{
				
			} else {
				NodeList globalList = configDocument.getElementsByTagName("global");
				for(int i = 0; i < globalList.getLength(); i++)
				{
					NamedNodeMap optionsMap = globalList.item(i).getAttributes();
					Node nodeOption = optionsMap.getNamedItem((String) option);
			
					nodeOption.setNodeValue((String) ConfigManager.getInstance().getConfigOption((String) option));
					this.saveConfigFile();
				}
			}
		}
	}
	
	public void saveConfigFile()
	{
		try {
			TransformerFactory tf = TransformerFactory.newInstance();		        
			Transformer t = tf.newTransformer();		        
			t.transform(new DOMSource(configDocument), new StreamResult(new File("config.xml")));
		} catch (TransformerConfigurationException e) {
			e.printStackTrace(System.out);
		} catch (TransformerException e) {
			e.printStackTrace(System.out);
		}
	}

   	//parse config file for database info
	public void parseDatabaseInfo(String file)
	{
		ConfigManager.getInstance().addObserver(this);
		DocumentBuilderFactory factory =
            DocumentBuilderFactory.newInstance();
		 factory.setNamespaceAware(true);
		 //factory.

        try {
    		factory.newDocumentBuilder();
    		DocumentBuilder builder = factory.newDocumentBuilder();
			configDocument = builder.parse( file );

		    /* Validate the config file */
		    SchemaFactory factory2 = SchemaFactory.newInstance(XMLConstants.W3C_XML_SCHEMA_NS_URI);
		    Source schemaFile = new StreamSource(new File("config.xsd"));
		    Schema schema = factory2.newSchema(schemaFile);
		    Validator validator = schema.newValidator(); 
		    validator.setErrorHandler(this);

		    try {
		    	System.out.println("Validating " + file + " ...");
		        validator.validate(new DOMSource(configDocument));
		        System.out.println(file + " successfully validated");
		    } catch (SAXException e) {
		    	e.printStackTrace(System.out);
		    	System.exit(1);
		    }
			
			this.parseDatabaseElements();
            loaded = true;
		} catch (SAXException e) {
			e.printStackTrace(System.out);
	    	System.exit(1);
		} catch (IOException e) {
			e.printStackTrace(System.out);
	    	System.exit(1);
		} catch (ParserConfigurationException e) {
			e.printStackTrace(System.out);
	    	System.exit(1);
		}
	}


	@Override
	public void error(SAXParseException exception) throws SAXException {
		System.out.println("Error occured during parsing of config.xml\n");
		System.out.println(exception.getMessage() + " Line: " +
				exception.getLineNumber() + " Column: " + exception.getColumnNumber());
        exception.printStackTrace(System.out);
        System.exit(1);
	}

	@Override
	public void fatalError(SAXParseException exception) throws SAXException {
		System.out.println("Fatal Error occured during parsing of config.xml\n");
		System.out.println(exception.getMessage());
        exception.printStackTrace(System.out);
        System.exit(1);
	}

	@Override
	public void warning(SAXParseException exception) throws SAXException {
		System.out.println("Warning occured during parsing of config.xml\n");
		System.out.println(exception.getMessage());
        exception.printStackTrace(System.out);
    }
}
