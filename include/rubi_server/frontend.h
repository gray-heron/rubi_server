#pragma once

#include <boost/optional.hpp>
#include <memory>

class FrontendBoardHandler;
class RubiFrontend;

#include "communication.h"
#include "descriptors.h"

class RubiFrontend
{
  public:
    void operator=(RubiFrontend const &) = delete;

    virtual ~RubiFrontend() = default;

    virtual bool Init(int argc, char **argv) = 0;
    virtual std::vector<std::string> GetCansNames() = 0;
    virtual void Spin() = 0;
    virtual bool Quit() = 0;

    virtual void LogInfo(std::string msg) = 0;
    virtual void LogWarning(std::string msg) = 0;
    virtual void LogError(std::string msg) = 0;

    virtual void ReportCansUtilization(std::vector<float> util) = 0;

    virtual std::shared_ptr<FrontendBoardHandler>
    NewBoard(BoardInstance inst) = 0;
};

class FrontendBoardHandler
{
  public:
    virtual void FFDataInbound(std::vector<uint8_t> &data, int ffid) = 0;

    // A new handler has appeard
    virtual void ReplaceBackendHandler(sptr<BoardCommunicationHandler>) = 0;

    // The board has disconnected voluntarily.
    virtual void Shutdown() = 0;

    // Protocol with the board has been dropped.
    virtual void ConnectionLost() = 0;

  protected:
    // only RubiFrontend can create FrontendBoardHandler

    BoardInstance id;
};
