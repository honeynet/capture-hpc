package capture;

import java.util.HashMap;
import java.util.LinkedList;

public class Element {
	public String name;
	public HashMap<String, String> attributes;
	public String data;
	public LinkedList<Element> childElements;
	public Element parent;
	
	public Element()
	{
		attributes = new HashMap<String, String>();
		childElements = new LinkedList<Element>();
		parent = null;
		data = "";
	}

}
