
#include <iomanip>

#include <dyna_cpp/dyna/KeyFile.hpp>
#include <dyna_cpp/dyna/NodeKeyword.hpp>

namespace qd {

NodeKeyword::NodeKeyword(DB_Nodes* _db_nodes,
                         const std::vector<std::string>& _lines,
                         int64_t _iLine)
  : Keyword(_lines, _iLine)
  , db_nodes(_db_nodes)
{

  kw_type = KeywordType::NODE;

  // find first card line
  size_t header_size = iCard_to_iLine(0, false);
  size_t iLine = header_size;

  // extra card treatment
  size_t nAdditionalLines = 0;
  auto keyword_name_lower = to_lower_copy(get_keyword_name());

  // extract node data
  std::string remaining_data;
  int32_t node_id;
  std::vector<float> coords(3);
  field_size = has_long_fields() ? 16 : 8;
  auto field_size_x2 = 2 * field_size;
  auto field_size_x3 = 3 * field_size;
  auto field_size_x5 = 5 * field_size;
  auto field_size_x6 = 6 * field_size;

  for (; iLine < lines.size(); ++iLine) {

    const auto& line = lines[iLine];
    auto line_trimmed = trim_copy(line);

    if (line_trimmed.empty() || is_keyword(line) || is_comment(line))
      break;

    // parse line
    try {

      // node stuff
      node_id = std::stoi(line.substr(0, field_size));
      // wtf optional coordinates ?!?!?! should not cause too many cache misses
      if (field_size < line.size())
        coords[0] = std::stof(line.substr(field_size, field_size_x2));
      else
        coords[0] = 0.f;
      if (field_size_x3 < line.size())
        coords[1] = std::stof(line.substr(field_size_x3, field_size_x2));
      else
        coords[1] = 0.f;
      if (field_size_x5 < line.size())
        coords[2] = std::stof(line.substr(field_size_x5, field_size_x2));
      else
        coords[2] = 0.f;

      // remainder
      if (field_size_x6 < line.size())
        remaining_data = std::string(line.begin() + field_size_x6, line.end());
      else
        remaining_data = std::string();

      db_nodes->add_node(node_id, coords);
      node_indexes_in_card.push_back(db_nodes->get_index_from_id(node_id));
      unparsed_node_data.push_back(remaining_data);

    } catch (std::exception& err) {
      std::cout << "Parsing error in line: " << (_iLine + iLine + 1) << '\n'
                << "error:" << err.what() << '\n'
                << "line :" << line << '\n';
    }
  }

  // push the gear in the rear :P
  for (; iLine < lines.size(); ++iLine)
    trailing_lines.push_back(lines[iLine]);

  // remove all lines below keyword
  lines.resize(header_size);
}

/** Get the keyword as a string
 *
 * @return keyword as string
 */
std::string
NodeKeyword::str()
{
  // build header
  std::stringstream ss;
  for (const auto& entry : lines)
    ss << entry << '\n';

  // insert nodes
  const size_t id_width = field_size;
  const size_t float_width = 2 * field_size;

  ss.precision(7); // float
  for (size_t iNode = 0; iNode < this->get_nNodes(); ++iNode) {

    auto node = this->get_nodeByIndex(iNode);
    auto coords = node->get_coords()[0];
    ss << std::setw(id_width) << node->get_nodeID() << std::setw(float_width)
       << coords[0] << std::setw(float_width) << coords[1]
       << std::setw(float_width) << coords[2] << unparsed_node_data[iNode]
       << '\n';
  }

  // trailing lines
  for (const auto& line : trailing_lines)
    ss << line << '\n';

  return ss.str();
}

} // NAMESPACE:qd