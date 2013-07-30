#include <iostream>
#include <fstream>

#include <gtest/gtest.h>
#include <bike/s11n.h>
#include <bike/s11n-text.h>
#include <cmath>
#include <limits>

template <typename _T>
bool real_eq(_T a, _T b) {
    return std::fabs(a - b) < std::numeric_limits<_T>::epsilon();
}

using namespace bike;

TEST(Int, 0) {
	int x = 5;

	{
	std::ofstream fout("test.txt");
	OutputTextSerializer out(fout);
	out << x;
	}

	int nx;
	{
	std::ifstream fin("test.txt");
	InputTextSerializer in(fin);
	in >> nx;
	}

	ASSERT_EQ(x, nx);
}

struct Vec2 {
	
	template <class _Node>
	void ser(_Node& node, int version) {
		node & x & y;
	}

	Vec2() : x(0), y(0) {}
	Vec2(int x, int y) : x(x), y(y) {}

	bool operator == (const Vec2& rhs) const { return x == rhs.x && y == rhs.y; }

	int x, y;
};

TEST(Vec2, 0) {
	Vec2 vec(4, 2);

	{
	std::ofstream fout("test.txt");
	OutputTextSerializer out(fout);
	out << vec;
	}

	Vec2 nvec;
	{
	std::ifstream fin("test.txt");
	InputTextSerializer in(fin);
	in >> nvec;
	}

	ASSERT_EQ(vec, nvec);
}

TEST(Vec2, 1) {
	Vec2 vec(4, 2);
	Vec2 vec2(-324, 422);

	{
	std::ofstream fout("test.txt");
	OutputTextSerializer out(fout);
	out << vec << vec2;
	}

	Vec2 nvec, nvec2;
	{
	std::ifstream fin("test.txt");
	InputTextSerializer in(fin);
	in >> nvec >> nvec2;
	}

	ASSERT_EQ(vec, nvec);
	ASSERT_EQ(vec2, nvec2);
}

struct Vec2F {
	
	template <class _Node>
	void ser(_Node& node, int version) {
		//node & x & y;
		S11N_VAR(x, node);
		S11N_VAR(y, node);
	}

	Vec2F() : x(0), y(0) {}
	Vec2F(float x, float y) : x(x), y(y) {}

	// Need exact match
	bool operator == (const Vec2F& rhs) const { return x == rhs.x && y == rhs.y; }

	float x, y;
};

TEST(Vec2F, 0) {
	Vec2F vec(1.5f, 2.f);
	Vec2F vec2(-324.345f, 0.88886f);

	{
	std::ofstream fout("test.txt");
	OutputTextSerializer out(fout);
	out << vec << vec2;
	}

	Vec2F nvec, nvec2;
	{
	std::ifstream fin("test.txt");
	InputTextSerializer in(fin);
	in >> nvec >> nvec2;
	}

	ASSERT_EQ(vec,  nvec);
	ASSERT_EQ(vec2, nvec2);
}

TEST(Vec2F, Invalid_read) {
	Vec2F vec(1.5f, 2.f);
	Vec2F vec2(-324.34525f, 0.88888886f);

	{
	std::ofstream fout("test.txt");
	OutputTextSerializer out(fout);
	out << vec << vec2;
	}

	Vec2F nvec, nvec2;// Remove `F` prefix
	{
	std::ifstream fin("test.txt");
	InputTextSerializer in(fin);
	in >> nvec >> nvec2; // 0x3 fail there
	}
}

struct Vec2D {
	
	template <class _Node>
	void ser(_Node& node, int version) {
		node & x & y;
	}

	Vec2D() : x(0), y(0) {}
	Vec2D(double x, double y) : x(x), y(y) {}

	// Need exact match
	bool operator == (const Vec2D& rhs) const { return x == rhs.x && y == rhs.y; }

	double x, y;
};

TEST(Vec2D, 0) {
	Vec2D vec(1.5, 2.);
	Vec2D vec2(-324.346665, 0.888884444);

	{
	std::ofstream fout("test.txt");
	fout.precision(10);
	OutputTextSerializer out(fout);
	out << vec << vec2;
	}

	Vec2D nvec, nvec2;
	{
	std::ifstream fin("test.txt");
	InputTextSerializer in(fin);
	in >> nvec >> nvec2;
	}

	ASSERT_EQ(vec,  nvec);
	ASSERT_EQ(vec2, nvec2);
}

class A {
public:
	A(const std::string& name) : name_(name) {}

	template <class _Node>
	void ser(_Node& node, int version) {
		node.version(1);
		node & name_;
	}

private:
	std::string name_;
};

class B : public A {
public:
	B(const std::string& name, int n) : A(name), _num(n) {}

	template <class _Node>
	void ser(_Node& node, int version) {
		node.base<A>(this) & _num;
	}

private:
	int _num;
};

TEST(AB, 0) {
	
	B b("my cool string", 3);

	std::ofstream fout("test.txt");

	OutputTextSerializer out(fout);
	out << b;

	fout.close();

	B nb("", 0);
	std::ifstream fin("test.txt");
	InputTextSerializer in(fin);
	in >> nb;
}

TEST(AB, 1) {
	
	B b("my \"cool\" string // \\!", 3);

	std::ofstream fout("test.txt");

	OutputTextSerializer out(fout);
	out << b;

	fout.close();

	B nb("", 0);
	std::ifstream fin("test.txt");
	InputTextSerializer in(fin);
	in >> nb;
}

TEST(Vector, 0) {
	std::vector<int> q;
	std::ofstream fout("test.txt");
	OutputTextSerializer out(fout);
	out << q;
	fout.close();
	std::vector<int> nq;
	std::ifstream fin("test.txt");
	InputTextSerializer in(fin);
	in >> nq;
}

TEST(Vector, 1) {
	std::vector<int> v;
	v.push_back(3);
	v.push_back(6);

	std::ofstream fout("test.txt");
	OutputTextSerializer out(fout);
	out << v;
	fout.close();

	std::vector<int> nv;
	std::ifstream fin("test.txt");
	InputTextSerializer in(fin);
	in >> nv;
}

class A2 {
public:

	A2(const std::string& name) : name_(name), boo_(15) { std::cout << "A2()\n"; }

	virtual ~A2() { std::cout << "~A2()\n"; }

	bool operator == (const A2& r) const {
		return name_ == r.name_ && boo_ == r.boo_;
	}

	template <class _Node>
	void ser(_Node& node, int version) {
		node.version(1);
		node.named(boo_,  "boo");
		node.named(name_, "name");
	}

private:
	std::string name_;
	int boo_;
};

template <class _Node>
class Ctor<std::shared_ptr<A2>, _Node> {
public:
	static std::shared_ptr<A2> ctor(_Node& node) {
		std::string name;
		node.search(name, "name");
		if (!node.null()) 
			return std::make_shared<A2>(name);
		else
			return std::shared_ptr<A2>();
	}
};

TEST(Shared_ptr, 1) {
	std::shared_ptr<A2> a, b;
	{
	a = std::make_shared<A2>("cool a");
	std::ofstream fout("test.txt");
	OutputTextSerializer out(fout);
	out << a;
	}

	{
	std::ifstream fin("test.txt");
	InputTextSerializer in(fin);
	in >> b;
	}

	ASSERT_EQ(*a, *b);
}

TEST(Shared_ptr, Null) {
	std::shared_ptr<A2> a, b;
	{
	std::ofstream fout("test.txt");
	OutputTextSerializer out(fout);
	out << a;
	}

	{
	std::ifstream fin("test.txt");
	InputTextSerializer in(fin);
	in >> b;
	}

	ASSERT_EQ(b.get(), nullptr);
}

template <class _Node>
class Ctor<A2*, _Node> {
public:
	static A2* ctor(_Node& node) {
		std::string name;
		node.search(name, "name");
		return new A2(name);
	}
};

TEST(Usual_ptr, 1) {
	A2* a = new A2("cool a");
	std::ofstream fout("test.txt");
	OutputTextSerializer out(fout);
	out << a;
	fout.close();

	A2* b;
	std::ifstream fin("test.txt");
	InputTextSerializer in(fin);
	in >> b;

	ASSERT_EQ(*a, *b);
}

TEST(List, 0) {
	std::list<A2*> l;
	l.push_back(new A2("a"));
	l.push_back(new A2("b"));

	std::ofstream fout("test.txt");
	OutputTextSerializer out(fout);
	out << l;
	fout.close();

	std::list<A2*> nl;
	std::ifstream fin("test.txt");
	InputTextSerializer in(fin);
	in >> nl;

	int p = 0;
	//ASSERT_EQ(l, nl);
}

//TEST(List, Invalid) {
//	std::list<A2*> v;
//	v.push_back(new A2("a"));
//	v.push_back(new A2("b"));
//
//	std::ofstream fout("test.txt");
//	OutputTextSerializer out(fout);
//	out << v;
//	fout.close();
//
//	std::vector<A2*> nv;
//	std::ifstream fin("test.txt");
//	InputTextSerializer in(fin);
//	in >> nv;
//
//	int p  =0;
//	//ASSERT_EQ(v, nv);
//}


class A3 {
public:

	A3(const std::string& name, int boo) : name_(name), _boo(boo) { std::cout << "A3()\n"; }

	virtual ~A3() { std::cout << "~A3()\n"; }

	bool operator == (const A3& r) const {
		return name_ == r.name_ && _boo == r._boo;
	}

	template <class _Node>
	void ser(_Node& node, int version) {
		node.version(1);
		node.named(_boo, "boo");
		node.named(name_, "name");
	}

private:
	std::string name_;
	int _boo;
};

template <class _Node>
class Ctor<A3*, _Node> {
public:
	static A3* ctor(_Node& node) {
		std::string name;
		int boo;
		node.search(name, "name");
		node.search(boo, "boo");
		return new A3(name, boo);
	}
};

TEST(Usual_ptr, 2) {
	A3* a = new A3("cool a", 15);
	A3* a2 = new A3("cool b", -15);
	std::ofstream fout("test.txt");
	OutputTextSerializer out(fout);
	out << a << a2;
	fout.close();

	A3* b, *b2;
	std::ifstream fin("test.txt");
	InputTextSerializer in(fin);
	in >> b >> b2;

	ASSERT_EQ(*a, *b);
}

TEST(Usual_ptr, Null) {
	A3* a = new A3("cool a", 15);
	A3* a2 = nullptr;
	std::ofstream fout("test.txt");
	OutputTextSerializer out(fout);
	out << a << a2;
	fout.close();

	A3* b, *b2;
	std::ifstream fin("test.txt");
	InputTextSerializer in(fin);
	in >> b >> b2;

	ASSERT_EQ(*a, *b);
	ASSERT_EQ(a2, b2);
}

class A4 {
public:

	A4(const std::string& name, int boo) : name_(name), _boo(boo) { std::cout << "A4()\n"; }

	virtual ~A4() { std::cout << "~A4()\n"; }

	bool operator == (const A4& r) const {
		return name_ == r.name_ && _boo == r._boo;
	}

	template <class _Node>
	void ser(_Node& node, int version) {
		node.version(1);
		node.named(_boo, "boo");
		node.named(name_, "name");
	}

private:
	std::string name_;
	int _boo;
};

template <class _Node>
class Ctor<A4*, _Node> {
public:
	static A4* ctor(_Node& node) {
		std::string name;
		int boo;
		node.named(boo, "boo");
		node.named(name, "name");
		return new A4(name, boo);
	}
};

TEST(A4, 0) {
	A4* a = new A4("cool a", 15);
	A4* a2 = new A4("cool b", -15);
	std::ofstream fout("test.txt");
	OutputTextSerializer out(fout);
	out << a << a2;
	fout.close();

	A4* b, *b2;
	std::ifstream fin("test.txt");
	InputTextSerializer in(fin);
	in >> b >> b2;

	ASSERT_EQ(*a, *b);
}

TEST(A4, 1) {
	A4* a = new A4("cool a", 15);
	A4* a2 = nullptr;
	std::ofstream fout("test.txt");
	OutputTextSerializer out(fout);
	out << a << a2;
	fout.close();

	A4* b, *b2;
	std::ifstream fin("test.txt");
	InputTextSerializer in(fin);
	in >> b >> b2;

	ASSERT_EQ(*a, *b);
	ASSERT_EQ(a2, b2);
}

GTEST_API_ int main(int argc, char **argv) {
	bike::Static::add_std_renames();
	testing::InitGoogleTest(&argc, argv);
	int code = RUN_ALL_TESTS();
	return code;
}
