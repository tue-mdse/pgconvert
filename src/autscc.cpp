#include "parsers/aut.h"
#include "detail/scc.h"
#include "lts.h"

#include "mcrl2/utilities/input_output_tool.h"
#include "mcrl2/utilities/logger.h"

#include <sstream>
#include <iostream>
#include <fstream>

/**
 * @class pgconvert
 * @brief Tool class that can execute parity game reductions.
 */
class autscc : public mcrl2::utilities::tools::input_output_tool
{
  private:
    std::auto_ptr<std::ifstream> m_ifstream;
    std::auto_ptr<std::ofstream> m_ofstream;
  public:
    typedef graph::KripkeStructure<graph::Vertex<graph::lts::DivLabel> > graph_t;
    autscc() :
        mcrl2::utilities::tools::input_output_tool(
        // Tool name:
            "autscc",
            // Author:
            "S. Cranen",
            // Tool summary:
            "Dumps SCCs within a .aut file.",
            // Tool description:
            "Dumps SCCs within a .aut file.",
            // Known issues:
            "None")
    {
    }

    void
    load(graph_t& graph, std::istream& s)
    {
      mCRL2log(mcrl2::log::verbose)
        << "Loading statespace." << std::endl;
      timer().start("load");
      graph::Parser<graph_t::vertex_t, graph::aut> parser(
          graph);
      parser.load(s);
      timer().finish("load");
      mCRL2log(mcrl2::log::verbose)
        << "Parity game contains " << graph.size() << " nodes and "
            << graph.num_edges() << " edges." << std::endl;
    }

    void
    save(std::vector<std::list<graph::VertexIndex> >& sccs, std::ostream& s)
    {
      timer().start("save");
      for (size_t i = 0; i < sccs.size(); ++i)
      {
        if (!sccs[i].empty())
        {
          for (std::list<graph::VertexIndex>::iterator n = sccs[i].begin(); n != sccs[i].end(); ++n)
            s << *n << ' ';
          s << std::endl;
        }
      }
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
      graph_t* lts = new graph_t;
      load(*lts, instream);
      timer().start("scc decomposition");
      std::vector<graph::VertexIndex> scc;
      scc.resize(lts->size());
      size_t highscc = graph::impl::tarjan_iterative(lts->vertices(), scc);
      delete lts;

      std::vector<std::list<graph::VertexIndex> > sccs;
      sccs.resize(highscc + 1);
      for (graph::VertexIndex i = 0; i < scc.size(); ++i)
      {
          sccs[scc[i]].push_back(i);
      }
      scc.clear();

      timer().finish("scc decomposition");
      save(sccs, outstream);
    }

    /// @brief Runs the tool (see mcrl2::utilities::tools::input_output_tool::run).
    bool
    run()
    {
      std::istream& instream = open_input();
      std::ostream& outstream = open_output();
      run_scc(instream, outstream);
      return true;
    }
};

int
main(int argc, char** argv)
{
  return std::auto_ptr<autscc>(new autscc())->execute(argc, argv);
}
