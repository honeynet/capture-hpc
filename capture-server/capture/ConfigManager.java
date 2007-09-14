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
		//configLock.readLock().lock();
		if(configOptions.containsKey(option))
		{
			//System.out.println("Found option: " + option);
			value = configOptions.get(option);
		} else {
			;//System.out.println("NOT Found option: " + option);
		}
		//configLock.readLock().unlock();
		return value;
	}
	
	public boolean containsOption(String option)
	{
		return configOptions.containsKey(option);
	}
	
	public boolean setConfigOption(String option, String value)
	{
		boolean set = false;
		//configLock.writeLock().lock();
		if(configOptions.containsKey(option))
		{
			configOptions.remove(option);
			configOptions.put(option, value);
			this.setChanged();
			this.notifyObservers(option);
			set = true;
		}
		//configLock.writeLock().unlock();
		return set;
	}
	
	public boolean addConfigOption(String option, String value)
	{
		boolean added = false;
		//configLock.writeLock().lock();
		if(!configOptions.containsKey(option))
		{
			configOptions.put(option, value);
			System.out.println("Option added: " + option + " => " + value);
			added = true;
		}
		//configLock.writeLock().unlock();
		return added;
	}
}
