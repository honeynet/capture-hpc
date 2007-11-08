#pragma once
#include <vector>
#include <string>

class Attribute
{
public:
	Attribute(const std::wstring& n, const std::wstring& v) { name = n; value = v; }
	virtual ~Attribute() {}
	
	inline const std::wstring& getName() const { return name; }
	inline const std::wstring& getValue() const { return value; }

private:
	std::wstring name;
	std::wstring value;
};

class Element
{
public:
	Element()
	{
		parent = NULL;
		data = NULL;
		dataSize = 0;
		_hasParent = false;
	}

	virtual ~Element()
	{
		attributes.clear();
		for each(Element* child in childElements)
		{
			//delete child;
		}
		free(data);
		dataSize = 0;
	}

	void addAttribute(const std::wstring& name, const std::wstring& value)
	{
		attributes.push_back(Attribute(name, value));
	}

	const std::wstring getAttributeValue(const std::wstring& name) const
	{
		for each(Attribute a in attributes)
		{
			if(a.getName() == name)
			{
				return a.getValue();
			}
		}
		return std::wstring();
	}

	void addChildElement(Element* childElement)
	{
		childElement->setParent(this);
		childElements.push_back(childElement);
	}

	void setData(const char* data, size_t dataSize)
	{
		this->data = (char*)malloc(dataSize);
		memcpy(this->data, data, dataSize);
		this->dataSize = dataSize;
	}

	void setName(const std::wstring& elementName)
	{
		name = elementName;
	}

	void setParent(Element* element)
	{
		parent = element;
		_hasParent = true;
	}

	std::wstring toString() const
	{
		std::wstring document = L"<";
		document += name;
		for each(Attribute attribute in attributes)
		{
			document += L" ";
			document += attribute.getName();
			document += L"=\"";
			document += attribute.getValue();
			document += L"\"";
		}

		document += L">";

		if(childElements.size() > 0)
		{
			_ASSERT(dataSize == 0);

			for each(Element* childElement in childElements)
			{
				document += childElement->toString();
			}
		}

		if(dataSize > 0)
		{
			_ASSERT(childElements.size() == 0);
			// TODO fix this ugly hack, data uses up twice the amount of space it should
			wchar_t* tempData = (wchar_t*)malloc((dataSize+1)*sizeof(wchar_t));
			for(unsigned int i = 0; i < dataSize; i++)
			{
				// We get away with this cast for now as the data is base64 encoded
				// so will always be an ASCII char and map correctly to a unicode wchar
				tempData[i] = (wchar_t)data[i];
			}
			tempData[dataSize] = '\0';
			document += tempData;
			free(tempData);
		}

		document += L"</";
		document += name;
		document += L">\r\n";

		return document;
	}

	inline const std::wstring& getName() const { return name; }
	inline const std::vector<Attribute>& getAttributes() const { return attributes; }
	inline Element* getParent() { return parent; }
	inline const char* getData() const { return data; }
	inline const std::vector<Element*>& getChildElements() const { return childElements; }
	inline const size_t getDataSize() const { return dataSize; }
	inline bool hasParent() { return _hasParent; }

private:
	bool _hasParent;
	std::wstring name;
	std::vector<Attribute> attributes;
	char * data;
	size_t dataSize;
	std::vector<Element*> childElements;
	Element* parent;
};
