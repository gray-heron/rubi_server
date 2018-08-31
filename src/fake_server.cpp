#include <boost/algorithm/string.hpp>
#include <cstring>
#include <vector>

#include "board.h"
#include "exceptions.h"
#include "fake_communication.h"
#include "ros_frontend.h"
#include "rubi_autodefs.h"

using std::vector;
using std::string;
using boost::any;
using std::pair;
using std::make_pair;

BoardManager::BoardManager() {}

sptr<BoardCommunicationHandler>
BoardManager::RequestNewHandler(BoardInstance inst,
                                sptr<FrontendBoardHandler> frontend)
{
    return nullptr;
}

int main(int argc, char **argv)
{
// required for the ROS' arguments jugglery
#pragma GCC diagnostic ignored "-Wwrite-strings"
    char *fake_args[] = {
        "/", "_cans:=can0,can1",
    };
#pragma GCC diagnostic pop

    sptr<RosModule> frontend = std::make_shared<RosModule>();
    BoardManager::inst().frontend = frontend;

    frontend->Init(2, fake_args);
    CommunicationFaker faker;

    vector<sptr<BoardCommunicationHandler>> publishers;
    vector<pair<fields_desc, int>> board1_fields, board2_fields, board3_fields;

    board1_fields.push_back(make_pair<fields_desc, int>(
        fields_desc("FloatTest", {}, _RUBI_TYPECODES_float, any(666.6f)),
        RUBI_READONLY));
    board1_fields.push_back(make_pair<fields_desc, int>(
        fields_desc("StringTest", {}, _RUBI_TYPECODES_shortstring,
                    any(string("Hello world"))),
        RUBI_WRITEONLY));
    board1_fields.push_back(make_pair<fields_desc, int>(
        fields_desc("IntTest", {}, _RUBI_TYPECODES_int32_t, any(333)),
        RUBI_READWRITE));
    board1_fields.push_back(make_pair<fields_desc, int>(
        fields_desc("BoolTest", {}, _RUBI_TYPECODES_bool, any(1)),
        RUBI_READWRITE));

    board2_fields.push_back(make_pair<fields_desc, int>(
        fields_desc("FloatTest", {"Subfield1"}, _RUBI_TYPECODES_float,
                    any(666.6f)),
        RUBI_READONLY));
    board2_fields.push_back(make_pair<fields_desc, int>(
        fields_desc("StringTest", {"Subfield1", "Subfield2"},
                    _RUBI_TYPECODES_shortstring, any(string("Hello world"))),
        RUBI_WRITEONLY));
    board2_fields.push_back(make_pair<fields_desc, int>(
        fields_desc("IntTest", {"Subfield1", "Subfield", "SuBfIlEd"},
                    _RUBI_TYPECODES_int32_t, any(333)),
        RUBI_WRITEONLY));
    board2_fields.push_back(make_pair<fields_desc, int>(
        fields_desc("BoolTest", {"Subfield1", "sqi", "SuBfIlEd"},
                    _RUBI_TYPECODES_bool, any(0)),
        RUBI_WRITEONLY));

    board3_fields.push_back(std::make_pair<fields_desc, int>(
        fields_desc("FloatTest", {}, _RUBI_TYPECODES_float, any(666.6f)),
        RUBI_READONLY));
    board3_fields.push_back(std::make_pair<fields_desc, int>(
        fields_desc(
            "StringTest", {}, _RUBI_TYPECODES_longstring,
            any(string("1234567890123456789012345678901234567890123456789012345"
                       "6789012345678901234567890123456789012345678901234567890"
                       "1234567890123456789012345678901234567890123456789012345"
                       "6789012345678901234567890123456789012345678901234567890"
                       "12345678901234567890123456789012345"))),
        RUBI_READONLY));
    board3_fields.push_back(std::make_pair<fields_desc, int>(
        fields_desc("StringTest2", {}, _RUBI_TYPECODES_shortstring,
                    any(string("omedetou kurisumasu"))),
        RUBI_READONLY));
    board3_fields.push_back(std::make_pair<fields_desc, int>(
        fields_desc("IntTest", {}, _RUBI_TYPECODES_uint32_t, any(123)),
        RUBI_READONLY));
    board3_fields.push_back(std::make_pair<fields_desc, int>(
        fields_desc("IntTest2", {}, _RUBI_TYPECODES_uint32_t, any(987)),
        RUBI_WRITEONLY));

    publishers.push_back(faker.FakeBoard("FakeBoard1", board1_fields));
    publishers.push_back(faker.FakeBoard("FakeBoard2", board2_fields));
    faker.FakeBoard("FakeBoard3", board3_fields);
    // faker.FakeBoard("b", board1_fields);
    // faker.FakeBoard("c", board1_fields);
    // faker.FakeBoard("d", board1_fields);
    // faker.FakeBoard("e", board1_fields);
    // faker.FakeBoard("f", board1_fields);
    // faker.FakeBoard("t", board2_fields);
    // faker.FakeBoard("u", board2_fields);
    // faker.FakeBoard("w", board2_fields);
    // faker.FakeBoaptrrd("x", board2_fields);
    // faker.FakeBoard("y", board2_fields);
    // faker.FakeBoard("z", board1_fields);
    // faker.FakeBoard("z1", board1_fields);
    // faker.FakeBoard("z2", board1_fields);
    // faker.FakeBoard("z3", board1_fields);
    // faker.FakeBoard("z4", board2_fields);
    // faker.FakeBoard("z5", board2_fields);
    // faker.FakeBoard("zz5", board2_fields);
    // faker.FakeBoard("zz5", board2_fields);
    // faker.FakeBoard("zz6", board2_fields);

    for (int iter = 0;; iter++)
    {
        frontend->Spin();
        if (!(iter++ % 100))
            frontend->ReportCansUtilization(
                {0.0300009791f + float(iter), 125001.1f + float(iter)});

        for (auto pub : publishers)
        {
            vector<uint8_t> data;
            // FIXME unbstable as hell
            for (int i = 0;
                 i <
                 pub->GetBoard().descriptor->fieldfunctions[2]->GetFFSize() / 4;
                 i++)
            {
                data.push_back(0);
                data.push_back(0);
                data.push_back(i);
                data.push_back(iter);
            }

            pub->FFDataInbound(2, data);
            data.clear();

            for (int i = 0;
                 i < pub->GetBoard().descriptor->fieldfunctions[3]->GetFFSize();
                 i++)
            {
                data.push_back((iter + i) % 2);
            }
            pub->FFDataInbound(3, data);
        }
    }
}
