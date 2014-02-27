
#include "s11n-tests.h"
#include <bike/s11n.h>
#include <bike/s11n-xml.h>
#include <gtest/gtest.h>
#include <memory>

using namespace bike;

class Widget;
typedef std::unique_ptr<Widget> WidgetUPtr;

class Widget {
public:
	Widget(Widget* parent = nullptr)
	:	parent_(parent) {}

	void add_child(WidgetUPtr&& ptr) {
		widgets_.emplace_back(std::move(ptr));
	}

	typedef std::vector<WidgetUPtr> Widgets;

	Widgets& childs() { return widgets_; }

	Widget* parent() const { return parent_; }

	void set_parent(Widget* parent) { parent_ = parent; }

	const std::string& name() const { return name_; }

	void set_name(const std::string& name) { name_ = name; }

protected:
	
	Widget* parent_;

	Widgets widgets_;

	std::string name_;
};

template <class Node>
void serialize_widget(Widget& widget, Node& node) {
	Accessor<Widget, Node> acc(&widget, node);
	acc.access("parent", &Widget::parent, &Widget::set_parent);
	acc.optional("name", "", &Widget::name, &Widget::set_name);
}

S11N_XML_OUT(Widget, serialize_widget);

bool operator == (const Widget& a, const Widget& b) {
	return a.name() == b.name();// FIXME: add child widgets comparison
}

TEST(Complex, 0) {
	WidgetUPtr root, root_read;
	root.reset(new Widget(S11N_NULLPTR));
	root->set_name("Root");

	WidgetUPtr fst_child, fst_child_read;
	fst_child.reset(new Widget(root.get()));
	root->add_child(std::move(fst_child));
	
	WidgetUPtr snd_child, snd_child_read;
	snd_child.reset(new Widget(root.get()));
	root->add_child(std::move(snd_child));

	std::ofstream fout("complex.xml");
	OutputXmlSerializer out(fout);
	out << root;
	fout.close();

	std::ifstream fin("complex.xml");
	InputXmlSerializer in(fin);
	in >> root_read;

	ASSERT_EQ(*root, *root_read);
}
