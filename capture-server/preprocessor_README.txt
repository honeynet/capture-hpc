1. Capture Preprocessor Plug-in Architecture README
---------------------------------------------------
The Capture Preprocessor Plug-in Architecture is a feature that was added with version 2.2 of Capture-HPC. It allows one to change the behavior of Capture-HPC in its way to process the input urls. Prior to the existence preprocessors, Capture-HPC simply forwarded all input URLs to the client honeypot for inspection. The preprocessors allow one to adjust this flow. Instead of forwarding the input URLs to Capture-HPC directly, they are first forwarded to the preprocessor. The preprocessor can process the URLs and then forward input URLs to be inspected by the Capture system to its liking. Common examples of preprocessors are crawlers and filters. Currently, only one preprocessor at a time is supported by Capture-HPC.

Preprocessors are configured and instantiated based on the preprocessor directive in the config.xml file. Capture will use the dynamic class loader to instantiate the preprocessor using the classname as the fully qualifying class name. The preprocessor plug-in’s configuration is provided via CDATA of the preprocessor tag. 

As input URLs are read from a file, they are passed to the preprocessor plug-in. To instruct Capture-HPC to inspect a URL, the preprocessor plug-in needs to pass URLs to the Capture queue. A priority setting on the URL allows the preprocessor plug-in to influence what URLs are inspected first. The priority values range from 0.0 (low) to 1.0 (high). 

2. Development
--------------
To develop a preprocessor plug-in, one simply needs to extend the abstract capture.Preprocessor class, implement a default constructor (can be empty) and the following functions:
   /* The meat of the preprocessor. Urls are passed from the input file to the preprocessor.
    * If URLs shall be processed by Capture, the preprocessor must call the addUrlToCaptureQueue function (which will not block).
	* The preprocessor can tag ::<priority> to the URL to influence which URLs Capture will inspect first 
	* (0.0 for low and 1.0 for high priority)
    *
    * @param url - url as per capture format (optionally including prg and delay, for instance http://www.foo.com::iexplore::30)
    */
    abstract public void preprocess(String url);

    /* Sets the configuration of the preprocessor. Allows the preprocessor to be configured via the
     * existing config.xml configuration file.
     *
     * @param configuration - from the CDATA element of the preprocessor xml tag of config.xml
     */
    abstract public void setConfiguration(String configuration);

3. Running existing Preprocessor plugins
----------------------------------------
To run an existing plug-in, its class and libraries need to be added to the classpath when starting the Capture Server. A preprocessor configuration section must be added to the config.xml file according to the preprocessor plug-in’s documentation. The classname attribute of the preprocessor tag needs to be the fully qualifying class name of the preprocessor plug-in.
