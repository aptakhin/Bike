Using s11n
====================

Currently s11n has support of one format s11n-text.

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
	std::ofstream fout("integer.txt");
	bike::OutputTextSerializer out(fout);
	out << x;
}

void read_only_int(int& x)
{
	std::ifstream fin("integer.txt");
	bike::InputTextSerializer in(fin);
	in >> x;
}

int main()
{
	int saved, loaded;
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

	// Just implement serialization method
	template <class Node>
	void ser(Node& node, int version) 
	{
		node & x & y;
	}
};

// Code of serialization is mostly the same

void write_only_vec2(const Vector2& v)
{
	std::ofstream fout("vec2.txt");
	bike::OutputTextSerializer out(fout);
	out << v;
}

void read_only_vec2(Vector2& v)
{
	std::ifstream fin("vec2.txt");
	bike::InputTextSerializer in(fin);
	in >> v;
}

int main()
{
	Vector2 saved, loaded;
	write_only_vec2(saved);
	read_only_vec2(loaded);
	return 0;
}

```

— It's not a problem at all! I can define operators << and >> for streams for my type!

— Yep, but you have to keep structure of format in two places: reading and writing. It's simple for small structures, but not for big and nested ones.


### Non-default constructors and declaring out of class
```cpp
// headers missed...
#include <string> // Ok, add one

// Someone wrote this class, but he didn't know about our serialization system
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

//
// I will show in this example two features
//
// First serialization method is out of class, which is needed in some cases
// (Feature doesn't ready)
//
template <typename Node>
void serialize(Node& node, Human& human)
{
	node.version(1);

	//
	// Second is initialization with non-default constructor
	// And name name member by name name
	//
	node.write(human.name(), "name");
}

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
```

### Inheritance

I continue previous example
```cpp
// ... from previous series

class Superman : public Human
{
public:
	Superman() : Human("Clark Kent"), superpower_(100000) {}	

	void fly();

	template <class Node>
	void ser(Node& node, int version) 
	{
		// Serialize base class and value of super power too
		node.base<Human>(this) & superpower_;
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
	bike::reg_type<Superman>();

	// Then we can save and load any types, which Superman derives

	return 0;
}
```