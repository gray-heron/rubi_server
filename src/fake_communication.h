#include <boost/any.hpp>
#include <string>
#include <tuple>

#include "communication.h"

typedef std::tuple<std::string, std::vector<std::string>, int, boost::any>
    fields_desc;

class CommunicationFaker
{
  public:
    CommunicationFaker();
    std::vector<std::vector<boost::any>> default_values;

    sptr<BoardCommunicationHandler>
    FakeBoard(std::string board_name,
              std::vector<std::pair<fields_desc, int>> fields);
    void Tick();
};
