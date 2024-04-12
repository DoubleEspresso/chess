#include <iostream>

#include "xml_eval.h"


namespace haVoc {

	XmlEval::XmlEval() { }
	
	XmlEval::XmlEval(const std::string& xmlFile) {
		m_parser = std::make_unique<XML>(xmlFile);
		load();
	}
	
	XmlEval::~XmlEval() { }


	void XmlEval::load() {
		auto nodes = m_parser->get_nodes("eval");
		std::string fen = "";
		int staticEval = 0;
		for (const auto& n : nodes) {
			m_parser->try_get(*n, "fen", fen);
			m_parser->try_get(*n, "staticEval", staticEval);

			m_entries[fen] = staticEval;
		}
		std::cout << "..loaded " << m_entries.size() << " unique fen strings for evaluation tuning" << std::endl;
	}

}