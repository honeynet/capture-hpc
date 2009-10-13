#include "Precompiled.h"

#include "Permission.h"
#include <boost/algorithm/string/classification.hpp>
#include <boost/algorithm/string/find_iterator.hpp>
#include <boost/algorithm/string/finder.hpp> 
#include <boost/tokenizer.hpp> 

PERMISSION_CLASSIFICATION
Permission::Check(std::wstring subject, std::wstring object)
{
	std::list<boost::wregex>::iterator lit;
	boost::wcmatch what;
	bool found = false;

	for(lit = subjects.begin(); lit != subjects.end(); lit++)
	{
		if(boost::regex_match(subject.c_str(), what, (*lit)))
		{
			found = true;
		}
	}
	if(!found)
	{
		return NO_MATCH;
	}

	for(lit = objects.begin(); lit != objects.end(); lit++)
	{
		if(boost::regex_match(object.c_str(), what, (*lit)))
		{
			if(allow)
			{
				return ALLOWED;
			} else {
				return DISALLOWED;
			}			
		}
	}
	return NO_MATCH;
}