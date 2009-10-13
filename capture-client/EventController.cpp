#include "Precompiled.h"

#include "EventController.h"

EventController::EventController(void)
{
	currentElement = NULL;
}

EventController::~EventController(void)
{
	instanceCreated = false;
	if(currentElement)
		delete currentElement;
}

EventController*
EventController::getInstance()
{
	if(!instanceCreated)
	{
		pEventController = new EventController();
		instanceCreated = true;	
	}
	return pEventController;
}

boost::signals::connection 
EventController::connect_onServerEvent(const std::wstring& eventType, const signal_serverEvent::slot_type& s)
{
	stdext::hash_map<std::wstring, signal_serverEvent*>::iterator it;
	if((it = onServerEventMap.find(eventType)) != onServerEventMap.end())
	{
		signal_serverEvent* signal_onServerEvent = it->second;
		return signal_onServerEvent->connect(s);
	} else {
		signal_serverEvent* signal_onServerEvent = new signal_serverEvent();
		boost::signals::connection connection = signal_onServerEvent->connect(s);
		onServerEventMap.insert(OnServerEventPair(eventType, signal_onServerEvent)); 
		return connection;
	}
}

void
EventController::notifyListeners()
{
	if(currentElement->getName().length() > 0)
	{
		stdext::hash_map<std::wstring, signal_serverEvent*>::iterator it;
		it = onServerEventMap.find(currentElement->getName());
		if(it != onServerEventMap.end())
		{
			signal_serverEvent* signal_onServerEvent = it->second;
			(*signal_onServerEvent)(*currentElement);
		}
		// TODO fix possible race condition here ... 
		delete currentElement;
		currentElement = NULL;
		//currentElement = new Element();
	}
}

void 
EventController::receiveServerEvent(const char* xmlDocument)
{
	LOG(INFO, "EventController: received server event");
	this->parseString(xmlDocument);
}

void 
EventController::startElement(const char* name, const char** atts)
{
	size_t nameLength = strlen(name) + 1;
	wchar_t* wszElementName = (wchar_t*)malloc((strlen(name) + 1)*sizeof(wchar_t));
	size_t convertedChars = 0;
	mbstowcs_s(&convertedChars, wszElementName, nameLength, name, _TRUNCATE);
	std::wstring elementName = wszElementName;
	free(wszElementName);

	if(!currentElement) {
		currentElement = new Element();
	} else if(currentElement->getName() != elementName) {
		Element* e = new Element();
		currentElement->addChildElement(e);
		currentElement = e;
	}

	currentElement->setName(elementName);

	if (atts)
	{
		for(int i = 0; atts[i]; i+=2)
		{
			if(atts[i+1] != NULL)
			{
				size_t attributeNameLength = strlen(atts[i]) + 1;
				size_t attributeValueLength = strlen(atts[i+1]) + 1;
				wchar_t* wszAttributeName = (wchar_t*)malloc(attributeNameLength*sizeof(wchar_t));
				wchar_t* wszAttributeValue = (wchar_t*)malloc(attributeValueLength*sizeof(wchar_t));
			
				mbstowcs_s(&convertedChars, wszAttributeName, attributeNameLength, atts[i], _TRUNCATE);
				mbstowcs_s(&convertedChars, wszAttributeValue, attributeValueLength, atts[i+1], _TRUNCATE);

				currentElement->addAttribute(wszAttributeName, wszAttributeValue);

				free(wszAttributeName);
				free(wszAttributeValue);

			} else {
				LOG(ERR, "malformed XML received");
			}
		}
	}
}


void 
EventController::endElement(const char* name)
{
	size_t nameLength = strlen(name) + 1;
	wchar_t* wszElementName = (wchar_t*)malloc((strlen(name) + 1)*sizeof(wchar_t));
	size_t convertedChars = 0;
	mbstowcs_s(&convertedChars, wszElementName, nameLength, name, _TRUNCATE);
	std::wstring elementName = wszElementName;
	free(wszElementName);

	if(!currentElement->hasParent())
	{
		// At the root parent so notify listeners
		notifyListeners();
	} else {
		// Current element has finished go up to the childs parent
		assert(currentElement->getName() == elementName);
		if(currentElement->getName() == elementName)
		{
			currentElement = currentElement->getParent();
		}
	}
}


void 
EventController::charData(const char *s, int len)
{
	currentElement->setData(s, len);
}
