#include "pg.h"
#include "govstut.h"
#include "stut.h"

#include "mcrl2/utilities/input_output_tool.h"
#include "mcrl2/utilities/logger.h"

#include <sstream>
#include <iostream>
#include <fstream>

namespace pg {

class Equivalence
{
public:
	enum Eq {
		scc     = 0,
		stut    = 1,
		gstut   = 2,
		invalid = 3
	};
	Equivalence() {}
	Equivalence(const Equivalence& other) : m_value(other.m_value) {}
	Equivalence(const std::string& name)
	{
		if (m_names[(int)scc] == name) m_value = scc; else
		if (m_names[(int)stut] == name) m_value = stut; else
		if (m_names[(int)gstut] == name) m_value = gstut; else
		m_value = invalid;
	}
	const std::string name() const
	{
		return m_names[m_value];
	}
	const std::string desc() const
	{
		return m_descs[m_value];
	}
	static const std::string name(unsigned int index)
	{
		if (index < invalid)
			return m_names[index];
		return std::string();
	}
	static const std::string desc(unsigned int index)
	{
		if (index < invalid)
			return m_descs[index];
		return std::string();
	}
	bool operator==(Eq other) const
	{
		return m_value == other;
	}
private:
	Eq m_value;
	static const char* m_names[4];
	static const char* m_descs[4];
};

const char* Equivalence::m_names[4] = {"scc", "stut", "gstut", "invalid"};
const char* Equivalence::m_descs[4] =
{
		"strongly connected component",
		"stuttering equivalence",
		"governed stuttering equivalence",
		"invalid equivalence name"
};

}

class pgconvert : public mcrl2::utilities::tools::input_output_tool
{
private:
	pg::Equivalence m_equivalence;
public:
	pgconvert() : mcrl2::utilities::tools::input_output_tool(
		// Tool name:
		"pgconvert",
		// Author:
		"S. Cranen",
		// Tool summary:
		"Implements various parity game reductions.",
		// Tool description:
		"Tool that can reduce parity games modulo stuttering equivalence and "
		"governed stuttering equivalence.",
		// Known issues:
		"None") {}
	bool run()
	{
		std::istream* instream = &std::cin;
		std::ifstream infstream;
		if (not m_input_filename.empty())
		{
			mCRL2log(info) << "Using input file: " << m_input_filename << std::endl;
			infstream.open(m_input_filename.c_str(), std::ios::in);
			instream = &infstream;
		}
		else
		{
			mCRL2log(info) << "Reading from standard input." << std::endl;
		}
		std::ostream* outstream = &std::cout;
		std::ofstream outfstream;
		if (not m_output_filename.empty())
		{
			mCRL2log(info) << "Using output file: " << m_output_filename << std::endl;
			outfstream.open(m_output_filename.c_str(), std::ios::out);
			outstream = &outfstream;
		}
		else
		{
			mCRL2log(info) << "Writing to standard output." << std::endl;
		}
		pg::ParityGame pg;
		mCRL2log(info) << "Loading parity game." << std::endl;
		pg.load(*instream);
		mCRL2log(info) << "Performing " << m_equivalence.desc() << " reduction." << std::endl;
		if (m_equivalence == pg::Equivalence::scc)
		{
			pg.collapse_sccs();
			pg.dump(*outstream);
		}
		else if (m_equivalence == pg::Equivalence::stut)
		{
			pg.collapse_sccs();
			pg::StutteringPartitioner p(pg);
			pg::ParityGame output;
			p.partition(&output);
			output.dump(*outstream);
		}
		else if (m_equivalence == pg::Equivalence::gstut)
		{
			pg::GovernedStutteringPartitioner p(pg);
			pg::ParityGame output;
			p.partition(&output);
			output.dump(*outstream);
		}
		return true;
	}
protected:
	void add_options(mcrl2::utilities::interface_description& desc)
	{
		unsigned int i = 0;
		std::stringstream eqs;
		std::string eq = pg::Equivalence::name(0);
		while (not eq.empty())
		{
			eqs << std::endl << "  " << eq << ": " << pg::Equivalence::desc(i) << " reduction";
			eq = pg::Equivalence::name(++i);
		}
		desc.add_option("equivalence", mcrl2::utilities::make_mandatory_argument("NAME"), "The conversion method to use, choose from" + eqs.str(), 'e');
	}
	void parse_options(const mcrl2::utilities::command_line_parser& parser)
	{
		mcrl2::utilities::tools::input_output_tool::parse_options(parser);
		if (parser.options.count("equivalence"))
		{
			m_equivalence = pg::Equivalence(parser.option_argument("equivalence"));
			if (m_equivalence == pg::Equivalence::invalid)
			{
				parser.error("option -e/--equivalence has illegal argument '" +
							 parser.option_argument("equivalence") + "'");
			}
		}
	}
};

int main(int argc, char** argv)
{
	return pgconvert().execute(argc, argv);
}
