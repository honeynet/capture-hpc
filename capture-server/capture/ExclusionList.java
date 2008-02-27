package capture;

import java.io.BufferedReader;
import java.io.File;
import java.io.FileInputStream;
import java.io.FileNotFoundException;
import java.io.FileOutputStream;
import java.io.FileReader;
import java.io.IOException;
import java.io.InputStreamReader;
import java.io.OutputStreamWriter;
import java.util.LinkedList;

public class ExclusionList {
	private String monitor;
	private String file;
	private LinkedList<Element> exclusionElements;
	
	public ExclusionList(String monitor, String file) {
		this.monitor = monitor;
		this.file = file;
		exclusionElements = new LinkedList<Element>();
	}
	
	public boolean parseExclusionList() {
		boolean parsed = true;
		try {
			
			BufferedReader in = new BufferedReader(new InputStreamReader(new FileInputStream(file), "UTF-8"));
			String line = "";
			int num = 0;
			while((line = in.readLine()) != null) 
			{
				num++;
				if(line.length() > 0 && !line.startsWith("#")) 
				{
					String[] tok = line.split("\t");
					if(tok.length == 4) 
					{
						Element e = new Element();
						e.name = monitor + "-exclusion";
						e.attributes.put("excluded", tok[0]);
						e.attributes.put("action", tok[1]);
						e.attributes.put("subject", tok[2]);
						e.attributes.put("object", tok[3]);
						exclusionElements.add(e);
					} else {
						System.out.println("ExclusionList: WARNING Error in exclusion list, line " + num + " in " + file);
					}
				}
			}
		} catch (FileNotFoundException e) {
			System.out.println("ExclusionList: " + monitor + " - " + file + ": File not found");
			parsed = false;
		} catch (IOException e) {
			e.printStackTrace(System.out);
			parsed = false;
		}
		return parsed;
	}
	
	public LinkedList<Element> getExclusionListElements()
	{
		return exclusionElements;
	}
}
