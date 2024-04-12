#pragma once
#ifndef HAVOC_XML_PARSER_H
#define HAVOC_XML_PARSER_H

#include <string>
#include <memory>
#include <rapidxml.hpp>
#include <rapidxml_utils.hpp>


using namespace rapidxml;

namespace haVoc {

	class XML {

		xml_document<> m_doc;
		std::unique_ptr<file<>> m_file;

		bool first_child(const xml_node<> * parent, std::string name, xml_node<>*& child);
		bool first_attr(const xml_node<> * node, const std::string& name, char*& attr_val);

	public:
		XML() {}
		XML(const std::string& xmlFile);
		~XML() { }

		template<typename T>
		bool try_get(std::string node, std::string attr, T& out);

		template<typename T>
		bool try_get(const xml_node<>& node, std::string attr, T& out);

		/// <summary>
		/// Returns all nodes of a given name
		/// </summary>
		/// <param name="name"></param>
		/// <returns></returns>
		std::vector<xml_node<>*> get_nodes(std::string name);
	};
}
#endif