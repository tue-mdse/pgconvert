#include <string>

/**
 * @class Equivalence
 * @brief Class representing an equivalence that can be used to reduce a parity game.
 */
class Equivalence
{
public:
	/**
	 * Equivalences that an Equivalence object may represent.
	 */
	enum Eq {
		scc     = 0,//!< SCC decomposition
		stut    = 1,//!< Stuttering equivalence
		gstut   = 2,//!< Governed stuttering equivalence
		invalid = 3 //!< Value indicating invalid equivalence name.
	};
	/// @brief Default constructor.
	Equivalence() {}
	/// @brief Copy constructor.
	Equivalence(const Equivalence& other) : m_value(other.m_value) {}
	/**
	 * @brief Constructs an Equivalence object based on a textual representation.
	 * @param name The name of the equivalence. Can be one of @a m_names, any other
	 *   value is interpreted as @c invalid.
	 */
	Equivalence(const std::string& name)
	{
		if (m_names[(int)scc] == name) m_value = scc; else
		if (m_names[(int)stut] == name) m_value = stut; else
		if (m_names[(int)gstut] == name) m_value = gstut; else
		m_value = invalid;
	}
	/// @brief Returns the (short) name of the equivalence this object represents.
	const std::string name() const
	{
		return m_names[m_value];
	}
	/// @brief Returns a description (long name) of the equivalence this object represents.
	const std::string desc() const
	{
		return m_descs[m_value];
	}
	/// @brief Used to iterate through valid equivalence names.
	static const std::string name(unsigned int index)
	{
		if (index < invalid)
			return m_names[index];
		return std::string();
	}
	/// @brief Used to iterate through valid equivalence descriptions.
	static const std::string desc(unsigned int index)
	{
		if (index < invalid)
			return m_descs[index];
		return std::string();
	}
	/// @brief Compare with one of the predefined constants.
	bool operator==(Eq other) const
	{
		return m_value == other;
	}
private:
	Eq m_value;
	static const char* m_names[4];
	static const char* m_descs[4];
};
