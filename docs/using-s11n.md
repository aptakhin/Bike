Using s11n
====================

Currently s11n has support of only XML format.

Examples
---------------------

### First example
```cpp
#include <fstream>
#include <iostream>

// First common header files
#include <bike/s11n.h>

// Second file for specific format
#include <bike/s11n-text.h>

void write_only_int(int x)
{
	std::ofstream fout("integer.xml");
	bike::OutputTextSerializer out(fout);
	out << x;
}

void read_only_int(int& x)
{
	std::ifstream fin("integer.xml");
	bike::InputTextSerializer in(fin);
	in >> x;
}

int main()
{
	int saved = 5, loaded;
	write_only_int(saved);
	read_only_int(loaded);
	return 0;
}

```

— Hey stop! I can do this with simple standard streams!

— Yep, we go next.


### Structures
```cpp
// headers missed...

struct Vector2
{
	double x, y;
	
	Vector2(double x = 0., double y = 0.) : x(x), y(y) {}

	// Just implement serialization method
	template <class Node>
	void ser(Node& node, bike::Version version) 
	{
		node & x & y;
	}
};

// Code of serialization is mostly the same
void write_only_vec2(Vector2& v)
{
	std::ofstream fout("vec2.xml");
	bike::OutputXmlSerializer out(fout);
	out << v;
}

void read_only_vec2(Vector2& v)
{
	std::ifstream fin("vec2.xml");
	bike::InputXmlSerializer in(fin);
	in >> v;
}

int main()
{
	Vector2 saved(-1, 1), loaded;
	write_only_vec2(saved);
	read_only_vec2(loaded);
	return 0;
}

```

— It's not a problem at all! I can define operators << and >> for streams for my type!

— Yep, but you have to keep structure of format in two places: reading and writing. It's simple for small structures, but not for big and nested ones.


### Non-default constructors
```cpp
// headers missed...
#include <string> // Ok, add one

class Human
{
public:
	// Also only non-default constructor was made
	// (It seems every human must have name)
	Human(const std::string& name) : name_(name) {}	

	virtual ~Human() {}

	const std::string& name() const { return name_; }

protected:
	std::string name_;
};

// Define (or exactly specialize) this template class
template <typename Node>
class Ctor<Human*, Node>
{
public:
	static Human* ctor(Node& node) 
	{
		std::string name;
		// It searches between all members for our name
		node.search(name, "name");
		return new Human(name); // We did it
	}
};

### Inheritance
```cpp
// I continue previous example with human class

class Superman : public Human
{
public:
    Superman(int superpower = 100000) 
	:	Human("Clark Kent"), 
		superpower_(superpower) {
	}    

	void fly() { /* Clark don't need and code to fly */ }

    template <class Node>
    void ser(Node& node, bike::Version version) {
        node.base<Human>(this);
		node.named(superpower_, "power");
    }

protected:
    int superpower_;
};
```

— Is that all?

— Unfortunately, not. In C++ we haven't any tools to bind information about type with it's constructing. So we have to register new derived type.

```cpp
int main()
{
	// Registering such way
	bike::Register<bike::XmlSerializer> reg;
	reg.reg_type<Human>();
	reg.reg_type<Superman>();

	// Then we can save and load any types, which Superman derives
	Human* superman = new Superman(200000), *man = 0;
	
	// Writing
	std::ofstream fout("superman.xml");
	bike::OutputXmlSerializer out(fout);
	out << superman;
	fout.close();
	
	// Reading
	std::ifstream fin("superman.xml");
	bike::InputXmlSerializer in(fin);
	in >> man;
	
	// And finally 
	std::cout << man->name();
	
	// Not yet finally. Easy to forget, yes?
	delete superman, delete man; 
	return 0;
}
```

### STL-stuff

STL-support included.

```cpp

int main()
{
	//...
	std::vector<int>        magic_numbers;
	std::list<unsigned int> ip4_white_list;
	std::unique_ptr<Human>  man;
	
	std::ifstream fin("stuff.xml");
	bike::InputXmlSerializer in(fin);
	in >> magic_numbers;
	in >> ip4_white_list;
	in >> man;
	return 0;
}
```

### Other shortly
#### C++03 compability
This define turns on C++03 compability mode.
```cpp
#define S11N_CPP03
```
