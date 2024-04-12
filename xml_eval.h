#pragma once
#ifndef HAVOC_XML_EVAL_H
#define HAVOC_XML_EVAL_H

#include <string>
#include <memory>
#include <map>

#include "xml_parser.h"


namespace haVoc {

	typedef std::map<std::string, int> EvalEntries;

	class XmlEval {

	private:
		std::unique_ptr<XML> m_parser;
		EvalEntries m_entries;

		void load();

	public:
		XmlEval();
		XmlEval(const std::string& xmlFile);
		~XmlEval();

		EvalEntries& entries() {
			return m_entries;
		}

	};
}
#endif