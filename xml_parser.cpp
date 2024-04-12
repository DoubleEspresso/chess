
#include "xml_parser.h"

namespace haVoc {

	XML::XML(const std::string& xmlFile) {
		m_file = std::make_unique<file<>>(xmlFile.c_str());
		m_doc.parse<0>(m_file->data());
	}

	// ------------ Try get attr with string ---------------//
	template<>
	bool XML::try_get(std::string node, std::string attr, std::string& out) {
		xml_node<> * xnode = nullptr;
		if (!first_child(m_doc.first_node("root"), node, xnode))
			return false;

		char * attr_val = nullptr;
		if (!first_attr(xnode, attr, attr_val))
			return false;
		out = std::string(attr_val);
		return true;
	}

	template<>
	bool XML::try_get(std::string node, std::string attr, float& out) {
		std::string sval = "";
		if (!try_get<std::string>(node, attr, sval)) {
			return false;
		}
		out = std::stof(sval);
		return true;
	}

	template<>
	bool XML::try_get(std::string node, std::string attr, int& out) {
		std::string sval = "";
		if (!try_get<std::string>(node, attr, sval)) {
			return false;
		}
		out = std::stoi(sval);
		return true;
	}

	template<>
	bool XML::try_get(std::string node, std::string attr, bool& out) {
		std::string sval = "";
		if (!try_get<std::string>(node, attr, sval)) {
			return false;
		}
		out = (sval == "True" || sval == "true" ? true : false);
		return true;
	}



	// ------------ Try get attr with node ---------------//
	template<>
	bool XML::try_get(const xml_node<>& node, std::string attr, std::string& out) {
		char* attr_val = nullptr;
		if (!first_attr(&node, attr, attr_val))
			return false;
		out = std::string(attr_val);
		return true;
	}

	template<>
	bool XML::try_get(const xml_node<>& node, std::string attr, float& out) {
		std::string sval = "";
		if (!try_get<std::string>(node, attr, sval))
			return false;
		out = std::stof(sval);
		return true;
	}

	template<>
	bool XML::try_get(const xml_node<>& node, std::string attr, int& out) {
		std::string sval = "";
		if (!try_get<std::string>(node, attr, sval))
			return false;
		out = std::stoi(sval);
		return true;
	}

	template<>
	bool XML::try_get(const xml_node<>& node, std::string attr, bool& out) {
		std::string sval = "";
		if (!try_get<std::string>(node, attr, sval))
			return false;
		out = (sval == "True" || sval == "true" ? true : false);
		return true;
	}



	// ------------ Parsing utils ---------------//
	bool XML::first_child(const xml_node<> * parent, std::string name, xml_node<>*& child) {
		auto xnode = parent->first_node(name.c_str());
		if (strcmp(xnode->name(), name.c_str()) == 0)
		{
			child = xnode;
			return true;
		}
		return false;
	}

	bool XML::first_attr(const xml_node<> * node, const std::string& name, char*& attr_val) {
		for (xml_attribute<> *xattr = node->first_attribute(); xattr; xattr = xattr->next_attribute()) {
			if (strcmp(xattr->name(), name.c_str()) == 0) {
				attr_val = xattr->value();
				return true;
			}
		}
		return false;
	}

	std::vector<xml_node<>*> XML::get_nodes(std::string name) {
		xml_node<>* xnode = nullptr;
		std::vector<xml_node<>*> results;

		if (!first_child(m_doc.first_node("root"), name, xnode))
			return results;
		while (xnode) {
			results.emplace_back(xnode);
			xnode = xnode->next_sibling();
		}

		return results;
	}
}