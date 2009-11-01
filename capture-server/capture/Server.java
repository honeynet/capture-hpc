package capture;

public class Server
{
	public Server(String[] args)
	{
        System.out.println("PROJECT: Capture-HPC\n" +
                "VERSION: 3.0\n" +
                "DATE: Oct 24, 2009\n" +
                "COPYRIGHT HOLDER: Victoria University of Wellington, NZ\n" +
                "AUTHORS:\n" +
                "\tChristian Seifert (christian.seifert@gmail.com)\n" +
                "\tRamon Steenson(ramon.steenson@gmail.com)\n" +
                "\tVan Lam Le (vanlamle@gmail.com)\n" +
				"\n" +
				"For help, please refer to Capture-HPC mailing list at:\n" +
				"\thttps://public.honeynet.org/mailman/listinfo/capture-hpc" +
                "\n" +
                "Capture-HPC is free software; you can redistribute it and/or modify\n" +
                "it under the terms of the GNU General Public License, V2 as published by\n" +
                "the Free Software Foundation.\n" +
                "\n" +
                "Capture-HPC is distributed in the hope that it will be useful,\n" +
                "but WITHOUT ANY WARRANTY; without even the implied warranty of\n" +
                "MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the\n" +
                "GNU General Public License for more details.\n" +
                "\n" +
                "You should have received a copy of the GNU General Public License\n" +
                "along with Capture-HPC; if not, write to the Free Software\n" +
                "Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301,USA\n\n");

        for(int i = 0; i < args.length; i++)
		{
        	if(args[i].equals("stop")) {
            	ConfigFile configFile=new ConfigFile();
                configFile.parseDatabaseInfo("config.xml");
                if (ConfigManager.getInstance().getConfigOption("database-url")!=null)
                {
                    Database.initialize();
                    Database.getInstance().setSystemStatus(false);
                    System.out.println("Stop status is set, the instance of CaptureServer is going to stop soon!");
                    System.exit(0);
                }
                else
                {
                    System.out.println("ERROR: Database option is disable!");
                    System.exit(1);
                }
            }
            else if(args[i].equals("resume")) {
                ConfigManager.getInstance().addConfigOption("resume", "yes");
            }
            else if(args[i].equals("start")) {
                ConfigManager.getInstance().addConfigOption("start", "yes");
            }
            else if(args[i].equals("-s")) {
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
            } else if(args[i].equals("-i")) {
                if(((i+1) < args.length) && !args[i+1].startsWith("-")) {
                ConfigManager.getInstance().addConfigOption("input_urls", args[++i]);
                ConfigManager.getInstance().addConfigOption("import_urls", "yes");
                }
            } else if(args[i].equals("-c")) {
                if(((i+1) < args.length) && !args[i+1].startsWith("-")) {
                ConfigManager.getInstance().addConfigOption("import_check", args[++i]);
                }
			} 
		}

		/* Import URLs from file only, no operation*/
		if (ConfigManager.getInstance().getConfigOption("import_urls")!= null)
		{
			ConfigFile configFile=new ConfigFile();
			configFile.parseDatabaseInfo("config.xml");
            if (ConfigManager.getInstance().getConfigOption("database-url")!=null)
            {
                Database.initialize();
                Database.getInstance().importUrlFromFile();
                System.out.println("Finish import - please run Capture with start command");
				System.exit(0);
            }
            else
            {
                System.out.println("ERROR: Database option is disable!");
                System.exit(1);
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


		/* Load temporary value into memory */
		if (ConfigManager.getInstance().getConfigOption("database-url")!=null)
		{
			Database.initialize();
			Database.getInstance().loadTemporaryValue();
		}
        Postprocessor postprocessor = PostprocessorFactory.getDefaultPostprocessor();
        try {
            String postprocessorClassName = ConfigManager.getInstance().getConfigOption("postprocessor-classname");
            if(postprocessorClassName!=null && !postprocessorClassName.equals("")) {
                postprocessor = PostprocessorFactory.getPostprocessor(postprocessorClassName);
                String postprocessorConfig = ConfigManager.getInstance().getConfigOption("postprocessor-configuration");
                postprocessor.setConfiguration(postprocessorConfig);
            }
        } catch (FactoryException e) {
            System.out.println("Unable to create postprocessor. Proceeding without postprocessor");
            e.printStackTrace(System.out);
        }
        //post processor is an observer of URL

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
			
			if (ConfigManager.getInstance().getConfigOption("database-url")!=null)
			{
				Database.getInstance().InputUrlFromFile(file);
			} else
			{
				preprocessor.readInputUrls(file);
			}
		} else if (ConfigManager.getInstance().getConfigOption("database-url")!=null)
		{
            if (ConfigManager.getInstance().getConfigOption("resume")!=null)
            {
                Database.getInstance().resumeLastOperation();
            }
            else if (ConfigManager.getInstance().getConfigOption("start")!=null)
            {
                Database.getInstance().loadInputUrlFromDatabase();
            } else {
				System.out.println("Command not recogized. Pls use start or resume."
				System.exit(-1);
			}
		}
	}

    private static String getUsageString() {
        StringBuffer usageString = new StringBuffer();
        usageString.append("\nUsage: java -cp [classpath] -Xmx[memory] [-DfixIds=false] -Djava.net.preferIPv4Stack=true capture.Server -s listen address -f input_urls [-r false]");
        usageString.append("\nExample java -cp ./CaptureServer.jar -Xmx1024m -Djava.net.preferIPv4Stack=true capture.Server -s 192.168.1.100:7070 -f input_urls.txt");
        return usageString.toString();
    }

	/**
	 * @param args
	 */
	public static void main(String[] args) {
		Server server = new Server(args);
	}

}
