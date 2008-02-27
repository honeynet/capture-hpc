package capture;

import java.io.BufferedReader;
import java.io.BufferedWriter;
import java.io.File;
import java.io.FileInputStream;
import java.io.FileNotFoundException;
import java.io.FileOutputStream;
import java.io.IOException;
import java.io.InputStreamReader;
import java.io.OutputStreamWriter;
import java.io.UnsupportedEncodingException;

public class FileUrlTail implements Runnable {
	private String urlList;

	
	public FileUrlTail(String urlList)
	{
		this.urlList = urlList;
	}

	public void run() {
		try {
			BufferedReader in = new BufferedReader(new InputStreamReader(new FileInputStream(urlList), "UTF-8"));
	
			//BufferedReader in = new BufferedReader(new InputStreamReader(new FileInputStream(urlList)));
			while(true)
			{
				try {
					String line = in.readLine();
					if(line != null)
					{
                        if((line.length() > 0))
						{
							this.parseLine(line);
						}
					} else {
						try {
							Thread.sleep(500);
						} catch (InterruptedException e) {}
					}
				} catch (IOException e) {
					e.printStackTrace(System.out);
				}
			}
		} catch (FileNotFoundException e) {
			e.printStackTrace(System.out);
		} catch (UnsupportedEncodingException e1) {
			e1.printStackTrace(System.out);
		}
	}
	
	private void parseLine(String line)
	{
		line = line.trim();
		if(!line.startsWith("#"))
		{
			Element e = new Element();
			e.name = "url";		
			e.attributes.put("add", "");
			e.attributes.put("url", line);
			EventsController.getInstance().notifyEventObservers(e);
		}
	}
}