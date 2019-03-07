#pragma once

#include <boost/optional.hpp>
#include <memory>
#include <string>

#include "board.h"
#include "communication.h"
#include "descriptors.h"
#include "frontend.h"

class RosBoardHandler;
class BoardCommunicationHandler;

class RosModule : public RubiFrontend
{
    friend class RosBoardHandler;

  private:
    struct ros_stuff_t;
    ros_stuff_t *ros_stuff;

    std::vector<sptr<RosBoardHandler>> boards;
    std::vector<std::string> cans_names;

    Logger log{"RosModule"};

  public:
    RosModule();
    RosModule(RosModule const &) = delete;
    void operator=(RosModule const &) = delete;

    bool Init(int argc, char **argv) override;
    std::vector<std::string> GetCansNames() override;

    void Spin() override;
    bool Quit() override;

    void LogInfo(std::string msg) override;
    void LogWarning(std::string msg) override;
    void LogError(std::string msg) override;

    void ReportCansUtilization(std::vector<float> util) override;

    std::shared_ptr<FrontendBoardHandler> NewBoard(BoardInstance inst) override;
};

class RosBoardHandler : public FrontendBoardHandler,
                        public std::enable_shared_from_this<RosBoardHandler>
{
    friend class RosModule;
    RosModule *ros_module;

    struct roshandler_stuff_t;
    std::unique_ptr<roshandler_stuff_t> ros_stuff;

    enum fftype_t
    {
        fftype_field = 1,
        fftype_function
    };

    std::vector<int> fieldtable;
    std::vector<std::pair<fftype_t, int>> fftable;

    Logger log{"RosBoardHandler"};

  public:
    sptr<BoardCommunicationHandler> BackendReady();
    BoardInstance board;

    void FFDataInbound(std::vector<uint8_t> &data, int ffid) override;

    int GetFieldFfid(int field_id);
    int GetFunctionFfid(int function_id);

    virtual void
        ReplaceBackendHandler(sptr<BoardCommunicationHandler>) override;
    virtual void Shutdown() override;
    virtual void ConnectionLost() override;

    RosBoardHandler(BoardInstance inst, RosModule *ros_module);
    void Init();
};
