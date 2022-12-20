#pragma once

#include <memory>
#include <string>
#include <tuple>
#include <vector>

#include <boost/optional.hpp>

#include "rubi_server/types.hpp"

class BoardDescriptor;
class BoardCommunicationHandler;
std::string DataToString(std::vector<uint8_t> data);

class FFDescriptor
{
public:
  std::string name;
  int typecode;
  const int ffid;

  bool BuildOrCheck(int desc_id, std::string value);
  virtual void Build(int desc_id, std::string value) = 0;
  virtual int GetFFSize() = 0;
  virtual int GetFFType() = 0;

  explicit FFDescriptor(int ffid)
  : typecode(0), ffid(ffid), complete(false) {}
  virtual ~FFDescriptor() {}
  virtual bool CheckCompleteness();

  static std::shared_ptr<FFDescriptor> BuildDescriptor(
    int ffid, int msg_type,
    std::string value);

  virtual bool operator==(const sptr<FFDescriptor> & rhs) = 0;
  virtual bool operator!=(const sptr<FFDescriptor> & rhs);

private:
  bool complete;
};

class FieldDescriptor : public FFDescriptor
{
public:
  std::vector<std::string> subfields_names;
  int access;

  explicit FieldDescriptor(int ffid)
  : FFDescriptor(ffid), access(0) {}

  void Build(int desc_id, std::string value) override;
  int GetFFSize() override;
  int GetFFType() override;
  bool operator==(const sptr<FFDescriptor> & rhs) override;
};

class FunctionDescriptor : public FFDescriptor
{
public:
  std::vector<std::string> arg_names;
  int out_typecode;

  explicit FunctionDescriptor(int ffid)
  : FFDescriptor(ffid), out_typecode(0) {}

  void Build(int desc_id, std::string value) override;
  int GetFFSize() override;
  int GetFFType() override;
  bool operator==(const sptr<FFDescriptor> & rhs) override;
};

class BoardDescriptor
{
public:
  std::string board_name, version, driver, description;
  std::vector<std::shared_ptr<FFDescriptor>> fieldfunctions;

  void ApplyInfo(uint8_t desc_type, std::string value);
  std::string GetBoardPrefix(boost::optional<std::string> id);
  bool operator==(const BoardDescriptor & rhs);
  bool operator!=(const BoardDescriptor & rhs);
};

class BoardInstance
{
public:
  explicit BoardInstance(std::weak_ptr<BoardCommunicationHandler> inst)
  : backend_handler(inst) {}
  std::shared_ptr<BoardDescriptor> descriptor;
  boost::optional<std::string> id;

  std::weak_ptr<BoardCommunicationHandler> backend_handler;
  explicit operator std::string() const;

  BoardInstance & operator=(const BoardInstance & rhs)
  {
    id = rhs.id;
    descriptor = rhs.descriptor;
    backend_handler = rhs.backend_handler;

    return *this;
  }

  BoardInstance()
  : id(boost::none) {}
};

bool operator<(BoardInstance & lhs, BoardInstance & rhs);
