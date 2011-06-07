#include "pg.h"
#include "graph.h"

#include <limits>
#include <stdexcept>
#include <sstream>

namespace graph
{

  template<typename Vertex>
  class Parser<Vertex, pgsolver>
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
        if (m_pg.empty())
          return;
        s << "parity " << m_pg.size() - 1 << ";" << std::endl;
        for (size_t i = 0; i < m_pg.size(); ++i)
        {
          typename graph_t::vertex_t& v = m_pg.vertex(i);
          s << i << ' ' << v.label.prio << ' '
              << (v.label.player == pg::even ? '0' : '1');
          VertexSet::const_iterator succ = v.out.begin();
          if (succ != v.out.end())
          {
            s << ' ' << *succ;
            while (++succ != v.out.end())
            {
              s << ',' << *succ;
            }
          }
          else
            s << " \"no outgoing edges!\"";
          s << ';' << std::endl;
        }
      }
    private:
      graph_t& m_pg;
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
        if (firstword != "parity")
        {
          s.seekg(-firstword.length(), std::ios::cur);
          return;
        }
        size_t n;
        char c;
        s >> n;
        m_pg.resize(n + 1);
        s >> c;
        if (c != ';')
          parse_error(s, "Invalid header, expected semicolon.");
      }

      void
      parse_vertex(std::istream& s)
      {
        size_t index;
        size_t succ;
        char c;
        s >> index;
        if (s.fail())
        {
          if (s.eof())
            return;
          parse_error(s, "Could not parse vertex index.");
        }
        if (index >= m_pg.size())
          m_pg.resize(index + 1);
        typename graph_t::vertex_t& v = m_pg.vertex(index);
        s >> v.label.prio;
        if (s.fail())
          parse_error(s, "Could not parse vertex priority.");
        s >> c;
        if (s.fail() or c < '0' or c > '1')
          parse_error(s, "Could not parse vertex player.");
        v.label.player = c == '0' ? pg::even : pg::odd;
        do
        {
          s >> succ;
          if (s.fail())
            parse_error(s, "Could not parse successor index.");
          v.out.insert(succ);
          if (succ >= m_pg.size())
            m_pg.resize(succ + 1);
          m_pg.vertex(succ).in.insert(index);
          s >> c;
          if (s.fail())
            c = ';'; // Allow missing semicolon at end of file.
        }
        while (c == ',');
        if (c == '"')
        {
          s.ignore(std::numeric_limits<std::streamsize>::max(), '"');
          s >> c;
        }
        if (c != ';')
          parse_error(s, "Invalid vertex specification, expected semicolon.");
      }

      void
      parse_body(std::istream& s)
      {
        size_t n = 0, N = m_pg.size();
        N = N ? N : (size_t) -1;
        while (!s.eof() && n != N)
        {
          try
          {
            parse_vertex(s);
            ++n;
          }
          catch (std::runtime_error& e)
          {
            std::stringstream msg;
            msg << "Could not parse vertex " << n << ": " << e.what();
            throw std::runtime_error(msg.str());
          }
        }
      }
  };

} // namespace graph
