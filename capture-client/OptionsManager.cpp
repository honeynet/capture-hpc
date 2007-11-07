#include "OptionsManager.h"
#include "EventController.h"
#include <boost/bind.hpp>

OptionsManager::OptionsManager(void)
{
	onOptionEventConnection = EventController::getInstance()->connect_onServerEvent(L"option", boost::bind(&OptionsManager::onOptionEvent, this, _1));
}

OptionsManager::~OptionsManager(void)
{
	optionsMap.clear();
	onOptionEventConnection.disconnect();
	instanceCreated = false;
}

OptionsManager*
OptionsManager::getInstance()
{
	if(!instanceCreated)
	{
		optionsManager = new OptionsManager();
		instanceCreated = true;	
	}
	return optionsManager;
}

void
OptionsManager::onOptionEvent(const Element& element)
{
	if(element.getName() == L"option")
	{	
		std::wstring option = L"";
		std::wstring value = L"";
		std::vector<Attribute>::const_iterator it;
		for(it = element.getAttributes().begin(); it != element.getAttributes().end(); it++)
		{
			if((*it).getName() == L"name") {
				option = (*it).getValue();
			} else if((*it).getName() == L"value") {
				value = (*it).getValue();
			}
		}
		if(option != L"" && value != L"")
		{
			DebugPrint(L"Received option event: %ls => %ls\n", option.c_str(), value.c_str());
			addOption(option, value);
		}
	}
}

const std::wstring
OptionsManager::getOption(const std::wstring& option)
{
	stdext::hash_map<std::wstring, std::wstring>::iterator it;
	it = optionsMap.find(option);
	if(it != optionsMap.end())
	{
		return it->second;
	}
	return L"";
}

bool
OptionsManager::addOption(const std::wstring& option, const std::wstring& value)
{
	stdext::hash_map<std::wstring, std::wstring>::iterator it;
	it = optionsMap.find(option);
	if(it == optionsMap.end())
	{
		DebugPrint(L"Adding option: %ls => %ls\n", option.c_str(), value.c_str());
		optionsMap.insert(std::pair<wstring, wstring>(option, value));
	} else {
		DebugPrint(L"Changing option: %ls => %ls\n", option.c_str(), value.c_str());
		it->second = value;
	}
	signalOnOptionChanged(option);
	return true;
}

boost::signals::connection 
OptionsManager::connect_onOptionChanged(const signal_optionChanged::slot_type& s)
{
	boost::signals::connection conn = signalOnOptionChanged.connect(s);
	
	stdext::hash_map<std::wstring, std::wstring>::iterator it;
	for(it = optionsMap.begin(); it != optionsMap.end(); it++)
	{
		signalOnOptionChanged(it->first);
	}
	
	return conn;
}
