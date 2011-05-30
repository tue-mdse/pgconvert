#include "pg.h"

#include <string>
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <limits>

namespace pg
{

namespace detail {

  void parse_error(std::istream& s, const char* msg)
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

  void ParityGame::parse_header(std::istream& s)
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
	  m_vertices.resize(n + 1);
	  s >> c;
	  if (c != ';')
		  parse_error(s, "Invalid header, expected semicolon.");
  }

  void ParityGame::parse_vertex(std::istream& s)
  {
	  size_t index;
	  size_t succ;
	  char c;
	  s >> index;
	  if (s.fail())
	  {
		  if (s.eof()) return;
		  parse_error(s, "Could not parse vertex index.");
	  }
	  if (index >= m_vertices.size())
		  m_vertices.resize(index + 1);
	  Vertex& v = m_vertices[index];
	  s >> v.prio;
	  if (s.fail())
		  parse_error(s, "Could not parse vertex priority.");
	  s >> c;
	  if (s.fail() or c < '0' or c > '1')
		  parse_error(s, "Could not parse vertex player.");
	  v.player = c == '0' ? even : odd;
	  s >> succ;
	  if (s.fail())
		  parse_error(s, "Could not parse successor index (vertex with outdegree 0?).");
	  do
	  {
		  v.out.insert(succ);
	  	  if (succ >= m_vertices.size())
			  m_vertices.resize(succ + 1);
		  m_vertices[succ].in.insert(index);
	  	  s >> c;
	  	  if (s.fail()) c = ';'; // Allow missing semicolon at end of file.
	  	  if (c == ';') break;
		  s >> succ;
		  if (s.fail())
			  parse_error(s, "Could not parse successor index after comma.");
	  } while (c == ',');
	  if (c == '"')
	  {
		  s.ignore(std::numeric_limits<std::streamsize>::max(), '"');
	  }
	  if (c != ';')
		  parse_error(s, "Invalid vertex specification, expected semicolon.");
  }

  void ParityGame::parse_body(std::istream& s)
  {
	  size_t n = 0;
	  while (!s.eof())
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

}

void ParityGame::load(std::istream& s)
{
	parse_header(s);
	parse_body(s);
}

void ParityGame::dump(std::ostream& s)
{
	if (m_vertices.empty()) return;
	  s << "parity " << m_vertices.size() - 1 << ";" << std::endl;
	  size_t index = 0;
	  for (std::vector<Vertex>::const_iterator v = m_vertices.begin(); v != m_vertices.end(); ++v, ++index)
	  {
		  s << index << ' ' << v->prio << ' ' << (v->player == even ? '0' : '1');
		  VertexSet::const_iterator succ = v->out.begin();
		  if (succ != v->out.end())
		  {
			  s << ' ' << *succ;
			  while (++succ != v->out.end())
			  {
				  s << ',' << *succ;
			  }
		  }
		  else
			  s << " \"no outgoing edges!\"";
		  s << ';' << std::endl;
	  }
}

}
