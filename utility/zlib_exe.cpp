
#include <boost/iostreams/filtering_stream.hpp>
#include <boost/iostreams/filter/zlib.hpp>
#include <fmt/format.h>
#include <fstream>
using namespace std::string_literals;

int main(int argc, char* argv[]) {
  if (argc != 4 || (argv[1] != "-c"s && argv[1] != "-d"s)) {
    fmt::print("Usage: {1} -c <uncompressed file> <compressed file>\nor\n"
               "       {1} -d <compressed file> <uncompressed file>\n", argv[0]);
    return 1;
  }

  auto infile = std::ifstream{argv[2], std::ios::binary};
  auto outfile = std::ofstream{argv[3], std::ios::binary};
  boost::iostreams::filtering_istream in;
  boost::iostreams::filtering_ostream out;
  if (argv[1] == "-c"s)
    out.push(boost::iostreams::zlib_compressor{});
  else
    in.push(boost::iostreams::zlib_decompressor{});
  in.push(infile);
  out.push(outfile);
  auto buffer = std::vector<char>(1024,0);
  while (in.read(buffer.data(), buffer.size())) {
    auto num = in.gcount();
    out.write(buffer.data(), num);
  }
  return 0;
}
