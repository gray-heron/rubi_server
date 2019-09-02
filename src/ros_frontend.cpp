#include <boost/algorithm/string.hpp>
#include <functional>
#include <thread>

#include <rubi_server/BoardAnnounce.h>
#include <rubi_server/BoardDescriptor.h>
#include <rubi_server/BoardInstances.h>
#include <rubi_server/FieldDescriptor.h>
#include <rubi_server/FuncDescriptor.h>
#include <rubi_server/ShowBoards.h>

#include <rubi_server/BoardOnline.h>
#include <rubi_server/BoardWake.h>
#include <rubi_server/CansNames.h>
#include <rubi_server/RubiBool.h>
#include <rubi_server/RubiFloat.h>
#include <rubi_server/RubiInt.h>
#include <rubi_server/RubiString.h>
#include <rubi_server/RubiUnsignedInt.h>

#include <std_msgs/Empty.h>
#include <std_msgs/Float32MultiArray.h>

#include <ros/ros.h>

#include "descriptors.h"
#include "exceptions.h"
#include "ros_frontend.h"
#include "rubi_autodefs.h"

using std::string;

struct RosModule::ros_stuff_t
{
    ros::NodeHandle *n;
    ros::Publisher board_announcer;
    ros::ServiceServer can_names_server;
    ros::Publisher can_load_publisher;
    ros::Subscriber panic_subscriber;

    ros::ServiceServer field_server;
    ros::ServiceServer func_server;
    ros::ServiceServer board_shower;
    ros::ServiceServer board_descriptor;
    ros::ServiceServer board_instances;

    ros::Rate *loop_rate;
};

struct RosBoardHandler::roshandler_stuff_t
{
    std::vector<boost::optional<ros::Publisher>> field_publishers;
    std::vector<boost::optional<ros::Subscriber>> field_subscribers;

    ros::Subscriber reboot_subscriber;
    ros::Subscriber sleep_subscriber;
    ros::Subscriber wake_subscriber;
    ros::ServiceServer board_online;
    ros::ServiceServer board_wake;
};

RosModule::RosModule() { ros_stuff = new ros_stuff_t; }

int64_t InterpretInt(std::vector<uint8_t> &data, int typecode, int i)
{
    switch (typecode)
    {
    case _RUBI_TYPECODES_int32_t:
        return *((int32_t *)&data.data()[i]);
    case _RUBI_TYPECODES_int16_t:
        return *((int16_t *)&data.data()[i]);
    case _RUBI_TYPECODES_int8_t:
        return *((int8_t *)&data.data()[i]);
    case _RUBI_TYPECODES_uint32_t:
        return *((uint32_t *)&data.data()[i]);
    case _RUBI_TYPECODES_uint16_t:
        return *((uint16_t *)&data.data()[i]);
    case _RUBI_TYPECODES_uint8_t:
        return *((uint8_t *)&data.data()[i]);
    default:
        ASSERT(false);
    }

    return 0;
}

float InterpretFloat(std::vector<uint8_t> &data, int typecode, int i)
{
    return *((float *)&data.data()[i]);
}

std::string InterpretString(std::vector<uint8_t> &data, int typecode, int i)
{
    return std::string((char *)data.data());
}

void DeinterpretInt(uint8_t *target, int64_t num, int typecode)
{
    switch (typecode)
    {
    case _RUBI_TYPECODES_int32_t:
        *((int32_t *)target) = (int32_t)num;
        break;
    case _RUBI_TYPECODES_int16_t:
        *((int16_t *)target) = (int16_t)num;
        break;
    case _RUBI_TYPECODES_int8_t:
        *((int8_t *)target) = (int8_t)num;
        break;
    case _RUBI_TYPECODES_uint32_t:
        *((uint32_t *)target) = (uint32_t)num;
        break;
    case _RUBI_TYPECODES_uint16_t:
        *((uint16_t *)target) = (uint16_t)num;
        break;
    case _RUBI_TYPECODES_uint8_t:
        *target = (uint8_t)num;
        break;
    default:
        ASSERT(false);
    }
}

void DeinterpretFloat(uint8_t *target, float num, int typecode)
{
    *((float *)target) = num;
}

void DeinterpretShortString(uint8_t *target, std::string num, int typecode)
{
    strcpy((char *)target, num.c_str());
}

void PanicHandler(const std_msgs::Empty::ConstPtr &data) { ASSERT(0); }

bool CansNamesHandler(std::vector<string> names,
                      rubi_server::CansNames::Request &req,
                      rubi_server::CansNames::Response &res)
{
    res.names = names;
    return true;
}

bool ShowBoardsHandler(rubi_server::ShowBoards::Request &req,
                       rubi_server::ShowBoards::Response &res)
{
    for (const auto &entry : BoardManager::inst().descriptor_map)
    {
        res.boards_names.push_back(entry.first);
    }

    return true;
}

bool BoardInstancesHandler(
    rubi_server::BoardInstances::Request&  req,
    rubi_server::BoardInstances::Response& res)
{
    auto descriptor = BoardManager::inst().descriptor_map.find(req.board);
    if(descriptor == BoardManager::inst().descriptor_map.end())
        return false;

    auto instances = BoardManager::inst().handlers.find(descriptor->second);
    if(instances == BoardManager::inst().handlers.end())
        return false;

    for(const auto instance : instances->second)
    {
        auto id = instance.id;
        if(id)
            res.ids.push_back(id.get());
        else
            res.ids.push_back(std::string(""));
    }

    return true;
}

bool BoardDescriptorHandler(rubi_server::BoardDescriptor::Request &req,
                            rubi_server::BoardDescriptor::Response &res)
{
    auto board_type = BoardManager::inst().descriptor_map.find(req.board);
    if (board_type == BoardManager::inst().descriptor_map.end())
        return false;

    for (const auto &ff : board_type->second->fieldfunctions)
    {
        auto field = std::dynamic_pointer_cast<FieldDescriptor>(ff);
        if (field)
        {
            res.fields.push_back(field->name);
        }
    }

    res.description = board_type->second->description;
    res.driver = board_type->second->driver;
    res.version = board_type->second->version;

    return true;
}

bool BoardOnlineHandler(std::shared_ptr<RosBoardHandler> handler,
                        rubi_server::BoardOnline::Request &req,
                        rubi_server::BoardOnline::Response &res)
{
    auto backend_handler = handler->BackendReady();
    if (!backend_handler)
    {
        res.online = false;
    }
    else
    {
        res.online = !backend_handler->IsLost() && !backend_handler->IsDead();
    }

    return true;
}

bool BoardWakeHandler(std::shared_ptr<RosBoardHandler> handler,
                      rubi_server::BoardWake::Request &req,
                      rubi_server::BoardWake::Response &res)
{
    auto backend_handler = handler->BackendReady();
    if (!backend_handler)
    {
        res.wake = false;
    }
    else
    {
        res.wake = backend_handler->IsWake();
    }

    return true;
}

// fixme those should be methods of the RosModule
bool FieldDescriptorHandler(rubi_server::FieldDescriptor::Request &req,
                            rubi_server::FieldDescriptor::Response &res)
{
    auto &boards = BoardManager::inst().descriptor_map;

    if (boards.find(req.board_name) == boards.end())
        return false;

    auto &board = boards[req.board_name];

    unsigned int field_id = 0;
    for (field_id = 0; field_id < board->fieldfunctions.size(); field_id++)
        if (board->fieldfunctions[field_id]->name == req.field_name)
            break;

    if (field_id == board->fieldfunctions.size())
        return false;

    const auto &field = std::static_pointer_cast<FieldDescriptor>(
        board->fieldfunctions[field_id]);

    res.subfields = std::vector<std::string>(field->subfields_names);
    res.typecode = field->typecode;
    res.input =
        field->access == RUBI_READONLY || field->access == RUBI_READWRITE;
    res.output =
        field->access == RUBI_WRITEONLY || field->access == RUBI_READWRITE;

    return true;
}

bool FuncDescriptorHandler(rubi_server::FuncDescriptor::Request &req,
                           rubi_server::FuncDescriptor::Response &res)
{
    ASSERT(0);
}

void InboundFieldCallbackInt(std::shared_ptr<RosBoardHandler> handler,
                             int field_id,
                             const rubi_server::RubiInt::ConstPtr &data)
{
    auto backend_handler = handler->BackendReady();
    if (!backend_handler)
        return;

    std::vector<uint8_t> ffdata;

    unsigned int ffid = handler->GetFieldFfid(field_id);
    unsigned int element_size = rubi_type_size(
        handler->board.descriptor->fieldfunctions[ffid]->typecode);
    unsigned int datasize =
        handler->board.descriptor->fieldfunctions[ffid]->GetFFSize();
    ffdata.resize(datasize);
    ASSERT(datasize / element_size == data->data.size());

    for (unsigned int i = 0; i < data->data.size(); i++)
        DeinterpretInt(
            &ffdata.data()[i * element_size], data->data[i],
            handler->board.descriptor->fieldfunctions[ffid]->typecode);

    backend_handler->FFDataOutbound(
        handler->board.descriptor->fieldfunctions[ffid], ffdata);
}

void InboundFieldCallbackUnsignedInt(
    std::shared_ptr<RosBoardHandler> handler, int field_id,
    const rubi_server::RubiUnsignedInt::ConstPtr &data)
{
    auto backend_handler = handler->BackendReady();
    if (!backend_handler)
        return;

    std::vector<uint8_t> ffdata;

    unsigned int ffid = handler->GetFieldFfid(field_id);
    unsigned int element_size = rubi_type_size(
        handler->board.descriptor->fieldfunctions[ffid]->typecode);
    unsigned int datasize =
        handler->board.descriptor->fieldfunctions[ffid]->GetFFSize();
    ffdata.resize(datasize);
    ASSERT(datasize / element_size == data->data.size());

    for (unsigned int i = 0; i < data->data.size(); i++)
        DeinterpretInt(
            &ffdata.data()[i * element_size], data->data[i],
            handler->board.descriptor->fieldfunctions[ffid]->typecode);

    backend_handler->FFDataOutbound(
        handler->board.descriptor->fieldfunctions[ffid], ffdata);
}

void InboundFieldCallbackBool(std::shared_ptr<RosBoardHandler> handler,
                              int field_id,
                              const rubi_server::RubiBool::ConstPtr &data)
{
    auto backend_handler = handler->BackendReady();
    if (!backend_handler)
        return;

    std::vector<uint8_t> ffdata;

    unsigned int ffid = handler->GetFieldFfid(field_id);
    ffdata.resize(data->data.size());
    ASSERT(backend_handler->GetBoard()
               .descriptor->fieldfunctions[ffid]
               ->GetFFSize() == data->data.size());

    for (unsigned int i = 0; i < data->data.size(); i++)
        ffdata[i] = data->data[i];

    backend_handler->FFDataOutbound(
        handler->board.descriptor->fieldfunctions[ffid], ffdata);
}

void InboundFieldCallbackFloat(std::shared_ptr<RosBoardHandler> handler,
                               int field_id,
                               const rubi_server::RubiFloat::ConstPtr &data)
{
    auto backend_handler = handler->BackendReady();
    if (!backend_handler)
        return;

    std::vector<uint8_t> ffdata;

    unsigned int ffid = handler->GetFieldFfid(field_id);
    unsigned int element_size = rubi_type_size(
        handler->board.descriptor->fieldfunctions[ffid]->typecode);
    unsigned int datasize =
        handler->board.descriptor->fieldfunctions[ffid]->GetFFSize();
    ffdata.resize(datasize);
    ASSERT(datasize / element_size == data->data.size());

    for (unsigned int i = 0; i < data->data.size(); i++)
        DeinterpretFloat(
            &ffdata.data()[i * element_size], data->data[i],
            handler->board.descriptor->fieldfunctions[ffid]->typecode);

    backend_handler->FFDataOutbound(
        handler->board.descriptor->fieldfunctions[ffid], ffdata);
}

void InboundFieldCallbackString(std::shared_ptr<RosBoardHandler> handler,
                                int field_id,
                                const rubi_server::RubiString::ConstPtr &data)
{
    auto backend_handler = handler->BackendReady();
    if (!backend_handler)
        return;

    std::vector<uint8_t> ffdata;

    unsigned int ffid = handler->GetFieldFfid(field_id);
    unsigned int element_size = rubi_type_size(
        handler->board.descriptor->fieldfunctions[ffid]->typecode);
    unsigned int datasize =
        handler->board.descriptor->fieldfunctions[ffid]->GetFFSize();
    ffdata.resize(datasize);
    ASSERT(datasize / element_size == data->data.size());

    for (unsigned int i = 0; i < data->data.size(); i++)
        DeinterpretShortString(
            &ffdata.data()[i * element_size], data->data[i],
            handler->board.descriptor->fieldfunctions[ffid]->typecode);

    backend_handler->FFDataOutbound(
        handler->board.descriptor->fieldfunctions[ffid], ffdata);
}

void BoardWakeCallback(std::shared_ptr<RosBoardHandler> handler,
                       const std_msgs::Empty::ConstPtr &data)
{
    auto backend_handler = handler->BackendReady();
    if (!backend_handler)
        return;

    backend_handler->CommandWake();
}

void BoardRebootCallback(std::shared_ptr<RosBoardHandler> handler,
                         const std_msgs::Empty::ConstPtr &data)
{
    auto backend_handler = handler->BackendReady();
    if (!backend_handler)
        return;

    backend_handler->CommandReboot();
}

void BoardSleepCallback(std::shared_ptr<RosBoardHandler> handler,
                        const std_msgs::Empty::ConstPtr &data)
{
    auto backend_handler = handler->BackendReady();
    if (!backend_handler)
        return;

    backend_handler->CommandSleep();
}

bool RosModule::Init(int argc, char **argv)
{
    string cans_names_raw;

    ros::init(argc, argv, "rubi_server");
    ros_stuff->n = new ros::NodeHandle("~");

    if (ros_stuff->n->getParam("cans", cans_names_raw))
    {
        boost::split(cans_names, cans_names_raw, boost::is_any_of(","));
    }
    else
    {
        log.Warning("No can interfaces given! Assuming its can0.");
        cans_names = {"can0"};
    }

    if (ros::console::set_logger_level(ROSCONSOLE_DEFAULT_NAME,
                                       ros::console::levels::Info))
    {
        ros::console::notifyLoggerLevelsChanged();
    }

    ros_stuff->field_server = ros_stuff->n->advertiseService(
        "/rubi/get_field_descriptor", FieldDescriptorHandler);
    ros_stuff->func_server = ros_stuff->n->advertiseService(
        "/rubi/get_func_descriptor", FuncDescriptorHandler);
    ros_stuff->board_shower =
        ros_stuff->n->advertiseService("/rubi/show_boards", ShowBoardsHandler);
    ros_stuff->board_descriptor = ros_stuff->n->advertiseService(
        "/rubi/get_board_descriptor", BoardDescriptorHandler);
    ros_stuff->board_instances = ros_stuff->n->advertiseService(
        "/rubi/get_board_instances", BoardInstancesHandler);

    ros_stuff->board_announcer =
        ros_stuff->n->advertise<rubi_server::BoardAnnounce>("/rubi/new_boards",
                                                            10);

    ros_stuff->can_load_publisher =
        ros_stuff->n->advertise<std_msgs::Float32MultiArray>("/rubi/cans_load",
                                                             1);

    ros_stuff->panic_subscriber = ros_stuff->n->subscribe<std_msgs::Empty>(
        "/rubi/panic", 1, PanicHandler);

    // std::func inference seems broken
    boost::function<bool(rubi_server::CansNames::Request &,
                         rubi_server::CansNames::Response &)>
        cans_names_callback =
            std::bind(CansNamesHandler, cans_names, std::placeholders::_1,
                      std::placeholders::_2);
    ros_stuff->can_names_server = ros_stuff->n->advertiseService(
        "/rubi/get_cans_names", cans_names_callback);

    ros_stuff->loop_rate = new ros::Rate(30);

    return true;
}

void RosModule::ReportCansUtilization(std::vector<float> util)
{
    ASSERT(util.size() == cans_names.size());

    std_msgs::Float32MultiArray msg;
    msg.data = util;

    ros_stuff->can_load_publisher.publish(msg);
}

void RosModule::LogInfo(string msg) { ROS_INFO("%s", msg.c_str()); }

void RosModule::LogWarning(string msg) { ROS_WARN("%s", msg.c_str()); }

void RosModule::LogError(string msg) { ROS_FATAL("%s", msg.c_str()); }

std::shared_ptr<FrontendBoardHandler> RosModule::NewBoard(BoardInstance inst)
{
    std::shared_ptr<RosBoardHandler> ret =
        std::make_shared<RosBoardHandler>(inst, this);
    ret->Init();

    boards.push_back(ret);

    rubi_server::BoardAnnounce msg;
    msg.driver = inst.descriptor->driver;
    msg.name = inst.descriptor->board_name;
    msg.version = inst.descriptor->version;
    msg.id = inst.id.is_initialized() ? inst.id.get() : "";

    for (auto ff : inst.descriptor->fieldfunctions)
    {
        auto field = std::dynamic_pointer_cast<FieldDescriptor>(ff);
        auto function = std::dynamic_pointer_cast<FieldDescriptor>(ff);

        if (field)
            msg.fields.push_back(field->name);
        else if (function)
            msg.functions.push_back(function->name);
        else
            ASSERT(false);
    }

    ros_stuff->board_announcer.publish(msg);
    log.Info("Frontend registered new board type: " +
             inst.descriptor->board_name);

    return std::shared_ptr<FrontendBoardHandler>(ret);
}

std::vector<std::string> RosModule::GetCansNames() { return cans_names; }

RosBoardHandler::RosBoardHandler(BoardInstance inst, RosModule *_ros_module)
    : board(inst), ros_module(_ros_module)
{
    ros_stuff = std::unique_ptr<roshandler_stuff_t>(new roshandler_stuff_t());
}

void RosBoardHandler::Init()
{
    auto &n = *(ros_module->ros_stuff->n);

    int fc = 0, tc = 0;
    fftable.resize(board.descriptor->fieldfunctions.size());
    auto id = board.id;

    auto wake_callback =
        std::bind(BoardWakeCallback, shared_from_this(), std::placeholders::_1);
    auto sleep_callback = std::bind(BoardSleepCallback, shared_from_this(),
                                    std::placeholders::_1);
    auto reboot_callback = std::bind(BoardRebootCallback, shared_from_this(),
                                     std::placeholders::_1);

    // std::func inference seems broken
    boost::function<bool(rubi_server::BoardOnline::Request &,
                         rubi_server::BoardOnline::Response &)>
        online_handler =
            std::bind(BoardOnlineHandler, shared_from_this(),
                      std::placeholders::_1, std::placeholders::_2);

    boost::function<bool(rubi_server::BoardWake::Request &,
                         rubi_server::BoardWake::Response &)>
        wake_handler = std::bind(BoardWakeHandler, shared_from_this(),
                                 std::placeholders::_1, std::placeholders::_2);

    ros_stuff->wake_subscriber = n.subscribe<std_msgs::Empty>(
        board.descriptor->GetBoardPrefix(id) + "wake", 1, wake_callback);
    ros_stuff->sleep_subscriber = n.subscribe<std_msgs::Empty>(
        board.descriptor->GetBoardPrefix(id) + "sleep", 1, sleep_callback);
    ros_stuff->reboot_subscriber = n.subscribe<std_msgs::Empty>(
        board.descriptor->GetBoardPrefix(id) + "reboot", 1, reboot_callback);

    ros_stuff->board_online = n.advertiseService(
        board.descriptor->GetBoardPrefix(id) + "is_online", online_handler);
    ros_stuff->board_wake = n.advertiseService(
        board.descriptor->GetBoardPrefix(id) + "is_wake", wake_handler);

    for (const auto &ff : board.descriptor->fieldfunctions)
    {
        auto field = std::dynamic_pointer_cast<FieldDescriptor>(ff);
        tc += 1;

        if (!field)
            continue;

        fc += 1;
        fftable[tc - 1] =
            std::pair<fftype_t, int>(fftype_t::fftype_field, fc - 1);
        fieldtable.push_back(tc - 1);

        switch (field->typecode)
        {

        case _RUBI_TYPECODES_int32_t:
        case _RUBI_TYPECODES_int16_t:
        case _RUBI_TYPECODES_int8_t:
            if (field->access == RUBI_WRITEONLY ||
                field->access == RUBI_READWRITE)
            {
                ros_stuff->field_publishers.push_back(
                    n.advertise<rubi_server::RubiInt>(
                        board.descriptor->GetBoardPrefix(id) +
                            "fields_from_board/" + field->name,
                        10, true));
            }
            else
            {
                ros_stuff->field_publishers.push_back(boost::none);
            }

            if (field->access == RUBI_READONLY ||
                field->access == RUBI_READWRITE)
            {
                auto callback =
                    std::bind(InboundFieldCallbackInt, shared_from_this(),
                              fc - 1, std::placeholders::_1);

                ros_stuff->field_subscribers.push_back(
                    n.subscribe<rubi_server::RubiInt>(
                        board.descriptor->GetBoardPrefix(id) +
                            "fields_to_board/" + field->name,
                        1, callback));
            }
            break;

        case _RUBI_TYPECODES_uint32_t:
        case _RUBI_TYPECODES_uint16_t:
        case _RUBI_TYPECODES_uint8_t:
            if (field->access == RUBI_WRITEONLY ||
                field->access == RUBI_READWRITE)
            {
                ros_stuff->field_publishers.push_back(
                    n.advertise<rubi_server::RubiUnsignedInt>(
                        board.descriptor->GetBoardPrefix(id) +
                            "fields_from_board/" + field->name,
                        10, true));
            }
            else
            {
                ros_stuff->field_publishers.push_back(boost::none);
            }

            if (field->access == RUBI_READONLY ||
                field->access == RUBI_READWRITE)
            {
                auto callback = std::bind(InboundFieldCallbackUnsignedInt,
                                          shared_from_this(), fc - 1,
                                          std::placeholders::_1);

                ros_stuff->field_subscribers.push_back(
                    n.subscribe<rubi_server::RubiUnsignedInt>(
                        board.descriptor->GetBoardPrefix(id) +
                            "fields_to_board/" + field->name,
                        1, callback));
            }
            break;

        case _RUBI_TYPECODES_bool:
            if (field->access == RUBI_WRITEONLY ||
                field->access == RUBI_READWRITE)
            {
                ros_stuff->field_publishers.push_back(
                    n.advertise<rubi_server::RubiBool>(
                        board.descriptor->GetBoardPrefix(id) +
                            "fields_from_board/" + field->name,
                        10, true));
            }
            else
            {
                ros_stuff->field_publishers.push_back(boost::none);
            }

            if (field->access == RUBI_READONLY ||
                field->access == RUBI_READWRITE)
            {
                auto callback =
                    std::bind(InboundFieldCallbackBool, shared_from_this(),
                              fc - 1, std::placeholders::_1);

                ros_stuff->field_subscribers.push_back(
                    n.subscribe<rubi_server::RubiBool>(
                        board.descriptor->GetBoardPrefix(id) +
                            "fields_to_board/" + field->name,
                        1, callback));
            }
            break;

        case _RUBI_TYPECODES_float:
            if (field->access == RUBI_WRITEONLY ||
                field->access == RUBI_READWRITE)
            {
                ros_stuff->field_publishers.push_back(
                    n.advertise<rubi_server::RubiFloat>(
                        board.descriptor->GetBoardPrefix(id) +
                            "fields_from_board/" + field->name,
                        10, true));
            }
            else
            {
                ros_stuff->field_publishers.push_back(boost::none);
            }

            if (field->access == RUBI_READONLY ||
                field->access == RUBI_READWRITE)
            {
                auto callback =
                    std::bind(InboundFieldCallbackFloat, shared_from_this(),
                              fc - 1, std::placeholders::_1);

                ros_stuff->field_subscribers.push_back(
                    n.subscribe<rubi_server::RubiFloat>(
                        board.descriptor->GetBoardPrefix(id) +
                            "fields_to_board/" + field->name,
                        1, callback));
            }
            break;

        case _RUBI_TYPECODES_shortstring:
        case _RUBI_TYPECODES_longstring:
            if (field->access == RUBI_WRITEONLY ||
                field->access == RUBI_READWRITE)
            {
                ros_stuff->field_publishers.push_back(
                    n.advertise<rubi_server::RubiString>(
                        board.descriptor->GetBoardPrefix(id) +
                            "fields_from_board/" + field->name,
                        10, true));
            }
            else
            {
                ros_stuff->field_publishers.push_back(boost::none);
            }

            if (field->access == RUBI_READONLY ||
                field->access == RUBI_READWRITE)
            {
                auto callback =
                    std::bind(InboundFieldCallbackString, shared_from_this(),
                              fc - 1, std::placeholders::_1);

                ros_stuff->field_subscribers.push_back(
                    n.subscribe<rubi_server::RubiString>(
                        board.descriptor->GetBoardPrefix(id) +
                            "fields_to_board/" + field->name,
                        1, callback));
            }
            break;

        default:
            throw new RubiException("RosModule::NewBoard switch default");
        }
    }
}

void RosBoardHandler::FFDataInbound(std::vector<uint8_t> &data, int ffid)
{
    // Yuck!
    rubi_server::RubiInt i32;
    rubi_server::RubiUnsignedInt u32;
    rubi_server::RubiFloat f32;
    rubi_server::RubiString str;
    rubi_server::RubiBool bools;
    std_msgs::Empty emp;

    boost::optional<ros::Publisher> publisher;

    ASSERT(board.descriptor->fieldfunctions[ffid]->GetFFSize() ==
           (int)data.size());

    switch (board.descriptor->fieldfunctions[ffid]->typecode)
    {
    case _RUBI_TYPECODES_int32_t:
    case _RUBI_TYPECODES_int16_t:
    case _RUBI_TYPECODES_int8_t:
        for (unsigned int i = 0; i < data.size();
             i +=
             rubi_type_size(board.descriptor->fieldfunctions[ffid]->typecode))
        {
            i32.data.push_back((int32_t)InterpretInt(
                data, board.descriptor->fieldfunctions[ffid]->typecode, i));
        }

        ASSERT(fftable[ffid].first == fftype_t::fftype_field);
        publisher = ros_stuff->field_publishers[fftable[ffid].second];
        ASSERT(publisher);
        publisher.get().publish(i32);

        break;

    case _RUBI_TYPECODES_uint32_t:
    case _RUBI_TYPECODES_uint16_t:
    case _RUBI_TYPECODES_uint8_t:
        for (unsigned int i = 0; i < data.size();
             i +=
             rubi_type_size(board.descriptor->fieldfunctions[ffid]->typecode))
        {
            u32.data.push_back((uint32_t)InterpretInt( // FIXME
                data, board.descriptor->fieldfunctions[ffid]->typecode, i));
        }

        ASSERT(fftable[ffid].first == fftype_t::fftype_field);
        publisher = ros_stuff->field_publishers[fftable[ffid].second];
        ASSERT(publisher);
        publisher.get().publish(u32);

        break;

    case _RUBI_TYPECODES_bool:
        for (unsigned int i = 0; i < data.size(); i += 1)
        {
            bools.data.push_back(data[i]);
        }

        ASSERT(fftable[ffid].first == fftype_t::fftype_field);
        publisher = ros_stuff->field_publishers[fftable[ffid].second];
        ASSERT(publisher);
        publisher.get().publish(bools);

        break;
    case _RUBI_TYPECODES_float:
        for (unsigned int i = 0; i < data.size();
             i +=
             rubi_type_size(board.descriptor->fieldfunctions[ffid]->typecode))
        {
            f32.data.push_back(InterpretFloat( // FIXME
                data, board.descriptor->fieldfunctions[ffid]->typecode, i));
        }

        ASSERT(fftable[ffid].first == fftype_t::fftype_field);
        publisher = ros_stuff->field_publishers[fftable[ffid].second];
        ASSERT(publisher);
        publisher.get().publish(f32);

        break;

    case _RUBI_TYPECODES_shortstring:
    case _RUBI_TYPECODES_longstring:
        for (unsigned int i = 0; i < data.size();
             i +=
             rubi_type_size(board.descriptor->fieldfunctions[ffid]->typecode))
        {
            str.data.push_back(InterpretString( // FIXME
                data, board.descriptor->fieldfunctions[ffid]->typecode, i));
        }

        ASSERT(fftable[ffid].first == fftype_t::fftype_field);
        publisher = ros_stuff->field_publishers[fftable[ffid].second];
        ASSERT(publisher);
        publisher.get().publish(str);

        break;
    default:
        ASSERT(false);
    }
}

void RosModule::Spin()
{
    ros::spinOnce();
    ros_stuff->loop_rate->sleep();
}

bool RosModule::Quit() { return !ros::ok(); }

int RosBoardHandler::GetFieldFfid(int field_id) { return fieldtable[field_id]; }

int RosBoardHandler::GetFunctionFfid(int function_id)
{
    ASSERT(false);
    return 0;
}

void RosBoardHandler::ReplaceBackendHandler(
    std::shared_ptr<BoardCommunicationHandler> new_handler)
{
    // FIXME: make instance's field const and make a new one each time
    board.backend_handler = new_handler;
}

void RosBoardHandler::Shutdown() {}

void RosBoardHandler::ConnectionLost() {}

sptr<BoardCommunicationHandler> RosBoardHandler::BackendReady()
{
    sptr<BoardCommunicationHandler> ret;
    if (!(ret = board.backend_handler.lock()) || ret->IsDead())
    {
        ret = BoardManager::inst().RequestNewHandler(board, shared_from_this());
        if (ret)
        {
            board.backend_handler = ret;
            return ret;
        }
        else
        {
            return nullptr;
        }
    }

    if (ret->IsLost())
    {
        return nullptr;
    }

    return ret;
}
