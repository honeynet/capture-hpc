package capture;

public class Server
{
	public Server(String[] args)
	{	
		for(int i = 0; i < args.length; i++)
		{
			if(args[i].equals("-s")) {
				if(((i+1) < args.length) && !args[i+1].startsWith("-")) {
					String serverOption = args[++i];
					if(serverOption.contains(":"))
					{
						String[] s = serverOption.split(":");
						if(s.length == 2)
						{
							ConfigManager.getInstance().addConfigOption("server-listen-port", s[1]);
							ConfigManager.getInstance().addConfigOption("server-listen-address", s[0]);
						} else {
							System.out.println("ERROR in server option (-s): " + serverOption);
						}
					} else {
						ConfigManager.getInstance().addConfigOption("server-listen-port", "7070");
						ConfigManager.getInstance().addConfigOption("server-listen-address", serverOption);
					}
				}
			} else if(args[i].equals("-h")) {
			    System.out.println(getUsageString());
			    System.exit(1);
			} else if(args[i].equals("-f")) {
			    if(((i+1) < args.length) && !args[i+1].startsWith("-")) {
				ConfigManager.getInstance().addConfigOption("input_urls", args[++i]);
			    }
            } else if(args[i].equals("-r")) {
                if(((i+1) < args.length) && !args[i+1].startsWith("-")) {
                ConfigManager.getInstance().addConfigOption("halt_on_revert", args[++i]);
                }
			}
		}


		/* Check that an address was specified to listen on */
		if(ConfigManager.getInstance().getConfigOption("server-listen-address") == null)
		{
			System.out.println("Capture Server must be run with at least the -s argument set");
			System.out.println(getUsageString());
			System.exit(1);
		}
		
		/* Initialise all of the singleton objects */
		EventsController.getInstance();
		VirtualMachineServerController.getInstance();		
		UrlsController.getInstance();
		ClientsController.getInstance();
		ConfigManager.getInstance().loadConfigurationFile();
		
		if(ConfigManager.getInstance().getConfigOption("input_urls") != null)
		{
			String file = ConfigManager.getInstance().getConfigOption("input_urls");

            Preprocessor preprocessor = PreprocessorFactory.getDefaultPreprocessor();
            try {
                String preprocessorClassName = ConfigManager.getInstance().getConfigOption("preprocessor-classname");
                if(preprocessorClassName!=null && !preprocessorClassName.equals("")) {
                    preprocessor = PreprocessorFactory.getPreprocessor(preprocessorClassName);
                    String preprocessorConfig = ConfigManager.getInstance().getConfigOption("preprocessor-configuration");
                    preprocessor.setConfiguration(preprocessorConfig);
                }
            } catch (FactoryException e) {
                System.out.println("Unable to create preprocessor. Proceeding without preprocessor");
                e.printStackTrace(System.out);
            }
            preprocessor.readInputUrls(file);
		}
	}

    private static String getUsageString() {
        StringBuffer usageString = new StringBuffer();
        usageString.append("\nUsage: java -Djava.net.preferIPv4Stack=true -jar CaptureServer.jar -s listen address [-f input_urls]");
        usageString.append("\\nExample java -Djava.net.preferIPv4Stack=true -jar CaptureServer.jar -s 192.168.1.100:7070 -f input_urls.txt");
        return usageString.toString();
    }

	/**
	 * @param args
	 */
	public static void main(String[] args) {
		Server server = new Server(args);
	}

}
