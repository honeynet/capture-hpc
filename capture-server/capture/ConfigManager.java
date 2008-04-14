package capture;

import java.util.HashMap;
import java.util.Observable;

public class ConfigManager extends Observable {
	private HashMap<String, String> configOptions;
	private ConfigFile configFile;
	
	private ConfigManager()
	{
		configOptions = new HashMap<String, String>();
		configFile = new ConfigFile();
	}
	
    private static ConfigManager instance = new ConfigManager();   // No lazy initialization unless needed
 
	 
	public static ConfigManager getInstance()
	{
		return instance;
	}
	
	public void loadConfigurationFile()
	{	
		configFile.parse("config.xml");
	}

	public String getConfigOption(String option)
	{
		String value = null;
		if(configOptions.containsKey(option))
		{
			//System.out.println("Found option: " + option);
			value = configOptions.get(option);
		} else {
			;//System.out.println("NOT Found option: " + option);
		}
		return value;
	}
	
	public boolean containsOption(String option)
	{
		return configOptions.containsKey(option);
	}
	
	public boolean setConfigOption(String option, String value)
	{
		boolean set = false;
		if(configOptions.containsKey(option))
		{
			configOptions.remove(option);
			configOptions.put(option, value);
			this.setChanged();
			this.notifyObservers(option);
			set = true;
		}
		return set;
	}
	
	public boolean addConfigOption(String option, String value)
	{
		boolean added = false;
		if(!configOptions.containsKey(option))
		{
			configOptions.put(option, value);
			System.out.println("Option added: " + option + " => " + value);
			added = true;
		}
		return added;
	}
}
