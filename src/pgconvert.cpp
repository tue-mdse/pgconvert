#include "equivalence.h"
#include "parsers/pgsolver.h"
#include "govstut.h"
#include "stut.h"
#include "pg.h"

#include "mcrl2/utilities/input_output_tool.h"
#include "mcrl2/utilities/logger.h"

#include <sstream>
#include <iostream>
#include <fstream>

/**
 * @class pgconvert
 * @brief Tool class that can execute parity game reductions.
 */
class pgconvert : public mcrl2::utilities::tools::input_output_tool
{
private:
	Equivalence m_equivalence;
	std::auto_ptr<std::ifstream> m_ifstream;
	std::auto_ptr<std::ofstream> m_ofstream;
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

	template <typename graph_t>
	void collapse_sccs(graph_t& graph)
	{
		timer().start("scc reduction");
		graph.collapse_sccs();
		timer().finish("scc reduction");
		mCRL2log(debug) << "Parity game contains " << graph.size() << " nodes after SCC reduction." << std::endl;
	}

	template <typename partitioner_t, typename graph_t>
	void partition(Equivalence e, partitioner_t& partitioner, graph_t* output=NULL)
	{
		timer().start("partition refinement");
		partitioner.partition(output);
		timer().finish("partition refinement");
		if (output)
		{
			mCRL2log(debug) << "Parity game contains " << output->size() << " nodes after " << e.desc() << " reduction." << std::endl;
		}
	}

	template <typename graph_t>
	void load(graph_t& graph, std::istream& s)
	{
		mCRL2log(info) << "Loading parity game." << std::endl;
		timer().start("load");
		graph::pg::Parser<typename graph_t::vertex_t, graph::pg::pgsolver> parser(graph);
		parser.load(s);
		timer().finish("load");
		mCRL2log(debug) << "Parity game contains " << graph.size() << " nodes." << std::endl;
	}

	template <typename graph_t>
	void save(graph_t& graph, std::ostream& s)
	{
		timer().start("save");
		graph::pg::Parser<typename graph_t::vertex_t, graph::pg::pgsolver> parser(graph);
		parser.dump(s);
		timer().finish("save");
	}

	std::istream& open_input()
	{
		std::istream* instream = &std::cin;
		if (not m_input_filename.empty())
		{
			m_ifstream.reset(new std::ifstream());
			m_ifstream->open(m_input_filename.c_str(), std::ios::in);
			instream = m_ifstream.get();
		}
		else
			m_input_filename = "standard input";
		mCRL2log(info) << "Reading from " << m_input_filename << "." << std::endl;
		return *instream;
	}

	std::ostream& open_output()
	{
		std::ostream* outstream = &std::cout;
		if (not m_output_filename.empty())
		{
			m_ofstream.reset(new std::ofstream());
			m_ofstream->open(m_output_filename.c_str(), std::ios::out);
			outstream = m_ofstream.get();
		}
		else
			m_output_filename = "standard output";
		mCRL2log(info) << "Writing to " << m_output_filename << "." << std::endl;
		return *outstream;
	}

	/// @brief Runs the tool (see mcrl2::utilities::tools::input_output_tool::run).
	bool run()
	{

		std::istream& instream = open_input();
		std::ostream& outstream = open_output();
		mCRL2log(info) << "Performing " << m_equivalence.desc() << " reduction." << std::endl;
		if (m_equivalence == Equivalence::scc)
		{
			graph::KripkeStructure<graph::Vertex<graph::pg::Label> > pg;
			load(pg, instream);
			timer().start("reduction");
			collapse_sccs(pg);
			timer().finish("reduction");
			save(pg, outstream);
		}
		else if (m_equivalence == Equivalence::stut)
		{
			graph::StutteringPartitioner<graph::pg::DivLabel>::graph_t pg;
			graph::StutteringPartitioner<graph::pg::DivLabel>::graph_t output;
			graph::StutteringPartitioner<graph::pg::DivLabel> p(pg);
			load(pg, instream);
			timer().start("reduction");
			collapse_sccs(pg);
			partition(m_equivalence, p, &output);
			timer().finish("reduction");
			for (size_t i = 0; i < output.size(); ++i)
				if (output.vertex(i).label.div)
					output.vertex(i).out.insert(i);
			save(output, outstream);
		}
		else if (m_equivalence == Equivalence::gstut)
		{
			graph::pg::GovernedStutteringTraits::graph_t pg;
			graph::pg::GovernedStutteringTraits::graph_t output;
			graph::pg::GovernedStutteringPartitioner p(pg);
			load(pg, instream);
			partition(m_equivalence, p, &output);
			save(output, outstream);
		}
		return true;
	}
protected:
	/// @brief Adds the --equivalence option (see mcrl2::utilities::tools::input_output_tool::add_options).
	void add_options(mcrl2::utilities::interface_description& desc)
	{
		mcrl2::utilities::tools::input_output_tool::add_options(desc);
		unsigned int i = 0;
		std::stringstream eqs;
		std::string eq = Equivalence::name(0);
		while (not eq.empty())
		{
			eqs << std::endl << "  " << eq << ": " << Equivalence::desc(i) << " reduction";
			eq = Equivalence::name(++i);
		}
		desc.add_option("equivalence", mcrl2::utilities::make_mandatory_argument("NAME"), "The conversion method to use, choose from" + eqs.str(), 'e');
	}
	/// @brief Parses the --equivalence option (see mcrl2::utilities::tools::input_output_tool::parse_options).
	void parse_options(const mcrl2::utilities::command_line_parser& parser)
	{
		mcrl2::utilities::tools::input_output_tool::parse_options(parser);
		if (parser.options.count("equivalence"))
		{
			m_equivalence = Equivalence(parser.option_argument("equivalence"));
			if (m_equivalence == Equivalence::invalid)
			{
				parser.error("option -e/--equivalence has illegal argument '" +
							 parser.option_argument("equivalence") + "'");
			}
		}
	}
};

int main(int argc, char** argv)
{
	return std::auto_ptr<pgconvert>(new pgconvert())->execute(argc, argv);
}
