#include <boost/algorithm/string/regex.hpp>
#include <boost/range/combine.hpp>
#include <boost/regex.hpp>
#include <tuple>

#include "descriptors.h"
#include "exceptions.h"
#include "protocol_defs.h"
#include "rubi_autodefs.h"

void SplitNames(std::vector<std::string> &ret, std::string str)
{
    std::string buf;

    for (unsigned int i = 0; i < str.length(); i++)
    {
        if (str[i] == ' ')
            continue;

        if (str[i] == ',')
        {
            ret.push_back(buf);
            buf = "";
        }
        else
            buf += str[i];
    }

    if (buf != "")
        ret.push_back(buf);
}

std::string DataToString(std::vector<uint8_t> data)
{
    std::string out;

    for (auto &ch : data)
        if (ch != '\0')
            out += ch;
        else
            break;

    return out;
}

void BoardDescriptor::ApplyInfo(uint8_t field_type, std::string value)
{
    uint8_t ffid = fieldfunctions.size();

    switch (field_type)
    {
    case RUBI_INFO_BOARD_NAME:
        board_name = value;
        break;
    case RUBI_INFO_BOARD_VERSION:
        version = value;
        break;
    case RUBI_INFO_BOARD_DRIVER:
        driver = value;
        break;
    case RUBI_INFO_BOARD_DESC:
        description = value;
        break;

    case RUBI_INFO_FUNC_NAME:
    case RUBI_INFO_FIELD_NAME:
        if (fieldfunctions.size() > 0)
            ASSERT((*(fieldfunctions.end() - 1))->CheckCompleteness());

        fieldfunctions.push_back(FFDescriptor::BuildDescriptor(
            fieldfunctions.end() - fieldfunctions.begin(), field_type, value));

    default:
        (*(fieldfunctions.end() - 1))->Build(field_type, value);
    }
}

void FieldDescriptor::Build(int desc_id, std::string value)
{
    switch (desc_id)
    {
    case RUBI_INFO_FIELD_NAME:
        name = value;
        break;
    case RUBI_INFO_FIELD_TYPE:
        typecode = value.c_str()[0];
        break;
    case RUBI_INFO_FIELD_ACCESS:
        access = value.c_str()[0];
        break;
    case RUBI_INFO_SUBFIELDS_C:
        break;
    case RUBI_INFO_SUBFIELDS:
        SplitNames(subfields_names, value);
        break;
    default:
        ASSERT(0);
    }
}

void FunctionDescriptor::Build(int desc_id, std::string value)
{
    switch (desc_id)
    {
    case RUBI_INFO_FUNC_NAME:
        name = value;
        break;
    case RUBI_INFO_FUNC_OUT_TYPE:
        out_typecode = value.c_str()[0];
        break;
    case RUBI_INFO_FUNC_ARGS_TYPE:
        typecode = value.c_str()[0];
        break;
    case RUBI_INFO_FUNC_ARGS_N:
        break;
    case RUBI_INFO_FUNC_ARGS_NAMES:
        SplitNames(arg_names, value);
        break;
    default:
        ASSERT(0);
    }
}

int FieldDescriptor::GetFFSize()
{
    return rubi_type_size(typecode) *
           (std::max(subfields_names.size(), (size_t)1));
}

int FunctionDescriptor::GetFFSize()
{
    return rubi_type_size(typecode) * arg_names.size();
}

bool FFDescriptor::CheckCompleteness() { return name != ""; }

std::shared_ptr<FFDescriptor>
FFDescriptor::BuildDescriptor(int ffid, int msg_type, std::string value)
{
    std::shared_ptr<FFDescriptor> out;
    switch (msg_type)
    {
    case RUBI_INFO_FIELD_NAME:
        out = std::shared_ptr<FieldDescriptor>(new FieldDescriptor(ffid));
        break;
    case RUBI_INFO_FUNC_NAME:
        out = std::shared_ptr<FunctionDescriptor>(new FunctionDescriptor(ffid));
        break;
    default:
        ASSERT(false);
    }

    out->name = value;

    return out;
}

std::string BoardDescriptor::GetBoardPrefix(boost::optional<std::string> id)
{
    std::string ret = std::string("/rubi/boards/") + board_name + "/";
    if (id.is_initialized())
        ret += id.get() + "/";

    return ret;
}

int FieldDescriptor::GetFFType() { return RUBI_MSG_FIELD; }

int FunctionDescriptor::GetFFType() { return RUBI_MSG_FUNCTION; }

BoardInstance::operator std::string() const
{
    if (id)
    {
        return descriptor->board_name + ":" + id.get();
    }
    else
    {
        return descriptor->board_name;
    }
}

bool BoardDescriptor::operator!=(const BoardDescriptor &rhs)
{
    return !(*this == rhs);
}

bool BoardDescriptor::operator==(const BoardDescriptor &rhs)
{
    if (rhs.board_name != board_name)
        return false;

    if (rhs.description != description)
        return false;

    if (rhs.version != version)
        return false;

    if (rhs.driver != driver)
        return false;

    if (rhs.fieldfunctions.size() != fieldfunctions.size())
        return false;

    for (auto ffs : boost::combine(rhs.fieldfunctions, fieldfunctions))
    {
        if (*ffs.get<0>() != ffs.get<1>())
            return false;
    }

    return true;
}

bool FFDescriptor::operator!=(const sptr<FFDescriptor> &rhs)
{
    return !(*this == rhs);
}

bool FieldDescriptor::operator==(const sptr<FFDescriptor> &rhs)
{
    auto ptr_rhs = std::dynamic_pointer_cast<FieldDescriptor>(rhs);
    if (!ptr_rhs)
        return false;

    if (ptr_rhs->name != name)
        return false;

    if (ptr_rhs->typecode != typecode)
        return false;

    if (ptr_rhs->subfields_names != subfields_names)
        return false;

    if (ptr_rhs->access != access)
        return false;

    return true;
}

bool FunctionDescriptor::operator==(const sptr<FFDescriptor> &rhs)
{
    auto ptr_rhs = std::dynamic_pointer_cast<FunctionDescriptor>(rhs);
    ASSERT(0);
}
