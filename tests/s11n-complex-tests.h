
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
	:	parent_(parent) {
	}

	void add_child(WidgetUPtr&& ptr) {
		widgets_.emplace_back(std::move(ptr));
	}

	typedef std::vector<WidgetUPtr> Widgets;

	Widgets& childs() { return widgets_; }

	const Widget* parent() const { return parent_; }

	void set_parent(Widget* parent) { parent_ = parent; }

protected:
	
	Widget* parent_;

	Widgets widgets_;
};

template <class Node>
void serialize_widget(Widget& widget, Node& node) {
	access_free<Widget*>(&widget, "parent", &Widget::parent, &Widget::set_parent, node);
}

S11N_XML_OUT(Widget, serialize_widget);

TEST(Complex, 0) {
	WidgetUPtr root, root_read;
	root.reset(new Widget(S11N_NULLPTR));

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

	Widget::Widgets& childs = root->childs();
}
