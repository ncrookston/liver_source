#ifndef JHMI_UTILITY_LOAD_PROTOBUF_HPP_NRC_20160520
#define JHMI_UTILITY_LOAD_PROTOBUF_HPP_NRC_20160520

#include "utility/scope_exit.hpp"
#include <google/protobuf/io/coded_stream.h>
#include <google/protobuf/io/gzip_stream.h>
#include <google/protobuf/io/zero_copy_stream_impl.h>
#include <boost/filesystem.hpp>

#ifndef WIN32
#include <sys/stat.h>
#include <fcntl.h>
#endif

namespace jhmi {

  template <typename T>
  T load_protobuf(boost::filesystem::path const& filename) {
    namespace io = google::protobuf::io;
#ifndef WIN32
    auto fd = open(filename.string().c_str(), O_RDONLY, S_IREAD);
    if (!fd)
      throw std::runtime_error("open failed on output file");
    auto ex = scope_exit([&] { close(fd); });
    auto file_stream = std::make_unique<io::FileInputStream>(fd);
#else
    auto f = std::ifstream{filename.string(), std::ios::binary};
    auto file_stream = std::make_unique<io::IstreamInputStream>(&f);
#endif
    io::GzipInputStream gzip_stream{file_stream.get()};
    auto cs = std::make_unique<io::CodedInputStream>(&gzip_stream);
    cs->SetTotalBytesLimit(1024*1024*1024, 1024*1024*1024);
    T vt;
    if (!vt.ParseFromCodedStream(cs.get()))
      throw std::runtime_error("Invalid pb file");
    return vt;
  }
}
#endif
