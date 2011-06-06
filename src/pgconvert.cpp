#include "equivalence.h"
#include "parsers/pgsolver.h"
#include "parsers/dot.h"
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
		mCRL2log(verbose) << "Parity game contains " << graph.size() << " nodes after SCC reduction." << std::endl;
	}

	template <typename partitioner_t, typename graph_t>
	void partition(Equivalence e, partitioner_t& partitioner, graph_t* output=NULL)
	{
		timer().start("partition refinement");
		partitioner.partition(output);
		timer().finish("partition refinement");
		if (output)
		{
			mCRL2log(verbose) << "Parity game contains " << output->size() << " nodes after " << e.desc() << " reduction." << std::endl;
		}
	}

	template <typename graph_t>
	void load(graph_t& graph, std::istream& s)
	{
		mCRL2log(verbose) << "Loading parity game." << std::endl;
		timer().start("load");
		graph::Parser<typename graph_t::vertex_t, graph::pgsolver> parser(graph);
		parser.load(s);
		timer().finish("load");
		mCRL2log(verbose) << "Parity game contains " << graph.size() << " nodes." << std::endl;
	}

	template <typename graph_t>
	void save(graph_t& graph, std::ostream& s)
	{
		timer().start("save");
		graph::Parser<typename graph_t::vertex_t, graph::pgsolver> parser(graph);
		parser.dump(s);
		s << std::flush;
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
		mCRL2log(verbose) << "Reading from " << m_input_filename << "." << std::endl;
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
		mCRL2log(verbose) << "Writing to " << m_output_filename << "." << std::endl;
		return *outstream;
	}

	/// @brief Runs the tool (see mcrl2::utilities::tools::input_output_tool::run).
	bool run()
	{

		std::istream& instream = open_input();
		std::ostream& outstream = open_output();
		mCRL2log(verbose) << "Performing " << m_equivalence.desc() << " reduction." << std::endl;
		if (m_equivalence == Equivalence::scc)
		{
      typedef graph::KripkeStructure<graph::Vertex<graph::pg::Label> > graph_t;
			graph_t pg;
			load(pg, instream);
			timer().start("reduction");
			collapse_sccs(pg);
			timer().finish("reduction");
			save(pg, outstream);
		}
		else if (m_equivalence == Equivalence::stut)
		{
			typedef graph::StutteringPartitioner<graph::pg::DivLabel>::graph_t graph_t;
			graph_t pg;
			graph_t output;
			graph::StutteringPartitioner<graph::pg::DivLabel> p(pg);
			load(pg, instream);
			timer().start("reduction");
			collapse_sccs(pg);
			pg.resize(pg.size()+1);
			graph_t::vertex_t& divmark = pg.vertex(pg.size()-1);
			divmark.label.div = true;
			divmark.in.insert(pg.size()-1);
			divmark.out.insert(pg.size()-1);
			for (size_t i = 0; i < pg.size() -1; ++i)
			{
				graph_t::vertex_t& v = pg.vertex(i);
				if (v.label.div)
				{
					v.out.insert(pg.size()-1);
					divmark.in.insert(i);
					v.label.div = false;
				}
			}
			partition(m_equivalence, p, &output);
			size_t div = 0;
			for (size_t i = 0; i < output.size(); ++i)
			{
			  if (output.vertex(i).label.div)
			  {
          div = i;
			    graph_t::vertex_t& v = output.vertex(div);
			    for (graph::VertexSet::iterator j = v.in.begin(); j != v.in.end(); ++j)
			    {
			      output.vertex(*j).out.erase(i);
			      output.vertex(*j).out.insert(*j);
            output.vertex(*j).in.insert(*j);
			    }
			    output.vertex(i).in.clear();
			  }
			}
			for (size_t i = 0; i < output.size(); ++i)
			{
			  graph::VertexSet in, out;
			  for (graph::VertexSet::iterator it = output.vertex(i).in.begin(); it != output.vertex(i).in.end(); ++it)
			    in.insert(*it - (*it > div ? 1 : 0));
        for (graph::VertexSet::iterator it = output.vertex(i).out.begin(); it != output.vertex(i).out.end(); ++it)
          out.insert(*it - (*it > div ? 1 : 0));
			  output.vertex(i).in.swap(in);
			  output.vertex(i).out.swap(out);
			  if (i > div)
			    output.vertex(i - 1) = output.vertex(i);
			}
			output.resize(output.size() - 1);
			timer().finish("reduction");
			save(output, outstream);
		}
		else if (m_equivalence == Equivalence::gstut)
		{
			graph::pg::GovernedStutteringTraits::graph_t pg;
			graph::pg::GovernedStutteringTraits::graph_t output;
			graph::pg::GovernedStutteringPartitioner p(pg);
			load(pg, instream);
			timer().start("reduction");
			partition(m_equivalence, p, &output);
      timer().finish("reduction");
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
		else
		  parser.error("please specify an conversion method using the -e option.");
	}
};

int main(int argc, char** argv)
{
	return std::auto_ptr<pgconvert>(new pgconvert())->execute(argc, argv);
}
