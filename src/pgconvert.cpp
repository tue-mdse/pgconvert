#include "equivalence.h"
#include "parsers/pgsolver.h"
#include "parsers/dot.h"
#include "govstut.h"
#include "wgovstut.h"
#include "bisim.h"
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
    pgconvert() :
        mcrl2::utilities::tools::input_output_tool(
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
            "None")
    {
    }

    template<typename graph_t, typename partitioner_t>
    void dump_dot(graph_t& graph, partitioner_t& partitioner)
    {
      typedef typename graph_t::vertex_t vertex_t;
      graph::pg::VertexFormatter<vertex_t> fmt;
      graph::Parser<vertex_t, graph::dot> p(graph, fmt);
      p.dump(std::cout, partitioner);
    }

    template<typename graph_t>
    void
    collapse_sccs(graph_t& graph)
    {
      timer().start("scc reduction");
      graph.collapse_sccs();
      timer().finish("scc reduction");
      mCRL2log(mcrl2::log::verbose)
        << "Parity game contains " << graph.size() << " nodes and "
            << graph.num_edges() << " edges after SCC reduction."
            << std::endl;
    }

    template<typename graph_t>
    void
    encode_divergence(graph_t& pg)
    {
      pg.resize(pg.size() + 1);
      typename graph_t::vertex_t& divmark = pg.vertex(pg.size() - 1);
      divmark.label.div = true;
      for (size_t i = 0; i < pg.size() - 1; ++i)
      {
        typename graph_t::vertex_t& v = pg.vertex(i);
        if (v.label.div)
        {
          v.out.insert(pg.size() - 1);
          divmark.in.insert(i);
          v.label.div = false;
        }
      }
    }

    template<typename graph_t>
    void
    decode_divergence(graph_t& pg)
    {
      size_t div = 0;
      for (size_t i = 0; i < pg.size(); ++i)
      {
        if (pg.vertex(i).label.div)
        {
          div = i;
          typename graph_t::vertex_t& v = pg.vertex(div);
          for (graph::VertexSet::iterator j = v.in.begin(); j != v.in.end();
              ++j)
              {
            pg.vertex(*j).out.erase(i);
            pg.vertex(*j).out.insert(*j);
            pg.vertex(*j).in.insert(*j);
          }
          pg.vertex(i).in.clear();
        }
      }
      for (size_t i = 0; i < pg.size(); ++i)
      {
        graph::VertexSet in, out;
        for (graph::VertexSet::iterator it = pg.vertex(i).in.begin();
            it != pg.vertex(i).in.end(); ++it)
          in.insert(*it - (*it > div ? 1 : 0));
        for (graph::VertexSet::iterator it = pg.vertex(i).out.begin();
            it != pg.vertex(i).out.end(); ++it)
          out.insert(*it - (*it > div ? 1 : 0));
        pg.vertex(i).in.swap(in);
        pg.vertex(i).out.swap(out);
        if (i > div)
          pg.vertex(i - 1) = pg.vertex(i);
      }
      pg.resize(pg.size() - 1);
      mCRL2log(mcrl2::log::verbose)
        << "Parity game contains " << pg.size() << " nodes and "
            << pg.num_edges() << " edges after restoring divergences."
            << std::endl;
    }

    template<typename partitioner_t, typename graph_t>
    void
    partition(Equivalence e, partitioner_t& partitioner, graph_t* output =
        NULL)
    {
      timer().start("partition refinement");
      partitioner.partition(output);
      timer().finish("partition refinement");
      if (output)
      {
        mCRL2log(mcrl2::log::verbose)
          << "Parity game contains " << output->size() << " nodes and "
              << output->num_edges() << " edges after " << e.desc()
              << " reduction." << std::endl;
      }
    }

    template<typename graph_t>
    void
    load(graph_t& graph, std::istream& s)
    {
      mCRL2log(mcrl2::log::verbose)
        << "Loading parity game." << std::endl;
      timer().start("load");
      graph::Parser<typename graph_t::vertex_t, graph::pgsolver> parser(
          graph);
      parser.load(s);
      timer().finish("load");
      mCRL2log(mcrl2::log::verbose)
        << "Parity game contains " << graph.size() << " nodes and "
            << graph.num_edges() << " edges." << std::endl;
    }

    template<typename graph_t>
    void
    save(graph_t& graph, std::ostream& s)
    {
      timer().start("save");
      graph::Parser<typename graph_t::vertex_t, graph::pgsolver> parser(
          graph);
      parser.dump(s);
      s << std::flush;
      timer().finish("save");
    }

    std::istream&
    open_input()
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
      mCRL2log(mcrl2::log::verbose)
        << "Reading from " << m_input_filename << "." << std::endl;
      return *instream;
    }

    std::ostream&
    open_output()
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
      mCRL2log(mcrl2::log::verbose)
        << "Writing to " << m_output_filename << "." << std::endl;
      return *outstream;
    }

    void
    run_scc(std::istream& instream, std::ostream& outstream)
    {
      typedef graph::KripkeStructure<graph::Vertex<graph::pg::DivLabel> > graph_t;
      graph_t pg;
      load(pg, instream);
      timer().start("reduction");
      collapse_sccs(pg);
      for (size_t i = 0; i < pg.size(); ++i)
      {
        if (pg.vertex(i).label.div)
          pg.vertex(i).out.insert(i);
      }
      timer().finish("reduction");
      save(pg, outstream);
    }

    void
    run_bisim(std::istream& instream, std::ostream& outstream)
    {
      typedef graph::BisimulationPartitioner<graph::pg::DivLabel>::graph_t graph_t;
      graph_t pg;
      graph_t output;
      graph::BisimulationPartitioner<graph::pg::DivLabel> p(pg);
      load(pg, instream);
      timer().start("reduction");
      partition(m_equivalence, p, &output);
      timer().finish("reduction");
      save(output, outstream);
    }

    void
    run_fmib(std::istream& instream, std::ostream& outstream)
    {
      throw mcrl2::runtime_error("FMIB not implemented yet.");
    }

    void
    run_stut(std::istream& instream, std::ostream& outstream)
    {
      typedef graph::StutteringPartitioner<graph::pg::DivLabel>::graph_t graph_t;
      graph_t pg;
      graph_t output;
      graph::StutteringPartitioner<graph::pg::DivLabel> p(pg);
      load(pg, instream);
      timer().start("reduction");
      collapse_sccs(pg);
      encode_divergence(pg);
      partition(m_equivalence, p, &output);
      decode_divergence(output);
      timer().finish("reduction");
      save(output, outstream);
    }

    void
    run_gstut(std::istream& instream, std::ostream& outstream)
    {
      typedef graph::pg::GovernedStutteringPartitioner<graph::pg::Label>::graph_t graph_t;
      graph_t pg;
      graph_t output;
      graph::pg::GovernedStutteringPartitioner<graph::pg::Label> p(pg);
      load(pg, instream);
      timer().start("reduction");
      partition(m_equivalence, p, &output);
      timer().finish("reduction");
      save(output, outstream);
    }

    void
    run_scc_gstut(std::istream& instream, std::ostream& outstream)
    {
      typedef graph::pg::GovernedStutteringPartitioner<graph::pg::DivLabel>::graph_t graph_t;
      graph_t pg;
      graph_t output;
      graph::pg::GovernedStutteringPartitioner<graph::pg::DivLabel> p(pg);
      load(pg, instream);
      timer().start("reduction");
      collapse_sccs(pg);
      encode_divergence(pg);
      partition(m_equivalence, p, &output);
      decode_divergence(output);
      timer().finish("reduction");
      save(output, outstream);
    }

    void
    run_wgstut(std::istream& instream, std::ostream& outstream)
    {
      typedef graph::pg::GovernedStutteringPartitioner<graph::pg::Label>::graph_t graph_t;
      graph_t pg1;
      graph_t pg2;
      graph::pg::ParadisePartitioner<graph::pg::Label> pp(pg1);
      graph::pg::GovernedStutteringPartitioner<graph::pg::Label> gsp(pg2);
      load(pg1, instream);
      timer().start("reduction");
      timer().start("paradise reduction");
      pp.partition(&pg2);
      timer().finish("paradise reduction");
      pg1.resize(0);
      partition(m_equivalence, gsp, &pg1);
      timer().finish("reduction");
      save(pg1, outstream);
    }

    /// @brief Runs the tool (see mcrl2::utilities::tools::input_output_tool::run).
    bool
    run()
    {
      std::istream& instream = open_input();
      std::ostream& outstream = open_output();
      mCRL2log(mcrl2::log::verbose)
        << "Performing " << m_equivalence.desc() << " reduction." << std::endl;
      if (m_equivalence == Equivalence::scc)
        run_scc(instream, outstream);
      else if (m_equivalence == Equivalence::bisim)
        run_bisim(instream, outstream);
      else if (m_equivalence == Equivalence::fmib)
        run_fmib(instream, outstream);
      else if (m_equivalence == Equivalence::stut)
        run_stut(instream, outstream);
      else if (m_equivalence == Equivalence::gstut)
        run_gstut(instream, outstream);
      else if (m_equivalence == Equivalence::scc_gstut)
        run_scc_gstut(instream, outstream);
      else if (m_equivalence == Equivalence::wgstut)
        run_wgstut(instream, outstream);
      return true;
    }
  protected:
    /// @brief Adds the --equivalence option (see mcrl2::utilities::tools::input_output_tool::add_options).
    void
    add_options(mcrl2::utilities::interface_description& desc)
    {
      mcrl2::utilities::tools::input_output_tool::add_options(desc);
      unsigned int i = 0;
      std::stringstream eqs;
      std::string eq = Equivalence::name(0);
      while (not eq.empty())
      {
        eqs << std::endl << "  " << eq << ": " << Equivalence::desc(i)
            << " reduction";
        eq = Equivalence::name(++i);
      }
      desc.add_option("equivalence",
          mcrl2::utilities::make_mandatory_argument("NAME"),
          "The conversion method to use, choose from" + eqs.str(), 'e');
    }
    /// @brief Parses the --equivalence option (see mcrl2::utilities::tools::input_output_tool::parse_options).
    void
    parse_options(const mcrl2::utilities::command_line_parser& parser)
    {
      mcrl2::utilities::tools::input_output_tool::parse_options(parser);
      if (parser.options.count("equivalence"))
      {
        m_equivalence = Equivalence(parser.option_argument("equivalence"));
        if (m_equivalence == Equivalence::invalid)
        {
          parser.error(
              "option -e/--equivalence has illegal argument '"
                  + parser.option_argument("equivalence") + "'");
        }
      }
      else
        parser.error(
            "please specify an conversion method using the -e option.");
    }
};

int
main(int argc, char** argv)
{
  return std::auto_ptr<pgconvert>(new pgconvert())->execute(argc, argv);
}
