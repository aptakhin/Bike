Using s11n
====================

Currently s11n has support of one format s11n-text.

First example:
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

```cpp
// headers missed...

struct Vector2
{
	double x, y;

	// Just implement ser method
	template <class _Node>
	void ser(_Node& node, int version) 
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

— Yep, but you have to keep structure of format in two places: reading and writing. It's simple for small structures, but not for big, nested, so next example.

```cpp
// headers missed...
#include <string> // Ok, add one

// Someone wrote this class, but he didn't know about our serialization system
class Human
{
public:
	// Also only non-default constructor was made
	// It seems every human must have name
	Human(const std::string& name) : name_(name) {}	

protected:
	std::string name_;
};

— We will show in this example two features.

// First is serialization method is out of class, which seems to be more useful for any cases
template <typename Node>
void serialize(Node& node, Human& human)
{
	node.version(1);

	// Second is initialization with non-default constructor
	// Name name member by name name
	node.named(name_, "name");
}

// Define (or exactly specialize) this template class
template <typename Node>
class Ctor<Human*, Node>
{
public:
	static Human* ctor(_Node& node) 
	{
		std::string name;
		// It searches between all members for our name
		node.search(name, "name");
		return new Human(name); // We did it
	}
};

