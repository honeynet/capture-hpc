package capture;

import java.util.HashMap;
import java.util.LinkedList;
import java.util.Set;
import java.util.Iterator;

public class EventsController {
	private HashMap<String, LinkedList<EventObserver>> eventObservers;
	
	private EventsController()
	{
		eventObservers = new HashMap<String, LinkedList<EventObserver>>();
	}
	
	private static class EventControllerHolder
	{ 
		private final static EventsController instance = new EventsController();
	}
	 
	public static EventsController getInstance()
	{
		return EventControllerHolder.instance;
	}
	
	public void addEventObserver(String eventName, EventObserver o)
	{
		if(!eventObservers.containsKey(eventName))
		{
			eventObservers.put(eventName, new LinkedList<EventObserver>());
		}
		//System.out.println("adding " + eventName + " event");
		LinkedList<EventObserver> observers = eventObservers.get(eventName);
		observers.add(o);
	}
	
	public void deleteEventObserver(String eventName, EventObserver o)
	{
		if(eventObservers.containsKey(eventName))
		{
			LinkedList<EventObserver> observers = eventObservers.get(eventName);
			if(observers.contains(o))
			{
				observers.remove(o);
			}
		}
	}
	
	public void notifyEventObservers(Element event)
	{
		Set keys = eventObservers.keySet();
        for (Iterator iterator = keys.iterator(); iterator.hasNext();) {
            Object key = iterator.next();
            Object value = eventObservers.get(key);
        }
		if(eventObservers.containsKey(event.name))
		{
			for(EventObserver observer : eventObservers.get(event.name))
			{
				observer.update(event);
			}
		}
	}

}
