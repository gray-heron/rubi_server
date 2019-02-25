#ifndef H_BOARD
#define H_BOARD

#include <boost/optional.hpp>
#include <chrono>
#include <ctime>
#include <map>
#include <map>
#include <set>
#include <string>
#include <vector>

#include "descriptors.h"
#include "frontend.h"

// this singleton is not beautiful
class BoardManager
{
public:
  static BoardManager &inst()
  {
    static BoardManager instance;
    return instance;
  }

  sptr<RubiFrontend> frontend;

  std::map<std::string, sptr<BoardDescriptor>> descriptor_map;
  std::map<sptr<BoardDescriptor>, std::vector<BoardInstance>> handlers;
  std::vector<std::pair<std::string, sptr<CanHandler>>> cans;
  std::set<sptr<BoardCommunicationHandler>> holden_handlers;

  std::chrono::system_clock::time_point last_keepalive;

  float cans_load_collection_time = 3.0;

private:
  BoardManager();

  Logger log{"BoardManager"};

public:
  BoardManager(BoardManager const &) = delete;
  void operator=(BoardManager const &) = delete;

  void Init(std::vector<std::string> cans);
  void Spin(std::chrono::system_clock::time_point);
  void RegisterNewHandler(sptr<BoardCommunicationHandler> handler);

  sptr<BoardCommunicationHandler>
  RequestNewHandler(BoardInstance inst, sptr<FrontendBoardHandler> frontend);
};

#endif