package capture;

import java.io.BufferedReader;
import java.io.IOException;
import java.io.InputStreamReader;

public class CommandPrompt implements Runnable {
	private BufferedReader in;
	
	public CommandPrompt(Server s)
	{
		in = new BufferedReader(new InputStreamReader(System.in));
	}

	public void run() {
		while(true)
		{
			try {
				String line = in.readLine();	
				if(line != null && line.length() > 0)
				{
					this.parseLine(line);
				}
			} catch (IOException e) {
				e.printStackTrace();
			}
		}
	}
	
	private void parseLine(String line)
	{
		String[] token = line.split(" ");
		if(token[0].equals("\\url") && (token.length >= 3))
		{
			if(token[1].equals("add"))
			{
				String url = line.substring(9);
				Element e = new Element();
				e.name = "url";
				e.attributes.put("add", "");
				e.attributes.put("url", url);
				EventsController.getInstance().notifyEventObservers(e);	
			} else if(token[1].equals("remove")) {
				String url = line.substring(12);
				Element e = new Element();
				e.name = "url";
				e.attributes.put("remove", "");
				e.attributes.put("url", url);
				EventsController.getInstance().notifyEventObservers(e);	
			}
		} else if(token[0].equals("\\exclusion-list")) {
			if(token.length >= 4) {
				Element e = new Element();
				e.name = "exclusion";
				e.attributes.put(token[1], "");
				e.attributes.put("monitor", token[2]);
				e.attributes.put("file", token[3]);
				EventsController.getInstance().notifyEventObservers(e);	
			}
		} else if(token[0].equals("\\quit")) {
			System.exit(1);
		}
	}
}
