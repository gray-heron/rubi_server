
#include "board.h"
#include "exceptions.h"
#include <memory>

BoardManager::BoardManager() {}

void BoardManager::Init(std::vector<std::string> cans_names)
{
    for (const auto &can_name : cans_names)
    {
        auto handler = std::make_shared<CanHandler>(can_name);
        cans.emplace_back(
            std::pair<std::string, sptr<CanHandler>>(can_name, handler));
    }
}

void BoardManager::Spin(std::chrono::system_clock::time_point time)
{
    std::vector<float> utilization;
    bool dump_traffic_load = false;

    if (std::chrono::duration_cast<std::chrono::seconds>(time - last_keepalive)
            .count() >= cans_load_collection_time)
    {
        last_keepalive = time;
        dump_traffic_load = true;
    }

    for (const auto &can_entry : cans)
    {
        can_entry.second->Tick(time);
        if (dump_traffic_load)
            utilization.push_back(can_entry.second->GetTrafficSoFar(true) /
                                  cans_load_collection_time);
    }

    if (dump_traffic_load)
        frontend->ReportCansUtilization(utilization);
}

void BoardManager::RegisterNewHandler(
    sptr<BoardCommunicationHandler> new_backend_handler)
{
    sptr<BoardCommunicationHandler> old_backend_handler;
    auto board_id = new_backend_handler->GetBoard().id;
    sptr<BoardDescriptor> board_descriptor =
        new_backend_handler->GetBoard().descriptor;

    new_backend_handler->HandshakeComplete();

    std::map<sptr<BoardDescriptor>, std::vector<BoardInstance>>::iterator
        handlers_bank = BoardManager::inst().handlers.end();

    for (auto handler_entry = BoardManager::inst().handlers.begin();
         handler_entry != BoardManager::inst().handlers.end(); ++handler_entry)
    {
        if (handler_entry->first->board_name ==
            new_backend_handler->GetBoard().descriptor->board_name)
        {
            ASSERT(*new_backend_handler->GetBoard().descriptor ==
                   *handler_entry->first);
            board_descriptor = handler_entry->first;
            new_backend_handler->GetBoard().descriptor = handler_entry->first;
            handlers_bank = handler_entry;

            break;
        }
    }

    std::vector<BoardInstance>::iterator i;

    if (handlers_bank != BoardManager::inst().handlers.end())
    {
        for (i = handlers_bank->second.begin();
             i != handlers_bank->second.end(); ++i)
        {
            if (i->id == board_id)
            {
                if ((old_backend_handler = i->backend_handler.lock()))
                    break;
            }
        }
    }
    else
    {
        BoardManager::inst().handlers[board_descriptor] =
            std::vector<BoardInstance>();
    }

    if (old_backend_handler)
    {
        if (old_backend_handler->IsDead())
        {
            log.Info(string("Replacing dead handler for board ") +
                     (string)old_backend_handler->GetBoard());

            BoardManager::inst()
                .handlers[board_descriptor][i - handlers_bank->second.begin()]
                .backend_handler.reset();
            BoardManager::inst()
                .handlers[board_descriptor][i - handlers_bank->second.begin()]
                .backend_handler = new_backend_handler;

            old_backend_handler->GetFrontendHandler()->ReplaceBackendHandler(
                new_backend_handler);

            new_backend_handler->Launch(
                old_backend_handler->GetFrontendHandler());
        }
        else
        {
            log.Warning(string("Handler for ") +
                        (string)old_backend_handler->GetBoard() +
                        " already exists. Putting new "
                        "connection on hold.");

            holden_handlers.insert(new_backend_handler);
        }
    }
    else
    {
        BoardManager::inst().handlers[board_descriptor].push_back(
            new_backend_handler->GetBoard());

        new_backend_handler->Launch(BoardManager::inst().frontend->NewBoard(
            new_backend_handler->GetBoard()));
    }
}

sptr<BoardCommunicationHandler>
BoardManager::RequestNewHandler(BoardInstance inst,
                                sptr<FrontendBoardHandler> frontend)
{
    auto board_id = inst.id;
    auto board_descriptor = inst.descriptor;

    std::vector<BoardInstance>::iterator i;

    sptr<BoardCommunicationHandler> new_handler, old_handler;
    for (const auto &holden_handler : holden_handlers)
    {
        if (!holden_handler->IsDead() && !holden_handler->IsLost() &&
            *holden_handler->GetBoard().descriptor == *inst.descriptor &&
            holden_handler->GetBoard().id == inst.id)
        {
            new_handler = holden_handler;
            break;
        }
    }

    if (!new_handler)
        return nullptr;

    holden_handlers.erase(new_handler);

    const auto &handlers_bank =
        BoardManager::inst().handlers.find(board_descriptor);
    for (i = handlers_bank->second.begin(); i != handlers_bank->second.end();
         ++i)
    {
        if (i->id == board_id)
        {
            old_handler = i->backend_handler.lock();

            auto &board_inst = BoardManager::inst()
                                   .handlers[board_descriptor]
                                            [i - handlers_bank->second.begin()];

            board_inst.backend_handler = new_handler;
            new_handler->Launch(frontend);

            log.Info("Replacing dead handler for board " +
                     (string)old_handler->GetBoard());

            break;
        }
    }

    if (!old_handler || !old_handler->IsDead())
    {
        old_handler->Hold();
        holden_handlers.insert(old_handler);
        log.Warning("Putting active handler for board " +
                    (string)old_handler->GetBoard() +
                    " on hold on replacement.");
    }

    return new_handler;
}
