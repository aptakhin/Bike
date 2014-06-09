// s11n
//
#include <fstream>
#include <iostream>

// First common header files
#include <bike/s11n.h>

// Second file for specific format
#include <bike/s11n-xml.h>

void write_only_int(int x)
{
	std::ofstream fout("integer.xml");
	bike::OutputXmlSerializer out(fout);
	out << x;
}

void read_only_int(int& x)
{
	std::ifstream fin("integer.xml");
	bike::InputXmlSerializer in(fin);
	in >> x;
}

TEST(Docs, Integer) {
	int saved = 5, loaded;
	write_only_int(saved);
	read_only_int(loaded);
	ASSERT_EQ(saved, loaded);
}

struct Vector2
{
	double x, y;
	
	Vector2(double x = 0., double y = 0.) : x(x), y(y) {}

	// Just implement serialization method
	template <class Node>
	void ser(Node& node) 
	{
		node & x & y;
	}
};

bool operator == (const Vector2& a, const Vector2& b) {
	return a.x == b.x && a.y == b.y; // Need exact match
}

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

TEST(Docs, Vec2) {
	Vector2 saved(-1, 1), loaded;
	write_only_vec2(saved);
	read_only_vec2(loaded);
	ASSERT_EQ(saved, loaded);
}
