#ifndef H_DESCRIPTORS
#define H_DESCRIPTORS

#include <string>
#include <tuple>
#include <vector>
#include <boost/optional.hpp>

#include "types.h"

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

  FFDescriptor(int ffid) : ffid(ffid), typecode(0), complete(0){};
  virtual ~FFDescriptor(){};
  virtual bool CheckCompleteness();

  static std::shared_ptr<FFDescriptor> BuildDescriptor(int ffid, int msg_type,
                                                       std::string value);

  virtual bool operator==(const sptr<FFDescriptor> &rhs) = 0;
  virtual bool operator!=(const sptr<FFDescriptor> &rhs);

private:
  bool complete;
};

class FieldDescriptor : public FFDescriptor
{
public:
  std::vector<std::string> subfields_names;
  int access;

  FieldDescriptor(int ffid) : FFDescriptor(ffid), access(0) {}

  virtual void Build(int desc_id, std::string value) override;
  virtual int GetFFSize() override;
  virtual int GetFFType() override;
  virtual bool operator==(const sptr<FFDescriptor> &rhs) override;
};

class FunctionDescriptor : public FFDescriptor
{
public:
  std::vector<std::string> arg_names;
  int out_typecode;

  FunctionDescriptor(int ffid) : FFDescriptor(ffid), out_typecode(0) {}

  virtual void Build(int desc_id, std::string value) override;
  virtual int GetFFSize() override;
  virtual int GetFFType() override;
  virtual bool operator==(const sptr<FFDescriptor> &rhs) override;
};

class BoardDescriptor
{
private:
public:
  std::string board_name, version, driver, description;
  std::vector<std::shared_ptr<FFDescriptor>> fieldfunctions;

  void ApplyInfo(uint8_t desc_type, std::string value);
  std::string GetBoardPrefix(boost::optional<std::string> id);
  bool operator==(const BoardDescriptor &rhs);
  bool operator!=(const BoardDescriptor &rhs);
};

class BoardInstance
{
public:
  BoardInstance(std::weak_ptr<BoardCommunicationHandler> inst)
      : backend_handler(inst){};
  std::shared_ptr<BoardDescriptor> descriptor;
  boost::optional<std::string> id;

  std::weak_ptr<BoardCommunicationHandler> backend_handler;
  explicit operator std::string() const;

  BoardInstance &operator=(const BoardInstance &rhs)
  {
    id = rhs.id;
    descriptor = rhs.descriptor;
    backend_handler = rhs.backend_handler;

    return *this;
  }

  BoardInstance() : id(boost::none) {}
};

bool operator<(BoardInstance &lhs, BoardInstance &rhs);

#endif
