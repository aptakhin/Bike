Using s11n
====================

Currently s11n has support of one format s11n-text.

First example:
```
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

```
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

— Yep, but you have to keep structure of format in two places^: reading and writing!
