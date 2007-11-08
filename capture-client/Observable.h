/*
 *	PROJECT: Capture
 *	FILE: Observable.h
 *	AUTHORS: Ramon Steenson (rsteenson@gmail.com) & Christian Seifert (christian.seifert@gmail.com)
 *
 *	Developed by Victoria University of Wellington and the New Zealand Honeynet Alliance
 *
 *	This file is part of Capture.
 *
 *	Capture is free software; you can redistribute it and/or modify
 *	it under the terms of the GNU General Public License as published by
 *	the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  Capture is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with Capture; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
 */
#pragma once
#include <vector>

template <class T>
class Observer
{
public:
	Observer() {}
	virtual ~Observer() {}
	virtual void update(int type, const T& message) = 0;
}; 

template <class T>
class Observable
{
public:
	Observable() {}

	virtual ~Observable()
	{
		observers.clear();
	}

	void attach(Observer<T>* observer)
	{
		observers.push_back(observer);
	}

	void attach(const std::wstring& type, Observer<T>* observer)
	{

	}

	void detach(Observer<T>* observer)
	{
		
		std::vector<Observer<T>*>::iterator it;
		for (it = observers.begin(); it != observers.end(); it++)
		{
			if((*it) == observer)
			{
				observers.erase(it);
			}
		}
	}

	void notify(int type, const T& msg)
	{
		std::vector<Observer<T>*>::iterator it;
		for (it = observers.begin(); it != observers.end(); it++)
		{
			(*it)->update(type, msg);
		}
	}
private:
	std::vector<Observer<T>*> observers;
};