#ifndef JHMI_UTILITY_PROTOBUF_ZIP_OSTREAM_HPP_NRC_20160909
#define JHMI_UTILITY_PROTOBUF_ZIP_OSTREAM_HPP_NRC_20160909

#include <google/protobuf/io/coded_stream.h>
#include <google/protobuf/io/gzip_stream.h>
#include <google/protobuf/io/zero_copy_stream_impl.h>
//#ifndef WIN32
#if 0
# include <sys/stat.h>
# include <fcntl.h>
#endif

namespace jhmi {
  class protobuf_zip_ostream {
//#ifndef WIN32
#if 0
    int fd_;
    std::unique_ptr<google::protobuf::io::FileOutputStream> file_stream_;
#else
    std::ofstream f_;
    std::unique_ptr<google::protobuf::io::OstreamOutputStream> file_stream_;
#endif
    std::unique_ptr<google::protobuf::io::GzipOutputStream> out_stream_;

  public:
    protobuf_zip_ostream(boost::filesystem::path const& filename) {
      namespace io = google::protobuf::io;
//#ifndef WIN32
#if 0
      fd_ = open(filename.string().c_str(),
                 O_WRONLY | O_CREAT | O_TRUNC, S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);
      if (fd_ == -1)
        throw std::runtime_error("open failed on output file");
      file_stream_ = std::make_unique<io::FileOutputStream>(fd_);
#else
      f_ = std::ofstream{filename.string(), std::ios::binary};
      file_stream_ = std::make_unique<io::OstreamOutputStream>(&f_);
#endif
      io::GzipOutputStream::Options options;
      options.format = io::GzipOutputStream::GZIP;
      out_stream_ = std::make_unique<io::GzipOutputStream>(file_stream_.get(), options);
  }

  auto get() const {
    return out_stream_.get();
  }

  ~protobuf_zip_ostream() {
//#ifndef WIN32
#if 0
     close(fd_);
#endif
  }
};
}

#endif
