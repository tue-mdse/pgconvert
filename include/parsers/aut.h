#include "pg.h"
#include "graph.h"

#include <limits>
#include <stdexcept>
#include <sstream>

namespace graph
{

  template<typename Vertex>
  class Parser<Vertex, aut>
  {
    public:
      typedef graph::KripkeStructure<Vertex> graph_t;
      Parser(graph_t& pg) :
        m_pg(pg)
      {
      }
      void
      load(std::istream& s)
      {
        parse_header(s);
        parse_body(s);
      }
      void
      dump(std::ostream& s)
      {
        throw std::runtime_error("Dumping .aut file is not supported.");
      }
    private:
      graph_t& m_pg;
      size_t m_trans;
      size_t m_first;

      void
      parse_error(std::istream& s, const char* msg)
      {
        s.clear();
        std::stringstream buf;
        std::string token;
        std::streampos p = s.tellg();
        std::streamsize l = 0;
        std::streamsize b = 0;
        s >> token;
        s.seekg(0, std::ios::beg);
        while (s.tellg() < p)
        {
          ++l;
          b = s.tellg();
          s.ignore(p, '\n');
        }
        buf << msg << " Error occurred while parsing '" << token
            << "' at line " << l << ", column " << (p - b) << ".";
        throw std::runtime_error(buf.str());
      }

      void
      parse_header(std::istream& s)
      {
        std::string firstword;
        s >> std::skipws >> firstword;
        if (firstword != "des")
          parse_error(s, "Invalid header, expected 'des'.");
        size_t states;
        char c;
        s >> c;
        if (c != '(')
          parse_error(s, "Invalid header, expected '(' after 'des'.");
        s >> m_first;
        if (s.fail())
          parse_error(s, "Invalid header, could not parse initial state number.");
        s >> c;
        if (c != ',')
          parse_error(s, "Invalid header, expected ',' after initial state number.");
        s >> m_trans;
        if (s.fail())
          parse_error(s, "Invalid header, could not parse transition count.");
        s >> c;
        if (c != ',')
          parse_error(s, "Invalid header, expected ',' after transition count.");
        s >> states;
        if (s.fail())
          parse_error(s, "Invalid header, could not parse state count.");
        s >> c;
        if (c != ')')
          parse_error(s, "Invalid header, expected ')' after state count.");
        m_pg.resize(states);
      }

      void
      parse_trans(std::istream& s)
      {
        size_t from;
        size_t to;

        char c;
        s >> c;
        if (s.fail() && s.eof())
          return;
        if (c != '(')
          parse_error(s, "Invalid transition, expected it to start with '('.");
        s >> from;
        if (s.fail())
          parse_error(s, "Could not parse source state index.");
        s >> c;
        if (c != ',')
          parse_error(s, "Invalid transition, expected ',' after source state.");
        s >> c;
        if (c != '"')
          parse_error(s, "Invalid transition, label must start with '\"'.");
        do
        {
            s >> c;
        }
        while (c != '"' && s.good());
        s >> c;
        if (c != ',')
          parse_error(s, "Invalid transition, expected ',' after transition label.");
        s >> to;
        if (s.fail())
          parse_error(s, "Could not parse target state index.");
        s >> c;
        if (c != ')')
          parse_error(s, "Invalid transition, expected it to end with ')'.");

        typename graph_t::vertex_t& vf = m_pg.vertex(from);
        vf.out.insert(to);
        typename graph_t::vertex_t& vt = m_pg.vertex(to);
        vt.in.insert(from);
      }

      void
      parse_body(std::istream& s)
      {
        size_t n = 0;
        while (!s.eof() && n < m_trans)
        {
          try
          {
            parse_trans(s);
            ++n;
          }
          catch (std::runtime_error& e)
          {
            std::stringstream msg;
            msg << "Could not parse vertex " << n << ": " << e.what();
            throw std::runtime_error(msg.str());
          }
        }
        parse_trans(s);
        if (!s.eof())
          parse_error(s, "More transitions in file than specified in header.");
      }
  };

} // namespace graph
