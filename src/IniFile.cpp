#include <asl/IniFile.h>
#include <asl/TextFile.h>

namespace asl {

IniFile::IniFile(const String& name, bool shouldwrite)
{
	_modified = false;
	_filename = name;
	_currentTitle = "-";
	_ok = false;
	_shouldwrite = shouldwrite;
	TextFile file(name, File::READ);
	if(!file) {
		return;
	}
	_ok = true;
	while(!file.end())
	{
		String line=file.readLine();
		if(shouldwrite)
			_lines << line.trimmed();
		if(line.length()==0)
			continue;
		if(file.end())
			break;

		char firstchar = line[0];
		if(firstchar=='[')
		{
			int end = line.indexOf(']', 1);
			if(end < 0)
				continue;
			String name = line.substring(1, end);
			_currentTitle = name;
			sections[_currentTitle] = Section(_currentTitle);
		}
		else if(firstchar!='#' && firstchar>32 && firstchar!=';')
		{
			int i=line.indexOf('=');
			if(i<1)
				continue;
			String key = line.substring(0,i).trimmed();
			String value = line.substring(i+1).trimmed();
			for(char* p=key; *p; p++)
				if(*p == '/')
					*p = '\\';
			sections[_currentTitle][key] = value;
		}
	}
	_currentTitle = "-";
	for(int i=_lines.length()-1; i>0 && _lines[i][0] == '\0'; i--)
	{
		_lines.resize(_lines.length()-1);
	}
 
}

IniFile::Section& IniFile::section(const String& name)
{
	_currentTitle = name;
	sections[name]._title = name;
	return sections[name];
}

String& IniFile::operator[](const String& name)
{
	int slash = name.indexOf('/');
	if(slash < 0)
	{
		sections[_currentTitle]._title= _currentTitle;
		return sections[_currentTitle][name];
	}
	else
	{
		String sec = name.substring(0, slash);
		Section& section = sections[sec];
		section._title = sec;
		return section[name.substring(slash+1)];
	}
}

String IniFile::operator()(const String& name, const String& defaultVal)
{
	return has(name)? (*this)[name] : defaultVal;
}

bool IniFile::has(const String& name) const
{
	int slash = name.indexOf('/');
	if(slash < 0)
	{
		return sections[_currentTitle]._vars.has(name);
	}
	else
	{
		String sec = name.substring(0, slash);
		if(!sections.has(sec))
			return false;
		return sections[sec]._vars.has(name.substring(slash+1));
	}
}

void IniFile::write(const String& fname)
{
	Dic<Section> newsec = sections.clone();

	Section* section = &sections["-"];

	foreach(String& line, _lines)
	{
		if(line[0]=='[')
		{
			int end = line.indexOf(']');
			if (end < 0)
				continue;
			String name = line.substring(1, end);
			_currentTitle = name;
			section = &sections[name];
		}
		else if(line[0]!='#' && line[0]>31 && line[0]!=';')
		{
			int i=line.indexOf('=');
			if (i < 0)
				continue;
			String key = line.substring(0,i).trimmed();
			String value0 = line.substring(i+1).trimmed();
			const String& value1 = (*section)[key];
			line = key; line << '=' << value1;
			if(value0 != value1 && value1 != "")
			{
				_modified = true;
			}

			newsec[_currentTitle]._vars.remove(key);
		}
	}

	foreach(Section& section, newsec)
	{
		bool empty = true;
		foreach(String& value, section._vars)
		{
			if(value != "")
				empty = false;
		}
		if(empty)
			section._title = "";
	}

	foreach(Section& section, newsec)
	{
		if(section._vars.length()==0 || section._title == "")
			continue;
		int j=-1;
		for(int i=0; i<_lines.length(); i++)
		{
			if(section.title()=="-" || _lines[i]=='['+section.title()+']')
			{
				int k = (section.title()=="-")? 0 : 1;
				for(j=i+k; j<_lines.length() && _lines[j][0]!='['; j++)
				{}
				j--;
				if(j>0)
					while(_lines[j][0]=='\0') j--;
				j++;
				//_lines.insert(j, "");
				break;
			}
		}
		if(j==-1)
		{
			j=_lines.length()-1;
			if(j>0)
				while(_lines[j][0]=='\0') j--;
			j++;
			_lines.insert(j++, "");
			_lines.insert(j++, String(0, "[%s]", *section.title()));
		}

		foreach2(String& name, String& value, section._vars)
		{
			String line = name;
			line << "=" << value;
			_lines.insert(j++, line);
			_modified = true;
		}
	}
	
	if(_modified)
	{
		TextFile file ((fname!="")? fname: _filename, File::WRITE);
		if(!file)
			return;
		foreach(String& line, _lines)
		{
			file.printf("%s\n", *line);
		}
		file.close();
	}
}

IniFile::~IniFile()
{
	if(_shouldwrite)
		write(_filename);
}

int IniFile::arraysize(const String& name)
{
	section(name);
	return sections[_currentTitle]["size"];
}

String IniFile::array(const String& name, int index)
{
	String key = index+1;
	key << '\\' << name;
	return sections[_currentTitle][key];
}

}
