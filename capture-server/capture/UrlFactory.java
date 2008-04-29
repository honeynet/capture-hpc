package capture;

import java.net.URISyntaxException;

public class UrlFactory {

    public UrlFactory() {

    }

    public Url getUrl(Element event) throws URISyntaxException {
        //int visitTime = ((Integer) ConfigManager.getInstance().getConfigOption("client_default_visit_time")).intValue();

        return getUrl(event.attributes.get("url").toLowerCase());
    }

    public Url getUrl(String tokenizedUrl) throws URISyntaxException {
        String[] splitUrl = splitTokenizedUrl(tokenizedUrl);
        double defaultPriority = 0.0; //low
        int visitTime = Integer.parseInt((String) ConfigManager.getInstance().getConfigOption("client-default-visit-time"));

        if (splitUrl.length == 1) {
            // <url>
            return new Url(splitUrl[0], null, visitTime,defaultPriority );
        } else if (splitUrl.length == 2) {
            try {
                // <url>::<visit time>
                return new Url(splitUrl[0], null, Integer.parseInt(splitUrl[1]), defaultPriority);
            }
            catch (NumberFormatException nfe) {
                try {
                    // <url>::<suspicionScore>
                    return new Url(splitUrl[0], null, visitTime, Double.parseDouble(splitUrl[1]));
                }
                catch (NumberFormatException nfe2) {
                    // <url>::<client>
                    return new Url(splitUrl[0], splitUrl[1], visitTime, defaultPriority);
                }
            }

        } else if (splitUrl.length == 3) {
            // <url>::<client>::<visit time>
            try {
                return new Url(splitUrl[0], splitUrl[1], Integer.parseInt(splitUrl[2]), defaultPriority);
            } catch (NumberFormatException nfe) {
                // <url>::<client>::<suspiconScore>
                try {
                    return new Url(splitUrl[0], splitUrl[1], visitTime, Double.parseDouble(splitUrl[2]));
                } catch (NumberFormatException nfe2) {
                    // <url>::<visitTime>::<suspiconScore>
                    return new Url(splitUrl[0], null, Integer.parseInt(splitUrl[1]), Double.parseDouble(splitUrl[2]));
                }
            }

        } else if (splitUrl.length == 4) {
            return new Url(splitUrl[0], splitUrl[1], Integer.parseInt(splitUrl[2]), Double.parseDouble(splitUrl[3]));
        }

        return new Url(null, null, visitTime,defaultPriority );
    }

    private String[] splitTokenizedUrl(String tokenizedUrl) {
        return tokenizedUrl.split("::");
    }

}
