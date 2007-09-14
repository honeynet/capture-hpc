package capture;

import java.net.URISyntaxException;

public class UrlFactory {
	
	public UrlFactory()
	{
		
	}
	
	public Url getUrl(Element event) throws URISyntaxException
	{
		//int visitTime = ((Integer) ConfigManager.getInstance().getConfigOption("client_default_visit_time")).intValue();
		
		return getUrl(event.attributes.get("url"));
	}
	
	public Url getUrl(String tokenizedUrl) throws URISyntaxException
	{
		String[] splitUrl = splitTokenizedUrl(tokenizedUrl);
		int visitTime = Integer.parseInt((String) ConfigManager.getInstance().getConfigOption("client-default-visit-time"));
		
		if(splitUrl.length == 1) {
			// <url>
			return new Url(splitUrl[0], null, visitTime);
		} else if(splitUrl.length == 2) {
			try
			{
				// <url>::<visit time>
				return new Url(splitUrl[0], null, Integer.parseInt(splitUrl[1]));
			}
			catch (NumberFormatException nfe)
			{
				// <client>::<url>
				return new Url(splitUrl[0], splitUrl[1], visitTime);
			}
			
		} else if(splitUrl.length == 3) {
			// <client>::<url>::<visit time>	
			return new Url(splitUrl[0], splitUrl[1], Integer.parseInt(splitUrl[2]));
		}
		return new Url(null, null, visitTime);
	}
	
	private String[] splitTokenizedUrl(String tokenizedUrl)
	{
		return tokenizedUrl.split("::");
	}
	
}
